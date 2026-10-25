// tests/oracle/oracle_tests.cpp — checks that survive NDEBUG (the Release tree defines it). `assert` is FORBIDDEN in tests/oracle.
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "port/oracle/OracleGuards.h" // later sub-phases add their std-only headers here, always as "port/oracle/…"
#include "port/oracle/OracleHash.h"
#include "port/oracle/OracleRecording.h"
#include "port/oracle/OracleCli.h"

static int gFailures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++gFailures;                                                         \
        }                                                                        \
    } while (0)

static void test_guard_table(); // 1.2.2; later sub-phases add test_* functions and one call each in main()
static void test_guard_fatal_before_pin(); // 1.3 (tower note on 1.2)
static void test_fnv_known_answers();      // 1.3
static void test_recording_round_trip();   // 1.3.1
static void test_recording_bad_input();    // 1.3.1
static void test_cli_parse();              // 1.3.2

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    // 1.5 adds `--print-offsets` here (prints offsetof fields, returns 0)
    test_guard_table();
    test_guard_fatal_before_pin();
    test_fnv_known_answers();
    test_recording_round_trip();
    test_recording_bad_input();
    test_cli_parse();
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

// Tower note on 1.2: a refused session is decided BEFORE anything is pinned or forced, so a fatal output never carries a
// half-applied decision, and the fatal reasons are checked in a fixed order (debug launch, editor, freecam).
static void test_guard_fatal_before_pin() {
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 3, 1, DEBUG_MENU_SELECTED, 1, 1 });
        CHECK(fatalNaming(out, "gDebugMenuSelection"));
        CHECK(out.tickLogic == 3);
        CHECK(out.controller1Unplugged == 1);
        CHECK(!out.pinnedTick);
        CHECK(!out.forcedController);
    }
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 3, 1, 1, 1, 1 });
        CHECK(fatalNaming(out, "gEditorEnabled"));
        CHECK(out.tickLogic == 3 && !out.pinnedTick && !out.forcedController);
    }
    {
        OracleGuardOutput out = Oracle_DecideGuards(OracleGuardInput{ 3, 1, 1, 0, 1 });
        CHECK(fatalNaming(out, "gFreecam"));
        CHECK(out.tickLogic == 3 && !out.pinnedTick && !out.forcedController);
    }
}

// FNV-1a-64 published test vectors.
static void test_fnv_known_answers() {
    auto fnv = [](const char* s) {
        oracle::Hasher h;
        h.bytes(s, std::strlen(s));
        return h.value();
    };
    CHECK(fnv("") == 0xcbf29ce484222325ull);
    CHECK(fnv("a") == 0xaf63dc4c8601ec8cull);
    CHECK(fnv("foobar") == 0x85944171f73967e8ull);
}

