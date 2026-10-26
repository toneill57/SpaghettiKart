// Oracle hooks that touch game globals (FASE 1.2-1.4; grows in 1.5-1.7). The decisions live in std-only files
// (OracleGuards.cpp, OracleRecording.cpp, OracleCli.cpp, OracleHashLog.h, OracleIdentityHash.h), unit-tested in tests/oracle.
#include <libultraship.h>

#include "Oracle.h"
#include "OracleState.h"
#include "OracleGuards.h"
#include "OracleRecording.h"
#include "OracleHashLog.h"
#include "OracleStateHash.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

#include "engine/editor/Editor.h" // Editor_IsEnabled (C bridge, Editor.h:63)

extern "C" {
#include "main.h"          // gTickLogic (extern s32, main.h:162)
#include "code_800029B0.h" // sIsController1Unplugged (extern s16, code_800029B0.h:159)
#include "menus.h"         // gDebugMenuSelection (extern s8, menus.h:235), DEBUG_MENU_OPTION_SELECTED (menus.h:129)
#include "buffers/random.h" // gRandomSeed16 (the straddle self-test flips it)
// libultraship/src/libultraship/libultra/os.cpp:11 — osContGetReadData memsets this many pads into gControllerPads[4]
// (src/main.c:104). MAXCONTROLLERS is 4, or 6 under _HW_VERSION_1 (libultra/os.h:13-17; T6 C15).
extern uint8_t __osMaxControllers;
}

static_assert(DEBUG_MENU_SELECTED == DEBUG_MENU_OPTION_SELECTED, "OracleGuards.h constant drifted from src/menus.h");

