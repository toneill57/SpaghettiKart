#pragma once
// C++-internal oracle state (FASE 1.2). Game C code includes Oracle.h only.
#include <cstdint>
#include <string>

enum class OracleMode : uint8_t { Off = 0, Record = 1, Replay = 2 };

struct OracleConfig {
    OracleMode mode = OracleMode::Off;
    std::string recordPath;  // --oracle-record <path>
    std::string replayPath;  // --oracle-replay <path>
    std::string hashLogPath; // --oracle-hash-log <path>
    uint16_t seed = 0x1234;  // --oracle-seed N  (FASE 1.7)
    bool exitAtEnd = false;
    int32_t dumpAtIteration = -1; // --oracle-dump-at N (FASE 1.6)
};

OracleConfig& Oracle_Config(); // process-wide singleton, set by Oracle_ParseArgs (FASE 1.3)

// Fatal oracle failure: logs CRITICAL, flushes the log (and, from 1.3/1.4, the recording and hash files), exits 4.
[[noreturn]] void Oracle_Abort(const char* reason);