namespace {

std::filesystem::path TempPath(const char* name) {
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "mk64-oracle-tests";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::filesystem::path p = dir / name;
    std::filesystem::remove(p, ec);
    return p;
}

oracle::Header SampleHeader() {
    oracle::Header h{};
    std::memcpy(h.magic, oracle::kMagic, sizeof h.magic);
    h.version = oracle::kVersion;
    h.headerBytes = sizeof(oracle::Header);
    std::snprintf(h.buildVersion, sizeof h.buildVersion, "%s", "test-build");
    std::snprintf(h.compiler, sizeof h.compiler, "%s", "test-compiler");
    std::snprintf(h.buildType, sizeof h.buildType, "%s", "Release");
    std::snprintf(h.platform, sizeof h.platform, "%s", "test-platform");
    h.gameArchiveHash = 0x1111111111111111ull;
    h.portArchiveHash = 0x2222222222222222ull;
    h.binaryHash = 0x3333333333333333ull;
    h.fpWord = 0x0000020000000000ull;
    h.tickLogic = 2;
    h.skipIntro = 1;
    h.seed = 0x1234;
    return h;
}

// Deterministic, varied pads: every field (including negative sticks) changes with the iteration and the slot.
oracle::Pad SamplePad(uint32_t it, int slot) {
    oracle::Pad p;
    p.button = (uint16_t) (it * 7u + (uint32_t) slot * 0x1111u);
    p.stick_x = (int8_t) (int) ((it + (uint32_t) slot) % 256u - 128);
    p.stick_y = (int8_t) (int) ((it * 3u + (uint32_t) slot) % 256u - 128);
    p.right_stick_x = (int8_t) (int) ((it * 5u + (uint32_t) slot) % 256u - 128);
    p.right_stick_y = (int8_t) (int) ((it * 11u + (uint32_t) slot) % 256u - 128);
    return p;
}

bool WriteSample(const std::filesystem::path& path, uint32_t records, bool withEvent, bool closeIt) {
    oracle::Writer w;
    if (!w.open(path.string(), SampleHeader())) {
        return false;
    }
    for (uint32_t it = 0; it < records; it++) {
        oracle::Pad pads[4];
        for (int s = 0; s < 4; s++) {
            pads[s] = SamplePad(it, s);
        }
        if (!w.writePads(it, pads)) {
            return false;
        }
        if (withEvent && it == records / 2 && !w.writeEvent(oracle::kRecSetupRace, it)) {
            return false;
        }
    }
    return closeIt ? w.close() : w.flush();
}

void PatchBytes(const std::filesystem::path& path, std::streamoff offset, const void* data, size_t n) {
    std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary);
    f.seekp(offset);
    f.write(static_cast<const char*>(data), (std::streamsize) n);
}

void AppendBytes(const std::filesystem::path& path, const void* data, size_t n) {
    std::ofstream f(path, std::ios::app | std::ios::binary);
    f.write(static_cast<const char*>(data), (std::streamsize) n);
}

bool Contains(const std::string& s, const char* what) {
    return s.find(what) != std::string::npos;
}

// Non-throwing filesystem helpers: a missing file must fail a CHECK, never abort the whole suite.
uintmax_t FileSize(const std::filesystem::path& p) {
    std::error_code ec;
    const uintmax_t n = std::filesystem::file_size(p, ec);
    return ec ? UINTMAX_MAX : n;
}

bool Truncate(const std::filesystem::path& p, uintmax_t n) {
    std::error_code ec;
    std::filesystem::resize_file(p, n, ec);
    return !ec;
}

} // namespace

static void test_recording_round_trip() {
    const auto path = TempPath("round-trip.mk64rec");
    CHECK(WriteSample(path, 1000, true, true));
    CHECK(FileSize(path) == 192u + 29u * 1000u + 5u + 1u);

    oracle::Recording r;
    std::string err;
    CHECK(oracle::read(path.string(), r, err));
    CHECK(r.cleanEnd);
    const oracle::Header h = SampleHeader();
    CHECK(std::memcmp(&r.header, &h, sizeof h) == 0);
    CHECK(r.pads.size() == 1000);
    bool allEqual = r.pads.size() == 1000;
    for (uint32_t it = 0; allEqual && it < 1000; it++) {
        oracle::PadsRecord want{ oracle::kRecPads, it, {} };
        for (int s = 0; s < 4; s++) {
            want.pads[s] = SamplePad(it, s);
        }
        allEqual = std::memcmp(&r.pads[it], &want, sizeof want) == 0;
    }
    CHECK(allEqual);
    CHECK(r.events.size() == 1);
    CHECK(r.events.size() == 1 && r.events[0].type == oracle::kRecSetupRace && r.events[0].iteration == 500);

    // A recording with zero iterations (immediate quit): header + kRecEnd only.
    const auto empty = TempPath("empty.mk64rec");
    CHECK(WriteSample(empty, 0, false, true));
    CHECK(FileSize(empty) == 192u + 1u);
    oracle::Recording e;
    CHECK(oracle::read(empty.string(), e, err));
    CHECK(e.pads.empty() && e.events.empty() && e.cleanEnd);

    // close() is idempotent: a second call writes nothing more.
    const auto twice = TempPath("close-twice.mk64rec");
    {
        oracle::Writer w;
        CHECK(w.open(twice.string(), SampleHeader()));
        // Review fix 1: the header is on disk as soon as open() returns, so a native crash before the first periodic flush
        // still leaves the build identity readable.
        CHECK(FileSize(twice) == 192u);
        CHECK(w.close());
        CHECK(w.close());
        CHECK(!w.isOpen());
    }
    CHECK(FileSize(twice) == 192u + 1u);
}

