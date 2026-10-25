#include "OracleCli.h"
#include "OracleRecording.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

// Strict unsigned parse: the whole string, no sign, <= max. base 0 = strtoul's decimal / 0x hex / 0 octal; base 10 = decimal.
bool ParseUnsigned(const char* s, int base, unsigned long max, unsigned long& out) {
    if (s == nullptr || *s == '\0' || *s == '-' || *s == '+') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long v = std::strtoul(s, &end, base);
    if (errno != 0 || end == s || *end != '\0' || v > max) {
        return false;
    }
    out = v;
    return true;
}

int Usage(const std::string& message) {
    std::fprintf(stderr, "oracle: %s\n", message.c_str());
    return 2;
}

} // namespace

int Oracle_ParseArgsInto(int argc, char** argv, OracleConfig& cfg) {
    // No-window modes run only after the WHOLE command line is valid.
    const char* infoPath = nullptr;
    const char* verifyA = nullptr;
    const char* verifyB = nullptr;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if (a.rfind("--oracle", 0) != 0) {
            continue; // not ours (e.g. macOS passes -psn_… on a Finder launch)
        }
        const char* value = nullptr;
        auto need = [&]() -> bool { // a flag is never another flag's value
            if (i + 1 >= argc || std::string(argv[i + 1]).rfind("--oracle", 0) == 0) {
                return false;
            }
            value = argv[++i];
            return true;
        };
        unsigned long n = 0;
        if (a == "--oracle-record" || a == "--oracle-replay") {
            if (!need()) {
                return Usage(a + " needs a value");
            }
            if (cfg.mode != OracleMode::Off) {
                return Usage(a + ": only one of --oracle-record / --oracle-replay");
            }
            cfg.mode = (a == "--oracle-record") ? OracleMode::Record : OracleMode::Replay;
            (cfg.mode == OracleMode::Record ? cfg.recordPath : cfg.replayPath) = value;
        } else if (a == "--oracle-hash-log") {
            if (!need()) {
                return Usage(a + " needs a value");
            }
            cfg.hashLogPath = value;
        } else if (a == "--oracle-seed") {
            if (!need() || !ParseUnsigned(value, 0, 0xFFFF, n)) {
                return Usage(a + " needs a value in 0..0xFFFF");
            }
            cfg.seed = (uint16_t) n;
        } else if (a == "--oracle-exit-at-end") {
            cfg.exitAtEnd = true;
        } else if (a == "--oracle-dump-at") {
            if (!need() || !ParseUnsigned(value, 0, 0x7FFFFFFF, n)) {
                return Usage(a + " needs a non-negative iteration");
            }
            cfg.dumpAtIteration = (int32_t) n;
        } else if (a == "--oracle-probe-tick-at") {
            // Decimal, >= 1: iteration 0 is the startup read (main.c thread5_game_loop), which no guard call precedes.
            if (!need() || !ParseUnsigned(value, 10, 0x7FFFFFFF, n) || n == 0) {
                return Usage(a + " needs a decimal iteration >= 1");
            }
            cfg.probeTickAt = (int32_t) n;
        } else if (a == "--oracle-info") { // no window
            if (!need()) {
                return Usage(a + " needs a value");
            }
            infoPath = value;
        } else if (a == "--oracle-verify") { // no window; stub until FASE 1.6
            const char* x = nullptr;
            if (!need()) {
                return Usage(a + " needs two files");
            }
            x = value;
            if (!need()) {
                return Usage(a + " needs two files");
            }
            verifyA = x;
            verifyB = value;
        } else {
            return Usage("unknown flag " + a);
        }
    }
    const int noWindowModes = (infoPath != nullptr ? 1 : 0) + (verifyA != nullptr ? 1 : 0);
    if (noWindowModes > 1 || (noWindowModes == 1 && cfg.mode != OracleMode::Off)) {
        return Usage("--oracle-info / --oracle-verify run alone (no window, no --oracle-record / --oracle-replay)");
    }
    if (infoPath != nullptr) {
        return Oracle_PrintInfo(infoPath);
    }
    if (verifyA != nullptr) {
        return Oracle_Verify(verifyA, verifyB);
    }
    if (cfg.probeTickAt >= 0 && cfg.mode != OracleMode::Replay) {
        return Usage("--oracle-probe-tick-at is refused unless --oracle-replay (never in a recording)");
    }
    if (cfg.mode != OracleMode::Off && cfg.hashLogPath.empty()) {
        cfg.hashLogPath = (cfg.mode == OracleMode::Record ? cfg.recordPath : cfg.replayPath) + ".mk64hash";
    }
    return -1;
}

int Oracle_ParseArgs(int argc, char** argv) {
    return Oracle_ParseArgsInto(argc, argv, Oracle_Config());
}

int Oracle_PrintInfo(const char* path) {
    oracle::Recording r;
    std::string err;
    if (!oracle::read(path, r, err)) {
        std::fprintf(stderr, "oracle: --oracle-info %s: %s\n", path, err.c_str());
        return 2;
    }
    const oracle::Header& h = r.header;
    using ull = unsigned long long;
    std::printf("version=%u\nheaderBytes=%u\n", h.version, h.headerBytes);
    std::printf("buildVersion=%.*s\ncompiler=%.*s\n", (int) sizeof h.buildVersion, h.buildVersion, (int) sizeof h.compiler,
                h.compiler);
    std::printf("buildType=%.*s\nplatform=%.*s\n", (int) sizeof h.buildType, h.buildType, (int) sizeof h.platform,
                h.platform);
    std::printf("binaryHash=0x%016llx\ngameArchiveHash=0x%016llx\nportArchiveHash=0x%016llx\n", (ull) h.binaryHash,
                (ull) h.gameArchiveHash, (ull) h.portArchiveHash);
    std::printf("modListHash=0x%016llx\nconfigHash=0x%016llx\nfpWord=0x%016llx\n", (ull) h.modListHash,
                (ull) h.configHash, (ull) h.fpWord);
    std::printf("tickLogic=%d\nskipIntro=%d\nseed=0x%04x\n", h.tickLogic, h.skipIntro, (unsigned) h.seed);
    std::printf("pads=%zu\nevents=%zu\ncleanEnd=%s\n", r.pads.size(), r.events.size(), r.cleanEnd ? "true" : "false");
    return 0; // FASE 1.7 appends one "setup_race #k iter=N" line per SETUP_RACE event
}
