// STUB so FASE 1.3 links; FASE 1.6 replaces this file's body with the real verifier.
#include "OracleCli.h"

#include <cstdio>

extern "C" int Oracle_Verify(const char* pathA, const char* pathB) {
    (void) pathA;
    (void) pathB;
    std::fprintf(stderr, "oracle: --oracle-verify is not implemented until FASE 1.6\n");
    return 2;
}
