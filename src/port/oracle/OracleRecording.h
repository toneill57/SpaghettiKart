#pragma once
// The .mk64rec file format (FASE 1.3). Std-only: compiled into the game and into oracle_tests.
// Little-endian, packed. Offsets are measured by `oracle_tests --print-offsets` (FASE 1.5), never trusted from a table.
#include <bit>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

static_assert(std::endian::native == std::endian::little, ".mk64rec is defined little-endian; a big-endian host must refuse");

namespace oracle {

constexpr char kMagic[8] = { 'M', 'K', '6', '4', 'R', 'E', 'C', '1' };
constexpr uint32_t kVersion = 1;
constexpr uint8_t kRecPads = 0x01;      // iteration index + 4 pads
constexpr uint8_t kRecSetupRace = 0x02; // iteration index (FASE 1.7)
constexpr uint8_t kRecEnd = 0xFF;       // written by the recorder on clean shutdown
constexpr uint32_t kFlushEvery = 256;   // records between fflush calls, so a crash leaves a usable prefix

#pragma pack(push, 1)
struct Pad {
    uint16_t button;
    int8_t stick_x;
    int8_t stick_y;
    int8_t right_stick_x;
    int8_t right_stick_y;
};

struct Header {
    char magic[8];         // "MK64REC1"
    uint32_t version;      // kVersion
    uint32_t headerBytes;  // sizeof(Header): lets a v1 reader skip unknown trailing fields — never reorder fields
    char buildVersion[32]; // SPAGHETTI_VERSION (git describe) — INFORMATIONAL
    char compiler[48];
    char buildType[16];    // ORACLE_BUILD_TYPE ($<CONFIG>)
    char platform[16];     // "macos-arm64", "windows-x64"
    uint64_t gameArchiveHash; // FNV-1a-64 of mk64.o2r
    uint64_t portArchiveHash; // FNV-1a-64 of spaghetti.o2r
    uint64_t binaryHash;      // FNV-1a-64 of the running executable — INFORMATIONAL in Fase 1, enforced by Fase 3
    uint64_t modListHash;     // FASE 1.4 (0 until then)
    uint64_t configHash;      // FASE 1.4 (0 until then)
    uint64_t fpWord;          // Oracle_ReadFpEnvWord() (FASE 1.1)
    int32_t tickLogic;        // pinned 2 (FASE 1.2)
    int32_t skipIntro;        // CVar gSkipIntro at launch
    uint16_t seed;            // gRandomSeed16 written at setup_race (FASE 1.7)
    uint16_t reserved0;
    uint32_t reserved1;
};

struct PadsRecord {
    uint8_t type;
    uint32_t iteration;
    Pad pads[4];
};

struct EventRecord {
    uint8_t type;
    uint32_t iteration;
};
#pragma pack(pop)

static_assert(sizeof(Pad) == 6, "Pad must be 6 bytes");
static_assert(sizeof(PadsRecord) == 29, "PadsRecord must be 29 bytes");
static_assert(sizeof(EventRecord) == 5, "EventRecord must be 5 bytes");
static_assert(sizeof(Header) == 192, "Header layout: 8+4+4+32+48+16+16 + 6*8 + 4+4+2+2+4 = 192 bytes");

class Writer {
  public:
    bool open(const std::string& path, const Header& h); // writes the header; false on failure (lastError says why)
    bool writePads(uint32_t iteration, const Pad pads[4]);
    bool writeEvent(uint8_t type, uint32_t iteration);
    bool flush();  // pushes buffered records to the OS (used by Oracle_Abort: partial file, no kRecEnd)
    bool close();  // writes kRecEnd, fflush, fclose; idempotent
    bool isOpen() const {
        return f_ != nullptr;
    }
    std::string lastError;

  private:
    bool countAndMaybeFlush();
    std::FILE* f_ = nullptr;
    uint32_t sinceFlush_ = 0;
};

struct Recording {
    Header header{};
    std::vector<PadsRecord> pads;
    std::vector<EventRecord> events;
    bool cleanEnd = false;
};

// Where MakeHeader found the three hashed files (logged once by the recorder: a zero hash must be explainable).
struct IdentityProbe {
    std::string gameArchivePath;
    std::string portArchivePath;
    std::string binaryPath;
};

// The run's build identity (OracleIdentity.cpp — game target only: it needs libultraship's Context and CVars).
Header MakeHeader(int32_t tickLogic, uint16_t seed, IdentityProbe* probe = nullptr);

// Validates magic / version / headerBytes. A file truncated mid-record is NOT an error: the complete records are kept and
// cleanEnd stays false. An unknown record type is an error.
bool read(const std::string& path, Recording& out, std::string& error);

} // namespace oracle
