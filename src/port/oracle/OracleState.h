#pragma once
// C++-internal oracle state (FASE 1.2, extended in 1.3 and 1.4). Game C code includes Oracle.h only.
#include <cstdint>
#include <string>

#include "OracleHash.h" // GroupHashes (std-only)

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
    // FASE 1.4.5 probe 1 (audio thread vs. simulation state, T3 P9/N9):
    bool probeStraddle = false;    // --oracle-probe-straddle: re-hash after EndAudioFrame(), log each group that changed
    bool probeAudioSerial = false; // --oracle-probe-audio-serial: run the audio frame BEFORE the iteration (no overlap)
    bool probeSelftest = false;    // --oracle-probe-selftest: flip gRandomSeed16 for the re-hash (control; never recording)
};

// Process-wide singleton (OracleState.cpp, std-only), set by Oracle_ParseArgs (OracleCli.cpp).
OracleConfig& Oracle_Config();

// Fatal oracle failure (Oracle.cpp): logs CRITICAL, flushes the log, the partial recording (no kRecEnd) and the partial hash
// log (no END), exits 4.
[[noreturn]] void Oracle_Abort(const char* reason);

// Clean shutdown (Oracle.cpp): closes the recording with kRecEnd and the hash log with `END n`. Idempotent. Called before
// GameEngine::Instance->Destroy(). false = the recording or the hash log could not be completed (main then exits 4 after
// Destroy(), so the user's config is still saved).
bool Oracle_Shutdown();

// FASE 1.4.5 probe 1 (Oracle.cpp). AtIterationEnd keeps the iteration's hashes; AfterAudio (push_frame, right after
// GameEngine::EndAudioFrame()) re-hashes and logs `oracle: STRADDLE iter=N group=G` for every group that changed. No-ops
// unless --oracle-probe-straddle.
void Oracle_ProbeAtIterationEnd(uint32_t iteration, const oracle::GroupHashes& g);
void Oracle_ProbeAfterAudio();
