// tests/oracle/oracle_tests.cpp — checks that survive NDEBUG (the Release tree defines it). `assert` is FORBIDDEN in tests/oracle.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <set>
#include <vector>
#include "port/oracle/OracleGuards.h" // later sub-phases add their std-only headers here, always as "port/oracle/…"
#include "port/oracle/OracleHash.h"
#include "port/oracle/OracleRecording.h"
#include "port/oracle/OracleCli.h"
#include "port/oracle/OracleHashLog.h"
#include "port/oracle/OracleIdentityHash.h"
#include "port/oracle/OracleReplay.h"
#include "port/oracle/OracleArchiveHash.h"
#include "port/oracle/OracleCoverage.h"

#include <cstddef> // offsetof (--print-offsets)

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
static void test_hasher_scalars();         // 1.4.1
static void test_hasher_put_and_paths();   // 1.4.1
static void test_hash_log_format();        // 1.4.4
static void test_hash_log_writer();        // 1.4.4
static void test_mod_list_hash();          // 1.4.5 probes 3-4
static void test_config_hash();            // 1.4.5 probe 2
static void test_cvar_sweep();             // 1.4.5 probe 2: re-runs the sweep over src/ (critic)
static void test_cli_probe_flags();        // 1.4.5 probe 1
static void test_replay_compat();          // 1.5.2
static void test_replay_cursor();          // 1.5.2
static void test_apply_pads_keeps_gyro();  // 1.5.2
static void test_replay_hash_log_path();   // 1.5.4
static void test_zip_content_hash();       // 1.5 amendment: portArchiveHash = content hash
static void test_coverage_parser();        // 1.4b.2: the source-text member parser on a fixture (its positive control)
static void test_subclass_coverage();      // 1.4b.2: 0 unclassified / 0 stale / 0 duplicated (critic)
static int print_coverage();               // 1.4b.2: --print-coverage, the source-text member list (vs. the clang dump)

int main(int argc, char** argv) {
    // Packed offsets measured by the compiler that writes the files (FASE 1.5 Validation Commands): a byte patch of a
    // .mk64rec takes its offset from here, never from a table.
    if (argc > 1 && std::strcmp(argv[1], "--print-offsets") == 0) {
        std::printf("offsetof tickLogic=%zu\n", offsetof(oracle::Header, tickLogic));
        std::printf("sizeof Header=%zu\n", sizeof(oracle::Header));
        std::printf("sizeof PadsRecord=%zu\n", sizeof(oracle::PadsRecord));
        std::printf("offsetof PadsRecord.pads=%zu\n", offsetof(oracle::PadsRecord, pads));
        std::printf("sizeof Pad=%zu\n", sizeof(oracle::Pad));
        std::printf("offsetof Pad.stick_x=%zu\n", offsetof(oracle::Pad, stick_x));
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--print-coverage") == 0) {
        return print_coverage();
    }
    test_guard_table();
    test_guard_fatal_before_pin();
    test_fnv_known_answers();
    test_recording_round_trip();
    test_recording_bad_input();
    test_cli_parse();
    test_hasher_scalars();
    test_hasher_put_and_paths();
    test_hash_log_format();
    test_hash_log_writer();
    test_mod_list_hash();
    test_config_hash();
    test_cvar_sweep();
    test_cli_probe_flags();
    test_replay_compat();
    test_replay_cursor();
    test_apply_pads_keeps_gyro();
    test_replay_hash_log_path();
    test_zip_content_hash();
    test_coverage_parser();
    test_subclass_coverage();
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

// ---------------------------------------------------------------------------------------------------------------------
// FASE 1.4 — state hash. Expected values computed independently (Python FNV-1a-64 over struct.pack('<…')), never by the
// code under test.

namespace {

uint64_t HashOf(void (*fill)(oracle::Hasher&)) {
    oracle::Hasher h;
    fill(h);
    return h.value();
}

uint64_t BytesHash(std::initializer_list<uint8_t> b) {
    oracle::Hasher h;
    for (uint8_t x : b) {
        h.bytes(&x, 1);
    }
    return h.value();
}

struct RecordingSink : oracle::FieldSink {
    struct Field {
        std::string path;
        std::string type;
        uint64_t bits;
    };
    std::vector<Field> fields;
    void field(const std::string& path, const char* type, uint64_t bits) override {
        fields.push_back(Field{ path, type, bits });
    }
};

std::string ReadFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} // namespace

static void test_hasher_scalars() {
    // Little-endian canonical form, independent of the host.
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteU32(0x01020304u); }) == 0xb345225e3644edb5ull);
    CHECK(BytesHash({ 0x04, 0x03, 0x02, 0x01 }) == 0xb345225e3644edb5ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteU16(0x1234); }) == 0x07ee9e07b4b1c883ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteU64(0x0102030405060708ull); }) == 0x0c6d4496e17859d5ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteS8(-1); }) == 0xaf64724c8602eb6eull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteS16(-2); }) == 0x0a9dc907b6fa303aull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteS32(-3); }) == 0x3e39765878c1e873ull);
    // Bit patterns, not values: -0.0 and 0.0 compare equal as floats and must NOT hash equal.
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteF32(0.0f); }) == 0x4d25767f9dce13f5ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteF32(-0.0f); }) == 0x4d24f67f9dcd3a75ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteF64(0.0); }) == 0xa8c7f832281a39c5ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteF64(-0.0); }) == 0xa8c7783228196045ull);
    CHECK(HashOf([](oracle::Hasher& h) { h.WriteF32(1.5f); }) == 0x4a98c77f9ba36558ull);
    {
        oracle::Hasher h;
        h.WriteU32(1);
        h.WriteU8(2);
        CHECK(h.byteCount() == 5);
    }
    // Named writers hash exactly like the unnamed ones, with or without a sink.
    {
        RecordingSink sink;
        oracle::Hasher named(&sink);
        named.WriteF32("x", -0.0f);
        named.WriteS16("y", -2);
        oracle::Hasher plain;
        plain.WriteF32(-0.0f);
        plain.WriteS16(-2);
        CHECK(named.value() == plain.value());
        CHECK(sink.fields.size() == 2);
        CHECK(sink.fields.size() == 2 && sink.fields[0].path == "x" && sink.fields[0].type == "f32" &&
              sink.fields[0].bits == 0x80000000ull);
        CHECK(sink.fields.size() == 2 && sink.fields[1].type == "s16" && sink.fields[1].bits == 0xfffeull);
    }
    // `all` = FNV over the group values as LE u64 (ten groups since FASE 1.4b; python3 FNV-1a over struct.pack('<Q'),
    // which also reproduces the nine-group values this test pinned before: 0x60b3e1fa9c9c0ca4 / 0x3ecb33e15783bec5).
    {
        uint64_t g[oracle::kGroupCount];
        for (int i = 0; i < oracle::kGroupCount; i++) {
            g[i] = (uint64_t) (i + 1);
        }
        CHECK(oracle::Combine(g) == 0xcedc94dfee39ee6eull);
        uint64_t z[oracle::kGroupCount] = {};
        CHECK(oracle::Combine(z) == 0xf14b84b8290b8965ull);
    }
    {
        oracle::GroupHashes a{};
        oracle::GroupHashes b{};
        CHECK(oracle::DifferingGroups(a, b) == 0);
        b.g[oracle::kRng] = 1;
        b.g[oracle::kRaceArrays] = 1;
        b.all = 7; // `all` is not a group of its own
        CHECK(oracle::DifferingGroups(a, b) == ((1u << oracle::kRng) | (1u << oracle::kRaceArrays)));
    }
    CHECK(oracle::kGroupCount == 10);
    CHECK(std::string(oracle::kGroupNames[0]) == "rng" && std::string(oracle::kGroupNames[8]) == "racearrays");
    CHECK(std::string(oracle::kGroupNames[9]) == "world" && oracle::kWorld == 9); // FASE 1.4b
}

namespace {
enum class E16 : uint16_t { A = 0x1234 };
enum E32 { kE32 = 7 };
} // namespace

static void test_hasher_put_and_paths() {
    // put() dispatches on the type's size and signedness, never on the exact type name.
    auto same = [](void (*a)(oracle::Hasher&), void (*b)(oracle::Hasher&)) { return HashOf(a) == HashOf(b); };
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", (uint16_t) 0x1234); }, [](oracle::Hasher& h) { h.WriteU16(0x1234); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", true); }, [](oracle::Hasher& h) { h.WriteU8(1); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", false); }, [](oracle::Hasher& h) { h.WriteU8(0); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", E16::A); }, [](oracle::Hasher& h) { h.WriteU16(0x1234); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", kE32); }, [](oracle::Hasher& h) { h.WriteU32(7); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", (size_t) 5); }, [](oracle::Hasher& h) { h.WriteU64(5); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", (long) -1); }, [](oracle::Hasher& h) { h.WriteU64(~0ull); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", 'a'); }, [](oracle::Hasher& h) { h.WriteU8('a'); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", (int16_t) -2); }, [](oracle::Hasher& h) { h.WriteS16(-2); }));
    CHECK(same([](oracle::Hasher& h) { oracle::put(h, "x", -0.0); }, [](oracle::Hasher& h) { h.WriteF64(-0.0); }));
    CHECK(same(
        [](oracle::Hasher& h) {
            const uint8_t a[3] = { 1, 2, 3 };
            oracle::put(h, "a", a);
        },
        [](oracle::Hasher& h) { h.bytes("\x01\x02\x03", 3); }));

    // The sink sees dotted / indexed paths; the hash does not depend on the sink.
    {
        RecordingSink sink;
        oracle::Hasher h(&sink);
        const float pos[3] = { 1.0f, -0.0f, 2.5f };
        {
            oracle::Hasher::Scope a(h, "p");
            oracle::Hasher::Scope b(h, (size_t) 2);
            oracle::put(h, "pos", pos);
        }
        const float m[2][2] = { { 0.0f, 0.0f }, { 3.0f, 0.0f } };
        oracle::put(h, "m", m);
        oracle::put(h, "top", (int32_t) -1);
        CHECK(sink.fields.size() == 8);
        if (sink.fields.size() == 8) {
            CHECK(sink.fields[0].path == "p[2].pos[0]" && sink.fields[0].type == "f32" && sink.fields[0].bits == 0x3f800000ull);
            CHECK(sink.fields[1].path == "p[2].pos[1]" && sink.fields[1].bits == 0x80000000ull);
            CHECK(sink.fields[2].path == "p[2].pos[2]");
            CHECK(sink.fields[3].path == "m[0][0]");
            CHECK(sink.fields[5].path == "m[1][0]" && sink.fields[5].bits == 0x40400000ull);
            CHECK(sink.fields[7].path == "top" && sink.fields[7].type == "s32" && sink.fields[7].bits == 0xffffffffull);
        }
        oracle::Hasher plain;
        oracle::put(plain, "pos", pos);
        oracle::put(plain, "m", m);
        oracle::put(plain, "top", (int32_t) -1);
        CHECK(plain.value() == h.value());
    }
}

