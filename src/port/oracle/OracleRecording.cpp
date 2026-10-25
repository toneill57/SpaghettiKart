#include "OracleRecording.h"

#include <cerrno>
#include <cstring>

namespace {

std::string ErrnoText(const char* what, const std::string& path) {
    return std::string(what) + " " + path + ": " + std::strerror(errno);
}

} // namespace

bool oracle::Writer::open(const std::string& path, const Header& h) {
    if (f_ != nullptr) {
        lastError = "writer already open";
        return false;
    }
    f_ = std::fopen(path.c_str(), "wb");
    if (f_ == nullptr) {
        lastError = ErrnoText("fopen failed:", path);
        return false;
    }
    sinceFlush_ = 0;
    // The header goes to the OS immediately: a native crash before the first periodic flush (no signal handler flushes
    // stdio on macOS) must still leave the build identity readable.
    if (std::fwrite(&h, sizeof h, 1, f_) != 1 || std::fflush(f_) != 0) {
        lastError = ErrnoText("header write failed:", path);
        std::fclose(f_);
        f_ = nullptr;
        return false;
    }
    return true;
}

bool oracle::Writer::countAndMaybeFlush() {
    if (++sinceFlush_ < kFlushEvery) {
        return true;
    }
    sinceFlush_ = 0;
    return flush();
}

bool oracle::Writer::writePads(uint32_t iteration, const Pad pads[4]) {
    if (f_ == nullptr) {
        lastError = "writer not open";
        return false;
    }
    PadsRecord r{ kRecPads, iteration, {} };
    for (int i = 0; i < 4; i++) {
        r.pads[i] = pads[i];
    }
    if (std::fwrite(&r, sizeof r, 1, f_) != 1) {
        lastError = std::string("pads write failed: ") + std::strerror(errno);
        return false;
    }
    return countAndMaybeFlush();
}

bool oracle::Writer::writeEvent(uint8_t type, uint32_t iteration) {
    if (f_ == nullptr) {
        lastError = "writer not open";
        return false;
    }
    const EventRecord r{ type, iteration };
    if (std::fwrite(&r, sizeof r, 1, f_) != 1) {
        lastError = std::string("event write failed: ") + std::strerror(errno);
        return false;
    }
    return countAndMaybeFlush();
}

bool oracle::Writer::flush() {
    if (f_ == nullptr) {
        return false;
    }
    if (std::fflush(f_) != 0) {
        lastError = std::string("fflush failed: ") + std::strerror(errno);
        return false;
    }
    return true;
}

bool oracle::Writer::close() {
    if (f_ == nullptr) {
        return true; // idempotent
    }
    const uint8_t end = kRecEnd;
    bool ok = std::fwrite(&end, 1, 1, f_) == 1;
    ok = (std::fflush(f_) == 0) && ok;
    ok = (std::fclose(f_) == 0) && ok;
    f_ = nullptr;
    if (!ok) {
        lastError = std::string("close failed: ") + std::strerror(errno);
    }
    return ok;
}

bool oracle::read(const std::string& path, Recording& out, std::string& error) {
    out = Recording{};
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        error = ErrnoText("cannot open", path);
        return false;
    }
    auto fail = [&](const std::string& why) {
        std::fclose(f);
        error = why;
        return false;
    };

    const size_t got = std::fread(&out.header, 1, sizeof out.header, f);
    if (got != sizeof out.header) {
        return fail("truncated header: " + std::to_string(got) + " of " + std::to_string(sizeof out.header) + " bytes");
    }
    if (std::memcmp(out.header.magic, kMagic, sizeof kMagic) != 0) {
        return fail("bad magic (not a .mk64rec file)");
    }
    if (out.header.version != kVersion) {
        return fail("unsupported version " + std::to_string(out.header.version));
    }
    if (out.header.headerBytes < sizeof(Header)) {
        return fail("bad headerBytes " + std::to_string(out.header.headerBytes));
    }
    if (out.header.headerBytes > sizeof(Header)) {
        // fseek past EOF succeeds on a regular file, so the extension is checked against the file size, not the seek.
        if (std::fseek(f, 0, SEEK_END) != 0) {
            return fail("cannot size the file");
        }
        const long size = std::ftell(f);
        if (size < 0 || (unsigned long) size < out.header.headerBytes) {
            return fail("truncated header: headerBytes " + std::to_string(out.header.headerBytes) + " > file size " +
                        std::to_string(size));
        }
        if (std::fseek(f, (long) out.header.headerBytes, SEEK_SET) != 0) {
            return fail("cannot skip header extension");
        }
    }

    // A record cut short (crash, abort) ends the stream without an error: the complete records before it are kept.
    for (;;) {
        const int type = std::fgetc(f);
        if (type == EOF) {
            break;
        }
        if (type == kRecPads) {
            PadsRecord r{};
            r.type = kRecPads;
            if (std::fread(reinterpret_cast<char*>(&r) + 1, sizeof r - 1, 1, f) != 1) {
                break;
            }
            out.pads.push_back(r);
        } else if (type == kRecSetupRace) {
            EventRecord r{};
            r.type = kRecSetupRace;
            if (std::fread(reinterpret_cast<char*>(&r) + 1, sizeof r - 1, 1, f) != 1) {
                break;
            }
            out.events.push_back(r);
        } else if (type == kRecEnd) {
            out.cleanEnd = true;
            break;
        } else {
            char buf[64];
            std::snprintf(buf, sizeof buf, "unknown record type 0x%02x at offset %ld", (unsigned) type, std::ftell(f) - 1);
            return fail(buf);
        }
    }
    // EOF and a read error look the same to fgetc/fread; only ferror tells a failing volume from a crash-truncated file.
    if (std::ferror(f)) {
        return fail(std::string("read error at offset ") + std::to_string(std::ftell(f)));
    }
    std::fclose(f);
    return true;
}
