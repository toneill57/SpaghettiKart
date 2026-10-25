#include "OracleGuards.h"

// Order matters: a refused session is fatal before anything is pinned, so a fatal output never carries a half-applied decision.
OracleGuardOutput Oracle_DecideGuards(const OracleGuardInput& in) {
    OracleGuardOutput out{ in.tickLogic, in.controller1Unplugged, false, nullptr, false, false };
    if (in.debugMenuSelection == DEBUG_MENU_SELECTED) {
        out.fatal = true;
        out.fatalReason = "debug launch (gDebugMenuSelection==0x40) is refused in oracle mode";
        return out;
    }
    if (in.editorEnabled != 0) {
        out.fatal = true;
        out.fatalReason = "Track Editor (gEditorEnabled) must be off in oracle mode";
        return out;
    }
    if (in.freecam != 0) {
        out.fatal = true;
        out.fatalReason = "freecam (gFreecam) must be off in oracle mode (spec consequence #3)";
        return out;
    }
    if (in.tickLogic != ORACLE_TICK_LOGIC) {
        out.tickLogic = ORACLE_TICK_LOGIC;
        out.pinnedTick = true;
    }
    if (in.controller1Unplugged != 0) { // defensive: the port always reports controller 1 present (ControlDeck.cpp:27)
        out.controller1Unplugged = 0;
        out.forcedController = true;
    }
    return out;
}