static void test_recording_bad_input() {
    std::string err;
    {
        const auto path = TempPath("bad-magic.mk64rec");
        CHECK(WriteSample(path, 3, false, true));
        const char x = 'X';
        PatchBytes(path, 0, &x, 1);
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "magic"));
    }
    {
        const auto path = TempPath("bad-version.mk64rec");
        CHECK(WriteSample(path, 3, false, true));
        const uint32_t v = 2;
        PatchBytes(path, 8, &v, sizeof v);
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "version"));
    }
    {
        // Review fix 2: a header extension longer than the file is a truncated header, not an empty recording.
        const auto path = TempPath("header-past-eof.mk64rec");
        CHECK(WriteSample(path, 3, false, true));
        const uint32_t hb = 0x1000;
        PatchBytes(path, 12, &hb, sizeof hb);
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "header"));
    }
    {
        const auto path = TempPath("bad-header-bytes.mk64rec");
        CHECK(WriteSample(path, 3, false, true));
        const uint32_t hb = 100;
        PatchBytes(path, 12, &hb, sizeof hb);
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "headerBytes"));
    }
    {
        const auto path = TempPath("short-header.mk64rec");
        CHECK(WriteSample(path, 3, false, true));
        CHECK(Truncate(path, 100));
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "header"));
    }
    {
        // Truncated mid-record (a crash): the complete records are kept, cleanEnd stays false, read() succeeds.
        const auto path = TempPath("truncated.mk64rec");
        CHECK(WriteSample(path, 10, false, true));
        CHECK(Truncate(path, 192u + 29u * 5u + 10u));
        oracle::Recording r;
        CHECK(oracle::read(path.string(), r, err));
        CHECK(r.pads.size() == 5);
        CHECK(!r.cleanEnd);
        CHECK(r.pads.size() == 5 && r.pads[4].iteration == 4);
    }
    {
        // A partial file left by flush() (Oracle_Abort): no kRecEnd, every record complete.
        const auto path = TempPath("flushed.mk64rec");
        CHECK(WriteSample(path, 300, false, false));
        oracle::Recording r;
        CHECK(oracle::read(path.string(), r, err));
        CHECK(r.pads.size() == 300);
        CHECK(!r.cleanEnd);
    }
    {
        const auto path = TempPath("unknown-type.mk64rec");
        CHECK(WriteSample(path, 2, false, false));
        const uint8_t t = 0x7E;
        AppendBytes(path, &t, 1);
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read(path.string(), r, err));
        CHECK(Contains(err, "record type"));
    }
    {
        oracle::Recording r;
        err.clear();
        CHECK(!oracle::read((std::filesystem::temp_directory_path() / "mk64-no-such-dir" / "x.mk64rec").string(), r, err));
        CHECK(!err.empty());
    }
    {
        oracle::Writer w;
        CHECK(!w.open((std::filesystem::temp_directory_path() / "mk64-no-such-dir" / "x.mk64rec").string(), SampleHeader()));
        CHECK(!w.lastError.empty());
        oracle::Pad pads[4]{};
        CHECK(!w.writePads(0, pads)); // never opened
    }
}

namespace {

// Runs Oracle_ParseArgsInto on a fresh config; argv[0] is the program name.
int Parse(std::vector<std::string> args, OracleConfig& cfg) {
    args.insert(args.begin(), "SpaghettiKart");
    std::vector<char*> argv;
    for (auto& a : args) {
        argv.push_back(a.data());
    }
    argv.push_back(nullptr);
    cfg = OracleConfig{};
    return Oracle_ParseArgsInto((int) args.size(), argv.data(), cfg);
}

} // namespace