namespace {

oracle::Writer sWriter;
uint32_t sIteration = 0;  // read_controllers() calls since launch while the oracle is active (0 = the startup read, main.c:1154)
bool sOpened = false;
oracle::Header sHeader{}; // the run's identity: FASE 1.4 prints it in the hash log; FASE 1.5 sets it from the recording
oracle::HashLogWriter sHashLog; // FASE 1.4: one line per PADS record, in record AND replay

// Hash cost (logged once at iteration 0 and summarised at shutdown).
uint64_t sHashMicrosTotal = 0;
uint64_t sHashMicrosMax = 0;

// FASE 1.4.5 probe 1 (straddle).
struct StraddleStats {
    bool pending = false;     // an iteration-end hash is waiting for its after-audio re-hash
    uint32_t iteration = 0;
    oracle::GroupHashes atEnd{};
    uint32_t probed = 0;      // iterations re-hashed after the audio frame
    uint32_t differing = 0;   // … of which at least one group changed
    uint32_t byGroup[oracle::kGroupCount] = {};
};
StraddleStats sStraddle;

const char* FormatReason(const char* what, const std::string& detail) { // Oracle_Abort takes a const char*
    static std::string sReason;
    sReason = std::string(what) + ": " + detail;
    return sReason.c_str();
}

void EnsureOpened() {
    const OracleConfig& cfg = Oracle_Config();
    if (sOpened || cfg.mode != OracleMode::Record) {
        return;
    }
    oracle::IdentityProbe probe;
    sHeader = oracle::MakeHeader(gTickLogic, cfg.seed, &probe); // STORED, not a temporary
    if (!sWriter.open(cfg.recordPath, sHeader)) {
        SPDLOG_CRITICAL("oracle: {}", sWriter.lastError);
        Oracle_Abort("record open failed");
    }
    SPDLOG_WARN("oracle: record {} binaryHash=0x{:016x} gameArchiveHash=0x{:016x}", cfg.recordPath, sHeader.binaryHash,
                sHeader.gameArchiveHash);
    SPDLOG_WARN("oracle: identity game={} 0x{:016x} port={} 0x{:016x} binary={} 0x{:016x}", probe.gameArchivePath,
                sHeader.gameArchiveHash, probe.portArchivePath, sHeader.portArchiveHash, probe.binaryPath,
                sHeader.binaryHash);
    SPDLOG_WARN("oracle: identity configHash=0x{:016x} modListHash=0x{:016x} modListEntries={}", sHeader.configHash,
                sHeader.modListHash, probe.modListEntries);
    sOpened = true;
}

void OpenHashLog() {
    if (sHeader.magic[0] == 0) { // a wiring bug, never an empty metadata line
        Oracle_Abort("hash log: the run's header is not set (record: EnsureOpened; replay: FASE 1.5 sets it from the "
                     "recording)");
    }
    const std::string& path = Oracle_Config().hashLogPath;
    if (!sHashLog.open(path, sHeader)) {
        SPDLOG_CRITICAL("oracle: {}", sHashLog.lastError);
        Oracle_Abort(FormatReason("hash log open failed", sHashLog.lastError));
    }
    SPDLOG_WARN("oracle: hash log {}", path);
}

void LogHashCost(uint32_t iteration, uint64_t micros) {
    const oracle::HashCost c = oracle::LastHashCost();
    uint64_t total = 0;
    for (uint64_t b : c.bytes) {
        total += b;
    }
    SPDLOG_WARN("oracle: hash cost iter={} us={} bytes total={} rng={} race={} players={}/{} controllers={}/{} "
                "objects={}/{} actors={} oobjects={} cameras={}/{} racearrays={} (hashed/footprint)",
                iteration, micros, total, c.bytes[oracle::kRng], c.bytes[oracle::kRace], c.bytes[oracle::kPlayers],
                c.footprint[oracle::kPlayers], c.bytes[oracle::kControllers], c.footprint[oracle::kControllers],
                c.bytes[oracle::kObjects], c.footprint[oracle::kObjects], c.bytes[oracle::kActors],
                c.bytes[oracle::kOObjects], c.bytes[oracle::kCameras], c.footprint[oracle::kCameras],
                c.bytes[oracle::kRaceArrays]);
}

void LogShutdownSummaries() {
    if (sHashLog.lines() > 0) {
        SPDLOG_WARN("oracle: hash summary lines={} us_mean={} us_max={}", sHashLog.lines(),
                    sHashMicrosTotal / sHashLog.lines(), sHashMicrosMax);
    }
    const OracleConfig& cfg = Oracle_Config();
    if (cfg.probeStraddle) {
        const StraddleStats& s = sStraddle;
        SPDLOG_WARN("oracle: STRADDLE summary mode={} selftest={} probed={} differing={} rng={} race={} players={} "
                    "controllers={} objects={} actors={} oobjects={} cameras={} racearrays={}",
                    cfg.probeAudioSerial ? "serial" : "concurrent", cfg.probeSelftest ? 1 : 0, s.probed, s.differing,
                    s.byGroup[oracle::kRng], s.byGroup[oracle::kRace], s.byGroup[oracle::kPlayers],
                    s.byGroup[oracle::kControllers], s.byGroup[oracle::kObjects], s.byGroup[oracle::kActors],
                    s.byGroup[oracle::kOObjects], s.byGroup[oracle::kCameras], s.byGroup[oracle::kRaceArrays]);
    }
}

} // namespace

[[noreturn]] void Oracle_Abort(const char* reason) {
    SPDLOG_CRITICAL("oracle: {}", reason);
    std::fprintf(stderr, "oracle: %s\n", reason);
    // A refused run still leaves a readable partial recording: every complete record, no kRecEnd (cleanEnd=false).
    if (sWriter.isOpen()) {
        sWriter.flush();
    }
    // …and every complete hash line, with no END: 1.6 reads an aborted log as truncated, on purpose.
    sHashLog.flush();
    // Release logs through an async logger: flush() waits for the queue (spdlog 1.14), so the CRITICAL line is on disk.
    if (auto logger = spdlog::default_logger()) {
        logger->flush();
    }
    std::fflush(nullptr);
    // _Exit, not exit: a refused run must not run the half-initialised engine's static destructors or save CVars.
    std::_Exit(4);
}

