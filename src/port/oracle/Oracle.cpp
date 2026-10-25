// Oracle hooks that touch game globals (FASE 1.2; grows in 1.3-1.7). The decisions live in OracleGuards.cpp (pure, unit-tested).
#include <libultraship.h>

#include "Oracle.h"
#include "OracleState.h"
#include "OracleGuards.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

extern "C" {
#include "main.h"          // gTickLogic (extern s32, main.h:162)
#include "code_800029B0.h" // sIsController1Unplugged (extern s16, code_800029B0.h:159)
#include "menus.h"         // gDebugMenuSelection (extern s8, menus.h:235), DEBUG_MENU_OPTION_SELECTED (menus.h:129)
}

static_assert(DEBUG_MENU_SELECTED == DEBUG_MENU_OPTION_SELECTED, "OracleGuards.h constant drifted from src/menus.h");

OracleConfig& Oracle_Config() {
    static OracleConfig sConfig;
    return sConfig;
}

[[noreturn]] void Oracle_Abort(const char* reason) {
    SPDLOG_CRITICAL("oracle: {}", reason);
    std::fprintf(stderr, "oracle: %s\n", reason);
    // Release logs through an async logger: flush() waits for the queue (spdlog 1.14), so the CRITICAL line is on disk.
    if (auto logger = spdlog::default_logger()) {
        logger->flush();
    }
    std::fflush(nullptr);
    // _Exit, not exit: a refused run must not run the half-initialised engine's static destructors or save CVars.
    std::_Exit(4);
}

// TEMPORARY (FASE 1.2 only): until 1.3 adds the CLI, MK64_ORACLE_FORCE_ACTIVE=1 turns the oracle on. 1.3 deletes it.
static bool ForceActiveFromEnv() {
    static const bool sForce = [] {
        const char* v = std::getenv("MK64_ORACLE_FORCE_ACTIVE");
        return v != nullptr && std::strcmp(v, "1") == 0;
    }();
    return sForce;
}

extern "C" int32_t Oracle_IsActive(void) {
    return (Oracle_Config().mode != OracleMode::Off || ForceActiveFromEnv()) ? 1 : 0;
}

extern "C" void Oracle_ApplySessionGuards(void) {
    if (!Oracle_IsActive()) {
        return;
    }
    static bool sLoggedTick = false;
    static bool sLoggedCtl = false;
    // Probe only: MK64_ORACLE_PROBE_TICK_AT=N injects gTickLogic = 3 at guard call N — the toolbar's "+", which is
    // unreachable in oracle mode (its window needs gEditorEnabled, which is fatal). 1.3 replaces it by --oracle-probe-tick-at.
    static const char* sProbe = std::getenv("MK64_ORACLE_PROBE_TICK_AT");
    static const unsigned long sProbeAt = sProbe ? std::strtoul(sProbe, nullptr, 10) : 0;
    static uint32_t sGuardCalls = 0;
    if (sProbe != nullptr && sGuardCalls == sProbeAt) {
        gTickLogic = 3;
    }
    sGuardCalls++;

    const OracleGuardInput in{ gTickLogic, sIsController1Unplugged, gDebugMenuSelection,
                               CVarGetInteger("gEditorEnabled", 0), CVarGetInteger("gFreecam", 0) };
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

// Stubs: owned by later sub-phases (cross-doc interface contract, plan.md).
extern "C" void Oracle_OnPadsRead(OSContPad* pads) { // FASE 1.3 record / 1.5 replay
    (void) pads;
}

extern "C" void Oracle_OnIterationEnd(void) { // FASE 1.4
}

extern "C" void Oracle_OnSetupRace(void) { // FASE 1.7
}
