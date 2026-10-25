#pragma once
// Command-line surface of the oracle (FASE 1.3). Std-only: compiled into the game and into oracle_tests.
#include "OracleState.h"

// Returns -1 to continue into the game, or an exit code for a no-window mode / usage error. The caller leaves with
// fflush + _Exit(rc) (Game.cpp main: returning would run ~World before the world exists).
int Oracle_ParseArgs(int argc, char** argv);                       // parses into Oracle_Config()
int Oracle_ParseArgsInto(int argc, char** argv, OracleConfig& cfg); // the same, into a given config (unit tests)

// --oracle-info: key=value lines on stdout (headerBytes=, pads=, events=, cleanEnd=, every header field). 0 ok, 2 error.
int Oracle_PrintInfo(const char* path);

// --oracle-verify: stub returning 2 until FASE 1.6 (OracleVerify.cpp).
extern "C" int Oracle_Verify(const char* pathA, const char* pathB);