static void test_cli_parse() {
    OracleConfig cfg;
    CHECK(Parse({}, cfg) == -1 && cfg.mode == OracleMode::Off);
    CHECK(Parse({ "-psn_0_12345", "--some-other-flag" }, cfg) == -1 && cfg.mode == OracleMode::Off); // non-oracle argv ignored

    CHECK(Parse({ "--oracle-record", "x.mk64rec" }, cfg) == -1);
    CHECK(cfg.mode == OracleMode::Record && cfg.recordPath == "x.mk64rec" && cfg.hashLogPath == "x.mk64rec.mk64hash");
    CHECK(Parse({ "--oracle-record", "x.mk64rec", "--oracle-hash-log", "h.mk64hash" }, cfg) == -1 && cfg.hashLogPath == "h.mk64hash");
    CHECK(Parse({ "--oracle-replay", "r.mk64rec" }, cfg) == -1);
    CHECK(cfg.mode == OracleMode::Replay && cfg.replayPath == "r.mk64rec" && cfg.hashLogPath == "r.mk64rec.mk64hash");

    CHECK(Parse({ "--oracle-bogus" }, cfg) == 2);
    CHECK(Parse({ "--oracle-record" }, cfg) == 2);                             // missing value: returns, never _Exit
    CHECK(Parse({ "--oracle-record", "a", "--oracle-replay", "b" }, cfg) == 2); // two modes

    CHECK(Parse({ "--oracle-record", "x", "--oracle-probe-tick-at", "5" }, cfg) == 2); // never in a recording
    CHECK(Parse({ "--oracle-probe-tick-at", "5" }, cfg) == 2);                         // nor with the oracle off
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-tick-at", "5" }, cfg) == -1 && cfg.probeTickAt == 5);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-tick-at", "-3" }, cfg) == 2);
    // Review fixes 8-9: the probe iteration is decimal (spec: base 10) and >= 1 — iteration 0 is the startup read, which no
    // guard call precedes, so N = 0 could never fire.
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-tick-at", "010" }, cfg) == -1 && cfg.probeTickAt == 10);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-tick-at", "0x10" }, cfg) == 2);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-tick-at", "0" }, cfg) == 2);
    // Review fix 5: a flag is never taken as another flag's value.
    CHECK(Parse({ "--oracle-record", "--oracle-seed", "5" }, cfg) == 2);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-hash-log", "--oracle-exit-at-end" }, cfg) == 2);

    CHECK(Parse({ "--oracle-record", "x", "--oracle-seed", "0x4321" }, cfg) == -1 && cfg.seed == 0x4321);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-seed", "70000" }, cfg) == 2); // > 0xFFFF: refused, not truncated
    CHECK(Parse({ "--oracle-record", "x", "--oracle-seed", "12abc" }, cfg) == 2);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-dump-at", "40" }, cfg) == -1 && cfg.dumpAtIteration == 40);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-dump-at", "abc" }, cfg) == 2);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-exit-at-end" }, cfg) == -1 && cfg.exitAtEnd);

    CHECK(Parse({ "--oracle-verify", "a", "b" }, cfg) == 2); // stub until FASE 1.6
    // Usage error (one file). NOTE for FASE 1.6: while Oracle_Verify is a stub returning 2 this check cannot tell the usage
    // error from the stub; once the real verifier exists, `--oracle-verify a` must still be 2 and a valid pair must not be.
    CHECK(Parse({ "--oracle-verify", "a" }, cfg) == 2);
    CHECK(Parse({ "--oracle-info", (std::filesystem::temp_directory_path() / "mk64-no-such-dir" / "x").string() }, cfg) == 2);
    const auto rec = TempPath("cli-info.mk64rec");
    CHECK(WriteSample(rec, 4, true, true));
    CHECK(Parse({ "--oracle-info", rec.string() }, cfg) == 0);
    // Review fix 6: a no-window mode validates the whole command line first, and does not combine with a game mode.
    CHECK(Parse({ "--oracle-info", rec.string(), "--oracle-bogus" }, cfg) == 2);
    CHECK(Parse({ "--oracle-info", rec.string(), "--oracle-record", "x" }, cfg) == 2);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-info", rec.string() }, cfg) == 2);
}
