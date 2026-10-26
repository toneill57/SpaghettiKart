// Oracle hooks that touch game globals (FASE 1.2-1.4; grows in 1.5-1.7). The decisions live in std-only files
// (OracleGuards.cpp, OracleRecording.cpp, OracleCli.cpp, OracleHashLog.h, OracleIdentityHash.h), unit-tested in tests/oracle.
#include <libultraship.h>

#include "Oracle.h"
#include "OracleState.h"
#include "OracleGuards.h"
#include "OracleRecording.h"
#include "OracleReplay.h"
#include "OracleHashLog.h"
#include "OracleStateHash.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
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
// The recorder narrows and the replay widens OSContPad::button through oracle::Pad::button (CONTROLLERBUTTONS_T is
// overridable in libultraship's controller.h): a wider type would silently drop buttons in both directions.
static_assert(sizeof(OSContPad::button) == sizeof(oracle::Pad::button), "OSContPad::button width != .mk64rec Pad::button");

namespace {

oracle::Writer sWriter;
uint32_t sIteration = 0;  // read_controllers() calls since launch while the oracle is active (0 = the startup read, main.c:1154)
bool sOpened = false;
oracle::Header sHeader{}; // the run's identity: FASE 1.4 prints it in the hash log; FASE 1.5 sets it from the recording
oracle::HashLogWriter sHashLog; // FASE 1.4: one line per PADS record, in record AND replay

// FASE 1.5 replay. sRecording is written once (EnsureLoaded), then read-only.
oracle::Recording sRecording;
oracle::ReplayCursor sCursor(sRecording); // binds by reference; used only after EnsureLoaded filled sRecording
bool sReplayLoaded = false;
bool sLive = false;        // the recording is exhausted and live pads are fed (no --oracle-exit-at-end)
bool sReplayEnded = false; // the end of the recording was reached (exit-at-end or the switch to live pads)

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

// The live identity and where it was read from (a zero hash must be explainable). Record: the recording's header; replay:
// the header the recording is checked against.
void LogIdentity(const oracle::Header& h, const oracle::IdentityProbe& probe) {
    SPDLOG_WARN("oracle: identity game={} 0x{:016x} port={} 0x{:016x} binary={} 0x{:016x}", probe.gameArchivePath,
                h.gameArchiveHash, probe.portArchivePath, h.portArchiveHash, probe.binaryPath, h.binaryHash);
    SPDLOG_WARN("oracle: identity configHash=0x{:016x} modListHash=0x{:016x} modListEntries={} portArchiveEntries={}",
                h.configHash, h.modListHash, probe.modListEntries, probe.portArchiveEntries);
    if (!probe.portArchiveError.empty()) {
        SPDLOG_WARN("oracle: portArchiveHash is 0: {}", probe.portArchiveError);
    }
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
    LogIdentity(sHeader, probe);
    sOpened = true;
}

// Every replay exit goes through here: the hash log is closed (`END n`) and the logger flushed by Oracle_Shutdown, then
// _Exit — the primitive main uses (Game.cpp), so no static destructor runs mid-iteration and no CVar/save is written.
[[noreturn]] void ReplayExit(int code, const std::string& msg) {
    if (code == 0) {
        SPDLOG_WARN("oracle: {}", msg);
    } else {
        SPDLOG_CRITICAL("oracle: {}", msg);
    }
    std::fprintf(code == 0 ? stdout : stderr, "oracle: %s\n", msg.c_str());
    if (!Oracle_Shutdown() && code == 0) {
        code = 4; // the hash log could not be completed: never report a clean end
    }
    std::fflush(nullptr);
    std::_Exit(code);
}

// FASE 1.5.3: load the recording and check it at the FIRST pads read (the startup read, main.c thread5_game_loop) — after
// GameEngine::Create() (MakeHeader needs the CVar store and the app paths) and before the first hash line (1.4 opens the
// hash log at the first iteration end, which needs sHeader).
void EnsureLoaded() {
    const OracleConfig& cfg = Oracle_Config();
    if (sReplayLoaded || cfg.mode != OracleMode::Replay) {
        return;
    }
    std::string err;
    if (!oracle::read(cfg.replayPath, sRecording, err)) {
        ReplayExit(2, "cannot load recording " + cfg.replayPath + ": " + err);
    }
    const std::string pathProblem = oracle::CheckReplayHashLogPath(cfg.replayPath, cfg.hashLogPath, cfg.hashLogDefaulted);
    if (!pathProblem.empty()) {
        ReplayExit(2, pathProblem);
    }
    // Checked against the tick count the session WILL run at: the 1.2 pin has not run yet at the startup read, and the
    // recorder wrote gTickLogic at the same moment (2) — a recorded value != 2 means the recording session was not pinned.
    oracle::IdentityProbe probe;
    const oracle::Header live = oracle::MakeHeader(ORACLE_TICK_LOGIC, cfg.seed, &probe);
    LogIdentity(live, probe);
    const oracle::Compat c = oracle::CheckCompatible(sRecording.header, live);
    if (!c.ok) {
        // The header layout did not change in 1.5, the meaning of this field did: say so, or the refusal reads as an asset
        // change.
        const char* hint = c.field == "portArchiveHash"
                               ? " (a content hash since FASE 1.5: a recording made before 1.5 holds the raw-file hash — re-record it)"
                               : "";
        ReplayExit(3, "recording incompatible: field " + c.field + " recorded=" + c.recorded + " live=" + c.live + hint);
    }
    for (const std::string& n : c.notes) {
        SPDLOG_WARN("oracle: META {} (informational, not refused)", n);
    }
    if (c.archivesUnchecked) {
        SPDLOG_WARN("oracle: archive hash is 0 in the recording — asset identity NOT checked");
    }
    sHeader = sRecording.header; // the hash log's metadata = the RECORDING's identity ⇒ record and replay logs cmp equal
    if (!sRecording.cleanEnd) {
        SPDLOG_WARN("oracle: recording has no END record (truncated) — replaying {} complete PADS records",
                    sRecording.pads.size());
    }
    if (sRecording.pads.empty()) {
        SPDLOG_WARN("oracle: recording holds 0 PADS records");
    }
    SPDLOG_WARN("oracle: replay {} — {} PADS, {} events, cleanEnd={}, hash log {}", cfg.replayPath, sRecording.pads.size(),
                sRecording.events.size(), sRecording.cleanEnd, cfg.hashLogPath);
    sReplayLoaded = true;
}

// FASE 1.5.4. iterations 0..N-1 were fed (and hashed by 1.4); this is the read of iteration N = the PADS count.
void OnEndOfRecording() {
    const std::string msg = "end of recording at iteration " + std::to_string(sIteration) + " (" +
                            std::to_string(sCursor.fed()) + " PADS fed)";
    sReplayEnded = true;
    if (Oracle_Config().exitAtEnd) {
        if (!sHashLog.isOpen()) {
            OpenHashLog(); // 0 PADS: a header-only log with `END 0`, never no file (a stale one would be read instead)
        }
        ReplayExit(0, msg + ", --oracle-exit-at-end");
    }
    SPDLOG_WARN("oracle: {} — switching to LIVE pads from iteration {}", msg, sIteration); // once: sLive stops the cursor
    sLive = true;
}

void ReplayPads(OSContPad* pads) {
    if (sLive) {
        return;
    }
    const oracle::PadsRecord* rec = nullptr;
    uint32_t recIter = 0;
    switch (sCursor.next(sIteration, &rec, &recIter)) {
        case oracle::Step::Feed:
            oracle::ApplyRecordedPads(*rec, pads);
            return;
        case oracle::Step::Misaligned: {
            static char sReason[96];
            std::snprintf(sReason, sizeof sReason, "replay misaligned: live iteration %u, recording holds iteration %u",
                          sIteration, recIter);
            Oracle_Abort(sReason);
        }
        case oracle::Step::End:
            OnEndOfRecording();
            return;
    }
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
    if (sReplayLoaded && !sReplayEnded) {
        // A replay stopped before the end of its recording (window closed, Cmd-Q): its log is a prefix, not a run. No
        // `END`, so 1.6 reads it as truncated, and main exits 4 — an incomplete replay never looks like a clean one.
        SPDLOG_CRITICAL("oracle: replay stopped at iteration {} before the end of the recording ({} of {} PADS fed) — "
                        "hash log left without END",
                        sIteration, sCursor.fed(), sRecording.pads.size());
        sHashLog.flush();
        sOk = false;
    } else if (sHashLog.isOpen() && !sHashLog.close()) { // writes `END n`
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
    EnsureLoaded();
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
    } else if (cfg.mode == OracleMode::Replay) {
        ReplayPads(pads); // overwrites the PRE-alias pad: update_controller() recomputes the alias and the edges identically
    }
    sIteration++; // the one counter for PADS records (1.3), replay feeding (1.5) and hash lines (1.4)
}

const oracle::Recording* Oracle_ReplayRecording() {
    EnsureLoaded();
    return sReplayLoaded ? &sRecording : nullptr;
}

bool Oracle_ReplayPastEnd() {
    return sLive;
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
