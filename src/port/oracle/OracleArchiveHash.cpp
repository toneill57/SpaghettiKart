#include "OracleArchiveHash.h"
#include "OracleHash.h"

#include <algorithm>
#include <cstdio>
#include <tuple>
#include <vector>

namespace {

// Zip record layouts (PKWARE APPNOTE 4.3.12 / 4.3.16).
constexpr uint32_t kEocdSig = 0x06054b50;
constexpr uint32_t kCentralSig = 0x02014b50;
constexpr size_t kEocdBytes = 22;
constexpr size_t kCentralBytes = 46;
constexpr size_t kMaxComment = 0xFFFF;

uint32_t Le16(const unsigned char* p) {
    return (uint32_t) p[0] | (uint32_t) p[1] << 8;
}

uint32_t Le32(const unsigned char* p) {
    return Le16(p) | Le16(p + 2) << 16;
}

struct Entry {
    std::string name;
    uint32_t crc;
    uint32_t size;
    bool operator<(const Entry& o) const {
        return std::tie(name, crc, size) < std::tie(o.name, o.crc, o.size);
    }
};

bool ReadAt(std::FILE* f, long offset, std::vector<unsigned char>& buf) {
    return std::fseek(f, offset, SEEK_SET) == 0 && std::fread(buf.data(), 1, buf.size(), f) == buf.size();
}

} // namespace

oracle::ZipContent oracle::HashZipContent(const std::string& path) {
    ZipContent z;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        z.error = "cannot open " + path;
        return z;
    }
    auto fail = [&](const std::string& why) {
        std::fclose(f);
        z.error = path + ": " + why;
        return z;
    };
    if (std::fseek(f, 0, SEEK_END) != 0) {
        return fail("cannot size the file");
    }
    const long size = std::ftell(f);
    if (size < (long) kEocdBytes) {
        return fail("too short for a zip end record");
    }
    // The end record sits in the last 22..22+65535 bytes; searched backwards so a comment holding the signature is skipped
    // unless its declared length also lands exactly on the end of the file.
    const size_t tailBytes = (size_t) std::min<long>(size, (long) (kEocdBytes + kMaxComment));
    std::vector<unsigned char> tail(tailBytes);
    if (!ReadAt(f, size - (long) tailBytes, tail)) {
        return fail("cannot read the file tail");
    }
    long eocd = -1;
    for (size_t i = tailBytes - kEocdBytes + 1; i-- > 0;) {
        const unsigned char* p = tail.data() + i;
        if (Le32(p) == kEocdSig && i + kEocdBytes + Le16(p + 20) == tailBytes) {
            eocd = (long) i;
            break;
        }
    }
    if (eocd < 0) {
        return fail("no end-of-central-directory record (not a zip)");
    }
    const unsigned char* e = tail.data() + eocd;
    const uint32_t disk = Le16(e + 4), cdDisk = Le16(e + 6), onDisk = Le16(e + 8), total = Le16(e + 10);
    const uint32_t cdSize = Le32(e + 12), cdOffset = Le32(e + 16);
    if (total == 0xFFFF || cdSize == 0xFFFFFFFFu || cdOffset == 0xFFFFFFFFu) {
        return fail("ZIP64 archive (not supported)");
    }
    if (disk != 0 || cdDisk != 0 || onDisk != total) {
        return fail("multi-disk archive (not supported)");
    }
    const long eocdAbs = size - (long) tailBytes + eocd;
    if ((uint64_t) cdOffset + cdSize > (uint64_t) eocdAbs) { // in 64 bits: a 32-bit long (MSVC) would wrap
        return fail("central directory overlaps the end record");
    }
    std::vector<unsigned char> cd(cdSize);
    if (!ReadAt(f, (long) cdOffset, cd)) {
        return fail("cannot read the central directory");
    }
    std::fclose(f);

    std::vector<Entry> entries;
    size_t pos = 0;
    for (uint32_t k = 0; k < total; k++) {
        if (pos + kCentralBytes > cd.size() || Le32(cd.data() + pos) != kCentralSig) {
            z.error = path + ": central directory entry " + std::to_string(k) + " is truncated or malformed";
            return z;
        }
        const unsigned char* c = cd.data() + pos;
        const size_t nameLen = Le16(c + 28), extraLen = Le16(c + 30), commentLen = Le16(c + 32);
        if (pos + kCentralBytes + nameLen + extraLen + commentLen > cd.size()) {
            z.error = path + ": central directory entry " + std::to_string(k) + " runs past the directory";
            return z;
        }
        entries.push_back(Entry{ std::string(reinterpret_cast<const char*>(c + kCentralBytes), nameLen), Le32(c + 16),
                                 Le32(c + 24) });
        pos += kCentralBytes + nameLen + extraLen + commentLen;
    }
    if (pos != cd.size()) {
        z.error = path + ": central directory holds more than the " + std::to_string(total) + " entries the end record declares";
        return z;
    }
    std::sort(entries.begin(), entries.end());
    Hasher h;
    h.WriteU32((uint32_t) entries.size());
    for (const Entry& en : entries) {
        h.WriteU32((uint32_t) en.name.size());
        h.bytes(en.name.data(), en.name.size());
        h.WriteU32(en.crc);
        h.WriteU32(en.size);
    }
    z.hash = h.value();
    z.entries = entries.size();
    return z;
}