namespace {

oracle::Header MetaHeader() {
    oracle::Header h = SampleHeader();
    std::snprintf(h.buildVersion, sizeof h.buildVersion, "%s", "base-5073d2e-17-g04dd07156");
    std::snprintf(h.platform, sizeof h.platform, "%s", "macos-arm64");
    std::snprintf(h.compiler, sizeof h.compiler, "%s", "clang 17.0.0 (clang-1700.6.4.2)");
    h.configHash = 0x4444444444444444ull;
    h.modListHash = 0x5555555555555555ull;
    return h;
}

oracle::GroupHashes SampleGroups() {
    oracle::GroupHashes g{};
    for (int i = 0; i < oracle::kGroupCount; i++) {
        g.g[i] = 0x0123456789abcdefull + (uint64_t) i;
    }
    g.all = 0xfedcba9876543210ull;
    return g;
}

const char* kMetaExpected =
    "MK64HASH 1\n"
    "# build=base-5073d2e-17-g04dd07156 platform=macos-arm64 buildType=Release compiler=clang_17.0.0_(clang-1700.6.4.2) "
    "binaryHash=0x3333333333333333\n"
    "# tickLogic=2 seed=0x1234 fpWord=0x0000020000000000 configHash=0x4444444444444444 modListHash=0x5555555555555555 "
    "gameArchiveHash=0x1111111111111111 portArchiveHash=0x2222222222222222\n"
    "# groups=rng,race,players,controllers,objects,actors,oobjects,cameras,racearrays,world,all\n";

const char* kLine7Expected =
    "7 rng=0123456789abcdef race=0123456789abcdf0 players=0123456789abcdf1 controllers=0123456789abcdf2 "
    "objects=0123456789abcdf3 actors=0123456789abcdf4 oobjects=0123456789abcdf5 cameras=0123456789abcdf6 "
    "racearrays=0123456789abcdf7 world=0123456789abcdf8 all=fedcba9876543210\n";

} // namespace

static void test_hash_log_format() {
    // Every metadata token is key=value: the compiler's spaces become '_' (1.6 splits `# ` lines on spaces).
    CHECK(oracle::FormatHashLogHeader(MetaHeader()) == kMetaExpected);
    {
        oracle::Header h = MetaHeader();
        std::memset(h.buildVersion, 'x', sizeof h.buildVersion); // full width, no NUL: must not run into `compiler`
        h.platform[0] = '\x01';
        const std::string head = oracle::FormatHashLogHeader(h);
        CHECK(Contains(head, ("# build=" + std::string(sizeof h.buildVersion, 'x') + " platform=_acos-arm64 ").c_str()));
    }
    char line[512];
    const int n = oracle::FormatHashLine(line, sizeof line, 7, SampleGroups());
    CHECK(n == (int) std::strlen(kLine7Expected));
    CHECK(n > 0 && std::string(line, (size_t) n) == kLine7Expected);
    CHECK(oracle::FormatHashLine(line, 40, 7, SampleGroups()) == -1); // never a silently cut line
    CHECK(oracle::FormatHashLogEnd(1801) == "END 1801\n");
}

static void test_hash_log_writer() {
    const auto path = TempPath("writer.mk64hash");
    {
        oracle::HashLogWriter w;
        CHECK(w.open(path.string(), MetaHeader()));
        CHECK(ReadFile(path) == kMetaExpected); // the header is on disk at open()
        oracle::GroupHashes g = SampleGroups();
        CHECK(w.write(0, g));
        CHECK(w.write(1, g));
        // Alignment: line k carries iteration k — a skipped or repeated iteration is refused, never written.
        CHECK(!w.write(3, g));
        CHECK(Contains(w.lastError, "misalignment"));
        CHECK(!w.write(1, g));
        CHECK(w.write(2, g));
        CHECK(w.lines() == 3);
        CHECK(w.close());
        CHECK(w.close()); // idempotent
        CHECK(!w.isOpen());
        CHECK(!w.write(3, g)); // closed
    }
    {
        const std::string s = ReadFile(path);
        char l0[512];
        const int n0 = oracle::FormatHashLine(l0, sizeof l0, 0, SampleGroups());
        CHECK(n0 > 0 && s.rfind(std::string(kMetaExpected) + std::string(l0, (size_t) n0), 0) == 0);
        CHECK(s.size() > 9 && s.substr(s.size() - 6) == "END 3\n");
        CHECK(std::count(s.begin(), s.end(), '\n') == 4 + 3 + 1);
    }
    // Zero iterations (immediate quit): the four header lines + END 0.
    {
        const auto p0 = TempPath("writer-empty.mk64hash");
        oracle::HashLogWriter w;
        CHECK(w.open(p0.string(), MetaHeader()));
        CHECK(w.close());
        CHECK(ReadFile(p0) == std::string(kMetaExpected) + "END 0\n");
    }
    // An aborted run (flush, no close) reads as truncated: complete lines, no END.
    {
        const auto pt = TempPath("writer-truncated.mk64hash");
        oracle::HashLogWriter w;
        CHECK(w.open(pt.string(), MetaHeader()));
        CHECK(w.write(0, SampleGroups()));
        CHECK(w.flush());
        const std::string s = ReadFile(pt);
        CHECK(!Contains(s, "END"));
        CHECK(s.size() > 0 && s.back() == '\n');
    }
    // Unwritable path: refused with the path in the message (Oracle_Abort prints it).
    {
        oracle::HashLogWriter w;
        const auto bad = std::filesystem::temp_directory_path() / "mk64-no-such-dir" / "x.mk64hash";
        CHECK(!w.open(bad.string(), MetaHeader()));
        CHECK(Contains(w.lastError, bad.string().c_str()));
        CHECK(!w.isOpen());
    }
}

namespace {

oracle::ModListInput SampleModList() {
    oracle::ModListInput in;
    in.modsInLoadOrder = { "mk64-assets", "extended-assets", "spaghettikart-core", "my-mod" };
    in.tracks = { { 0, "mk:mario_raceway" }, { 1, "mk:choco_mountain" }, { 2, "mk:bowsers_castle" } };
    in.actors = { { 0, "mk:banana" }, { 1, "mk:green_shell" } };
    in.items = { { 0, "mk:mushroom" } };
    return in;
}

} // namespace

static void test_mod_list_hash() {
    const uint64_t base = oracle::HashModList(SampleModList());
    // FASE 1.4b.5 known answer, computed independently (implement/fase-1-artifacts/1.4b/kat.py: python3 FNV-1a-64 over
    // the documented layout) — pins the formula itself (tower mutant T07, sort by name only, survived 1.4's property
    // tests).
    CHECK(base == 0x137c2a11f7c131f4ull);
    // The registries' iteration order (unordered_map) is NOT identity: any insertion order gives the same value.
    {
        oracle::ModListInput in = SampleModList();
        std::reverse(in.tracks.begin(), in.tracks.end());
        std::reverse(in.actors.begin(), in.actors.end());
        CHECK(oracle::HashModList(in) == base);
    }
    // The ids ARE identity (mCounter++ in registration order, T3 P13).
    {
        oracle::ModListInput in = SampleModList();
        std::swap(in.tracks[0].id, in.tracks[1].id);
        CHECK(oracle::HashModList(in) != base);
    }
    // …even when every id moves by the same amount and the order is unchanged (an extra registration earlier on).
    {
        oracle::ModListInput in = SampleModList();
        for (oracle::RegistryEntry& e : in.tracks) {
            e.id += 10;
        }
        CHECK(oracle::HashModList(in) != base);
    }
    // The mod load order IS identity (later archives override earlier ones, T3 P14).
    {
        oracle::ModListInput in = SampleModList();
        std::swap(in.modsInLoadOrder[2], in.modsInLoadOrder[3]);
        CHECK(oracle::HashModList(in) != base);
    }
    {
        oracle::ModListInput in = SampleModList();
        in.items.push_back({ 1, "mod:extra_item" });
        CHECK(oracle::HashModList(in) != base);
    }
    // An entry moved between registries changes the value (each registry is length-prefixed).
    {
        oracle::ModListInput in = SampleModList();
        in.items.push_back(in.actors.back());
        in.actors.pop_back();
        CHECK(oracle::HashModList(in) != base);
    }
    // "Nothing loaded" is a real value, not the 1.3 placeholder 0.
    CHECK(oracle::HashModList(oracle::ModListInput{}) != 0);
    CHECK(oracle::HashModList(oracle::ModListInput{}) == oracle::HashModList(oracle::ModListInput{}));
}

static void test_config_hash() {
    const oracle::CVarSpec specs[2] = { { "gA", oracle::CVarKind::Integer, 0 }, { "gB", oracle::CVarKind::Float, 1.5f } };
    auto hash = [&](int32_t a, float b) {
        const oracle::CVarValue v[2] = { { a, 0.0f }, { 0, b } };
        return oracle::HashConfig(specs, v, 2);
    };
    const uint64_t base = hash(3, 1.5f);
    // FASE 1.4b.5 known answer, computed independently (1.4b/kat.py) — pins the byte layout (tower mutant T06, the
    // name's terminator dropped, survived 1.4's property tests).
    CHECK(base == 0x74d43566c974f4c2ull);
    CHECK(hash(3, 1.5f) == base);
    CHECK(hash(4, 1.5f) != base);
    CHECK(hash(3, 0.0f) != hash(3, -0.0f)); // floats by bit pattern
    {
        const oracle::CVarSpec renamed[2] = { { "gC", oracle::CVarKind::Integer, 0 }, specs[1] };
        const oracle::CVarValue v[2] = { { 3, 0.0f }, { 0, 1.5f } };
        CHECK(oracle::HashConfig(renamed, v, 2) != base); // the name is part of the input
    }

    // The table itself: unique, disjoint from the exclusions, and it covers the FASE's list (T3 P11 + gSkipIntro /
    // gEnableDebugMode).
    std::set<std::string> sim;
    for (const oracle::CVarSpec& s : oracle::kSimCVars) {
        CHECK(sim.insert(s.name).second);
    }
    std::set<std::string> excluded;
    for (const oracle::CVarExclusion& e : oracle::kExcludedCVars) {
        CHECK(excluded.insert(e.name).second);
        CHECK(sim.count(e.name) == 0);
        CHECK(e.reason != nullptr && std::strlen(e.reason) > 0);
    }
    const char* required[] = { "gCustomCC",       "gHarderCPU",        "gDisableRubberbanding", "gEnableMoonJump",
                               "gNoWallColision", "gShellsShootStraight", "gDisableItemboxes",  "gAllBombKartsChase",
                               "gGoFish",         "gEnableCustomCC",   "gSkipIntro",            "gEnableDebugMode" };
    for (const char* r : required) {
        CHECK(sim.count(r) == 1);
    }
    for (const oracle::CVarSpec& s : oracle::kSimCVars) {
        if (std::string(s.name) == "gCustomCC") {
            CHECK(s.kind == oracle::CVarKind::Float && s.defaultValue == 150.0f);
        }
    }
}

