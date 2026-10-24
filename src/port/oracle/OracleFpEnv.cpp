#include "OracleFpEnv.h"

#include <cfenv>

// Compile-time guard: this TU (and therefore the game target) must never be built under a fast FP model.
#if defined(__FAST_MATH__)
#error "FASE 1.1 FP contract violated: this TU is compiled with fast-math (-ffast-math). Fix cmake/SetFlags.cmake."
#endif
#if defined(_M_FP_FAST)
#error "FASE 1.1 FP contract violated: this TU is compiled with /fp:fast. Fix cmake/SetFlags.cmake."
#endif

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif
#if defined(_M_ARM64)
#include <intrin.h>
#endif

extern "C" uint64_t Oracle_ReadFpEnvWord(void) {
    uint64_t word = 0;
#if defined(__x86_64__) || defined(_M_X64)
    word |= (uint64_t) _mm_getcsr(); // RC bits 13-14, FTZ bit 15, DAZ bit 6
    word |= (1ull << 40);
#elif defined(__aarch64__) || defined(_M_ARM64)
    uint64_t fpcr = 0;
#if defined(_MSC_VER)
    fpcr = (uint64_t) _ReadStatusReg(ARM64_FPCR);
#else
    __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr)); // RMode bits 22-23, FZ bit 24
#endif
    word |= (fpcr & 0xFFFFFFFFull);
    word |= (1ull << 41);
#endif
    word |= ((uint64_t) ((uint32_t) std::fegetround() & 0xFFu)) << 32;
    return word;
}
