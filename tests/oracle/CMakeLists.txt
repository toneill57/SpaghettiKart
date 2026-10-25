# tests/oracle/CMakeLists.txt — the oracle's test suite (MK64 Online, FASE 1.2). Std-only TUs: no game, libultraship or spdlog
# headers. Every TU is listed explicitly (tests/ is outside the src/ globs of the root CMakeLists.txt).
set(ORACLE_SRC ${CMAKE_SOURCE_DIR}/src/port/oracle)
add_executable(oracle_tests oracle_tests.cpp ${ORACLE_SRC}/OracleGuards.cpp)        # FASE 1.2
# Appended by later sub-phases, one line each (never a second add_executable):
#   1.5 target_sources(oracle_tests PRIVATE ${ORACLE_SRC}/OracleReplay.cpp)
#   1.6 target_sources(oracle_tests PRIVATE ${ORACLE_SRC}/OracleDump.cpp)   (OracleVerify.cpp is already in, from 1.3: the CLI links it)
#   1.7 target_sources(oracle_tests PRIVATE ${ORACLE_SRC}/OracleSetupRace.cpp)
#   1.8 add_test(oracle_replay_verify_script …) · 1.9 the CLI tests
target_sources(oracle_tests PRIVATE ${ORACLE_SRC}/OracleRecording.cpp ${ORACLE_SRC}/OracleState.cpp
               ${ORACLE_SRC}/OracleCli.cpp ${ORACLE_SRC}/OracleVerify.cpp)                # FASE 1.3
target_include_directories(oracle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${ORACLE_SRC})   # tests include "port/oracle/X.h"
set_property(TARGET oracle_tests PROPERTY CXX_STANDARD 20)
target_compile_definitions(oracle_tests PRIVATE ORACLE_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
if(MSVC)
  target_compile_options(oracle_tests PRIVATE /fp:strict)
  set_property(TARGET oracle_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_compile_options(oracle_tests PRIVATE -fno-fast-math -ffp-contract=off)
endif()
add_test(NAME oracle_tests COMMAND oracle_tests)
set_tests_properties(oracle_tests PROPERTIES LABELS "oracle;unit" TIMEOUT 120)