#ifndef ORACLE_SOURCE_DIR
#error "ORACLE_SOURCE_DIR must point at the fork's src/ (tests/oracle/CMakeLists.txt)"
#endif

namespace {

struct CVarRead {
    std::string name;
    std::string accessor; // Integer / Float / String / Color / …
    std::string defaultText;
    std::string where;
};

// Every `CVarGet<Accessor>("<literal>", <default>)` in src/ (src/port/ui excluded, like the FASE sweep).
std::vector<CVarRead> ScanCVarReads(size_t& filesScanned) {
    std::vector<CVarRead> out;
    filesScanned = 0;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(ORACLE_SOURCE_DIR, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file()) {
            continue;
        }
        const std::string path = it->path().generic_string();
        const std::string ext = it->path().extension().string();
        if (path.find("/port/ui/") != std::string::npos || (ext != ".c" && ext != ".cpp" && ext != ".h" && ext != ".hpp")) {
            continue;
        }
        filesScanned++;
        const std::string s = ReadFile(it->path());
        for (size_t p = s.find("CVarGet"); p != std::string::npos; p = s.find("CVarGet", p + 1)) {
            size_t q = p + 7;
            while (q < s.size() && std::isalpha((unsigned char) s[q])) {
                q++;
            }
            if (q + 1 >= s.size() || s[q] != '(' || s[q + 1] != '"') {
                continue; // not a call with a literal name (e.g. CVarGetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0))
            }
            const size_t nameEnd = s.find('"', q + 2);
            const size_t close = s.find(')', nameEnd);
            if (nameEnd == std::string::npos || close == std::string::npos) {
                continue;
            }
            CVarRead r;
            r.accessor = s.substr(p + 7, q - (p + 7));
            r.name = s.substr(q + 2, nameEnd - (q + 2));
            std::string d = s.substr(nameEnd + 1, close - (nameEnd + 1));
            const size_t comma = d.find(',');
            d = comma == std::string::npos ? "" : d.substr(comma + 1);
            d.erase(0, d.find_first_not_of(" \t\n"));
            d.erase(d.find_last_not_of(" \t\n") + 1);
            r.defaultText = d;
            const long line = (long) std::count(s.begin(), s.begin() + (long) p, '\n') + 1;
            r.where = path.substr(path.find("/src/") + 1) + ":" + std::to_string(line);
            out.push_back(r);
        }
    }
    return out;
}

bool DefaultMatches(const std::string& text, float want) {
    if (text == "false") {
        return want == 0.0f;
    }
    if (text == "true") {
        return want == 1.0f;
    }
    char* end = nullptr;
    const float v = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || (*end != '\0' && !((*end == 'f' || *end == 'F') && end[1] == '\0'))) {
        return false;
    }
    return v == want;
}

} // namespace

// The classification in OracleIdentityHash.h stays complete as the code changes: a new CVar read anywhere in src/ fails
// here until it is classified sim (hashed) or excluded (with a reason); a classified name nobody reads any more fails too.
static void test_cvar_sweep() {
    size_t files = 0;
    const std::vector<CVarRead> reads = ScanCVarReads(files);
    // Positive control: the scan can see (measured 2026-10-25: 159 literal reads, 62 names, in src/ at netplay 98d07383f).
    CHECK(files > 500);
    CHECK(reads.size() > 100);
    std::set<std::string> seen;
    for (const CVarRead& r : reads) {
        seen.insert(r.name);
    }
    CHECK(seen.count("gHarderCPU") == 1 && seen.count("gNoCulling") == 1);

    for (const CVarRead& r : reads) {
        const oracle::CVarSpec* spec = nullptr;
        for (const oracle::CVarSpec& s : oracle::kSimCVars) {
            if (r.name == s.name) {
                spec = &s;
            }
        }
        bool isExcluded = false;
        for (const oracle::CVarExclusion& e : oracle::kExcludedCVars) {
            isExcluded = isExcluded || r.name == e.name;
        }
        if (spec == nullptr && !isExcluded) {
            std::fprintf(stderr, "  unclassified CVar %s (%s)\n", r.name.c_str(), r.where.c_str());
        }
        CHECK(spec != nullptr || isExcluded);
        if (spec != nullptr) {
            // The hash reads it the way the game does: same accessor, same default at EVERY call site (that equality is
            // what makes "unset" and "set to the default" indistinguishable to the game, so isSet need not be hashed).
            const char* want = spec->kind == oracle::CVarKind::Float ? "Float" : "Integer";
            const bool ok = r.accessor == want && DefaultMatches(r.defaultText, spec->defaultValue);
            if (!ok) {
                std::fprintf(stderr, "  %s read as CVarGet%s(…, %s) at %s\n", r.name.c_str(), r.accessor.c_str(),
                             r.defaultText.c_str(), r.where.c_str());
            }
            CHECK(ok);
        }
    }
    for (const oracle::CVarSpec& s : oracle::kSimCVars) {
        if (seen.count(s.name) == 0) {
            std::fprintf(stderr, "  stale sim CVar %s (no reader in src/)\n", s.name);
        }
        CHECK(seen.count(s.name) == 1);
    }
    for (const oracle::CVarExclusion& e : oracle::kExcludedCVars) {
        CHECK(seen.count(e.name) == 1);
    }
}

static void test_cli_probe_flags() {
    OracleConfig cfg;
    CHECK(Parse({ "--oracle-record", "x", "--oracle-probe-straddle" }, cfg) == -1 && cfg.probeStraddle);
    CHECK(!cfg.probeAudioSerial && !cfg.probeSelftest);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-probe-straddle", "--oracle-probe-audio-serial" }, cfg) == -1 &&
          cfg.probeStraddle && cfg.probeAudioSerial);
    CHECK(Parse({ "--oracle-record", "x", "--oracle-probe-audio-serial" }, cfg) == -1 && cfg.probeAudioSerial);
    // The selftest flips gRandomSeed16 around the re-hash: never in a recording (FASE 1.4.5), and it needs the straddle.
    CHECK(Parse({ "--oracle-record", "x", "--oracle-probe-straddle", "--oracle-probe-selftest" }, cfg) == 2);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-straddle", "--oracle-probe-selftest" }, cfg) == -1 &&
          cfg.probeStraddle && cfg.probeSelftest);
    CHECK(Parse({ "--oracle-replay", "r", "--oracle-probe-selftest" }, cfg) == 2);
    // A probe with the oracle off would silently do nothing.
    CHECK(Parse({ "--oracle-probe-straddle" }, cfg) == 2);
    CHECK(Parse({ "--oracle-probe-audio-serial" }, cfg) == 2);
    CHECK(Parse({ "--oracle-probe-selftest" }, cfg) == 2);
    // Flags take no value: the next token is parsed on its own.
    CHECK(Parse({ "--oracle-probe-straddle", "--oracle-record", "x" }, cfg) == -1 && cfg.probeStraddle &&
          cfg.mode == OracleMode::Record);
    // A probe next to a no-window mode is refused, never silently ignored (same rule as 1.3's review fix 6), including
    // 1.3's --oracle-probe-tick-at.
    const auto rec = TempPath("cli-probe-info.mk64rec");
    CHECK(WriteSample(rec, 2, false, true));
    CHECK(Parse({ "--oracle-info", rec.string() }, cfg) == 0); // control: the info itself is fine
    CHECK(Parse({ "--oracle-info", rec.string(), "--oracle-probe-straddle" }, cfg) == 2);
    CHECK(Parse({ "--oracle-info", rec.string(), "--oracle-probe-audio-serial" }, cfg) == 2);
    CHECK(Parse({ "--oracle-info", rec.string(), "--oracle-probe-tick-at", "5" }, cfg) == 2);
    CHECK(Parse({ "--oracle-verify", "a", "b", "--oracle-probe-straddle" }, cfg) == 2);
}

// ---------------------------------------------------------------------------------------------------------------------
// FASE 1.5 — replay mode. Written before OracleReplay.cpp / OracleArchiveHash.cpp (RED first).

namespace {

// Mirrors OSContPad's field NAMES, not its layout: ApplyRecordedPads is a template, so the test proves "only the five named
// fields are written" without pulling libultraship into this target.
struct FakePad {
    uint16_t button;
    int8_t stick_x, stick_y;
    uint8_t err_no;
    float gyro_x, gyro_y;
    int8_t right_stick_x, right_stick_y;
};

oracle::Header BaseHeader() {
    oracle::Header h{};
    std::snprintf(h.buildVersion, sizeof h.buildVersion, "1.0.0");
    std::snprintf(h.compiler, sizeof h.compiler, "clang 17.0.0");
    std::snprintf(h.buildType, sizeof h.buildType, "Release");
    std::snprintf(h.platform, sizeof h.platform, "macos-arm64");
    h.gameArchiveHash = 0x11;
    h.portArchiveHash = 0x22;
    h.binaryHash = 0x33;
    h.modListHash = 0x44;
    h.configHash = 0x55;
    h.tickLogic = 2;
    h.fpWord = 0x1F80;
    h.skipIntro = 3;
    h.seed = 0x1234;
    return h;
}

// One refusal field each, in the order the user sees them: toolchain identity, then archives and simulation parameters.
struct RefusalCase {
    const char* field;
    void (*mutate)(oracle::Header&);
};
const RefusalCase kRefusalCases[] = {
    { "compiler", [](oracle::Header& h) { std::snprintf(h.compiler, sizeof h.compiler, "clang 18.0.0"); } },
    { "buildType", [](oracle::Header& h) { std::snprintf(h.buildType, sizeof h.buildType, "Debug"); } },
    { "platform", [](oracle::Header& h) { std::snprintf(h.platform, sizeof h.platform, "windows-x64"); } },
    { "gameArchiveHash", [](oracle::Header& h) { h.gameArchiveHash = 0x99; } },
    { "portArchiveHash", [](oracle::Header& h) { h.portArchiveHash = 0x99; } },
    { "tickLogic", [](oracle::Header& h) { h.tickLogic = 3; } },
    { "fpWord", [](oracle::Header& h) { h.fpWord = 0x9F80; } },
    { "configHash", [](oracle::Header& h) { h.configHash = 0x99; } },
    { "modListHash", [](oracle::Header& h) { h.modListHash = 0x99; } },
    { "skipIntro", [](oracle::Header& h) { h.skipIntro = 0; } },
};

} // namespace

