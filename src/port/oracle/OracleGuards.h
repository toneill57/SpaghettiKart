#pragma once
#include <cstdint>

// Session-guard decision table (FASE 1.2) — pure, no game globals, so the unit suite can link it (tests/oracle).
// Oracle_ApplySessionGuards() (Oracle.cpp) snapshots the globals into an OracleGuardInput once per thread5_iteration
// and writes the decision back.
struct OracleGuardInput {
    int32_t tickLogic;            // gTickLogic (src/main.c:110)
    int32_t controller1Unplugged; // sIsController1Unplugged (src/code_800029B0.c:164, s16)
    int32_t debugMenuSelection;   // gDebugMenuSelection (extern s8, src/menus.h:235)
    int32_t editorEnabled;        // CVar gEditorEnabled
    int32_t freecam;              // CVar gFreecam
};

struct OracleGuardOutput {
    int32_t tickLogic;
    int32_t controller1Unplugged;
    bool fatal;
    const char* fatalReason; // static string, non-null iff fatal
    bool pinnedTick;
    bool forcedController;
};

constexpr int32_t ORACLE_TICK_LOGIC = 2;      // src/main.c:110 default
constexpr int32_t DEBUG_MENU_SELECTED = 0x40; // src/menus.h:129 DEBUG_MENU_OPTION_SELECTED

OracleGuardOutput Oracle_DecideGuards(const OracleGuardInput& in);
