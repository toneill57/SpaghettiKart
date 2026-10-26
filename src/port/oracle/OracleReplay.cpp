#include "OracleReplay.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace {

// A fixed char[] field filled to its full size has no NUL; bytes after a NUL are not part of the text.
std::string Str(const char* a, size_t n) {
    return std::string(a, strnlen(a, n));
}

std::string Hex(uint64_t v) {
    char b[24];
    std::snprintf(b, sizeof b, "0x%016" PRIx64, v);
    return b;
}

} // namespace

oracle::Compat oracle::CheckCompatible(const Header& rec, const Header& live) {
    Compat c;
    auto refuse = [&](const char* name, std::string a, std::string b) {
        c.ok = false;
        c.field = name;
        c.recorded = std::move(a);
        c.live = std::move(b);
    };
    auto text = [&](const char* name, const char* a, const char* b, size_t n) {
        if (c.ok && Str(a, n) != Str(b, n)) {
            refuse(name, Str(a, n), Str(b, n));
        }
    };
    auto number = [&](const char* name, uint64_t a, uint64_t b) {
        if (c.ok && a != b) {
            refuse(name, Hex(a), Hex(b));
        }
    };
    auto note = [&](const char* name, const std::string& a, const std::string& b) {
        if (a != b) {
            c.notes.push_back(std::string(name) + " recorded=" + a + " live=" + b);
        }
    };
    // INFORMATIONAL — change on every commit / rebuild; printed as META, never refused.
    note("buildVersion", Str(rec.buildVersion, sizeof rec.buildVersion), Str(live.buildVersion, sizeof live.buildVersion));
    note("binaryHash", Hex(rec.binaryHash), Hex(live.binaryHash));
    // REFUSAL fields — "same simulation". The order is the order the user sees: toolchain identity first.
    text("compiler", rec.compiler, live.compiler, sizeof rec.compiler);
    text("buildType", rec.buildType, live.buildType, sizeof rec.buildType);
    text("platform", rec.platform, live.platform, sizeof rec.platform);
    number("gameArchiveHash", rec.gameArchiveHash, live.gameArchiveHash);
    number("portArchiveHash", rec.portArchiveHash, live.portArchiveHash);
    number("tickLogic", (uint32_t) rec.tickLogic, (uint32_t) live.tickLogic);
    number("fpWord", rec.fpWord, live.fpWord);
    number("configHash", rec.configHash, live.configHash);
    number("modListHash", rec.modListHash, live.modListHash);
    number("skipIntro", (uint32_t) rec.skipIntro, (uint32_t) live.skipIntro);
    if (c.ok && (rec.gameArchiveHash == 0 || rec.portArchiveHash == 0)) {
        c.archivesUnchecked = true; // equal (c.ok) zeros: the archive identity was never measured
    }
    return c;
}

oracle::Step oracle::ReplayCursor::next(uint32_t iteration, const PadsRecord** out, uint32_t* recIter) {
    if (i_ >= r_.pads.size()) {
        return Step::End;
    }
    const PadsRecord& p = r_.pads[i_];
    if (p.iteration != iteration) {
        *recIter = p.iteration;
        return Step::Misaligned;
    }
    *out = &p;
    i_++;
    return Step::Feed;
}

std::string oracle::CheckReplayHashLogPath(const std::string& replayPath, const std::string& hashLogPath,
                                           bool hashLogDefaulted) {
    std::error_code ec;
    if (hashLogPath == replayPath || std::filesystem::equivalent(hashLogPath, replayPath, ec)) {
        return "hash log " + hashLogPath + " is the recording itself — pass --oracle-hash-log <new path>";
    }
    // The 1.3 default makes record and replay of x.mk64rec both resolve to x.mk64rec.mk64hash: a replay without
    // --oracle-hash-log would truncate the very log it must be compared against.
    if (hashLogDefaulted && std::filesystem::exists(hashLogPath, ec)) {
        return "hash log " + hashLogPath + " already exists (the recording run's log?) — pass --oracle-hash-log <new path>";
    }
    return {};
}