static void test_replay_compat() {
    const oracle::Header live = BaseHeader();
    {
        const oracle::Compat c = oracle::CheckCompatible(BaseHeader(), live);
        CHECK(c.ok && c.field.empty() && c.notes.empty() && !c.archivesUnchecked);
    }
    // The TODO's named case: the recording came from a session that was not pinned at 2.
    {
        oracle::Header rec = BaseHeader();
        rec.tickLogic = 3;
        const oracle::Compat c = oracle::CheckCompatible(rec, live);
        CHECK(!c.ok && c.field == "tickLogic" && c.recorded == "0x0000000000000003" && c.live == "0x0000000000000002");
    }
    // Every refusal field is named on its own (one mutation per field, nothing else differs).
    for (const RefusalCase& rc : kRefusalCases) {
        oracle::Header rec = BaseHeader();
        rc.mutate(rec);
        const oracle::Compat c = oracle::CheckCompatible(rec, live);
        if (c.ok || c.field != rc.field) {
            std::fprintf(stderr, "  refusal case %s: ok=%d field=%s\n", rc.field, c.ok ? 1 : 0, c.field.c_str());
        }
        CHECK(!c.ok && c.field == rc.field);
    }
    // A string field reports both values as text.
    {
        oracle::Header rec = BaseHeader();
        std::snprintf(rec.buildType, sizeof rec.buildType, "Debug");
        const oracle::Compat c = oracle::CheckCompatible(rec, live);
        CHECK(c.field == "buildType" && c.recorded == "Debug" && c.live == "Release");
    }
    // Order: with fields k..end all differing, field k is named — the whole user-visible order, and the first refusal wins.
    for (size_t k = 0; k < std::size(kRefusalCases); k++) {
        oracle::Header rec = BaseHeader();
        for (size_t j = k; j < std::size(kRefusalCases); j++) {
            kRefusalCases[j].mutate(rec);
        }
        CHECK(oracle::CheckCompatible(rec, live).field == kRefusalCases[k].field);
    }
    // The seed is NOT compared: FASE 1.7 writes gRandomSeed16 = header.seed.
    {
        oracle::Header rec = BaseHeader();
        rec.seed = 0x4321;
        const oracle::Compat c = oracle::CheckCompatible(rec, live);
        CHECK(c.ok && c.notes.empty());
    }
    // buildVersion (git describe) and binaryHash (every rebuild) are informational: a LATER commit's binary replaying the
    // committed baseline is accepted, with one note each.
    {
        oracle::Header rec = BaseHeader();
        std::snprintf(rec.buildVersion, sizeof rec.buildVersion, "base-5073d2e-7-gabc1234");
        rec.binaryHash = 0x99;
        const oracle::Compat c = oracle::CheckCompatible(rec, live);
        CHECK(c.ok && c.notes.size() == 2);
        CHECK(c.notes.size() == 2 && Contains(c.notes[0], "buildVersion") && Contains(c.notes[0], "base-5073d2e-7-gabc1234") &&
              Contains(c.notes[1], "binaryHash"));
        rec = BaseHeader();
        rec.binaryHash = 0x99;
        CHECK(oracle::CheckCompatible(rec, live).notes.size() == 1);
    }
    // Equal zero archive hashes prove nothing: accepted, flagged unchecked (either archive).
    {
        oracle::Header rec = BaseHeader(), lv = BaseHeader();
        rec.gameArchiveHash = lv.gameArchiveHash = 0;
        const oracle::Compat c = oracle::CheckCompatible(rec, lv);
        CHECK(c.ok && c.archivesUnchecked);
        rec = BaseHeader();
        lv = BaseHeader();
        rec.portArchiveHash = lv.portArchiveHash = 0;
        CHECK(oracle::CheckCompatible(rec, lv).ok && oracle::CheckCompatible(rec, lv).archivesUnchecked);
        // …but a zero against a real hash is a difference, not "unchecked".
        rec = BaseHeader();
        rec.gameArchiveHash = 0;
        const oracle::Compat d = oracle::CheckCompatible(rec, live);
        CHECK(!d.ok && d.field == "gameArchiveHash" && !d.archivesUnchecked);
    }
    // Fixed char[] fields: a field filled to its full size has no NUL (strnlen-bounded), and bytes after a NUL are not text.
    {
        oracle::Header rec = BaseHeader(), lv = BaseHeader();
        std::memset(rec.compiler, 'x', sizeof rec.compiler);
        std::memset(lv.compiler, 'x', sizeof lv.compiler);
        CHECK(oracle::CheckCompatible(rec, lv).ok);
        lv.compiler[sizeof lv.compiler - 1] = 'y';
        const oracle::Compat c = oracle::CheckCompatible(rec, lv);
        CHECK(!c.ok && c.field == "compiler" && c.recorded.size() == sizeof rec.compiler);
        rec = BaseHeader();
        std::memcpy(rec.platform + 12, "junk", 4); // after "macos-arm64\0"
        CHECK(oracle::CheckCompatible(rec, live).ok);
    }
}

static void test_replay_cursor() {
    oracle::Recording r;
    const oracle::PadsRecord* p = nullptr;
    uint32_t got = 0;
    {
        oracle::ReplayCursor c(r); // empty recording: End at iteration 0
        CHECK(c.next(0, &p, &got) == oracle::Step::End && c.fed() == 0);
    }
    for (uint32_t i = 0; i < 3; i++) {
        r.pads.push_back({ oracle::kRecPads, i, {} });
    }
    {
        oracle::ReplayCursor c(r);
        for (uint32_t i = 0; i < 3; i++) {
            p = nullptr;
            CHECK(c.next(i, &p, &got) == oracle::Step::Feed && p != nullptr && p->iteration == i);
        }
        CHECK(c.next(3, &p, &got) == oracle::Step::End && c.fed() == 3);
        CHECK(c.next(4, &p, &got) == oracle::Step::End && c.fed() == 3); // stays at End
    }
    r.pads[1].iteration = 5; // a hole in the file
    {
        oracle::ReplayCursor c(r);
        CHECK(c.next(0, &p, &got) == oracle::Step::Feed);
        got = 0;
        CHECK(c.next(1, &p, &got) == oracle::Step::Misaligned && got == 5);
        got = 0;
        CHECK(c.next(1, &p, &got) == oracle::Step::Misaligned && got == 5 && c.fed() == 1); // never skips past it
    }
    r.pads[1].iteration = 0; // a repeated iteration
    {
        oracle::ReplayCursor c(r);
        CHECK(c.next(0, &p, &got) == oracle::Step::Feed);
        CHECK(c.next(1, &p, &got) == oracle::Step::Misaligned && got == 0);
    }
}

static void test_apply_pads_keeps_gyro() {
    oracle::PadsRecord r{ oracle::kRecPads, 0, {} };
    r.pads[1] = { 0x8004, -12, 77, 5, -6 };
    r.pads[3] = { 0x0001, 1, 2, 3, 4 };
    FakePad live[4] = {};
    for (FakePad& f : live) {
        f.button = 0xFFFF; // a live controller held during the replay: overwrite, never OR
        f.stick_x = 100;
        f.right_stick_y = 100;
    }
    live[1].gyro_x = 1.5f;
    live[1].gyro_y = -2.5f;
    live[1].err_no = 9;
    oracle::ApplyRecordedPads(r, live);
    CHECK(live[1].button == 0x8004 && live[1].stick_x == -12 && live[1].stick_y == 77 && live[1].right_stick_x == 5 &&
          live[1].right_stick_y == -6);
    // Untouched by design; 0x8004 keeps bit 0x4 (C-down), so update_controller's Z_TRIG alias still happens later.
    CHECK(live[1].gyro_x == 1.5f && live[1].gyro_y == -2.5f && live[1].err_no == 9);
    CHECK(live[0].button == 0 && live[0].stick_x == 0 && live[0].right_stick_y == 0); // every slot, slot 0 included
    CHECK(live[3].button == 0x0001 && live[3].stick_x == 1 && live[3].stick_y == 2 && live[3].right_stick_x == 3 &&
          live[3].right_stick_y == 4);
}

static void test_replay_hash_log_path() {
    const auto rec = TempPath("replay-path.mk64rec");
    CHECK(WriteSample(rec, 2, false, true));
    const std::string recPath = rec.string();
    const std::string def = recPath + ".mk64hash";
    std::error_code ec;
    std::filesystem::remove(def, ec);
    CHECK(oracle::CheckReplayHashLogPath(recPath, def, true).empty()); // the default does not exist yet
    { std::ofstream(def) << "MK64HASH 1\n"; }                           // the recording run's log
    const std::string why = oracle::CheckReplayHashLogPath(recPath, def, true);
    CHECK(Contains(why, "already exists") && Contains(why, "--oracle-hash-log"));
    CHECK(oracle::CheckReplayHashLogPath(recPath, def, false).empty()); // an explicit --oracle-hash-log always wins
    // …except onto the recording itself (it was just loaded; the log would truncate it), by any spelling.
    CHECK(Contains(oracle::CheckReplayHashLogPath(recPath, recPath, false), "recording"));
    const std::string other = (rec.parent_path() / "." / rec.filename()).string();
    CHECK(other != recPath && Contains(oracle::CheckReplayHashLogPath(recPath, other, false), "recording"));
    CHECK(oracle::CheckReplayHashLogPath(recPath, TempPath("replay-path.out.mk64hash").string(), false).empty());
    // The parser says whether the path was defaulted.
    OracleConfig cfg;
    CHECK(Parse({ "--oracle-replay", "r.mk64rec" }, cfg) == -1 && cfg.hashLogDefaulted);
    CHECK(Parse({ "--oracle-replay", "r.mk64rec", "--oracle-hash-log", "r.mk64rec.mk64hash" }, cfg) == -1 &&
          !cfg.hashLogDefaulted && cfg.hashLogPath == "r.mk64rec.mk64hash");
    CHECK(Parse({}, cfg) == -1 && !cfg.hashLogDefaulted);
}

