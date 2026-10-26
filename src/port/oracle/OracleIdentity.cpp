// Build identity of a recording (FASE 1.3). Game target only (Context paths, CVars); the format itself is std-only.
#include <libultraship.h>

#include "OracleRecording.h"
#include "OracleHash.h"
#include "OracleFpEnv.h"
#include "OracleStateHash.h" // ConfigHash, ModListHash (FASE 1.4)

#include <cstring>
#include <fstream>
#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifndef ORACLE_BUILD_TYPE
#error "ORACLE_BUILD_TYPE must be defined by cmake/SetFlags.cmake ($<CONFIG>)"
#endif

namespace {

// The same names and resolvers the engine uses to mount them: src/port/Engine.h:5-6 (names),
// src/engine/mods/ModManager.cpp:72-73 (mk64.o2r from the app directory; spaghetti.o2r across app dirs).
constexpr const char* kGameArchive = "mk64.o2r";
constexpr const char* kPortArchive = "spaghetti.o2r";

std::string ExecutablePath() { // "" ⇒ binaryHash 0 (Windows: GetModuleFileNameW, Fase 6)
#if defined(__APPLE__)
    char buf[4096];
    uint32_t n = sizeof buf;
    if (_NSGetExecutablePath(buf, &n) == 0) {
        return buf;
    }
#endif
    return {};
}

uint64_t HashFile(const std::string& path) { // 0 when the file is absent or unreadable — the header then says so
    if (path.empty()) {
        return 0;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return 0;
    }
    oracle::Hasher h;
    static char buf[1 << 16];
    while (in.read(buf, sizeof buf) || in.gcount() > 0) {
        h.bytes(buf, (size_t) in.gcount());
    }
    return h.value();
}

} // namespace

oracle::Header oracle::MakeHeader(int32_t tickLogic, uint16_t seed, IdentityProbe* probe) {
    Header h{};
    std::memcpy(h.magic, kMagic, sizeof h.magic);
    h.version = kVersion;
    h.headerBytes = sizeof(Header);
    std::snprintf(h.buildVersion, sizeof h.buildVersion, "%s", SPAGHETTI_VERSION);
#if defined(__clang_version__)
    std::snprintf(h.compiler, sizeof h.compiler, "clang %s", __clang_version__);
#elif defined(_MSC_FULL_VER)
    std::snprintf(h.compiler, sizeof h.compiler, "msvc %d", _MSC_FULL_VER);
#endif
    std::snprintf(h.buildType, sizeof h.buildType, "%s", ORACLE_BUILD_TYPE);
#if defined(__APPLE__) && defined(__aarch64__)
    std::snprintf(h.platform, sizeof h.platform, "macos-arm64");
#elif defined(__APPLE__)
    std::snprintf(h.platform, sizeof h.platform, "macos-x64");
#elif defined(_WIN32)
    std::snprintf(h.platform, sizeof h.platform, "windows-x64");
#endif
    const std::string gamePath = Ship::Context::GetPathRelativeToAppDirectory(kGameArchive);
    const std::string portPath = Ship::Context::LocateFileAcrossAppDirs(kPortArchive);
    const std::string binaryPath = ExecutablePath();
    h.gameArchiveHash = HashFile(gamePath);
    h.portArchiveHash = HashFile(portPath);
    h.binaryHash = HashFile(binaryPath); // spec consequence #6 — recorded and printed; enforced only by Fase 3
    h.fpWord = Oracle_ReadFpEnvWord();
    h.tickLogic = tickLogic;
    h.skipIntro = CVarGetInteger("gSkipIntro", 0);
    h.seed = seed;
    size_t modListEntries = 0;
    h.configHash = oracle::ConfigHash();                   // FASE 1.4.5 probe 2 (T3 P11)
    h.modListHash = oracle::ModListHash(&modListEntries); // FASE 1.4.5 probes 3-4 (T3 P13/P14)
    if (probe != nullptr) {
        *probe = IdentityProbe{ gamePath, portPath, binaryPath, modListEntries };
    }
    return h;
}
