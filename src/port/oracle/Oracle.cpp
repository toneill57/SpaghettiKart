// Oracle hooks that touch game globals (FASE 1.2-1.3; grows in 1.4-1.7). The decisions live in std-only files
// (OracleGuards.cpp, OracleRecording.cpp, OracleCli.cpp), unit-tested in tests/oracle.
#include <libultraship.h>

#include "Oracle.h"
#include "OracleState.h"
#include "OracleGuards.h"
#include "OracleRecording.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

#include "engine/editor/Editor.h" // Editor_IsEnabled (C bridge, Editor.h:63)

extern "C" {
#include "main.h"          // gTickLogic (extern s32, main.h:162)
#include "code_800029B0.h" // sIsController1Unplugged (extern s16, code_800029B0.h:159)
#include "menus.h"         // gDebugMenuSelection (extern s8, menus.h:235), DEBUG_MENU_OPTION_SELECTED (menus.h:129)
}

static_assert(DEBUG_MENU_SELECTED == DEBUG_MENU_OPTION_SELECTED, "OracleGuards.h constant drifted from src/menus.h");

namespace {

oracle::Writer sWriter;
uint32_t sIteration = 0;  // read_controllers() calls since launch while the oracle is active (0 = the startup read, main.c:1154)
bool sOpened = false;
oracle::Header sHeader{}; // the run's identity: FASE 1.4 prints it in the hash log; FASE 1.5 sets it from the recording

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
    sOpened = true;
}

} // namespace

[[noreturn]] void Oracle_Abort(const char* reason) {
    SPDLOG_CRITICAL("oracle: {}", reason);
    std::fprintf(stderr, "oracle: %s\n", reason);
    // A refused run still leaves a readable partial recording: every complete record, no kRecEnd (cleanEnd=false).
    if (sWriter.isOpen()) {
        sWriter.flush();
    }
    // Release logs through an async logger: flush() waits for the queue (spdlog 1.14), so the CRITICAL line is on disk.
    if (auto logger = spdlog::default_logger()) {
        logger->flush();
    }
    std::fflush(nullptr);
    // _Exit, not exit: a refused run must not run the half-initialised engine's static destructors or save CVars.
    std::_Exit(4);
}

bool Oracle_Shutdown() {
    if (sWriter.isOpen() && !sWriter.close()) {
        SPDLOG_CRITICAL("oracle: closing the recording failed: {}", sWriter.lastError);
        return false;
    }
    return true;
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

// Stubs: owned by later sub-phases (cross-doc interface contract, plan.md).
extern "C" void Oracle_OnIterationEnd(void) { // FASE 1.4
}

extern "C" void Oracle_OnSetupRace(void) { // FASE 1.7
}