namespace {

// Independent of the code under test: a bitwise CRC-32 (IEEE, reflected 0xEDB88320) and a minimal zip writer (stored
// entries), so the archives below are real zips whose only difference is the one each case names.
uint32_t TestCrc32(const std::string& s) {
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char b : s) {
        c ^= b;
        for (int k = 0; k < 8; k++) {
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
    }
    return ~c;
}

struct ZipEntry {
    std::string name, data;
    bool deflated = false;           // method 8 with a compressed size != the real size (the parser never inflates)
    std::string extra, comment = ""; // per-entry extra field and comment (central directory)
};

void Le(std::string& out, uint64_t v, int n) {
    for (int i = 0; i < n; i++) {
        out.push_back((char) (uint8_t) (v >> (8 * i)));
    }
}

// dosTime/dosDate stamp every entry (local header and central directory), like the packer's pack time.
std::string MakeZip(const std::vector<ZipEntry>& entries, uint16_t dosTime, uint16_t dosDate, const std::string& comment = "",
                    int64_t totalOverride = -1) {
    std::string out, cd;
    for (const ZipEntry& e : entries) {
        const uint32_t crc = TestCrc32(e.data);
        const uint32_t offset = (uint32_t) out.size();
        // A "deflated" entry stores a payload of another length: only the compressed size moves, never the content.
        const std::string payload = e.deflated ? std::string(e.data.size() + 3, 'c') : e.data;
        const uint16_t method = e.deflated ? 8 : 0;
        Le(out, 0x04034b50, 4);
        Le(out, 20, 2);
        Le(out, 0, 2);
        Le(out, method, 2);
        Le(out, dosTime, 2);
        Le(out, dosDate, 2);
        Le(out, crc, 4);
        Le(out, payload.size(), 4);
        Le(out, e.data.size(), 4);
        Le(out, e.name.size(), 2);
        Le(out, 0, 2);
        out += e.name;
        out += payload;
        Le(cd, 0x02014b50, 4);
        Le(cd, 20, 2);
        Le(cd, 20, 2);
        Le(cd, 0, 2);
        Le(cd, method, 2);
        Le(cd, dosTime, 2);
        Le(cd, dosDate, 2);
        Le(cd, crc, 4);
        Le(cd, payload.size(), 4);
        Le(cd, e.data.size(), 4);
        Le(cd, e.name.size(), 2);
        Le(cd, e.extra.size(), 2);
        Le(cd, e.comment.size(), 2);
        Le(cd, 0, 2);
        Le(cd, 0, 2);
        Le(cd, 0, 4);
        Le(cd, offset, 4);
        cd += e.name;
        cd += e.extra;
        cd += e.comment;
    }
    const uint32_t cdOffset = (uint32_t) out.size();
    out += cd;
    const uint64_t total = totalOverride >= 0 ? (uint64_t) totalOverride : entries.size();
    Le(out, 0x06054b50, 4);
    Le(out, 0, 2);
    Le(out, 0, 2);
    Le(out, total, 2);
    Le(out, total, 2);
    Le(out, cd.size(), 4);
    Le(out, cdOffset, 4);
    Le(out, comment.size(), 2);
    out += comment;
    return out;
}

std::filesystem::path WriteFile(const char* name, const std::string& bytes) {
    const auto p = TempPath(name);
    std::ofstream(p, std::ios::binary).write(bytes.data(), (std::streamsize) bytes.size());
    return p;
}

uint64_t RawFnv(const std::string& s) {
    oracle::Hasher h;
    h.bytes(s.data(), s.size());
    return h.value();
}

} // namespace

