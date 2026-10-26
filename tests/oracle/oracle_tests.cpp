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
    test_hasher_scalars();
    test_hasher_put_and_paths();
    test_hash_log_format();
    test_hash_log_writer();
    test_mod_list_hash();
    test_config_hash();
    test_cvar_sweep();
    test_cli_probe_flags();
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
    // `all` = FNV over the nine group values as LE u64.
    {
        uint64_t g[oracle::kGroupCount];
        for (int i = 0; i < oracle::kGroupCount; i++) {
            g[i] = (uint64_t) (i + 1);
        }
        CHECK(oracle::Combine(g) == 0x60b3e1fa9c9c0ca4ull);
        uint64_t z[oracle::kGroupCount] = {};
        CHECK(oracle::Combine(z) == 0x3ecb33e15783bec5ull);
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
    CHECK(oracle::kGroupCount == 9);
    CHECK(std::string(oracle::kGroupNames[0]) == "rng" && std::string(oracle::kGroupNames[8]) == "racearrays");
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
    "# groups=rng,race,players,controllers,objects,actors,oobjects,cameras,racearrays,all\n";

const char* kLine7Expected =
    "7 rng=0123456789abcdef race=0123456789abcdf0 players=0123456789abcdf1 controllers=0123456789abcdf2 "
    "objects=0123456789abcdf3 actors=0123456789abcdf4 oobjects=0123456789abcdf5 cameras=0123456789abcdf6 "
    "racearrays=0123456789abcdf7 all=fedcba9876543210\n";

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
}