bool Oracle_Shutdown() {
    static bool sDone = false;
    static bool sOk = true;
    if (sDone) {
        return sOk;
    }
    sDone = true;
    if (sWriter.isOpen() && !sWriter.close()) {
        SPDLOG_CRITICAL("oracle: closing the recording failed: {}", sWriter.lastError);
        sOk = false;
    }
    if (sHashLog.isOpen() && !sHashLog.close()) { // writes `END n`
        SPDLOG_CRITICAL("oracle: closing the hash log failed: {}", sHashLog.lastError);
        sOk = false;
    }
    LogShutdownSummaries();
    // main leaves with _Exit after Destroy(), which flushes nothing: without this the summaries never reach the log file
    // (measured: present on stdout, absent from the file).
    if (auto logger = spdlog::default_logger()) {
        logger->flush();
    }
    return sOk;
}

extern "C" int32_t Oracle_IsActive(void) {
    return Oracle_Config().mode != OracleMode::Off ? 1 : 0;
}

extern "C" void Oracle_ApplySessionGuards(void) {
    if (!Oracle_IsActive()) {
        return;
    }
    static bool sLoggedTick = false;
    static bool sLoggedCtl = false;
    // Probe (--oracle-probe-tick-at N, replay only, N >= 1): inject gTickLogic = 3 in the guard call that precedes read N,
    // so iteration N is the first one ticked with the injected value (the pin restores it in the same call) — the Track
    // Editor toolbar's "+", unreachable in oracle mode. Read 0 (startup) has no guard call before it. NOTE: 1.2's env probe
    // counted guard calls, so its "100" is this probe's 101.
    const int32_t probeAt = Oracle_Config().probeTickAt;
    if (probeAt >= 0 && sIteration == (uint32_t) probeAt) {
        gTickLogic = 3;
    }

    // The editor is refused on its real state (gEditor.bEditorEnabled) OR its window CVar: closing the Tools window clears
    // gEditorEnabled (GuiWindow::SyncVisibilityConsoleVariable) while the editor stays enabled (Editor::Tick never resets it).
    const int32_t editorEnabled = (Editor_IsEnabled() || CVarGetInteger("gEditorEnabled", 0) != 0) ? 1 : 0;
    const OracleGuardInput in{ gTickLogic, sIsController1Unplugged, gDebugMenuSelection, editorEnabled,
                               CVarGetInteger("gFreecam", 0) };
    const OracleGuardOutput out = Oracle_DecideGuards(in);
    if (out.fatal) {
        Oracle_Abort(out.fatalReason);
    }
    if (out.pinnedTick && !sLoggedTick) {
        SPDLOG_WARN("oracle: gTickLogic pinned (was {})", in.tickLogic);
        sLoggedTick = true;
    }
    if (out.forcedController && !sLoggedCtl) {
        SPDLOG_WARN("oracle: sIsController1Unplugged forced to false");
        sLoggedCtl = true;
    }
    gTickLogic = out.tickLogic;
    sIsController1Unplugged = (s16) out.controller1Unplugged;
}

extern "C" void Oracle_OnPadsRead(OSContPad* pads) {
    const OracleConfig& cfg = Oracle_Config();
    if (cfg.mode == OracleMode::Off) {
        return;
    }
    // FASE 1.4.5 probe 5 (T6 C15): the recorder, the replay and the hash all assume 4 pads. Under _HW_VERSION_1 the
    // engine would memset 6 OSContPads into gControllerPads[4].
    if (__osMaxControllers != 4) {
        Oracle_Abort(FormatReason("MAXCONTROLLERS != 4", "__osMaxControllers=" + std::to_string(__osMaxControllers)));
    }
    EnsureOpened();
    if (cfg.mode == OracleMode::Record) {
        oracle::Pad p[4];
        for (int i = 0; i < 4; i++) { // the PRE-alias raw pad: update_controller() has not run yet (main.c:367-368)
            p[i] = oracle::Pad{ pads[i].button, pads[i].stick_x, pads[i].stick_y, pads[i].right_stick_x,
                                pads[i].right_stick_y };
        }
        if (!sWriter.writePads(sIteration, p)) {
            static char sReason[96];
            std::snprintf(sReason, sizeof sReason, "record write failed at iteration %u", sIteration);
            Oracle_Abort(sReason);
        }
    }
    // FASE 1.5 adds the Replay branch here (overwrite pads[] from the recording BEFORE update_controller)
    sIteration++;
}