static void test_zip_content_hash() {
    const std::vector<ZipEntry> base = { { "a.bin", "hello" }, { "dir/b.bin", "world" } };
    const std::string zA = MakeZip(base, 0x6000, 0x5B5A);
    const std::string zStamp = MakeZip(base, 0x7421, 0x5B5B); // the same content packed at another time
    CHECK(zA.size() == zStamp.size() && RawFnv(zA) != RawFnv(zStamp)); // control: the raw bytes DO differ
    const oracle::ZipContent a = oracle::HashZipContent(WriteFile("zip-a.o2r", zA).string());
    const oracle::ZipContent stamp = oracle::HashZipContent(WriteFile("zip-stamp.o2r", zStamp).string());
    CHECK(a.error.empty() && a.entries == 2 && a.hash != 0);
    // Known answer, computed independently (python3: zlib.crc32 + struct.pack('<I…') over the canonical form documented in
    // OracleArchiveHash.h) — pins the formula, not only its properties.
    CHECK(a.hash == 0xeb69cd7d23b5e055ull);
    CHECK(stamp.error.empty() && stamp.hash == a.hash); // timestamps are not content
    // One entry's bytes change → its CRC changes → a different hash.
    const oracle::ZipContent bytes =
        oracle::HashZipContent(WriteFile("zip-bytes.o2r", MakeZip({ { "a.bin", "hello" }, { "dir/b.bin", "worle" } }, 0x6000, 0x5B5A)).string());
    CHECK(bytes.error.empty() && bytes.hash != a.hash);
    // A renamed entry → different.
    const oracle::ZipContent renamed =
        oracle::HashZipContent(WriteFile("zip-renamed.o2r", MakeZip({ { "c.bin", "hello" }, { "dir/b.bin", "world" } }, 0x6000, 0x5B5A)).string());
    CHECK(renamed.error.empty() && renamed.hash != a.hash);
    // Names and contents stay paired: swapping the two contents is a different archive.
    const oracle::ZipContent swapped =
        oracle::HashZipContent(WriteFile("zip-swapped.o2r", MakeZip({ { "a.bin", "world" }, { "dir/b.bin", "hello" } }, 0x6000, 0x5B5A)).string());
    CHECK(swapped.error.empty() && swapped.hash != a.hash);
    // Entry order in the file is not content (the packer may walk directories in any order).
    const oracle::ZipContent reversed =
        oracle::HashZipContent(WriteFile("zip-reversed.o2r", MakeZip({ base[1], base[0] }, 0x6000, 0x5B5A)).string());
    CHECK(reversed.error.empty() && reversed.hash == a.hash);
    // An archive comment is found behind (the end record is searched backwards) and is not content.
    const oracle::ZipContent commented =
        oracle::HashZipContent(WriteFile("zip-comment.o2r", MakeZip(base, 0x6000, 0x5B5A, "packed by test")).string());
    CHECK(commented.error.empty() && commented.hash == a.hash);
    // Compression is not content: the real packer deflates every entry (369/369), and a zlib version or level change moves
    // only the compressed size — the uncompressed size is what is hashed.
    {
        const std::vector<ZipEntry> deflated = { { "a.bin", "hello", true }, { "dir/b.bin", "world", true } };
        const oracle::ZipContent d = oracle::HashZipContent(WriteFile("zip-deflated.o2r", MakeZip(deflated, 0x6000, 0x5B5A)).string());
        CHECK(d.error.empty() && d.hash == a.hash);
    }
    // Per-entry extra fields and comments are skipped by their declared lengths, and are not content.
    {
        const std::vector<ZipEntry> extras = { { "a.bin", "hello", false, std::string("\x55\x54\x05\x00\x01\x00\x00\x00\x00", 9), "c1" },
                                               { "dir/b.bin", "world", false, "", "second entry comment" } };
        const oracle::ZipContent x = oracle::HashZipContent(WriteFile("zip-extras.o2r", MakeZip(extras, 0x6000, 0x5B5A)).string());
        CHECK(x.error.empty() && x.entries == 2 && x.hash == a.hash);
    }
    // An empty archive is valid and distinct from a failure (0).
    const oracle::ZipContent empty = oracle::HashZipContent(WriteFile("zip-empty.o2r", MakeZip({}, 0, 0)).string());
    CHECK(empty.error.empty() && empty.entries == 0 && empty.hash == 0x4d25767f9dce13f5ull); // FNV of U32(0)
    // Failures → hash 0 and the reason of THAT check (each case is caught where it should be, not by a later check that
    // happens to see the same corruption), never a crash or an out-of-bounds read.
    auto fails = [](const oracle::ZipContent& z, const char* why) {
        if (z.hash != 0 || !Contains(z.error, why)) {
            std::fprintf(stderr, "  zip failure case: hash=0x%016llx error=\"%s\" (want \"%s\")\n", (unsigned long long) z.hash,
                         z.error.c_str(), why);
        }
        return z.hash == 0 && Contains(z.error, why);
    };
    CHECK(fails(oracle::HashZipContent((std::filesystem::temp_directory_path() / "mk64-no-such-dir" / "x.o2r").string()), "cannot open"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-not.o2r", std::string(100, 'z')).string()), "not a zip"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-short.o2r", "PK").string()), "too short"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-cut.o2r", zA.substr(0, zA.size() - 10)).string()), "not a zip"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-count.o2r", MakeZip(base, 0x6000, 0x5B5A, "", 3)).string()), "truncated or malformed"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-count-low.o2r", MakeZip(base, 0x6000, 0x5B5A, "", 1)).string()), "more than"));
    CHECK(fails(oracle::HashZipContent(WriteFile("zip-zip64.o2r", MakeZip(base, 0x6000, 0x5B5A, "", 0xFFFF)).string()), "ZIP64"));
    {
        std::string bad = zA; // the second central-directory entry claims a 0xFFFF-byte name
        const size_t cd2 = bad.rfind(std::string("\x50\x4b\x01\x02", 4));
        CHECK(cd2 != std::string::npos);
        if (cd2 != std::string::npos) {
            bad[cd2 + 28] = (char) 0xFF;
            bad[cd2 + 29] = (char) 0xFF;
        }
        CHECK(fails(oracle::HashZipContent(WriteFile("zip-name-overrun.o2r", bad).string()), "runs past"));
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// FASE 1.4b — the coverage critic. The member list is re-derived FROM SOURCE TEXT (the class bodies of
// kCoverageHeaders), as test_cvar_sweep re-runs the CVar sweep; the compiler's record layouts are the second method
// (1.4b/inventory.py).

namespace {

struct SrcMember {
    std::string cls;
    std::string member;
    std::string where; // file:line
    bool isStatic;
};

struct SrcClass {
    std::string name;
    std::string where;
    std::vector<std::string> bases;
};

// Comments and the CONTENTS of string / char literals become spaces (newlines kept, so line numbers stay true), and so
// do preprocessor lines. Line comments end at the newline: `//[^\n]*`, never a dot-all `//.*` (that once ate a whole
// file).
std::string StripSource(const std::string& s) {
    std::string o = s;
    const size_t n = o.size();
    bool lineStart = true;
    size_t i = 0;
    while (i < n) {
        const char c = o[i];
        if (c == '/' && i + 1 < n && o[i + 1] == '/') {
            while (i < n && o[i] != '\n') {
                o[i++] = ' ';
            }
        } else if (c == '/' && i + 1 < n && o[i + 1] == '*') {
            o[i] = o[i + 1] = ' ';
            i += 2;
            while (i < n && !(o[i] == '*' && i + 1 < n && o[i + 1] == '/')) {
                if (o[i] != '\n') {
                    o[i] = ' ';
                }
                i++;
            }
            if (i + 1 < n) {
                o[i] = o[i + 1] = ' ';
                i += 2;
            }
        } else if (c == '"' || c == '\'') {
            i++;
            while (i < n && o[i] != c && o[i] != '\n') {
                if (o[i] == '\\' && i + 1 < n) {
                    o[i++] = ' ';
                }
                o[i++] = ' ';
            }
            i++;
            lineStart = false;
            continue;
        } else if (c == '#' && lineStart) {
            while (i < n && o[i] != '\n') {
                if (o[i] == '\\' && i + 1 < n && o[i + 1] == '\n') {
                    o[i] = ' ';
                    i += 2;
                    continue;
                }
                o[i++] = ' ';
            }
        } else {
            if (c == '\n') {
                lineStart = true;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                lineStart = false;
            }
            i++;
            continue;
        }
    }
    return o;
}

std::string Trim(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) {
        return "";
    }
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

bool IsIdentChar(char c) {
    return std::isalnum((unsigned char) c) || c == '_';
}

// Identifier tokens of a declaration (keywords included; `::`-qualified names split).
std::vector<std::string> Idents(const std::string& s) {
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        if (IsIdentChar(s[i]) && !std::isdigit((unsigned char) s[i])) {
            size_t j = i;
            while (j < s.size() && IsIdentChar(s[j])) {
                j++;
            }
            out.push_back(s.substr(i, j - i));
            i = j;
        } else {
            i++;
        }
    }
    return out;
}

bool StartsWithWord(const std::string& s, const char* w) {
    const size_t n = std::strlen(w);
    return s.compare(0, n, w) == 0 && (s.size() == n || !IsIdentChar(s[n]));
}

size_t MatchingClose(const std::string& t, size_t open) { // t[open] is '{', '(' or '['
    const char o = t[open];
    const char c = o == '{' ? '}' : o == '(' ? ')' : ']';
    int depth = 0;
    for (size_t i = open; i < t.size(); i++) {
        if (t[i] == o) {
            depth++;
        } else if (t[i] == c && --depth == 0) {
            return i;
        }
    }
    return std::string::npos;
}

// Top-level split on `sep` outside () [] {} and template <>.
std::vector<std::string> SplitTop(const std::string& s, char sep) {
    std::vector<std::string> out;
    int paren = 0, angle = 0;
    std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        const char c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            paren++;
        } else if (c == ')' || c == ']' || c == '}') {
            paren--;
        } else if (c == '<' && i > 0 && (IsIdentChar(s[i - 1]) || s[i - 1] == ' ') && paren == 0) {
            angle++;
        } else if (c == '>' && angle > 0 && paren == 0) {
            angle--;
        }
        if (c == sep && paren == 0 && angle == 0) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

// Position of the first top-level occurrence of any char in `chars` (outside () [] {} and template <>), or npos.
size_t FindTop(const std::string& s, const char* chars) {
    int paren = 0, angle = 0;
    for (size_t i = 0; i < s.size(); i++) {
        const char c = s[i];
        if (paren == 0 && angle == 0 && std::strchr(chars, c) != nullptr) {
            return i;
        }
        if (c == '(' || c == '[' || c == '{') {
            paren++;
        } else if (c == ')' || c == ']' || c == '}') {
            paren--;
        } else if (c == '<' && i > 0 && IsIdentChar(s[i - 1]) && paren == 0) {
            angle++;
        } else if (c == '>' && angle > 0 && paren == 0) {
            angle--;
        }
    }
    return std::string::npos;
}

// Leading object-like macros (EXTERN_C_START, ALIGNED8, …): ALL-CAPS words not followed by '(' are dropped.
std::string DropMacros(std::string h) {
    for (;;) {
        h = Trim(h);
        const std::vector<std::string> w = Idents(h);
        if (w.empty() || h.find(w[0]) != 0 || w[0].size() < 2 ||
            std::any_of(w[0].begin(), w[0].end(), [](char ch) { return std::islower((unsigned char) ch); })) {
            return h;
        }
        const std::string rest = Trim(h.substr(w[0].size()));
        if (!rest.empty() && rest[0] == '(') {
            return h;
        }
        h = rest;
    }
}

struct CoverageScan {
    std::vector<SrcClass> classes;
    std::vector<SrcMember> members;
    std::string file;
    const std::string* text = nullptr;

    std::string Where(size_t pos) const {
        return file + ":" + std::to_string((long) std::count(text->begin(), text->begin() + (long) pos, '\n') + 1);
    }

    // One data-member declaration statement (the brace bodies of nested types already cut out).
    void Declaration(const std::string& cls, std::string stmt, size_t pos) {
        stmt = DropMacros(stmt);
        for (const char* skip : { "using", "typedef", "friend", "template", "static_assert", "namespace" }) {
            if (StartsWithWord(stmt, skip)) {
                return;
            }
        }
        if (StartsWithWord(stmt, "enum") && stmt.find("{}") != std::string::npos) {
            return; // an enum definition (`enum MusicSeq Sequence;` without a body IS a member)
        }
        for (const std::string& w : Idents(stmt)) {
            if (w == "operator") {
                return; // operator==(…): its '=' would otherwise read as an initializer
            }
        }
        if (stmt.empty() || stmt == "{}") {
            return;
        }
        const std::string head = stmt.substr(0, FindTop(stmt, "={"));
        const std::vector<std::string> words = Idents(head);
        bool isStatic = false;
        for (const std::string& w : words) {
            isStatic = isStatic || w == "static";
        }
        if (isStatic) {
            // static constexpr / static const T: not state. But `static const char* tbl[]` IS mutable — the const binds
            // the pointee, so it only makes the object const when no '*' follows the last `const`.
            size_t lastConst = std::string::npos;
            for (const std::string& w : words) {
                if (w == "constexpr" || w == "consteval" || w == "constinit") {
                    return;
                }
            }
            for (size_t k = head.find("const"); k != std::string::npos; k = head.find("const", k + 5)) {
                if ((k == 0 || !IsIdentChar(head[k - 1])) && (k + 5 >= head.size() || !IsIdentChar(head[k + 5]))) {
                    lastConst = k;
                }
            }
            if (lastConst != std::string::npos && head.find('*', lastConst) == std::string::npos) {
                return;
            }
        }
        // A function declaration: a top-level '(' before any initializer ('=' or '{').
        const size_t paren = FindTop(stmt, "(");
        const size_t init = FindTop(stmt, "={");
        if (paren != std::string::npos && (init == std::string::npos || paren < init)) {
            return;
        }
        // A forward declaration (`class Foo;`, `struct Bar;`).
        if ((StartsWithWord(stmt, "class") || StartsWithWord(stmt, "struct")) && words.size() == 2) {
            return;
        }
        const std::vector<std::string> decls = SplitTop(init == std::string::npos ? stmt : stmt.substr(0, init), ',');
        for (std::string d : decls) {
            d = d.substr(0, FindTop(d, "={"));
            const size_t bracket = FindTop(d, "[");
            if (bracket != std::string::npos) {
                d = d.substr(0, bracket);
            }
            const size_t colon = d.rfind(':');
            const bool bitField = colon != std::string::npos && colon > 0 && d[colon - 1] != ':' &&
                                  (colon + 1 >= d.size() || d[colon + 1] != ':');
            if (bitField) {
                d = d.substr(0, colon); // bit-field width
            }
            const std::vector<std::string> ids = Idents(d);
            if (ids.empty()) {
                continue;
            }
            members.push_back(SrcMember{ cls, ids.back(), Where(pos), isStatic });
        }
    }

    // Scans [b, e) of the stripped text. `cls` is the enclosing class ("" at namespace / file scope).
    void Scope(size_t b, size_t e, const std::string& cls) {
        const std::string& t = *text;
        std::string stmt;
        size_t stmtPos = b;
        bool stmtIsType = false; // the statement defined a type: what follows its body are declarators
        bool stmtIsTypedef = false; // …or, after a typedef, the type's name (no member)
        std::string typeName;
        for (size_t i = b; i < e; i++) {
            const char c = t[i];
            if (stmt.empty() && std::isspace((unsigned char) c)) {
                stmtPos = i + 1;
                continue;
            }
            if (c == '{') {
                const size_t close = MatchingClose(t, i);
                if (close == std::string::npos || close > e) {
                    return;
                }
                const std::string head = DropMacros(stmt);
                if (StartsWithWord(head, "namespace") || StartsWithWord(head, "extern")) {
                    Scope(i + 1, close, cls);
                    stmt.clear();
                    i = close;
                    continue;
                }
                std::string h = head;
                if (StartsWithWord(h, "template")) {
                    const size_t gt = h.find('>');
                    h = gt == std::string::npos ? h : Trim(h.substr(gt + 1));
                }
                const bool isTypedef = StartsWithWord(h, "typedef");
                if (isTypedef) {
                    h = Trim(h.substr(7));
                }
                if (StartsWithWord(h, "class") || StartsWithWord(h, "struct") || StartsWithWord(h, "union")) {
                    // class|struct|union [alignas(..)] [Name] [final] [: bases]
                    const size_t colon = [&] {
                        for (size_t k = 0; k + 1 < h.size(); k++) {
                            if (h[k] == ':' && h[k + 1] != ':' && (k == 0 || h[k - 1] != ':')) {
                                return k;
                            }
                        }
                        return std::string::npos;
                    }();
                    std::vector<std::string> ids = Idents(colon == std::string::npos ? h : h.substr(0, colon));
                    ids.erase(std::remove_if(ids.begin(), ids.end(),
                                             [](const std::string& w) {
                                                 return w == "class" || w == "struct" || w == "union" || w == "final" ||
                                                        w == "alignas";
                                             }),
                              ids.end());
                    std::string name = ids.empty() ? "" : ids.back();
                    if (name.empty() && isTypedef) {
                        const size_t semi = t.find(';', close);
                        const std::vector<std::string> after = Idents(t.substr(close + 1, semi - close - 1));
                        name = after.empty() ? "" : after.front();
                    }
                    std::vector<std::string> bases;
                    if (colon != std::string::npos) {
                        for (const std::string& part : SplitTop(h.substr(colon + 1), ',')) {
                            std::string p = part.substr(0, part.find('<'));
                            const std::vector<std::string> pid = Idents(p);
                            if (!pid.empty()) {
                                bases.push_back(pid.back());
                            }
                        }
                    }
                    if (name.empty()) {
                        Scope(i + 1, close, cls); // anonymous struct/union: its members belong to the enclosing class
                        typeName.clear();
                    } else {
                        const std::string q = cls.empty() ? name : cls + "::" + name;
                        classes.push_back(SrcClass{ q, Where(i), bases });
                        Scope(i + 1, close, q);
                        typeName = q;
                    }
                    stmtIsType = !isTypedef;
                    stmtIsTypedef = isTypedef;
                    // What follows the body, up to ';', are declarators of the enclosing class (or the typedef name).
                    stmt = "{}";
                    i = close;
                    continue;
                }
                if (StartsWithWord(h, "enum")) {
                    stmt += "{}";
                    i = close;
                    continue;
                }
                // A function / constructor body (its head has a top-level '('): the statement ends at '}'.
                const size_t p = FindTop(head, "(");
                const size_t eq = FindTop(head, "=");
                if (p != std::string::npos && (eq == std::string::npos || p < eq)) {
                    stmt.clear();
                    stmtIsType = false;
                    i = close;
                    if (i + 1 < e && t[i + 1] == ';') {
                        i++;
                    }
                    continue;
                }
                stmt += "{}"; // a brace initializer of a data member
                i = close;
                continue;
            }
            if (c == ';') {
                if (!cls.empty() && !stmtIsTypedef) {
                    std::string s = Trim(stmt);
                    if (stmtIsType || s.rfind("{}", 0) == 0) {
                        s = Trim(s.substr(s.rfind("{}", 0) == 0 ? 2 : 0));
                        if (!s.empty() && !typeName.empty()) {
                            Declaration(cls, typeName + " " + s, stmtPos);
                        } else if (!s.empty()) {
                            Declaration(cls, "anon " + s, stmtPos);
                        }
                    } else {
                        Declaration(cls, s, stmtPos);
                    }
                }
                stmt.clear();
                stmtIsType = false;
                stmtIsTypedef = false;
                typeName.clear();
                continue;
            }
            if (c == ':' && !cls.empty()) {
                const std::string s = Trim(stmt);
                if ((s == "public" || s == "private" || s == "protected") && (i + 1 >= e || t[i + 1] != ':')) {
                    stmt.clear();
                    continue;
                }
            }
            if (c == '(') { // skip a parenthesised group whole: it can hold ';' (for), '{' (lambdas) and ':'
                const size_t close = MatchingClose(t, i);
                if (close == std::string::npos || close > e) {
                    return;
                }
                stmt += t.substr(i, close - i + 1);
                i = close;
                continue;
            }
            stmt += c;
        }
    }
};

// Every `static` variable outside a class body in a .cpp: file-scope and function-local (FASE 1.4b.1 b, the wider
// equivalent of `rg '\bstatic\b(?![^;{]*\bconst\b)[^;{(]*;'`: multi-line declarations and brace initializers included).
std::vector<SrcMember> ScanCppStatics(const std::string& raw, const std::string& file) {
    std::vector<SrcMember> out;
    const std::string t = StripSource(raw);
    CoverageScan probe;
    probe.file = file;
    probe.text = &t;
    for (size_t p = t.find("static"); p != std::string::npos; p = t.find("static", p + 6)) {
        if ((p > 0 && IsIdentChar(t[p - 1])) || (p + 6 < t.size() && IsIdentChar(t[p + 6]))) {
            continue; // static_cast, static_assert, …
        }
        if (p >= 2 && t[p - 1] == ':' && t[p - 2] == ':') {
            continue;
        }
        // The statement: up to the first top-level ';' — a '{' after '=' is an initializer; a '{' after a top-level '('
        // is a function body (a static function: not a variable).
        std::string stmt;
        size_t i = p;
        bool function = false;
        for (; i < t.size() && t[i] != ';'; i++) {
            if (t[i] == '(' || t[i] == '[') {
                const size_t close = MatchingClose(t, i);
                if (close == std::string::npos) {
                    break;
                }
                stmt += t.substr(i, close - i + 1);
                i = close;
                continue;
            }
            if (t[i] == '{') {
                const size_t paren = FindTop(stmt, "(");
                const size_t eq = FindTop(stmt, "=");
                if (paren != std::string::npos && (eq == std::string::npos || paren < eq)) {
                    function = true;
                    break;
                }
                const size_t close = MatchingClose(t, i);
                if (close == std::string::npos) {
                    break;
                }
                stmt += "{}";
                i = close;
                continue;
            }
            stmt += t[i];
        }
        if (function) {
            continue;
        }
        const size_t before = probe.members.size();
        probe.Declaration("<file>", stmt, p);
        for (size_t k = before; k < probe.members.size(); k++) {
            out.push_back(SrcMember{ file, probe.members[k].member, probe.members[k].where, true });
        }
    }
    probe.text = nullptr;
    return out;
}

// The body of a function DEFINITION named `site` ("OTrophy::HashState", "HashActors") in a stripped text, or npos.
std::pair<size_t, size_t> FindDefinition(const std::string& t, const std::string& site) {
    for (size_t p = t.find(site); p != std::string::npos; p = t.find(site, p + 1)) {
        if ((p > 0 && (IsIdentChar(t[p - 1]) || t[p - 1] == ':')) || !(p + site.size() < t.size())) {
            continue;
        }
        size_t q = p + site.size();
        while (q < t.size() && std::isspace((unsigned char) t[q])) {
            q++;
        }
        if (q >= t.size() || t[q] != '(') {
            continue;
        }
        q = MatchingClose(t, q);
        if (q == std::string::npos) {
            continue;
        }
        q++;
        while (q < t.size() && (std::isspace((unsigned char) t[q]) || IsIdentChar(t[q]))) {
            q++; // const, override
        }
        if (q < t.size() && t[q] == '{') {
            const size_t close = MatchingClose(t, q);
            if (close != std::string::npos) {
                return { q, close };
            }
        }
    }
    return { std::string::npos, std::string::npos };
}

CoverageScan ScanCoverageText(const std::string& raw, const std::string& file) {
    CoverageScan s;
    const std::string stripped = StripSource(raw);
    s.file = file;
    s.text = &stripped;
    s.Scope(0, stripped.size(), "");
    s.text = nullptr;
    return s;
}

const char* kParserFixture = R"FIX(
#include "x.h"
#define M(a) struct NotAClass { int x; };
// class Commented { int no; };
/* struct Blocked { int no; }; */
class ABase : public AActor {
  public:
    s16 a, b;                       // two declarators
    Vtx myVtx[12] = { { 1, 2 }, { 3, 4 } };
    FVector Pos{0.0f, 0.0f, 0.0f};
    static size_t _count;
    static constexpr int kLimit = 3;
    static const char* kName;
    const char* Name = "a;b{c}";
    std::map<uint32_t, std::vector<uint32_t>> Counts;
    std::function<void(int, float)> cb;
    u32 flags : 3;
    enum class Mode { A, B };
    Mode mode = Mode::A;
    struct Entry {
        OMole* Mole;
        bool Active;
    };
    std::vector<Entry> _entries;
    union {
        uint64_t crc;
        void* model;
    };
    ABase(const SpawnParams& p) : a(1), b(2) { for (int i = 0; i < 3; i++) { } }
    virtual void Tick() override;
    void Draw() const { if (a) { return; } };
    bool operator==(const ABase& o) const;
    ~ABase() = default;
  private:
    Player* _player = nullptr;
    enum MusicSeq Sequence;
};
EXTERN_C_START
EXTERN_C_END
namespace fwd { class Hasher; }
class Macroed {
    int m;
};
typedef struct {
    s16 isActive;
    Vec3f position;
} CarStuff;
namespace N { struct Inner : public OObject { int z; }; }
)FIX";

} // namespace

