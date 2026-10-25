#pragma once
// C++-internal oracle state (FASE 1.2, extended in 1.3). Game C code includes Oracle.h only.
#include <cstdint>
#include <string>

enum class OracleMode : uint8_t { Off = 0, Record = 1, Replay = 2 };

struct OracleConfig {
    OracleMode mode = OracleMode::Off;
    std::string recordPath;  // --oracle-record <path>
    std::string replayPath;  // --oracle-replay <path>
    std::string hashLogPath; // --oracle-hash-log <path>; defaults to <record|replay path>.mk64hash
    uint16_t seed = 0x1234;  // --oracle-seed N  (FASE 1.7)
    bool exitAtEnd = false;
    int32_t dumpAtIteration = -1; // --oracle-dump-at N (FASE 1.6)
    int32_t probeTickAt = -1;     // --oracle-probe-tick-at N: inject gTickLogic = 3 at iteration N (replay only)
};

// Process-wide singleton (OracleState.cpp, std-only), set by Oracle_ParseArgs (OracleCli.cpp).
OracleConfig& Oracle_Config();

// Fatal oracle failure (Oracle.cpp): logs CRITICAL, flushes the log and the partial recording (no kRecEnd), exits 4.
[[noreturn]] void Oracle_Abort(const char* reason);

// Clean shutdown (Oracle.cpp): closes the recording with kRecEnd. Idempotent. Called before GameEngine::Instance->Destroy().
// false = the recording could not be completed (main then exits 4 after Destroy(), so the user's config is still saved).
bool Oracle_Shutdown();
