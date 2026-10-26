#pragma once
// The `.mk64hash` v1 text format (FASE 1.4.4 SPEC; FASE 1.6 parses exactly this). Std-only: shared by the game writer
// (Oracle.cpp) and oracle_tests.
//   MK64HASH 1
//   # build=… platform=… buildType=… compiler=… binaryHash=0x…
//   # tickLogic=2 seed=0x… fpWord=0x… configHash=0x… modListHash=0x… gameArchiveHash=0x… portArchiveHash=0x…
//   # groups=rng,race,players,controllers,objects,actors,oobjects,cameras,racearrays,all
//   <iter> rng=<16 hex> … racearrays=<16 hex> all=<16 hex>        (one line per PADS record, <iter> from 0, +1 per line)
//   END <data lines>                                               (clean shutdown only; absent ⇒ truncated)
#include "OracleHash.h"
#include "OracleRecording.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace oracle {

inline constexpr const char* kHashLogMagic = "MK64HASH 1";

// A fixed-width header string as one metadata token value: up to its NUL (or its full width), every byte that is not
// printable non-space ASCII replaced by '_' — `compiler=clang 17.0.0 (…)` would otherwise split into tokens without `=`.
inline std::string MetaValue(const char* field, size_t width) {
    std::string out;
    for (size_t i = 0; i < width && field[i] != '\0'; i++) {
        const unsigned char c = (unsigned char) field[i];
        out += (c > 0x20 && c < 0x7f) ? (char) c : '_';
    }
    return out;
}

// Lines 1-4. Metadata comes from the run's header — in replay the RECORDING's header (FASE 1.5), so the record and replay
// logs of one recording are byte-identical (`cmp`).
inline std::string FormatHashLogHeader(const Header& h) {
    using ull = unsigned long long;
    char line[512];
    std::string out = std::string(kHashLogMagic) + "\n";
    std::snprintf(line, sizeof line, "# build=%s platform=%s buildType=%s compiler=%s binaryHash=0x%016llx\n",
                  MetaValue(h.buildVersion, sizeof h.buildVersion).c_str(), MetaValue(h.platform, sizeof h.platform).c_str(),
                  MetaValue(h.buildType, sizeof h.buildType).c_str(), MetaValue(h.compiler, sizeof h.compiler).c_str(),
                  (ull) h.binaryHash);
    out += line;
    std::snprintf(line, sizeof line,
                  "# tickLogic=%d seed=0x%04x fpWord=0x%016llx configHash=0x%016llx modListHash=0x%016llx "
                  "gameArchiveHash=0x%016llx portArchiveHash=0x%016llx\n",
                  (int) h.tickLogic, (unsigned) h.seed, (ull) h.fpWord, (ull) h.configHash, (ull) h.modListHash,
                  (ull) h.gameArchiveHash, (ull) h.portArchiveHash);
    out += line;
    out += "# groups=";
    for (int i = 0; i < kGroupCount; i++) {
        out += kGroupNames[i];
        out += ',';
    }
    out += "all\n";
    return out;
}

// One data line ending in '\n'. Returns its length, or -1 if `cap` is too small (callers pass 512; a line is < 320 bytes).
inline int FormatHashLine(char* out, size_t cap, uint32_t iter, const GroupHashes& g) {
    int n = std::snprintf(out, cap, "%u", (unsigned) iter);
    for (int i = 0; i < kGroupCount && n >= 0 && (size_t) n < cap; i++) {
        n += std::snprintf(out + n, cap - (size_t) n, " %s=%016llx", kGroupNames[i], (unsigned long long) g.g[i]);
    }
    if (n < 0 || (size_t) n >= cap) {
        return -1;
    }
    n += std::snprintf(out + n, cap - (size_t) n, " all=%016llx\n", (unsigned long long) g.all);
    return (n < 0 || (size_t) n >= cap) ? -1 : n;
}

inline std::string FormatHashLogEnd(uint32_t dataLines) {
    return "END " + std::to_string(dataLines) + "\n";
}

// Writes one `.mk64hash` v1 file. Enforces the alignment rule itself: line k carries iteration k, so a write whose
// iteration is not the number of lines already written is refused (a read_controllers() call without an iteration end, or
// the reverse, would otherwise shift every later line silently).
class HashLogWriter {
  public:
    HashLogWriter() = default;
    HashLogWriter(const HashLogWriter&) = delete;
    HashLogWriter& operator=(const HashLogWriter&) = delete;
    ~HashLogWriter() {
        if (f_ != nullptr) {
            std::fclose(f_); // no END: only close() marks a clean end
        }
    }

    bool open(const std::string& path, const Header& h) {
        if (f_ != nullptr) {
            lastError = "hash log already open";
            return false;
        }
        f_ = std::fopen(path.c_str(), "w");
        if (f_ == nullptr) {
            lastError = "fopen failed: " + path + ": " + std::strerror(errno);
            return false;
        }
        // The header goes to the OS immediately, like the recording's (OracleRecording.cpp).
        const std::string head = FormatHashLogHeader(h);
        if (std::fwrite(head.data(), 1, head.size(), f_) != head.size() || std::fflush(f_) != 0) {
            lastError = "header write failed: " + path + ": " + std::strerror(errno);
            std::fclose(f_);
            f_ = nullptr;
            return false;
        }
        return true;
    }

    bool write(uint32_t iteration, const GroupHashes& g) {
        if (f_ == nullptr) {
            lastError = "hash log not open";
            return false;
        }
        if (iteration != lines_) {
            lastError = "hash/pads misalignment: iteration " + std::to_string(iteration) + " but " + std::to_string(lines_) +
                        " lines written";
            return false;
        }
        char line[512];
        const int n = FormatHashLine(line, sizeof line, iteration, g);
        if (n < 0 || std::fwrite(line, 1, (size_t) n, f_) != (size_t) n) {
            lastError = "hash log write failed at iteration " + std::to_string(iteration);
            return false;
        }
        if ((++lines_ % kFlushEvery) == 0 && std::fflush(f_) != 0) {
            lastError = "hash log flush failed at iteration " + std::to_string(iteration);
            return false;
        }
        return true;
    }

    bool flush() { // Oracle_Abort: every complete line reaches the OS, no END (the log then reads as truncated)
        return f_ == nullptr || std::fflush(f_) == 0;
    }

    bool close() { // END n, fclose; idempotent
        if (f_ == nullptr) {
            return true;
        }
        const std::string end = FormatHashLogEnd(lines_);
        bool ok = std::fwrite(end.data(), 1, end.size(), f_) == end.size();
        ok = (std::fflush(f_) == 0) && ok;
        ok = (std::fclose(f_) == 0) && ok;
        f_ = nullptr;
        if (!ok) {
            lastError = "hash log END/close failed";
        }
        return ok;
    }

    bool isOpen() const {
        return f_ != nullptr;
    }
    uint32_t lines() const {
        return lines_;
    }
    std::string lastError;

  private:
    std::FILE* f_ = nullptr;
    uint32_t lines_ = 0;
};

} // namespace oracle