// The parser's own positive control: every construct the engine headers use, on a fixture, member by member.
static void test_coverage_parser() {
    const CoverageScan s = ScanCoverageText(kParserFixture, "fixture.h");
    std::vector<std::string> got;
    for (const SrcMember& m : s.members) {
        got.push_back(m.cls + "::" + m.member + (m.isStatic ? "(static)" : ""));
    }
    const std::vector<std::string> want = {
        "ABase::a",           "ABase::b",           "ABase::myVtx",         "ABase::Pos",
        "ABase::_count(static)", "ABase::kName(static)", "ABase::Name",     "ABase::Counts",
        "ABase::cb",          "ABase::flags",       "ABase::mode",          "ABase::Entry::Mole",
        "ABase::Entry::Active", "ABase::_entries",  "ABase::crc",           "ABase::model",
        "ABase::_player",     "ABase::Sequence",    "Macroed::m",           "CarStuff::isActive",
        "CarStuff::position", "Inner::z" };
    if (got != want) {
        std::fprintf(stderr, "  parser fixture members:");
        for (const std::string& g : got) {
            std::fprintf(stderr, " %s", g.c_str());
        }
        std::fprintf(stderr, "\n");
    }
    CHECK(got == want);
    std::vector<std::string> classes;
    for (const SrcClass& c : s.classes) {
        classes.push_back(c.name + (c.bases.empty() ? "" : ":" + c.bases[0]));
    }
    const std::vector<std::string> wantClasses = { "ABase:AActor", "ABase::Entry", "Macroed", "CarStuff",
                                                   "Inner:OObject" };
    CHECK(classes == wantClasses);
    CHECK(s.members.size() == want.size() && s.members[0].where == "fixture.h:8");

    // The .cpp statics sweep on its own fixture: file-scope, multi-line, brace-initialized and function-local statics
    // are found; static functions, const / constexpr statics, static_cast / static_assert and comments are not.
    const char* cpp = R"FIX(
static float sAngle = 0.0f;
static Vtx sVtx[2] = {
    { 1, 2 }, { 3, 4 },
};
static std::map<int,
                int> sMulti;
static const int kConst = 1;
static constexpr float kPi = 3.14f;
static const char* sTable[] = { "a", "b" };
static char* const kFixedPtr = nullptr;
static void Helper(int a) {
    static int sLocal = 3;
    int x = static_cast<int>(a);
}
static_assert(sizeof(int) == 4, "int");
// static int sCommented;
int NotStatic = 0;
)FIX";
    std::vector<std::string> statics;
    for (const SrcMember& m : ScanCppStatics(cpp, "fixture.cpp")) {
        statics.push_back(m.member + "@" + m.where);
    }
    const std::vector<std::string> wantStatics = { "sAngle@fixture.cpp:2", "sVtx@fixture.cpp:3", "sMulti@fixture.cpp:6",
                                                   "sTable@fixture.cpp:10", "sLocal@fixture.cpp:13" };
    if (statics != wantStatics) {
        std::fprintf(stderr, "  cpp statics fixture:");
        for (const std::string& x : statics) {
            std::fprintf(stderr, " %s", x.c_str());
        }
        std::fprintf(stderr, "\n");
    }
    CHECK(statics == wantStatics);
}

namespace {

struct CoverageSource {
    std::vector<SrcClass> classes;            // defined in kCoverageHeaders
    std::vector<SrcMember> members;           // of those classes
    std::vector<SrcClass> engineClasses;      // every class defined in any src/engine header (for root discovery)
    std::vector<SrcMember> otherStatics;      // class statics of classes NOT in kCoverageHeaders (engine, port/oracle)
    std::vector<SrcMember> cppStatics;        // file-scope / function-local statics of src/engine + port/oracle .cpp
    std::vector<std::pair<std::string, std::string>> code; // (raw, stripped) of each .cpp there: where hashing lives
    size_t files = 0;
};

CoverageSource ScanCoverageSource() {
    CoverageSource out;
    const std::filesystem::path root = std::filesystem::path(ORACLE_SOURCE_DIR) / "..";
    std::set<std::string> listed;
    for (const char* h : oracle::kCoverageHeaders) {
        listed.insert(h);
        const CoverageScan s = ScanCoverageText(ReadFile(root / h), h);
        out.files++;
        out.classes.insert(out.classes.end(), s.classes.begin(), s.classes.end());
        out.members.insert(out.members.end(), s.members.begin(), s.members.end());
    }
    for (const char* dir : { "src/engine", "src/port/oracle" }) {
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(root / dir, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            const std::string ext = it->path().extension().string();
            if (!it->is_regular_file() || (ext != ".h" && ext != ".cpp")) {
                continue;
            }
            const std::string rel = std::filesystem::relative(it->path(), root).generic_string();
            const std::string raw = ReadFile(it->path());
            if (ext == ".h") {
                const CoverageScan s = ScanCoverageText(raw, rel);
                out.engineClasses.insert(out.engineClasses.end(), s.classes.begin(), s.classes.end());
                if (listed.count(rel) == 0) {
                    for (const SrcMember& m : s.members) {
                        if (m.isStatic) { // keyed like a .cpp static: (file, "Class::name")
                            out.otherStatics.push_back(SrcMember{ rel, m.cls + "::" + m.member, m.where, true });
                        }
                    }
                }
            } else {
                const std::vector<SrcMember> st = ScanCppStatics(raw, rel);
                out.cppStatics.insert(out.cppStatics.end(), st.begin(), st.end());
                out.code.emplace_back(raw, StripSource(raw));
            }
        }
    }
    return out;
}

} // namespace

