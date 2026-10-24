#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 64-bit FP-environment word, read at startup (FASE 1.1) and carried by the recording header (1.3)
 * and the build-identity handshake (Fase 3). Layout:
 *   bits  0-31 : MXCSR (x86-64) or FPCR low 32 bits (arm64); 0 on other ISAs
 *   bits 32-39 : fegetround() value (FE_TONEAREST etc.), masked to 8 bits
 *   bit  40    : 1 = x86-64
 *   bit  41    : 1 = arm64
 *   bit  63    : compiled with fast-math (must be 0 after FASE 1.1; the #error in OracleFpEnv.cpp
 *                makes it unreachable)
 * The word only READS the environment; nothing here sets rounding, FTZ or DAZ (a Fase 6 decision).
 * Thread-local state: call it on the main thread, where the simulation runs. */
uint64_t Oracle_ReadFpEnvWord(void);

#ifdef __cplusplus
}
#endif
