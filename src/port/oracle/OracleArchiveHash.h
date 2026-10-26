#pragma once
// Content hash of a zip archive (.o2r) — FASE 1.5 amendment, Tarek 2026-10-25: "Hashear el contenido (Recommended)".
// The build re-packs spaghetti.o2r on every build and stamps every entry with the pack time, so a raw-file hash changes with
// no asset change. This hashes what the engine loads: each entry's name, CRC-32 and uncompressed size, read from the
// central directory. Timestamps, entry order, compression and the archive comment are not content.
// Std-only: compiled into the game and into oracle_tests.
#include <cstddef>
#include <cstdint>
#include <string>

namespace oracle {

struct ZipContent {
    uint64_t hash = 0;   // 0 = not hashed (error says why)
    size_t entries = 0;
    std::string error;
};

// Canonical form, FNV-1a-64, little-endian: U32 entryCount, then for each entry sorted by (name bytes, crc32, size):
// U32 nameLength · name bytes · U32 crc32 · U32 uncompressedSize. Refused (hash 0): unreadable file, no end-of-central-
// directory record, multi-disk or ZIP64 archives, a central directory that is truncated or disagrees with the entry count.
ZipContent HashZipContent(const std::string& path);

} // namespace oracle