// "0 unclassified" as a test: a member added later fails here until it is hashed or excluded with a reason.
static void test_subclass_coverage() {
    const CoverageSource src = ScanCoverageSource();
    std::set<std::string> outOfScope;
    for (const oracle::OutOfScopeClass& o : oracle::kOutOfScopeClasses) {
        outOfScope.insert(o.cls);
        CHECK(o.reason != nullptr && std::strlen(o.reason) > 0);
    }
    // Every class deriving (transitively) from a root is defined in a covered header — a new actor in a new file fails.
    std::set<std::string> inScopeNames;
    for (const SrcClass& c : src.classes) {
        if (outOfScope.count(c.name) == 0) {
            inScopeNames.insert(c.name);
        }
    }
    std::set<std::string> derived(std::begin(oracle::kCoverageRoots), std::end(oracle::kCoverageRoots));
    for (bool grew = true; grew;) {
        grew = false;
        for (const SrcClass& c : src.engineClasses) {
            if (derived.count(c.name) == 0 && !c.bases.empty() && derived.count(c.bases[0]) != 0) {
                derived.insert(c.name);
                grew = true;
            }
        }
    }
    size_t subclasses = 0;
    for (const std::string& d : derived) {
        const auto* rootsEnd = std::end(oracle::kCoverageRoots);
        if (std::find(std::begin(oracle::kCoverageRoots), rootsEnd, d) != rootsEnd) {
            continue;
        }
        subclasses++;
        if (inScopeNames.count(d) == 0) {
            std::fprintf(stderr, "  subclass %s is not defined in a kCoverageHeaders file\n", d.c_str());
        }
        CHECK(inScopeNames.count(d) == 1);
    }
    // The classification: each member in exactly one table, each row naming a member that exists.
    std::set<std::pair<std::string, std::string>> members;
    size_t unclassified = 0;
    for (const SrcMember& m : src.members) {
        if (outOfScope.count(m.cls) != 0) {
            continue;
        }
        members.insert({ m.cls, m.member });
        int hashed = 0, excluded = 0;
        for (const oracle::CoverageRow& r : oracle::kHashed) {
            hashed += m.cls == r.cls && m.member == r.member;
        }
        for (const oracle::CoverageRow& r : oracle::kExcluded) {
            excluded += m.cls == r.cls && m.member == r.member;
        }
        if (hashed + excluded == 0 && ++unclassified <= 10) {
            std::fprintf(stderr, "  unclassified %s::%s%s (%s)\n", m.cls.c_str(), m.member.c_str(),
                         m.isStatic ? " [static]" : "", m.where.c_str());
        }
        if (hashed + excluded > 1) {
            std::fprintf(stderr, "  duplicated %s::%s (%d hashed, %d excluded)\n", m.cls.c_str(), m.member.c_str(),
                         hashed, excluded);
        }
        CHECK(hashed + excluded == 1);
    }
    if (unclassified > 10) {
        std::fprintf(stderr, "  … %zu unclassified in all\n", unclassified);
    }
    auto checkRows = [&](const oracle::CoverageRow* rows, size_t n) {
        for (size_t i = 0; i < n; i++) {
            const bool exists = members.count({ rows[i].cls, rows[i].member }) == 1;
            if (!exists) {
                std::fprintf(stderr, "  stale row %s::%s (no such member)\n", rows[i].cls, rows[i].member);
            }
            CHECK(exists);
            CHECK(rows[i].rule != nullptr && std::strlen(rows[i].rule) > 0);
        }
    };
    checkRows(oracle::kHashed, std::size(oracle::kHashed));
    for (const oracle::CoverageRow& r : oracle::kHashed) { // every rule is a documented one
        bool known = std::strncmp(r.rule, "group:", 6) == 0;
        for (const char* k : oracle::kRules) {
            known = known || std::strcmp(r.rule, k) == 0;
        }
        if (!known) {
            std::fprintf(stderr, "  %s::%s: rule \"%s\" is not in kRules\n", r.cls, r.member, r.rule);
        }
        CHECK(known);
    }
    checkRows(oracle::kExcluded, std::size(oracle::kExcluded));
    for (const std::string& c : outOfScope) {
        bool defined = false;
        for (const SrcClass& k : src.classes) {
            defined = defined || k.name == c;
        }
        if (!defined) {
            std::fprintf(stderr, "  stale out-of-scope class %s\n", c.c_str());
        }
        CHECK(defined);
    }
    // The code hashes what the table says: every kHashed row's name appears as a literal in its class's hash function
    // (a put() dropped from an override fails here, not only in a run). Rows hashed by another group's walk are
    // skipped.
    auto site = [](const std::string& cls, const char* rule) -> std::string {
        for (const oracle::HashSite& s : oracle::kHashSites) {
            if (cls == s.cls) {
                return s.site;
            }
        }
        return cls + (std::strcmp(rule, "statics") == 0 ? "::HashStatics" : "::HashState");
    };
    size_t sitesChecked = 0;
    for (const oracle::CoverageRow& r : oracle::kHashed) {
        if (std::strncmp(r.rule, "group:", 6) == 0) {
            continue;
        }
        const std::string fn = site(r.cls, r.rule);
        bool defined = false, named = false;
        for (const auto& [raw, stripped] : src.code) {
            const auto [b, e] = FindDefinition(stripped, fn);
            if (b == std::string::npos) {
                continue;
            }
            defined = true;
            named = named || raw.substr(b, e - b).find("\"" + std::string(r.member) + "\"") != std::string::npos;
        }
        if (!defined || !named) {
            std::fprintf(stderr, "  %s::%s: %s\n", r.cls, r.member,
                         !defined ? ("no definition of " + fn).c_str() : ("not put() by " + fn).c_str());
        }
        CHECK(defined && named);
        sitesChecked++;
    }
    CHECK(sitesChecked > 400);

    // Statics outside the class tables (FASE 1.4b.1 b): .cpp file-scope / function-local statics, and the class statics
    // of classes outside kCoverageHeaders. Each is hashed (named in its site's body) or excluded with a reason.
    std::vector<SrcMember> statics = src.cppStatics;
    statics.insert(statics.end(), src.otherStatics.begin(), src.otherStatics.end());
    std::set<std::pair<std::string, std::string>> staticKeys;
    for (const SrcMember& m : statics) {
        staticKeys.insert({ m.cls, m.member });
        int found = 0;
        for (const oracle::StaticRow& r : oracle::kStaticsHashed) {
            found += m.cls == r.file && m.member == r.name;
        }
        for (const oracle::StaticRow& r : oracle::kStaticsExcluded) {
            found += m.cls == r.file && m.member == r.name;
        }
        if (found != 1) {
            std::fprintf(stderr, "  static %s %s (%s): %s\n", m.cls.c_str(), m.member.c_str(), m.where.c_str(),
                         found == 0 ? "unclassified" : "in more than one row");
        }
        CHECK(found == 1);
    }
    for (const oracle::StaticRow& r : oracle::kStaticsHashed) {
        CHECK(staticKeys.count({ r.file, r.name }) == 1);
        bool named = false;
        for (const auto& [raw, stripped] : src.code) {
            const auto [b, e] = FindDefinition(stripped, r.rule);
            named = named || (b != std::string::npos && stripped.substr(b, e - b).find(r.name) != std::string::npos);
        }
        if (!named) {
            std::fprintf(stderr, "  static %s %s: not hashed by %s\n", r.file, r.name, r.rule);
        }
        CHECK(named);
    }
    for (const oracle::StaticRow& r : oracle::kStaticsExcluded) {
        if (staticKeys.count({ r.file, r.name }) != 1) {
            std::fprintf(stderr, "  stale static row %s %s\n", r.file, r.name);
        }
        CHECK(staticKeys.count({ r.file, r.name }) == 1);
        CHECK(std::strlen(r.rule) > 0);
    }
    // Positive control of the static sweep: the two hoisted angles, Thwomp's hoisted temp_s7 and a brace-initialized
    // one.
    CHECK(staticKeys.count({ "src/engine/actors/SpaghettiShip.cpp", "sSpaghettiShipAngle" }) == 1);
    CHECK(staticKeys.count({ "src/engine/actors/Starship.cpp", "sStarshipAngle" }) == 1);
    CHECK(staticKeys.count({ "src/engine/objects/Thwomp.cpp", "sThwompTempS7" }) == 1);
    CHECK(statics.size() > 10);

    // Positive control: the scan can see (measured at 1.4b: the clang record layouts list 38 AActor/OObject
    // subclasses).
    CHECK(src.files >= 60);
    CHECK(members.size() > 300);
    CHECK(subclasses >= 38);
    CHECK(members.count({ "ATrain", "Locomotive" }) == 1);
    CHECK(members.count({ "OBombKart", "_target" }) == 1);
    CHECK(members.count({ "PlayerBombKart", "surfaceHeight" }) == 1);
}

static int print_coverage() {
    const CoverageSource src = ScanCoverageSource();
    for (const SrcMember& m : src.members) {
        std::printf("%s · %s%s · %s\n", m.cls.c_str(), m.member.c_str(), m.isStatic ? " [static]" : "",
                    m.where.c_str());
    }
    for (const SrcClass& c : src.engineClasses) {
        std::printf("#class %s : %s · %s\n", c.name.c_str(), c.bases.empty() ? "" : c.bases[0].c_str(),
                    c.where.c_str());
    }
    for (const SrcMember& m : src.cppStatics) {
        std::printf("#static %s · %s · %s\n", m.cls.c_str(), m.member.c_str(), m.where.c_str());
    }
    for (const SrcMember& m : src.otherStatics) {
        std::printf("#static %s · %s · %s\n", m.cls.c_str(), m.member.c_str(), m.where.c_str());
    }
    std::printf("#files %zu members %zu\n", src.files, src.members.size());
    return 0;
}
