// tests/oracle/oracle_tests.cpp — checks that survive NDEBUG (the Release tree defines it). `assert` is FORBIDDEN in tests/oracle.
#include <cstdio>
#include <cstring>
#include "port/oracle/OracleGuards.h" // later sub-phases add their std-only headers here, always as "port/oracle/…"

static int gFailures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++gFailures;                                                         \
        }                                                                        \
    } while (0)

static void test_guard_table(); // 1.2.2; later sub-phases add test_* functions and one call each in main()

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    // 1.5 adds `--print-offsets` here (prints offsetof fields, returns 0)
    test_guard_table();
    std::printf("oracle_tests: %d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}

// A result that changed nothing: the input passed through and no flag is raised.
static bool unchanged(const OracleGuardInput& in, const OracleGuardOutput& out) {
    return out.tickLogic == in.tickLogic && out.controller1Unplugged == in.controller1Unplugged && !out.fatal &&
           out.fatalReason == nullptr && !out.pinnedTick && !out.forcedController;
}

static bool fatalNaming(const OracleGuardOutput& out, const char* what) {
    return out.fatal && out.fatalReason != nullptr && std::strstr(out.fatalReason, what) != nullptr;
}

// Input order: { tickLogic, controller1Unplugged, debugMenuSelection, editorEnabled, freecam }
static void test_guard_table() {
    // 1. gTickLogic moved off 2 (the toolbar's "+") → pinned back, flagged.
    {
        OracleGuardInput in{ 3, 0, 1, 0, 0 };
        OracleGuardOutput out = Oracle_DecideGuards(in);
        CHECK(out.tickLogic == ORACLE_TICK_LOGIC);
        CHECK(out.pinnedTick);
        CHECK(!out.forcedController);
        CHECK(!out.fatal);
    }
    // 2. controller 1 reported unplugged → forced present, flagged (defensive: the port always sets bit 0).
    {
        OracleGuardInput in{ 2, 1, 1, 0, 0 };
        OracleGuardOutput out = Oracle_DecideGuards(in);
        CHECK(out.controller1Unplugged == 0);
        CHECK(out.forcedController);
        CHECK(!out.pinnedTick);
        CHECK(!out.fatal);
    }
    // 3. debug launch (gDebugMenuSelection == DEBUG_MENU_OPTION_SELECTED) → fatal.
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 2, 0, DEBUG_MENU_SELECTED, 0, 0 });
        CHECK(fatalNaming(out, "gDebugMenuSelection"));
    }
    // 4. Track Editor on → fatal.
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 2, 0, 1, 1, 0 });
        CHECK(fatalNaming(out, "gEditorEnabled"));
    }
    // 5. freecam on → fatal (spec consequence #3).
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 2, 0, 1, 0, 1 });
        CHECK(fatalNaming(out, "gFreecam"));
    }
    // 6. normal play (gDebugMenuSelection = gEnableDebugMode + 1 = 2) → nothing changes, nothing flagged.
    {
        OracleGuardInput in{ 2, 0, 2, 0, 0 };
        CHECK(unchanged(in, Oracle_DecideGuards(in)));
    }
    // Edge: the other normal value (debug mode off → 1) also passes; only 0x40 is fatal.
    {
        OracleGuardInput in{ 2, 0, 1, 0, 0 };
        CHECK(unchanged(in, Oracle_DecideGuards(in)));
    }
    // Edge: an absurd injected value is pinned the same way (the pin is per call, not per write).
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 10000, 0, 1, 0, 0 });
        CHECK(out.tickLogic == ORACLE_TICK_LOGIC);
        CHECK(out.pinnedTick);
    }
}
