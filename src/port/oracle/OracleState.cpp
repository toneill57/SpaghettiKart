// Std-only half of the oracle state, so the CLI parser links into oracle_tests (FASE 1.3).
#include "OracleState.h"

OracleConfig& Oracle_Config() {
    static OracleConfig sConfig;
    return sConfig;
}