// FASE 1.4.4: one `.mk64hash` line per PADS record — the state the pads of iteration `sIteration - 1` produced. Called at
// the end of thread5_game_loop (iteration 0, the startup read) and right after game_state_handler() (every later one).
extern "C" void Oracle_OnIterationEnd(void) {
    if (Oracle_Config().mode == OracleMode::Off) {
        return;
    }
    if (sIteration == 0) {
        Oracle_Abort("iteration end before any pads read");
    }
    const uint32_t it = sIteration - 1;
    if (!sHashLog.isOpen()) {
        OpenHashLog();
    }
    const auto t0 = std::chrono::steady_clock::now();
    const oracle::GroupHashes g = oracle::HashState(nullptr);
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    const uint64_t us = (uint64_t) std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    // The writer refuses a misaligned iteration (a read_controllers() call with no iteration end, or the reverse): a loud
    // abort, never a silent shift of every later line.
    if (!sHashLog.write(it, g)) {
        Oracle_Abort(FormatReason("hash log", sHashLog.lastError));
    }
    sHashMicrosTotal += us;
    sHashMicrosMax = us > sHashMicrosMax ? us : sHashMicrosMax;
    if (it == 0) {
        LogHashCost(it, us);
    }
    Oracle_ProbeAtIterationEnd(it, g);
}

void Oracle_ProbeAtIterationEnd(uint32_t iteration, const oracle::GroupHashes& g) {
    if (!Oracle_Config().probeStraddle) {
        return;
    }
    sStraddle.pending = true;
    sStraddle.iteration = iteration;
    sStraddle.atEnd = g;
}

// push_frame, right after GameEngine::EndAudioFrame(): the audio thread is idle again (waiting on its condition variable),
// and end_master_display_list() + display_and_vsync() have run. A group that differs from the iteration-end hash was
// written in between: by the audio thread (concurrent mode) or by the main thread's render path (--oracle-probe-audio-serial
// runs the audio frame before the iteration, so only the render path is left in the window). Never takes audio.mutex.
void Oracle_ProbeAfterAudio() {
    const OracleConfig& cfg = Oracle_Config();
    if (!cfg.probeStraddle || !sStraddle.pending) {
        return;
    }
    sStraddle.pending = false;
    oracle::GroupHashes now;
    if (cfg.probeSelftest) { // positive control: a write the re-hash must see; restored before anything else runs
        gRandomSeed16 ^= 1;
        now = oracle::HashState(nullptr);
        gRandomSeed16 ^= 1;
    } else {
        now = oracle::HashState(nullptr);
    }
    sStraddle.probed++;
    const uint32_t mask = oracle::DifferingGroups(sStraddle.atEnd, now);
    if (mask == 0) {
        return;
    }
    sStraddle.differing++;
    for (int i = 0; i < oracle::kGroupCount; i++) {
        if (mask & (1u << i)) {
            sStraddle.byGroup[i]++;
            SPDLOG_WARN("oracle: STRADDLE iter={} group={}", sStraddle.iteration, oracle::kGroupNames[i]);
        }
    }
}

// Stubs: owned by later sub-phases (cross-doc interface contract, plan.md).

extern "C" void Oracle_OnSetupRace(void) { // FASE 1.7
}
