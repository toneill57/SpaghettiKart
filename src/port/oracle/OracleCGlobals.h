#pragma once
// MK64 Online FASE 1.4c: what the state hash covers among the game's mutable C globals, definition by definition.
// Std-only: oracle_tests re-derives every file-scope and function-local non-const definition of every C file the game
// compiles (the CMakeLists.txt globs, their *.inc.c and the game headers they include) FROM SOURCE TEXT, and fails on a
// definition in no table, a row naming a definition that no longer exists, or a definition in two tables
// (test_cglobal_coverage). The compiler's IR is the second method (implement/fase-1-artifacts/1.4c/globals.py: which
// definitions an iteration STORES to); the two must agree.
//
// Three tables, every definition in exactly one:
//   kCGlobalsHashed    stored to in an iteration (or hashed by an older group) — hashed; `rule` names how
//   kCGlobalsExcluded  stored to, NOT hashed — `rule` is the reason, with the lines that prove it
//   kCGlobalsNotStored never stored to in an iteration — the IR class the sweep measured. A snapshot: a NEW
//                      definition is in no table and fails the critic until someone classifies it.
//
// Rules (kCGlobalsHashed's `rule`; the 1.4b rules of OracleCoverage.h, plus decision 3's pool rule):
//   scalar      put(h, "name", g) — scalars, enums, fixed arrays (element by element), POD structs with an
//               ORACLE_*_FIELDS list in OracleStatePut.h
//   size_t      put(h, "name", (uint64_t) g) — width differs on 32-bit targets
//   player-ptr  the index p - gPlayers in [0, 8), -1 for null; anything else is Oracle_Abort (never an address)
//   camera-ptr  the index in cameras[], -1 for null
//   screen-ptr  the index in gScreenContexts[], -1 for null
//   pool-ptr    decision 3: a pointer (or an address held in a uintptr_t) into racing/memory.c's bump pool, as its
//               offset from the pool's end (sPoolEnd), -1 for null; outside the pool is Oracle_Abort. Then, while
//               the allocation is live, the elements it holds, field-wise (OracleCGlobalsHash.cpp: LiveIn)
//   array-index a pointer into ONE fixed global array (named in the hashing statement), as its byte offset from the
//               array's first byte, -1 for null; outside the array is Oracle_Abort — 1.4b's "what it names" rule
//               for an array that has no index helper of its own
//   track-ptr   FINDING (1.4c): a uintptr_t holding the heap address of the C++ Track, which the game only compares
//               with the current track: -1 null, 0 current, 1 stale. A rule beyond 1.4b's table, for the tower
//   bytes       an object whose every byte is data (no padding, no pointer): its bytes
//   group:<g>   hashed by another group's walk (named): the 1.4 v1 globals, and `world` for the three 1.4b reads
#include <cstddef>

namespace oracle {

inline constexpr const char* kCGlobalRules[] = { "scalar",   "size_t",      "player-ptr", "camera-ptr", "screen-ptr",
                                                 "pool-ptr", "array-index", "track-ptr",  "bytes" };

struct CGlobalRow {
    const char* symbol; // as the IR names it: a function-local static is "<function>.<name>"
    const char* file;   // the defining file, repo-relative
    const char* rule;   // kCGlobalsHashed: one of kCGlobalRules or group:<g>; kCGlobalsExcluded: the reason
};

inline constexpr CGlobalRow kCGlobalsHashed[] = {
    // Hashed by an older group (the 1.4 v1 walk, and 1.4b's World::HashState tail).
    { "gRandomSeed16", "src/buffers/random.c", "group:rng" },
    { "cameras", "src/camera.c", "group:cameras" },
    { "gIsGamePaused", "src/code_800029B0.c", "group:race" },
    { "gIsInQuitToMenuTransition", "src/code_800029B0.c", "group:race" },
    { "cpu_BehaviourState", "src/code_80005FD0.c", "group:racearrays" },
    { "gCourseCompletionPercentByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gCurrentCPUBehaviourId", "src/code_80005FD0.c", "group:racearrays" },
    { "gGPCurrentRacePlayerIdByRank", "src/code_80005FD0.c", "group:racearrays" },
    { "gGPCurrentRaceRankByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gIsPlayerWrongDirection", "src/code_80005FD0.c", "group:racearrays" },
    { "gLapCompletionPercentByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gLapCountByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gNearestPathPointByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gNumPathPointsTraversed", "src/code_80005FD0.c", "group:racearrays" },
    { "gPathIndexByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gPlayersTrackSectionId", "src/code_80005FD0.c", "group:racearrays" },
    { "gPreviousGPCurrentRaceRankByPlayerId", "src/code_80005FD0.c", "group:racearrays" },
    { "gPreviousLapProgressScore", "src/code_80005FD0.c", "group:racearrays" },
    { "gSpeedCPUBehaviour", "src/code_80005FD0.c", "group:racearrays" },
    { "gWrongDirectionCounter", "src/code_80005FD0.c", "group:racearrays" },
    { "D_80165898", "src/code_80057C60.c", "group:world" },
    { "D_801658CE", "src/code_80057C60.c", "group:world" },
    { "D_801658DC", "src/code_80057C60.c", "group:world" },
    { "gObjectList", "src/code_80057C60.c", "group:objects" },
    { "objectListSize", "src/code_80057C60.c", "group:objects" },
    { "gRaceFrameCounter", "src/code_80057C60_var.c", "group:race" },
    { "gControllers", "src/main.c", "group:controllers" },
    { "gCourseTimer", "src/main.c", "group:race" },
    { "gGamestate", "src/main.c", "group:race" },
    { "gGamestateNext", "src/main.c", "group:race" },
    { "gGlobalTimer", "src/main.c", "group:race" },
    { "gPlayers", "src/main.c", "group:players" },
    { "gRaceState", "src/main.c", "group:race" },
    { "gTickCounter", "src/main.c", "group:race" },
    { "gTickLogic", "src/main.c", "group:race" },
    // Hashed by `cglobals` (OracleCGlobalsHash.cpp, the same order).
    // BEGIN GENERATED cglobals rows (1.4c/gen_hash_tu.py)
    { "isNotTheFirst", "src/animation.c", "scalar" },
    { "sMatrixShouldNotPop", "src/animation.c", "scalar" },
    { "sMatrixStackSize", "src/animation.c", "scalar" },
    { "sOriginalPosAnimation", "src/animation.c", "scalar" },
    { "D_802BFB80", "src/buffers/buffers.c", "bytes" },
    { "D_801649D8", "src/camera.c", "scalar" },
    { "D_801649E8", "src/camera.c", "scalar" },
    { "D_801649F8", "src/camera.c", "scalar" },
    { "D_80164A08", "src/camera.c", "scalar" },
    { "D_80164A18", "src/camera.c", "scalar" },
    { "D_80164A28", "src/camera.c", "scalar" },
    { "D_80164A2C", "src/camera.c", "scalar" },
    { "D_80164A30", "src/camera.c", "scalar" },
    { "D_80164A38", "src/camera.c", "scalar" },
    { "D_80164A48", "src/camera.c", "scalar" },
    { "D_80164A78", "src/camera.c", "scalar" },
    { "D_80164A88", "src/camera.c", "scalar" },
    { "D_80164A89", "src/camera.c", "scalar" },
    { "D_80164A90", "src/camera.c", "scalar" },
    { "D_80164AA0", "src/camera.c", "scalar" },
    { "sStagingTimer", "src/camera.c", "scalar" },
    { "D_800DC5A8", "src/code_800029B0.c", "scalar" },
    { "D_800DC5AC", "src/code_800029B0.c", "scalar" },
    { "D_800DC5B0", "src/code_800029B0.c", "scalar" },
    { "D_800DC5B8", "src/code_800029B0.c", "scalar" },
    { "D_800DC5C8", "src/code_800029B0.c", "scalar" },
    { "D_800DC5DC", "src/code_800029B0.c", "scalar" },
    { "D_800DC5E0", "src/code_800029B0.c", "scalar" },
    { "D_800DC5E4", "src/code_800029B0.c", "scalar" },
    { "D_8015F58C", "src/code_800029B0.c", "scalar" },
    { "D_8015F59C", "src/code_800029B0.c", "scalar" },
    { "D_8015F5A0", "src/code_800029B0.c", "scalar" },
    { "D_8015F5A4", "src/code_800029B0.c", "scalar" },
    { "D_8015F6F4", "src/code_800029B0.c", "scalar" },
    { "D_8015F6F6", "src/code_800029B0.c", "scalar" },
    { "D_8015F6FA", "src/code_800029B0.c", "scalar" },
    { "D_8015F6FC", "src/code_800029B0.c", "scalar" },
    { "D_8015F700", "src/code_800029B0.c", "scalar" },
    { "D_8015F702", "src/code_800029B0.c", "scalar" },
    { "D_8015F704", "src/code_800029B0.c", "scalar" },
    { "D_8015F708", "src/code_800029B0.c", "scalar" },
    { "D_8015F738", "src/code_800029B0.c", "scalar" },
    { "D_8015F748", "src/code_800029B0.c", "scalar" },
    { "D_8015F758", "src/code_800029B0.c", "scalar" },
    { "D_8015F768", "src/code_800029B0.c", "scalar" },
    { "D_8015F778", "src/code_800029B0.c", "scalar" },
    { "D_8015F890", "src/code_800029B0.c", "scalar" },
    { "D_8015F892", "src/code_800029B0.c", "scalar" },
    { "D_8015F894", "src/code_800029B0.c", "scalar" },
    { "D_8015F8D0", "src/code_800029B0.c", "scalar" },
    { "D_8015F8DC", "src/code_800029B0.c", "scalar" },
    { "D_8015F8E0", "src/code_800029B0.c", "scalar" },
    { "bDrawSkybox", "src/code_800029B0.c", "scalar" },
    { "bFog", "src/code_800029B0.c", "scalar" },
    { "gCollisionIndices", "src/code_800029B0.c", "pool-ptr" },
    { "gCollisionMesh", "src/code_800029B0.c", "pool-ptr" },
    { "gCollisionMeshCount", "src/code_800029B0.c", "scalar" },
    { "gCreditsCourseId", "src/code_800029B0.c", "scalar" },
    { "gCurrentCourseId", "src/code_800029B0.c", "scalar" },
    { "gCurrentlyLoadedTrackAddr", "src/code_800029B0.c", "track-ptr" },
    { "gDebugPathCount", "src/code_800029B0.c", "scalar" },
    { "gFreeMemoryCourseAnchor", "src/code_800029B0.c", "pool-ptr" },
    { "gFreeMemoryResetAnchor", "src/code_800029B0.c", "pool-ptr" },
    { "gFreeMemorySize", "src/code_800029B0.c", "size_t" },
    { "gIsMirrorMode", "src/code_800029B0.c", "scalar" },
    { "gNextFreeMemoryAddress", "src/code_800029B0.c", "pool-ptr" },
    { "gNmiUnknown1", "src/code_800029B0.c", "array-index" },
    { "gNmiUnknown2", "src/code_800029B0.c", "array-index" },
    { "gNmiUnknown3", "src/code_800029B0.c", "array-index" },
    { "gNmiUnknown4", "src/code_800029B0.c", "array-index" },
    { "gNmiUnknown5", "src/code_800029B0.c", "array-index" },
    { "gNmiUnknown6", "src/code_800029B0.c", "array-index" },
    { "gNumCollisionTriangles", "src/code_800029B0.c", "scalar" },
    { "gNumPermanentActors", "src/code_800029B0.c", "scalar" },
    { "gNumScreens", "src/code_800029B0.c", "scalar" },
    { "gNumSpawnedShells", "src/code_800029B0.c", "scalar" },
    { "gPlaceItemBoxes", "src/code_800029B0.c", "scalar" },
    { "gPlayerPositionLUT", "src/code_800029B0.c", "scalar" },
    { "gPlayerWinningIndex", "src/code_800029B0.c", "scalar" },
    { "gQuitToMenuTransitionCounter", "src/code_800029B0.c", "scalar" },
    { "gScreenContexts", "src/code_800029B0.c", "scalar" },
    { "gTimePlayerLastTouchedFinishLine", "src/code_800029B0.c", "scalar" },
    { "gTrackDirection", "src/code_800029B0.c", "scalar" },
    { "gTrackMaxX", "src/code_800029B0.c", "scalar" },
    { "gTrackMaxY", "src/code_800029B0.c", "scalar" },
    { "gTrackMaxZ", "src/code_800029B0.c", "scalar" },
    { "gTrackMinX", "src/code_800029B0.c", "scalar" },
    { "gTrackMinY", "src/code_800029B0.c", "scalar" },
    { "gTrackMinZ", "src/code_800029B0.c", "scalar" },
    { "gWaterLevel", "src/code_800029B0.c", "scalar" },
    { "gWaterVelocity", "src/code_800029B0.c", "scalar" },
    { "sIsController1Unplugged", "src/code_800029B0.c", "scalar" },
    { "D_80162E70", "src/code_80005FD0.c", "scalar" },
    { "D_80162EB0", "src/code_80005FD0.c", "scalar" },
    { "D_80162EB2", "src/code_80005FD0.c", "scalar" },
    { "D_80162F10", "src/code_80005FD0.c", "scalar" },
    { "D_80162F50", "src/code_80005FD0.c", "scalar" },
    { "D_80162FB0", "src/code_80005FD0.c", "scalar" },
    { "D_80162FC0", "src/code_80005FD0.c", "scalar" },
    { "D_80162FD0", "src/code_80005FD0.c", "scalar" },
    { "D_80162FF8", "src/code_80005FD0.c", "scalar" },
    { "D_80163010", "src/code_80005FD0.c", "scalar" },
    { "D_80163090", "src/code_80005FD0.c", "scalar" },
    { "D_801630E8", "src/code_80005FD0.c", "scalar" },
    { "D_80163100", "src/code_80005FD0.c", "scalar" },
    { "D_80163128", "src/code_80005FD0.c", "scalar" },
    { "D_80163150", "src/code_80005FD0.c", "scalar" },
    { "D_801631CC", "src/code_80005FD0.c", "scalar" },
    { "D_801631E0", "src/code_80005FD0.c", "scalar" },
    { "D_801631F8", "src/code_80005FD0.c", "scalar" },
    { "D_80163238", "src/code_80005FD0.c", "scalar" },
    { "D_80163240", "src/code_80005FD0.c", "scalar" },
    { "D_80163330", "src/code_80005FD0.c", "scalar" },
    { "D_80163344", "src/code_80005FD0.c", "scalar" },
    { "D_80163348", "src/code_80005FD0.c", "scalar" },
    { "D_8016334C", "src/code_80005FD0.c", "scalar" },
    { "D_80163368", "src/code_80005FD0.c", "scalar" },
    { "D_80163398", "src/code_80005FD0.c", "scalar" },
    { "D_801633B0", "src/code_80005FD0.c", "scalar" },
    { "D_801633C8", "src/code_80005FD0.c", "scalar" },
    { "D_801633E0", "src/code_80005FD0.c", "scalar" },
    { "D_801633F8", "src/code_80005FD0.c", "scalar" },
    { "D_80163410", "src/code_80005FD0.c", "scalar" },
    { "D_80163418", "src/code_80005FD0.c", "scalar" },
    { "D_80163428", "src/code_80005FD0.c", "scalar" },
    { "D_80163438", "src/code_80005FD0.c", "scalar" },
    { "D_8016347C", "src/code_80005FD0.c", "scalar" },
    { "D_8016347E", "src/code_80005FD0.c", "scalar" },
    { "D_80163480", "src/code_80005FD0.c", "scalar" },
    { "D_80163484", "src/code_80005FD0.c", "scalar" },
    { "D_80163488", "src/code_80005FD0.c", "scalar" },
    { "D_8016348C", "src/code_80005FD0.c", "scalar" },
    { "D_801634C0", "src/code_80005FD0.c", "scalar" },
    { "D_801634EC", "src/code_80005FD0.c", "scalar" },
    { "D_801634F0", "src/code_80005FD0.c", "scalar" },
    { "D_801634F4", "src/code_80005FD0.c", "scalar" },
    { "D_80163DD8", "src/code_80005FD0.c", "scalar" },
    { "D_80164358", "src/code_80005FD0.c", "scalar" },
    { "D_8016435A", "src/code_80005FD0.c", "scalar" },
    { "D_8016435C", "src/code_80005FD0.c", "scalar" },
    { "D_80164378", "src/code_80005FD0.c", "scalar" },
    { "D_8016448C", "src/code_80005FD0.c", "scalar" },
    { "D_80164498", "src/code_80005FD0.c", "scalar" },
    { "D_80164538", "src/code_80005FD0.c", "scalar" },
    { "D_801645D0", "src/code_80005FD0.c", "scalar" },
    { "D_801645E8", "src/code_80005FD0.c", "scalar" },
    { "D_801645F8", "src/code_80005FD0.c", "scalar" },
    { "D_80164608", "src/code_80005FD0.c", "scalar" },
    { "D_80164618", "src/code_80005FD0.c", "scalar" },
    { "D_80164628", "src/code_80005FD0.c", "scalar" },
    { "D_80164638", "src/code_80005FD0.c", "scalar" },
    { "D_80164648", "src/code_80005FD0.c", "scalar" },
    { "D_80164658", "src/code_80005FD0.c", "scalar" },
    { "D_80164670", "src/code_80005FD0.c", "scalar" },
    { "D_80164678", "src/code_80005FD0.c", "scalar" },
    { "D_80164680", "src/code_80005FD0.c", "scalar" },
    { "D_80164688", "src/code_80005FD0.c", "scalar" },
    { "D_80164698", "src/code_80005FD0.c", "scalar" },
    { "D_8016469C", "src/code_80005FD0.c", "scalar" },
    { "D_801646A0", "src/code_80005FD0.c", "scalar" },
    { "D_801646C0", "src/code_80005FD0.c", "scalar" },
    { "D_801646C8", "src/code_80005FD0.c", "scalar" },
    { "D_801646CC", "src/code_80005FD0.c", "scalar" },
    { "D_801646D0", "src/code_80005FD0.c", "scalar" },
    { "bInMultiPathSection", "src/code_80005FD0.c", "scalar" },
    { "bStopAICrossing", "src/code_80005FD0.c", "scalar" },
    { "cpu_ItemStrategy", "src/code_80005FD0.c", "scalar" },
    { "cpu_TargetSpeed", "src/code_80005FD0.c", "scalar" },
    { "cpu_enteringPathIntersection", "src/code_80005FD0.c", "scalar" },
    { "cpu_exitingPathIntersection", "src/code_80005FD0.c", "scalar" },
    { "gBestRankedHumanPlayer", "src/code_80005FD0.c", "scalar" },
    { "gCourseCompletionPercentByRank", "src/code_80005FD0.c", "scalar" },
    { "gCurrentCpuTargetSpeed", "src/code_80005FD0.c", "scalar" },
    { "gCurrentNearestPathPoint", "src/code_80005FD0.c", "scalar" },
    { "gCurrentPathPointExpectedRotationPath", "src/code_80005FD0.c", "pool-ptr" },
    { "gCurrentPlayerLookAhead", "src/code_80005FD0.c", "scalar" },
    { "gCurrentTrackConsecutiveCurveCountsPath", "src/code_80005FD0.c", "pool-ptr" },
    { "gCurrentTrackPath", "src/code_80005FD0.c", "pool-ptr" },
    { "gCurrentTrackSectionTypesPath", "src/code_80005FD0.c", "pool-ptr" },
    { "gGPCurrentRaceRankByPlayerIdDup", "src/code_80005FD0.c", "scalar" },
    { "gGetPlayerByCharacterId", "src/code_80005FD0.c", "scalar" },
    { "gIncrementUpdatePlayer", "src/code_80005FD0.c", "scalar" },
    { "gIsInExtra", "src/code_80005FD0.c", "scalar" },
    { "gIsPlayerInCurve", "src/code_80005FD0.c", "scalar" },
    { "gIsPlayerNewPathPoint", "src/code_80005FD0.c", "scalar" },
    { "gLookBehindActive", "src/code_80005FD0.c", "scalar" },
    { "gNearestPathPointByCameraId", "src/code_80005FD0.c", "scalar" },
    { "gOffsetPosition", "src/code_80005FD0.c", "scalar" },
    { "gPathCountByPathIndex", "src/code_80005FD0.c", "scalar" },
    { "gPathExpectedRotation", "src/code_80005FD0.c", "pool-ptr" },
    { "gPathStartZ", "src/code_80005FD0.c", "scalar" },
    { "gPlayerPathIndex", "src/code_80005FD0.c", "scalar" },
    { "gPlayerPathY", "src/code_80005FD0.c", "scalar" },
    { "gPlayerTrackPositionFactorInstruction", "src/code_80005FD0.c", "scalar" },
    { "gPreviousAngleSteering", "src/code_80005FD0.c", "scalar" },
    { "gPreviousCPUBehaviourId", "src/code_80005FD0.c", "scalar" },
    { "gPreviousCpuTargetSpeed", "src/code_80005FD0.c", "scalar" },
    { "gPreviousPlayerAiOffsetX", "src/code_80005FD0.c", "scalar" },
    { "gPreviousPlayerAiOffsetZ", "src/code_80005FD0.c", "scalar" },
    { "gPreviousPlayerZ", "src/code_80005FD0.c", "scalar" },
    { "gSelectedPathCount", "src/code_80005FD0.c", "scalar" },
    { "gTrackConsecutiveCurveCounts", "src/code_80005FD0.c", "pool-ptr" },
    { "gTrackLeftPaths", "src/code_80005FD0.c", "pool-ptr" },
    { "gTrackPaths", "src/code_80005FD0.c", "pool-ptr" },
    { "gTrackPositionFactor", "src/code_80005FD0.c", "scalar" },
    { "gTrackRightPaths", "src/code_80005FD0.c", "pool-ptr" },
    { "gTrackSectionTypes", "src/code_80005FD0.c", "pool-ptr" },
    { "gUnexpiredActorsList", "src/code_80005FD0.c", "scalar" },
    { "gVehicle2DPathPoint", "src/code_80005FD0.c", "pool-ptr" },
    { "gVehicle2DPathSize", "src/code_80005FD0.c", "scalar" },
    { "gVehiclePath", "src/code_80005FD0.c", "pool-ptr" },
    { "gVehiclePathSize", "src/code_80005FD0.c", "size_t" },
    { "sCrossingActiveTimer", "src/code_80005FD0.c", "scalar" },
    { "sPlayerAngle", "src/code_80005FD0.c", "scalar" },
    { "sSomeNearestPathPoint", "src/code_80005FD0.c", "scalar" },
    { "sVehicleSoundRenderCounter", "src/code_80005FD0.c", "scalar" },
    { "D_80165590", "src/code_80057C60.c", "scalar" },
    { "D_80165594", "src/code_80057C60.c", "scalar" },
    { "D_80165598", "src/code_80057C60.c", "scalar" },
    { "D_8016559C", "src/code_80057C60.c", "scalar" },
    { "D_801655A4", "src/code_80057C60.c", "scalar" },
    { "D_801655AC", "src/code_80057C60.c", "scalar" },
    { "D_801655B4", "src/code_80057C60.c", "scalar" },
    { "D_801655BC", "src/code_80057C60.c", "scalar" },
    { "D_801655C0", "src/code_80057C60.c", "scalar" },
    { "D_801655C4", "src/code_80057C60.c", "scalar" },
    { "D_801655CC", "src/code_80057C60.c", "scalar" },
    { "D_801655D8", "src/code_80057C60.c", "scalar" },
    { "D_801655E8", "src/code_80057C60.c", "scalar" },
    { "D_801655F8", "src/code_80057C60.c", "scalar" },
    { "D_80165608", "src/code_80057C60.c", "scalar" },
    { "D_80165618", "src/code_80057C60.c", "scalar" },
    { "D_80165628", "src/code_80057C60.c", "scalar" },
    { "D_80165638", "src/code_80057C60.c", "scalar" },
    { "D_80165648", "src/code_80057C60.c", "scalar" },
    { "D_80165658", "src/code_80057C60.c", "scalar" },
    { "D_801656B0", "src/code_80057C60.c", "scalar" },
    { "D_801656C0", "src/code_80057C60.c", "scalar" },
    { "D_801656D0", "src/code_80057C60.c", "scalar" },
    { "D_801656E0", "src/code_80057C60.c", "scalar" },
    { "D_801656F0", "src/code_80057C60.c", "scalar" },
    { "D_80165708", "src/code_80057C60.c", "scalar" },
    { "D_80165730", "src/code_80057C60.c", "scalar" },
    { "D_80165754", "src/code_80057C60.c", "scalar" },
    { "D_80165760", "src/code_80057C60.c", "scalar" },
    { "D_8016576A", "src/code_80057C60.c", "scalar" },
    { "D_80165770", "src/code_80057C60.c", "scalar" },
    { "D_80165780", "src/code_80057C60.c", "scalar" },
    { "D_8016578C", "src/code_80057C60.c", "scalar" },
    { "D_80165790", "src/code_80057C60.c", "scalar" },
    { "D_80165794", "src/code_80057C60.c", "scalar" },
    { "D_8016579C", "src/code_80057C60.c", "scalar" },
    { "D_8016579E", "src/code_80057C60.c", "scalar" },
    { "D_801657A2", "src/code_80057C60.c", "scalar" },
    { "D_801657AE", "src/code_80057C60.c", "scalar" },
    { "D_801657B2", "src/code_80057C60.c", "scalar" },
    { "D_801657B8", "src/code_80057C60.c", "scalar" },
    { "D_801657D0", "src/code_80057C60.c", "scalar" },
    { "D_801657D8", "src/code_80057C60.c", "scalar" },
    { "D_801657E1", "src/code_80057C60.c", "scalar" },
    { "D_801657E2", "src/code_80057C60.c", "scalar" },
    { "D_801657E3", "src/code_80057C60.c", "scalar" },
    { "D_801657E5", "src/code_80057C60.c", "scalar" },
    { "D_801657E6", "src/code_80057C60.c", "scalar" },
    { "D_801657E7", "src/code_80057C60.c", "scalar" },
    { "D_801657E8", "src/code_80057C60.c", "scalar" },
    { "D_801657F0", "src/code_80057C60.c", "scalar" },
    { "D_801657F8", "src/code_80057C60.c", "scalar" },
    { "D_801657FC", "src/code_80057C60.c", "scalar" },
    { "D_80165800", "src/code_80057C60.c", "scalar" },
    { "D_80165804", "src/code_80057C60.c", "scalar" },
    { "D_80165808", "src/code_80057C60.c", "scalar" },
    { "D_8016580C", "src/code_80057C60.c", "scalar" },
    { "D_80165810", "src/code_80057C60.c", "scalar" },
    { "D_80165814", "src/code_80057C60.c", "scalar" },
    { "D_80165818", "src/code_80057C60.c", "scalar" },
    { "D_8016581C", "src/code_80057C60.c", "scalar" },
    { "D_80165820", "src/code_80057C60.c", "scalar" },
    { "D_80165828", "src/code_80057C60.c", "scalar" },
    { "D_8016582C", "src/code_80057C60.c", "scalar" },
    { "D_80165832", "src/code_80057C60.c", "scalar" },
    { "D_80165834", "src/code_80057C60.c", "scalar" },
    { "D_80165840", "src/code_80057C60.c", "scalar" },
    { "D_80165860", "src/code_80057C60.c", "scalar" },
    { "D_8016586C", "src/code_80057C60.c", "scalar" },
    { "D_80165878", "src/code_80057C60.c", "scalar" },
    { "D_8016587C", "src/code_80057C60.c", "scalar" },
    { "D_80165888", "src/code_80057C60.c", "scalar" },
    { "D_80165890", "src/code_80057C60.c", "scalar" },
    { "D_801658A8", "src/code_80057C60.c", "scalar" },
    { "D_801658BC", "src/code_80057C60.c", "scalar" },
    { "D_801658C6", "src/code_80057C60.c", "scalar" },
    { "D_801658D6", "src/code_80057C60.c", "scalar" },
    { "D_801658E4", "src/code_80057C60.c", "scalar" },
    { "D_801658EC", "src/code_80057C60.c", "scalar" },
    { "D_801658F4", "src/code_80057C60.c", "scalar" },
    { "D_801658FE", "src/code_80057C60.c", "scalar" },
    { "D_80165900", "src/code_80057C60.c", "scalar" },
    { "D_80165908", "src/code_80057C60.c", "scalar" },
    { "D_80165A90", "src/code_80057C60.c", "scalar" },
    { "D_80183DA8", "src/code_80057C60.c", "scalar" },
    { "D_80183DC8", "src/code_80057C60.c", "scalar" },
    { "D_80183E40", "src/code_80057C60.c", "scalar" },
    { "D_80183E50", "src/code_80057C60.c", "scalar" },
    { "D_80183E70", "src/code_80057C60.c", "scalar" },
    { "D_80183E80", "src/code_80057C60.c", "scalar" },
    { "D_80183E98", "src/code_80057C60.c", "scalar" },
    { "D_8018C3B0", "src/code_80057C60.c", "scalar" },
    { "D_8018CE10", "src/code_80057C60.c", "scalar" },
    { "D_8018CF10", "src/code_80057C60.c", "scalar" },
    { "D_8018CF14", "src/code_80057C60.c", "camera-ptr" },
    { "D_8018CF18", "src/code_80057C60.c", "scalar" },
    { "D_8018CF1C", "src/code_80057C60.c", "player-ptr" },
    { "D_8018CF20", "src/code_80057C60.c", "scalar" },
    { "D_8018CF48", "src/code_80057C60.c", "scalar" },
    { "D_8018CF50", "src/code_80057C60.c", "scalar" },
    { "D_8018CF60", "src/code_80057C60.c", "scalar" },
    { "D_8018CF68", "src/code_80057C60.c", "scalar" },
    { "D_8018CF78", "src/code_80057C60.c", "scalar" },
    { "D_8018CF90", "src/code_80057C60.c", "scalar" },
    { "D_8018CF98", "src/code_80057C60.c", "scalar" },
    { "D_8018CFA8", "src/code_80057C60.c", "scalar" },
    { "D_8018CFAC", "src/code_80057C60.c", "scalar" },
    { "D_8018CFB0", "src/code_80057C60.c", "scalar" },
    { "D_8018CFB4", "src/code_80057C60.c", "scalar" },
    { "D_8018CFB8", "src/code_80057C60.c", "scalar" },
    { "D_8018CFBC", "src/code_80057C60.c", "scalar" },
    { "D_8018CFC0", "src/code_80057C60.c", "scalar" },
    { "D_8018CFC4", "src/code_80057C60.c", "scalar" },
    { "D_8018CFC8", "src/code_80057C60.c", "scalar" },
    { "D_8018CFCC", "src/code_80057C60.c", "scalar" },
    { "D_8018CFD0", "src/code_80057C60.c", "scalar" },
    { "D_8018CFD4", "src/code_80057C60.c", "scalar" },
    { "D_8018CFD8", "src/code_80057C60.c", "scalar" },
    { "gControllerRandom", "src/code_80057C60.c", "scalar" },
    { "gGPCurrentRaceCharacterIdByRank", "src/code_80057C60.c", "scalar" },
    { "gHUDDisable", "src/code_80057C60.c", "scalar" },
    { "gHUDModes", "src/code_80057C60.c", "scalar" },
    { "gIndexLakituList", "src/code_80057C60.c", "scalar" },
    { "gItemWindowObjectByPlayerId", "src/code_80057C60.c", "scalar" },
    { "gLakituTexturePtr", "src/code_80057C60.c", "array-index" },
    { "gLeafParticle", "src/code_80057C60.c", "scalar" },
    { "gNextFreeLeafParticle", "src/code_80057C60.c", "scalar" },
    { "gNextFreeObjectParticle1", "src/code_80057C60.c", "scalar" },
    { "gNextFreeObjectParticle2", "src/code_80057C60.c", "scalar" },
    { "gNextFreeObjectParticle3", "src/code_80057C60.c", "scalar" },
    { "gNextFreeObjectParticle4", "src/code_80057C60.c", "scalar" },
    { "gObjectParticle1", "src/code_80057C60.c", "scalar" },
    { "gObjectParticle2", "src/code_80057C60.c", "scalar" },
    { "gObjectParticle3", "src/code_80057C60.c", "scalar" },
    { "gObjectParticle4", "src/code_80057C60.c", "scalar" },
    { "indexObjectList1", "src/code_80057C60.c", "scalar" },
    { "indexObjectList2", "src/code_80057C60.c", "scalar" },
    { "indexObjectList3", "src/code_80057C60.c", "scalar" },
    { "indexObjectList4", "src/code_80057C60.c", "scalar" },
    { "playerHUD", "src/code_80057C60.c", "scalar" },
    { "D_8018CFE0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFE4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFE8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFEC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFF0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFF4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018CFF8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D000", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D008", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D00C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D010", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D018", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D020", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D028", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D048", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D050", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D070", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D078", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D098", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D0A0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D0C0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D0C8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D0E8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D0F0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D110", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D114", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D140", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D150", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D158", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D160", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D168", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D16C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D170", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D174", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D178", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D17C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D180", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D184", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D18C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D190", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1A0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1B4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1C4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1CC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1D4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1DC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1F0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1F8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D1FC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D204", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D20C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D214", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D21C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D224", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D228", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D230", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D2A4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D2AC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D2B4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D2BC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D2C8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D314", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D320", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D380", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D384", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D388", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3BC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3C0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3C4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3D4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3D8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3DC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3E0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3E4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3E8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3EC", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3F0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3F4", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D3F8", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D400", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D40C", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D410", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D620", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D650", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D6B0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D710", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D770", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D7A0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D7D0", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D800", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D830", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D860", "src/code_80057C60_var.c", "scalar" },
    { "D_8018D890", "src/code_80057C60_var.c", "scalar" },
    { "gIsHUDVisible", "src/code_80057C60_var.c", "scalar" },
    { "gMatrixHudCount", "src/code_80057C60_var.c", "scalar" },
    { "gMinimapHeight", "src/code_80057C60_var.c", "scalar" },
    { "gMinimapWidth", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonCount", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonDepartingTimer", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonPosX", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonPosY", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonPosZ", "src/code_80057C60_var.c", "scalar" },
    { "gPlayerBalloonStatus", "src/code_80057C60_var.c", "scalar" },
    { "xOrientation", "src/code_80057C60_var.c", "scalar" },
    { "D_8018EDB0", "src/code_800AF9B0.c", "scalar" },
    { "D_8018EDB2", "src/code_800AF9B0.c", "scalar" },
    { "D_8018EDB4", "src/code_800AF9B0.c", "scalar" },
    { "gKartGravityTable", "src/data/kart_attributes.c", "scalar" },
    { "gKartHopInitialVelocityTable", "src/data/kart_attributes.c", "scalar" },
    { "gMarioRacewayAIDistances", "src/data/path_spawn_metadata.c", "scalar" },
    { "D_8018D900", "src/effects.c", "scalar" },
    { "D_8018D920", "src/effects.c", "scalar" },
    { "D_8018D990", "src/effects.c", "scalar" },
    { "gPlayerBooEffectStartTime", "src/effects.c", "scalar" },
    { "gPlayerOtherScreensAlpha", "src/effects.c", "scalar" },
    { "gPlayerStarEffectStartTime", "src/effects.c", "scalar" },
    { "D_802856B0", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802856B4", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802856C4", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802876D4", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802876D8", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802876E0", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_80287818", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_80287998", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_80287B1E", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_80287B20", "src/ending/ceremony_and_credits.c", "scalar" },
    { "gCutsceneShotTimer", "src/ending/ceremony_and_credits.c", "scalar" },
    { "gOrderedSizeSlidingBorders", "src/ending/ceremony_and_credits.c", "scalar" },
    { "gSizeSlidingBorders", "src/ending/ceremony_and_credits.c", "scalar" },
    { "sCutsceneShot", "src/ending/ceremony_and_credits.c", "scalar" },
    { "sCutsceneSplineSegment", "src/ending/ceremony_and_credits.c", "scalar" },
    { "sCutsceneSplineSegmentProgress", "src/ending/ceremony_and_credits.c", "scalar" },
    { "D_802874A0", "src/ending/code_80280000.c", "scalar" },
    { "D_80287554", "src/ending/code_80281C40.c", "scalar" },
    { "gGotoMenu", "src/ending/code_80281C40.c", "scalar" },
    { "D_802874B0", "src/ending/podium_ceremony_actors.c", "scalar" },
    { "D_802874D8", "src/ending/podium_ceremony_actors.c", "scalar" },
    { "D_802874FC", "src/ending/podium_ceremony_actors.c", "scalar" },
    { "D_80287540", "src/ending/podium_ceremony_actors.c", "scalar" },
    { "sPodiumActorList", "src/ending/podium_ceremony_actors.c", "pool-ptr" },
    { "sRandomSeed16", "src/ending/podium_ceremony_actors.c", "scalar" },
    { "D_800DC514", "src/main.c", "scalar" },
    { "D_800DC540", "src/main.c", "scalar" },
    { "D_800DC544", "src/main.c", "scalar" },
    { "D_800DC568", "src/main.c", "scalar" },
    { "D_800DC56C", "src/main.c", "scalar" },
    { "D_80150118", "src/main.c", "scalar" },
    { "D_8015011E", "src/main.c", "scalar" },
    { "D_80150120", "src/main.c", "scalar" },
    { "D_8015014C", "src/main.c", "scalar" },
    { "D_80150150", "src/main.c", "scalar" },
    { "D_80152308", "src/main.c", "scalar" },
    { "gActiveScreenMode", "src/main.c", "scalar" },
    { "gCCSelection", "src/main.c", "scalar" },
    { "gCollisionGrid", "src/main.c", "scalar" },
    { "gControllerBits", "src/main.c", "scalar" },
    { "gControllerPads", "src/main.c", "scalar" },
    { "gControllerStatuses", "src/main.c", "scalar" },
    { "gDeltaTime", "src/main.c", "scalar" },
    { "gDemoMode", "src/main.c", "scalar" },
    { "gEnableDebugMode", "src/main.c", "scalar" },
    { "gGotoMode", "src/main.c", "scalar" },
    { "gMatrixObjectCount", "src/main.c", "scalar" },
    { "gModeSelection", "src/main.c", "scalar" },
    { "gNumActors", "src/main.c", "scalar" },
    { "gPlayerCountSelection1", "src/main.c", "scalar" },
    { "gScreenAspect", "src/main.c", "scalar" },
    { "gScreenModeSelection", "src/main.c", "scalar" },
    { "gTourComplete", "src/main.c", "scalar" },
    { "gVBlankTimer", "src/main.c", "scalar" },
    { "sBillBoardMtx", "src/main.c", "scalar" },
    { "sNumVBlanks", "src/main.c", "scalar" },
    { "sRenderedFramebuffer", "src/main.c", "scalar" },
    { "sRenderingFramebuffer", "src/main.c", "scalar" },
    { "wasSoftReset", "src/main.c", "scalar" },
    { "D_800E852C", "src/menu_items.c", "scalar" },
    { "D_8018D9D8", "src/menu_items.c", "scalar" },
    { "D_8018D9D9", "src/menu_items.c", "scalar" },
    { "D_8018DEE0", "src/menu_items.c", "scalar" },
    { "D_8018E838", "src/menu_items.c", "scalar" },
    { "D_8018E840", "src/menu_items.c", "scalar" },
    { "D_8018E850", "src/menu_items.c", "scalar" },
    { "D_8018E858", "src/menu_items.c", "scalar" },
    { "D_8018ED90", "src/menu_items.c", "scalar" },
    { "D_8018ED91", "src/menu_items.c", "scalar" },
    { "gCharacterIdByGPOverallRank", "src/menu_items.c", "scalar" },
    { "gControllerPak1FileNote", "src/menu_items.c", "scalar" },
    { "gControllerPak1MaxWriteableFiles", "src/menu_items.c", "scalar" },
    { "gControllerPak1NumFilesUsed", "src/menu_items.c", "scalar" },
    { "gControllerPak1NumPagesFree", "src/menu_items.c", "scalar" },
    { "gControllerPak2FileNote", "src/menu_items.c", "scalar" },
    { "gCurrentTransitionTime", "src/menu_items.c", "scalar" },
    { "gCycleFlashMenu", "src/menu_items.c", "scalar" },
    { "gGPPointsByCharacterId", "src/menu_items.c", "scalar" },
    { "gMenuItems", "src/menu_items.c", "scalar" },
    { "gNumD_8018E768Entries", "src/menu_items.c", "scalar" },
    { "gSaveData", "src/menu_items.c", "bytes" },
    { "gSomeDLBuffer", "src/menu_items.c", "pool-ptr" },
    { "gTextColor", "src/menu_items.c", "scalar" },
    { "gTransitionDuration", "src/menu_items.c", "scalar" },
    { "gTransitionType", "src/menu_items.c", "scalar" },
    { "pfsError", "src/menu_items.c", "scalar" },
    { "pfsState", "src/menu_items.c", "scalar" },
    { "sGPPointsCopy", "src/menu_items.c", "pool-ptr" },
    { "sIntroLogoTimer", "src/menu_items.c", "scalar" },
    { "sIntroModelMotionSpeed", "src/menu_items.c", "scalar" },
    { "sIntroModelSpeed", "src/menu_items.c", "scalar" },
    { "sMenuTextureEntries", "src/menu_items.c", "scalar" },
    { "sMenuTextureListIndex", "src/menu_items.c", "scalar" },
    { "sMenuTextureMap", "src/menu_items.c", "scalar" },
    { "sTKMK00_LowResBuffer", "src/menu_items.c", "pool-ptr" },
    { "gBattleResultCursorSelection", "src/menus.c", "scalar" },
    { "gCharacterGridIsSelected", "src/menus.c", "scalar" },
    { "gCharacterGridSelections", "src/menus.c", "scalar" },
    { "gCharacterSelections", "src/menus.c", "scalar" },
    { "gControllerPakMenuSelection", "src/menus.c", "scalar" },
    { "gControllerPakScrollDirection", "src/menus.c", "scalar" },
    { "gControllerPakSelectedTableRow", "src/menus.c", "scalar" },
    { "gControllerPakVisibleTableRows", "src/menus.c", "scalar" },
    { "gCourseIndexInCup", "src/menus.c", "scalar" },
    { "gCourseRecordsMenuSelection", "src/menus.c", "scalar" },
    { "gCourseRecordsSubMenuSelection", "src/menus.c", "scalar" },
    { "gCupSelection", "src/menus.c", "scalar" },
    { "gDebugGotoScene", "src/menus.c", "scalar" },
    { "gDebugMenuSelection", "src/menus.c", "scalar" },
    { "gDemoUseController", "src/menus.c", "scalar" },
    { "gFadeModeSelection", "src/menus.c", "scalar" },
    { "gGameModeMenuColumn", "src/menus.c", "scalar" },
    { "gGameModeSubMenuColumn", "src/menus.c", "scalar" },
    { "gGhostPlayerInit", "src/menus.c", "scalar" },
    { "gIntroModelPosX", "src/menus.c", "scalar" },
    { "gIntroModelPosY", "src/menus.c", "scalar" },
    { "gIntroModelPosZ", "src/menus.c", "scalar" },
    { "gIntroModelRotX", "src/menus.c", "scalar" },
    { "gIntroModelRotY", "src/menus.c", "scalar" },
    { "gIntroModelRotZ", "src/menus.c", "scalar" },
    { "gIntroModelScale", "src/menus.c", "scalar" },
    { "gIntroModelZEye", "src/menus.c", "scalar" },
    { "gMainMenuSelection", "src/menus.c", "scalar" },
    { "gMenuDelayTimer", "src/menus.c", "scalar" },
    { "gMenuFadeType", "src/menus.c", "scalar" },
    { "gMenuSelection", "src/menus.c", "scalar" },
    { "gMenuTimingCounter", "src/menus.c", "scalar" },
    { "gNextDemoId", "src/menus.c", "scalar" },
    { "gPlayerCount", "src/menus.c", "scalar" },
    { "gPlayerSelectMenuSelection", "src/menus.c", "scalar" },
    { "gScreenModeListIndex", "src/menus.c", "scalar" },
    { "gSoundMode", "src/menus.c", "scalar" },
    { "gSubMenuSelection", "src/menus.c", "scalar" },
    { "gTimeTrialDataCourseIndex", "src/menus.c", "scalar" },
    { "gTimeTrialsResultCursorSelection", "src/menus.c", "scalar" },
    { "gTrackMapInit", "src/menus.c", "scalar" },
    { "gVersusResultCursorSelection", "src/menus.c", "scalar" },
    { "sTempCupSelection", "src/menus.c", "scalar" },
    { "unref_D_8018EE0C", "src/menus.c", "scalar" },
    { "D_800DC664", "src/profiler.c", "scalar" },
    { "D_800DC668", "src/profiler.c", "scalar" },
    { "D_800DC66C", "src/profiler.c", "scalar" },
    { "gEnableResourceMeters", "src/profiler.c", "scalar" },
    { "D_802BA260", "src/racing/actors.c", "scalar" },
    { "gTLUTRedShell", "src/racing/actors.c", "scalar" },
    { "numTimes", "src/racing/collision.c", "scalar" },
    { "gN64ResFrameBuffer", "src/racing/framebuffer_effects.c", "scalar" },
    { "gReusableFrameBuffer", "src/racing/framebuffer_effects.c", "scalar" },
    { "D_802BA030", "src/racing/race_logic.c", "scalar" },
    { "D_802BA032", "src/racing/race_logic.c", "scalar" },
    { "D_802BA034", "src/racing/race_logic.c", "scalar" },
    { "D_802BA038", "src/racing/race_logic.c", "scalar" },
    { "D_802BA040", "src/racing/race_logic.c", "scalar" },
    { "D_802BA048", "src/racing/race_logic.c", "scalar" },
    { "D_802B87BC", "src/racing/render_courses.c", "scalar" },
    { "D_802B87C4", "src/racing/render_courses.c", "scalar" },
    { "D_802B87C8", "src/racing/render_courses.c", "scalar" },
    { "D_802B87CC", "src/racing/render_courses.c", "scalar" },
    { "D_802B87D0", "src/racing/render_courses.c", "scalar" },
    { "D_802B87D4", "src/racing/render_courses.c", "scalar" },
    { "currentScreenSection", "src/racing/render_courses.c", "scalar" },
    { "gFogMax", "src/racing/render_courses.c", "scalar" },
    { "gFogMin", "src/racing/render_courses.c", "scalar" },
    { "D_80164B80", "src/render_player.c", "scalar" },
    { "D_80165020", "src/render_player.c", "scalar" },
    { "D_80165150", "src/render_player.c", "scalar" },
    { "D_80165190", "src/render_player.c", "scalar" },
    { "D_801651D0", "src/render_player.c", "scalar" },
    { "gLastAnimFrameSelector", "src/render_player.c", "scalar" },
    { "gLastAnimGroupSelector", "src/render_player.c", "scalar" },
    { "gMatrixEffectCount", "src/render_player.c", "scalar" },
    { "gPlayerBlueEffect", "src/render_player.c", "scalar" },
    { "gPlayerCyanEffect", "src/render_player.c", "scalar" },
    { "gPlayerGreenEffect", "src/render_player.c", "scalar" },
    { "gPlayerLastVelocity", "src/render_player.c", "scalar" },
    { "gPlayerMagentaEffect", "src/render_player.c", "scalar" },
    { "gPlayerRedEffect", "src/render_player.c", "scalar" },
    { "gPlayerYellowEffect", "src/render_player.c", "scalar" },
    { "gPlayersToRenderCount", "src/render_player.c", "scalar" },
    { "gPlayersToRenderPlayer", "src/render_player.c", "player-ptr" },
    { "gPlayersToRenderPlayerId", "src/render_player.c", "scalar" },
    { "gPlayersToRenderScreenId", "src/render_player.c", "scalar" },
    { "D_80162D86", "src/replays.c", "scalar" },
    { "D_80162DC8", "src/replays.c", "scalar" },
    { "D_80162DCC", "src/replays.c", "scalar" },
    { "D_80162DD0", "src/replays.c", "scalar" },
    { "D_80162DD8", "src/replays.c", "scalar" },
    { "D_80162DDC", "src/replays.c", "scalar" },
    { "D_80162DE0", "src/replays.c", "scalar" },
    { "D_80162DE4", "src/replays.c", "scalar" },
    { "D_80162DE8", "src/replays.c", "scalar" },
    { "D_80162DF4", "src/replays.c", "scalar" },
    { "D_80162DFC", "src/replays.c", "scalar" },
    { "D_80162E00", "src/replays.c", "scalar" },
    { "bCourseGhostDisabled", "src/replays.c", "scalar" },
    { "bPlayerGhostDisabled", "src/replays.c", "scalar" },
    { "gPauseTriggered", "src/replays.c", "scalar" },
    { "gPostTimeTrialReplayCannotSave", "src/replays.c", "scalar" },
    { "sButtonsPrevCourseGhost", "src/replays.c", "scalar" },
    { "sCourseGhostFramesRemaining", "src/replays.c", "scalar" },
    { "sCourseGhostReplay", "src/replays.c", "array-index" },
    { "sCourseGhostReplayIdx", "src/replays.c", "scalar" },
    { "sPlayerGhostButtonsPrev", "src/replays.c", "scalar" },
    { "sPlayerGhostFramesRemaining", "src/replays.c", "scalar" },
    { "sPlayerGhostReplay", "src/replays.c", "array-index" },
    { "sPlayerGhostReplayIdx", "src/replays.c", "scalar" },
    { "sPlayerInputIdx", "src/replays.c", "scalar" },
    { "sPlayerInputs", "src/replays.c", "array-index" },
    { "sPostTTButtonsPrev", "src/replays.c", "scalar" },
    { "sPostTTFramesRemaining", "src/replays.c", "scalar" },
    { "sPostTTReplay", "src/replays.c", "array-index" },
    { "sPostTTReplayIdx", "src/replays.c", "scalar" },
    { "sReplayGhostBuffer", "src/replays.c", "array-index" },
    { "sReplayGhostBufferSize", "src/replays.c", "size_t" },
    { "sUnusedReplayCounter", "src/replays.c", "scalar" },
    { "staff_ghost_track_ptr", "src/replays.c", "track-ptr" },
    { "D_8018EE10", "src/save.c", "scalar" },
    { "gControllerPak1State", "src/save.c", "scalar" },
    { "sControllerPak2State", "src/save.c", "scalar" },
    { "D_80165210", "src/spawn_players.c", "scalar" },
    { "D_80165230", "src/spawn_players.c", "scalar" },
    { "D_80165270", "src/spawn_players.c", "scalar" },
    { "D_801652C0", "src/spawn_players.c", "scalar" },
    { "D_801652E0", "src/spawn_players.c", "scalar" },
    { "D_80165300", "src/spawn_players.c", "scalar" },
    { "D_80165330", "src/spawn_players.c", "scalar" },
    { "D_80165340", "src/spawn_players.c", "scalar" },
    { "D_8016556E", "src/spawn_players.c", "scalar" },
    { "D_80165570", "src/spawn_players.c", "scalar" },
    { "D_80165572", "src/spawn_players.c", "scalar" },
    { "D_80165574", "src/spawn_players.c", "scalar" },
    { "D_80165576", "src/spawn_players.c", "scalar" },
    { "D_80165578", "src/spawn_players.c", "scalar" },
    { "D_8016557A", "src/spawn_players.c", "scalar" },
    { "D_8016557C", "src/spawn_players.c", "scalar" },
    { "D_8016557E", "src/spawn_players.c", "scalar" },
    { "D_80165580", "src/spawn_players.c", "scalar" },
    { "D_80165582", "src/spawn_players.c", "scalar" },
    { "chooseCPUPlayers", "src/spawn_players.c", "scalar" },
    { "gCopyNearestWaypointByPlayerId", "src/spawn_players.c", "scalar" },
    { "gCopyPathIndexByPlayerId", "src/spawn_players.c", "scalar" },
    { "gCountASwitch", "src/spawn_players.c", "scalar" },
    { "gCountBChangement", "src/spawn_players.c", "scalar" },
    { "gFrameSinceLastACombo", "src/spawn_players.c", "scalar" },
    { "gFrameSinceLastBCombo", "src/spawn_players.c", "scalar" },
    { "gIsPlayerTripleAButtonCombo", "src/spawn_players.c", "scalar" },
    { "gIsPlayerTripleBButtonCombo", "src/spawn_players.c", "scalar" },
    { "gPlayerAButtonComboActiveThisFrame", "src/spawn_players.c", "scalar" },
    { "gPlayerBButtonComboActiveThisFrame", "src/spawn_players.c", "scalar" },
    { "gPlayerCurrentSpeed", "src/spawn_players.c", "scalar" },
    { "gPlayerIsBrakeActive", "src/spawn_players.c", "scalar" },
    { "gPlayerIsThrottleActive", "src/spawn_players.c", "scalar" },
    { "gPlayerWaterLevel", "src/spawn_players.c", "scalar" },
    { "gTimerBoostTripleACombo", "src/spawn_players.c", "scalar" },
    { "gTimerBoostTripleBCombo", "src/spawn_players.c", "scalar" },
    { "osAppNmiBuffer", "src/stubs.c", "scalar" },
    // END GENERATED cglobals rows
};

#define ORACLE_AUDIO_REASON                                                                                   \
    "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe covers " \
    "its writes into hashed state"
inline constexpr CGlobalRow kCGlobalsExcluded[] = {
    { "gAudioLoadLock", "src/audio/audio_session_presets.c", ORACLE_AUDIO_REASON },
    { "D_803B7178", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAiBufferLengths", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAiBuffers", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioCmd", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioCmdBuffers", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioErrorFlags", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioRandom", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioTask", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioTaskIndex", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gAudioTasks", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gCurrAiBufferIndex", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "gRefreshRate", "src/audio/data.c", ORACLE_AUDIO_REASON },
    { "D_800E9DA0", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9DB4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9DC4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9DD4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9DE4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9DF4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E14", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E24", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E34", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E54", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E64", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E74", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E84", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9E94", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9EA4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9EB4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9EC4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9ED4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9EE4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9EF4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F04", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F14", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F24", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F2C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F34", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F54", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F74", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F78", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800E9F7C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA06C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA0EC", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA0F0", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA0F4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA104", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA108", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA10C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA110", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA120", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA130", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA160", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA164", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA168", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA16C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA170", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA174", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA178", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA17C", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA180", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA184", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1C0", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1C4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1E4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1E8", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1EC", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA1F4", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_800EA244", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018EF10", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018EF18", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018EFD8", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018FB90", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018FB91", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018FC08", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_8018FC10", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192AB8", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192C38", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192C48", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192CA8", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192CC6", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_80192CD0", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "D_801930D0", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "gCameraLastPos", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "gCopyCamera", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "gCurrentMusicSeq", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "gVelocityCamera", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sNumProcessedSoundRequests", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sNumSoundsInBank", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundBankDisabled", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundBankFreeListFront", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundBankUsedListBack", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundBanks", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundRequestCount", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "sSoundRequests", "src/audio/external.c", ORACLE_AUDIO_REASON },
    { "gAudioInitPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gAudioResetFadeOutFramesLeft", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gAudioResetPresetIdToLoad", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gAudioResetStatus", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gAudioSessionPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gBankLoadStatus", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gBankLoadedPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gNoteSubsEu", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gNotes", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gNotesAndBuffersPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gNumSynthesisReverbs", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gPersistentCommonPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gSeqAndBankPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gSeqLoadStatus", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gSeqLoadedPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gTemporaryCommonPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gUnkLoadStatus", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gUnusedLoadedPool", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gUseReverb", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "gVolume", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "sPersistentCommonPoolSplit", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "sSeqAndBankPoolSplit", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "sSessionPoolSplit", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "sTemporaryCommonPoolSplit", "src/audio/heap.c", ORACLE_AUDIO_REASON },
    { "D_803B6E60", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "D_803B70A8", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gAudioBufferParameters", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gAudioFrameCount", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gAudioLibSoundMode", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gCurrAudioFrameDmaCount", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gLayerFreeList", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gMaxAudioCmds", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gMaxSimultaneousNotes", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gNoteFreeLists", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gSampleDmaNumListItems", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gSequenceChannels", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gSequenceCount", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gSequenceLayers", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gSequencePlayers", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "gTempoInternalToExternal", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaListSize1", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueue1", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueue2", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueueHead1", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueueHead2", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueueTail1", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmaReuseQueueTail2", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "sSampleDmas", "src/audio/load.c", ORACLE_AUDIO_REASON },
    { "rspa", "src/audio/mixer.c", ORACLE_AUDIO_REASON },
    { "D_800EA3A0", "src/audio/port_eu.c", ORACLE_AUDIO_REASON },
    { "D_800EA3A4", "src/audio/port_eu.c", ORACLE_AUDIO_REASON },
    { "D_800EA484", "src/audio/port_eu.c", ORACLE_AUDIO_REASON },
    { "D_800EA4A4", "src/audio/port_eu.c", ORACLE_AUDIO_REASON },
    { "sAudioCmd", "src/audio/port_eu.c", ORACLE_AUDIO_REASON },
    { "gSynthesisReverbs", "src/audio/synthesis.c", ORACLE_AUDIO_REASON },
    // Decision 3 (Tarek, 2026-10-27, verbatim "Según lo medido" = "as measured"): the two huge buffers outside the
    // bump pool.
    { "gAudioHeap", "src/buffers/audio_heap.c",
      "audio state: the audio pools are carved from it (src/audio/heap.c:199-200) and the audio thread writes them; its "
      "only direct writers clear it (src/audio/load.c:722, 734) in audio_init, called from SDL_main "
      "(src/port/Game.cpp:1076) and thread4_audio (src/main.c:1198); its contents hold pointers" },
    { "gFramebuffers", "src/buffers/buffers.c",
      "never written in the port: its only writer is thread3_video (src/main.c:1027-1034, through the gFramebufferN "
      "macros of src/buffers.h:97-99), created only by thread1_idle (src/main.c:238), created only by main_func "
      "(src/main.c:214), which nothing calls (the IR: no caller, never address-taken; libultraship, torch and src/port "
      "never name it)" },
    // Judged one by one (implement/fase-1-artifacts/1.4c/classify/decisions-judged.tsv): evidence gathered by four
    // read-only agents, adjudicated by the executor; every reason cites its writer / reader lines.
    // BEGIN GENERATED judged exclusions (1.4c/gen_judged_rows.py)
    { "ground_f3d_material_013_lights", "courses/harbour/track.c",
      "render-only: Only reader is a display-list command (track.c:5275) consumed by the renderer; both runtime "
      "writers are track-load constants (HM_Intro.cpp:64, Harbour.cpp:612); no simulation code references the "
      "symbol." },
    { "sRockSpawnData", "src/actors/falling_rock/update.inc.c",
      "dead (stored, never read): Only reader (update.inc.c:68) is unreachable code after `return;` at :61 and "
      "its call site is commented out (actors.c:2521). The only value ever stored is the address of a "
      "TestTrack::BeginPlay stack array (TestTrack.cpp:164)." },
    { "sReverbSettings", "src/audio/audio_session_presets.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gDefaultEnvelope", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gDefaultShortNoteDurationTable", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gDefaultShortNoteVelocityTable", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gUnknownWave7", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "sSawtoothWaves", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "sSineWaves", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "sSquareWaves", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "sTriangleWaves", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "sUnknownWave6", "src/audio/data.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_800EA150", "src/audio/external.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_800EA1C8", "src/audio/external.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_800EA1D4", "src/audio/external.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_800EA1DC", "src/audio/external.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gUnkPool1", "src/audio/heap.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_803B6720", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_803B6738", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_803B6740", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gCurrAudioFrameDmaMesgBufs", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gCurrAudioFrameDmaQueue", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gSequenceChannelNone", "src/audio/load.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_801937C0", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_801937D8", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_801937F0", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_80193808", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_80194020", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_80194028", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_80194038", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "D_8019403C", "src/audio/port_eu.c",
      "audio thread state (FASE 1.4c out of scope): src/audio runs on the audio thread; 1.4's straddle probe "
      "covers its writes into hashed state: ESCAPES-only in src/audio (by directory, like the 150 audio rows)" },
    { "gEncodedKartTexture", "src/buffers/buffers.c",
      "asset pointer: Pointer-only struct whose values are selected asset names (kart_dma.c:612-661), read "
      "solely to build texture loads in render_player.c:1362-1585. The inputs of the selection (effects, "
      "animFrameSelector, animGroupSelector, tyreSpeed, characterId, unk_0A8) are all in ORACLE_PLAYER_FIELDS "
      "already." },
    { "gPlayerPalettesList", "src/buffers/buffers.c",
      "render-only: Bytes are copies of asset palettes (kart_dma.c:679,686,719) selected by already-hashed "
      "player fields; the only reader is gDPLoadTLUT_pal256 in render_player.c (:1375-1488). Nothing "
      "simulation-side reads it." },
    { "gZBuffer", "src/buffers/buffers.c",
      "never written (every escape traced to a read): Every escape ends in a display-list depth/colour image "
      "command; the interpreter stores only the pointer (interpreter.cpp:3049) and compares it (:2982); Z "
      "clears become GL clears (ClearDepthRegion). Nothing stores into the 153,600-byte array. (ESCAPES-only; "
      "wave 2, E5)" },
    { "D_800DC610", "src/code_800029B0.c",
      "render-only: Direction is recomputed every render_track() from D_802B87D4 (a constant set at track "
      "load); readers are gSPSetLights1 display-list commands and the editor window. No simulation code reads "
      "the lights." },
    { "D_8015F590", "src/code_800029B0.c",
      "dead (stored, never read): Written through the func_802B5CAC out-parameter (ChocoMountain.cpp:157 -> "
      "math_util.c:397-399), but no code reads D_8015F590: rg finds only decl, extern and that call. Stored, "
      "never read. (ESCAPES-only; wave 2, E5)" },
    { "D_8015F790", "src/code_800029B0.c",
      "never written (every escape traced to a read): Only its address is stored (into ScreenContext.unkC, "
      "skybox_and_splitscreen.c:739,804); unkC is never read anywhere, so no code dereferences the 256-byte "
      "array. Zero-initialised, never written. (ESCAPES-only; wave 2, E5)" },
    { "D_801625F8", "src/code_800029B0.c",
      "dead (stored, never read): Only reader feeds D_801625FC, which nobody reads. Value = 0 - "
      "gNextFreeMemoryAddress (gHeapEndPtr never assigned), so address-derived; must never be hashed. Written "
      "only in podium/credits setup." },
    { "D_801625FC", "src/code_800029B0.c",
      "dead (stored, never read): moved OUT of the mechanical scalars: written only from the address-derived "
      "D_801625F8 (code_80280000.c:208, code_80281780.c:171), read by nothing (decl + extern + 2 writes)" },
    { "gFogColour", "src/code_800029B0.c",
      "render-only: All writers are track-load constants or editor UI; all readers are gDPSetFogColor / skybox "
      "vertex colours / editor save. No simulation reader. (Hashing it would also be safe: deterministic and "
      "tiny.)" },
    { "gScreenFourCtx", "src/code_800029B0.c",
      "never written (every escape traced to a read): The variable is only ever loaded. The 'escape' is an LLVM "
      "switch table of the variables' addresses (func_80057CE4), not an address-of in the source. All stores in "
      "the code go through the pointer into gScreenContexts[3], a separate row. (ESCAPES-only; wave 2, E5)" },
    { "gScreenOneCtx", "src/code_800029B0.c",
      "never written (every escape traced to a read): The variable is only ever loaded. The 'escape' is an LLVM "
      "switch table of the variables' addresses (func_80057CE4), not an address-of in the source. All stores in "
      "the code go through the pointer into gScreenContexts[0], a separate row. (ESCAPES-only; wave 2, E5)" },
    { "gScreenThreeCtx", "src/code_800029B0.c",
      "never written (every escape traced to a read): The variable is only ever loaded. The 'escape' is an LLVM "
      "switch table of the variables' addresses (func_80057CE4), not an address-of in the source. All stores in "
      "the code go through the pointer into gScreenContexts[2], a separate row. (ESCAPES-only; wave 2, E5)" },
    { "gScreenTwoCtx", "src/code_800029B0.c",
      "never written (every escape traced to a read): The variable is only ever loaded. The 'escape' is an LLVM "
      "switch table of the variables' addresses (func_80057CE4), not an address-of in the source. All stores in "
      "the code go through the pointer into gScreenContexts[1], a separate row. (ESCAPES-only; wave 2, E5)" },
    { "gVtxStretch", "src/code_800029B0.c",
      "UI / freecam: Only writer is the port-menu slider (PortMenu.cpp:462-470 -> Menu.cpp:369 -> "
      "UIWidgets::SliderFloat) and the only reader is that same widget; no simulation or render code uses "
      "gVtxStretch anywhere in the fork. (ESCAPES-only; wave 2, E5)" },
    { "vtxBuffer", "src/code_800029B0.c",
      "asset pointer: Only writer set_vtx_buffer stores Vtx addresses taken from static display lists or OTR "
      "resources (collision.c:2053/2069/2078); readers (:1708-1755) only copy them into CollisionTriangle at "
      "track load. Host-address scratch, nothing mutated through it." },
    { "gCoursesCPUBehaviour", "src/code_80005FD0.c",
      "dead (stored, never read): Stored at code_80005FD0.c:4162 and read nowhere in the repo (only decl + "
      "extern + that write). Every value is the same asset-table address anyway." },
    { "gCurrentTrackLeftPath", "src/code_80005FD0.c",
      "dead (stored, never read): Written at code_80005FD0.c:1336/3446 from gTrackLeftPaths[], never read "
      "anywhere in the repo (no reader in src, include or courses). Pool-block copy of an element of "
      "gTrackLeftPaths." },
    { "gCurrentTrackRightPath", "src/code_80005FD0.c",
      "dead (stored, never read): Written at code_80005FD0.c:1337/3447 from gTrackRightPaths[], never read "
      "anywhere in the repo (no reader in src, include or courses). Pool-block copy of an element of "
      "gTrackRightPaths." },
    { "gPaddleBoats", "src/code_80005FD0.c",
      "never written (every escape traced to a read): All three functions that store to or read-modify "
      "gPaddleBoats (init_vehicles_ferry, update_vehicle_paddle_boats, handle_paddle_boats_interactions) have "
      "no caller anywhere in the fork; the C++ OBoat replaced them. The array stays zero; the other uses only "
      "read it. (ESCAPES-only; wave 2, E5)" },
    { "sCurrentCPUBehaviour", "src/code_80005FD0.c",
      "asset pointer: All 3 writes point into the immutable AI-behaviour asset table and each read follows its "
      "write inside the same function (4184-4190, 4248-4252); no store through it. Pointer is a function-local "
      "in effect; the ids carry the state." },
    { "D_800E8900a", "src/code_80057C60.c",
      "never written (every escape traced to a read): The only use of the symbol is the address operand of 4 "
      "gSPVertex calls in func_80066BAC; the interpreter reads vertices through a const pointer "
      "(interpreter.cpp:1554-1580). Nothing stores through the address. (ESCAPES-only; wave 2, E3)" },
    { "D_801655F0", "src/code_80057C60.c",
      "never written (every escape traced to a read): Sole use is the single call at code_80057C60.c:1237, "
      "whose callee ignores it (effects.c:35-37). No code stores through or loads the address, and no other "
      "reference exists. (ESCAPES-only; wave 2, E3)" },
    { "D_80165678", "src/code_80057C60.c",
      "never written (every escape traced to a read): All three escapes go to stubs whose pointer parameter is "
      "declared UNUSED and never dereferenced (effects.c:32-37). No other reference to the symbol exists. "
      "(ESCAPES-only; wave 2, E3)" },
    { "D_80183D60", "src/code_80057C60.c",
      "render-only: Written once per iteration by guOrtho with all-literal arguments (math_util_2.c:604), so "
      "the content is the same constant matrix every run and cannot carry sim state; the only reader is the HUD "
      "projection gSPMatrix (code_80057C60.c:409). Before the first call it is zero. (ESCAPES-only; wave 2, E3)" },
    { "D_80183DA0", "src/code_80057C60.c",
      "dead (stored, never read): Stored by 4 init_hud_* calls to find_unused_obj_index (write-only out-param) "
      "and referenced nowhere else (rg: decl, 2 externs/comments, 4 call sites). Dead value; if the executor "
      "prefers conservative, `scalar` also works. (ESCAPES-only; wave 2, E3)" },
    { "D_8018C0B0", "src/code_80057C60.c",
      "render-only: Written (check_bounding_collision) and read (func_8004A630 render_objects.c:1354,1356) "
      "solely inside the draw path OLakitu::Draw, to place a decal; the result depends only on object->pos, and "
      "the stale meshIndex fields are just a search hint. (ESCAPES-only; wave 2, E3)" },
    { "D_8018C830", "src/code_80057C60.c",
      "render-only: Same shape as D_8018C0B0: written and read only inside the draw path (OBoos::Draw -> "
      "func_800523B8 -> func_8004A630) to position a decal; depends only on object->pos. (ESCAPES-only; wave 2, "
      "E3)" },
    { "D_8018CF28", "src/code_80057C60.c",
      "dead (stored, never read): Stored at code_8006E9C0.c:460 and code_80057C60.c:2256 and read nowhere in "
      "the repo (decl + extern + 2 writes). Pointees are always in gPlayers[] (player-ptr) if a reader ever "
      "appears." },
    { "gLakituTextureBuffer", "src/code_80057C60.c",
      "asset pointer: holds texture-NAME pointers (asset strings of the sLakitu* tables, Lakitu.cpp:233-786), "
      "written through gLakituTexturePtr (update_objects.c:1211); its one simulation-visible effect is "
      "Object.activeTexture's null-ness (update_objects.c:1234,1251,1266), hashed through the Object "
      "(OracleStatePut.h ptrMask, 1.4b precedent); which asset is shown is render state. gLakituTexturePtr "
      "itself is hashed as an offset into this array. (ESCAPES-only; wave 2, E3)" },
    { "D_8018D240", "src/code_80057C60_var.c",
      "asset pointer: One writer stores the track's minimap texture reference (const char*); the only two "
      "readers (render_objects.c:2545,2551) pass it to display-list texture loads. No simulation reader." },
    { "D_8018D420", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69B03C:other_textures.h:724); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D424", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69B140:other_textures.h:726); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D428", "src/code_80057C60_var.c",
      "never written (every escape traced to a read): No assignment to the pointer exists anywhere (rg: decl, "
      "extern, one table entry), and the table holding its address is only dereferenced to LOAD entries "
      "[0]/[1], never this one (code_80057C60.c:4817-4846). It stays NULL for the whole run and is never read. "
      "(ESCAPES-only; wave 2, E3)" },
    { "D_8018D42C", "src/code_80057C60_var.c",
      "never written (every escape traced to a read): No assignment to the pointer exists anywhere (rg: decl, "
      "extern, one table entry), and the table holding its address is only dereferenced to LOAD entries "
      "[0]/[1], never this one (code_80057C60.c:4817-4846). It stays NULL for the whole run and is never read. "
      "(ESCAPES-only; wave 2, E3)" },
    { "D_8018D430", "src/code_80057C60_var.c",
      "never written (every escape traced to a read): No assignment to the pointer exists anywhere (rg: decl, "
      "extern, one table entry), and the table holding its address is only dereferenced to LOAD entries "
      "[0]/[1], never this one (code_80057C60.c:4817-4846). It stays NULL for the whole run and is never read. "
      "(ESCAPES-only; wave 2, E3)" },
    { "D_8018D434", "src/code_80057C60_var.c",
      "never written (every escape traced to a read): No assignment to the pointer exists anywhere (rg: decl, "
      "extern, one table entry), and the table holding its address is only dereferenced to LOAD entries "
      "[0]/[1], never this one (code_80057C60.c:4817-4846). It stays NULL for the whole run and is never read. "
      "(ESCAPES-only; wave 2, E3)" },
    { "D_8018D438", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69CB84:other_textures.h:752); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D43C", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69CCEC:other_textures.h:754); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D440", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69CEB8:other_textures.h:756); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D444", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69D148:other_textures.h:758); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D448", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69D4E0:other_textures.h:760); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D44C", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69D8FC:other_textures.h:762); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D450", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69DCB4:other_textures.h:764); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D454", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69DFA0:other_textures.h:766); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D458", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69E25C:other_textures.h:768); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D45C", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69E518:other_textures.h:770); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D460", "src/code_80057C60_var.c",
      "asset pointer: Only writer func_8005D290 stores the address of a constant OTR texture-name string "
      "(gTexture69E7A8:other_textures.h:772); the only readers dereference it via a pointer table into "
      "gDPLoadTextureBlock display-list calls. No sim use." },
    { "D_8018D464", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2504); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D468", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2505); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D46C", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2506); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D470", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2507); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D478", "src/code_80057C60_var.c",
      "dead (stored, never read): Stored once (code_80057C60.c:2484) and never read: its only occurrences are "
      "the assignment, the definition (_var.c:185) and the extern (.h:591). Also asset-only, so nothing to hash "
      "either way." },
    { "D_8018D480", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2485); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D484", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2486); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D48C", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2487); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D490", "src/code_80057C60_var.c",
      "asset pointer: Only assigned D_8018D48C (itself the constant address of an asset-name string, :2488); "
      "readers copy it into object texture fields or pass it to a texture loader. Never mutated through, no "
      "state of its own." },
    { "D_8018D49C", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2491); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D4A0", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2492); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "D_8018D4BC", "src/code_80057C60_var.c",
      "dead (stored, never read): Stored once (code_80057C60.c:2489) and never read: the only other occurrences "
      "are the definition (_var.c:193) and the externs (.h:598, render_objects.h:394). Asset-only value in any "
      "case." },
    { "D_8018D4C0", "src/code_80057C60_var.c",
      "dead (stored, never read): Stored once (code_80057C60.c:2490) and never read: the only other mentions "
      "are the definition (_var.c:194), the externs (.h:599, render_objects.h:395) and a comment (:5961). "
      "Asset-only value in any case." },
    { "gTextureLoadedLightningBolt0", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2508); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "gTextureLoadedLightningBolt1", "src/code_80057C60_var.c",
      "asset pointer: Assigned only the constant address of an asset-name string (code_80057C60.c:2509); read "
      "only as a texture argument in render code; never indexed or stored through, carries no state." },
    { "cloudvtx", "src/code_8006E9C0.c",
      "never written (every escape traced to a read): The pointer only goes into SkyActor::mVtx / Object.vertex "
      "and then into gSPVertex (SkyCloud.cpp:108), which reads through a const pointer (interpreter.cpp:1554). "
      "Nothing stores into the Vtx data. (ESCAPES-only; wave 2, E5)" },
    { "cloudvtx2", "src/code_8006E9C0.c",
      "never written (every escape traced to a read): The pointer only goes into SkyActor::mVtx / Object.vertex "
      "and then into gSPVertex (SkyCloud.cpp:108), which reads through a const pointer (interpreter.cpp:1554). "
      "Nothing stores into the Vtx data. (ESCAPES-only; wave 2, E5)" },
    { "D_800E8680", "src/code_800AF9B0.c",
      "never written (every escape traced to a read): Both uses (code_800AF9B0.c:181-182) are gSPLight "
      "display-list commands, which the interpreter only copies from (interpreter.cpp:2320-2326). No store "
      "through any pointer to D_800E8680. (ESCAPES-only; wave 2, E5)" },
    { "D_8018EDB8", "src/code_800AF9B0.c",
      "render-only: Fully regenerated each draw from already-hashed phase variables and consumed by gSPVertex "
      "in the same function; no simulation reader." },
    { "D_8018EDBC", "src/code_800AF9B0.c",
      "render-only: Same as D_8018EDB8: regenerated each frame from hashed D_8018EDB0/D_8018EDB4 and read only "
      "by the vertex fetch of the same display list." },
    { "D_800E2360", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2400 initialiser "
      "(kart_attributes.c:20); D_800E2400 is only indexed as an rvalue (src/spawn_players.c:95,103,110). Every "
      "dereference of the holder reads; none stores through it, so D_800E2360 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2380", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2400 initialiser "
      "(kart_attributes.c:20); D_800E2400 is only indexed as an rvalue (src/spawn_players.c:95,103,110). Every "
      "dereference of the holder reads; none stores through it, so D_800E2380 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E23A0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2400 initialiser "
      "(kart_attributes.c:20); D_800E2400 is only indexed as an rvalue (src/spawn_players.c:95,103,110). Every "
      "dereference of the holder reads; none stores through it, so D_800E23A0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E23C0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2400 initialiser "
      "(kart_attributes.c:20); D_800E2400 is only indexed as an rvalue (src/spawn_players.c:95,103,110). Every "
      "dereference of the holder reads; none stores through it, so D_800E23C0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E23E0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2400 initialiser "
      "(kart_attributes.c:20); D_800E2400 is only indexed as an rvalue (src/spawn_players.c:95,103,110). Every "
      "dereference of the holder reads; none stores through it, so D_800E23E0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2414", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E24B4 initialiser "
      "(kart_attributes.c:40); D_800E24B4 is only indexed as an rvalue (src/spawn_players.c:96,104,111). Every "
      "dereference of the holder reads; none stores through it, so D_800E2414 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2434", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E24B4 initialiser "
      "(kart_attributes.c:40); D_800E24B4 is only indexed as an rvalue (src/spawn_players.c:96,104,111). Every "
      "dereference of the holder reads; none stores through it, so D_800E2434 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2454", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E24B4 initialiser "
      "(kart_attributes.c:40); D_800E24B4 is only indexed as an rvalue (src/spawn_players.c:96,104,111). Every "
      "dereference of the holder reads; none stores through it, so D_800E2454 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2474", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E24B4 initialiser "
      "(kart_attributes.c:40); D_800E24B4 is only indexed as an rvalue (src/spawn_players.c:96,104,111). Every "
      "dereference of the holder reads; none stores through it, so D_800E2474 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2494", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E24B4 initialiser "
      "(kart_attributes.c:40); D_800E24B4 is only indexed as an rvalue (src/spawn_players.c:96,104,111). Every "
      "dereference of the holder reads; none stores through it, so D_800E2494 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E24C8", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2568 initialiser "
      "(kart_attributes.c:60); D_800E2568 is only indexed as an rvalue (src/spawn_players.c:97,105,112). Every "
      "dereference of the holder reads; none stores through it, so D_800E24C8 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E24E8", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2568 initialiser "
      "(kart_attributes.c:60); D_800E2568 is only indexed as an rvalue (src/spawn_players.c:97,105,112). Every "
      "dereference of the holder reads; none stores through it, so D_800E24E8 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2508", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2568 initialiser "
      "(kart_attributes.c:60); D_800E2568 is only indexed as an rvalue (src/spawn_players.c:97,105,112). Every "
      "dereference of the holder reads; none stores through it, so D_800E2508 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2528", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2568 initialiser "
      "(kart_attributes.c:60); D_800E2568 is only indexed as an rvalue (src/spawn_players.c:97,105,112). Every "
      "dereference of the holder reads; none stores through it, so D_800E2528 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2548", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2568 initialiser "
      "(kart_attributes.c:60); D_800E2568 is only indexed as an rvalue (src/spawn_players.c:97,105,112). Every "
      "dereference of the holder reads; none stores through it, so D_800E2548 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E26D0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E26D0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E270C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E270C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2748", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E2748 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2784", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E2784 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E27C0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E27C0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E27FC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E27FC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2838", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E2838 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2874", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2AB0 initialiser "
      "(kart_attributes.c:173); D_800E2AB0 is only indexed as an rvalue (src/player_controller.c:2615,2618). "
      "Every dereference of the holder reads; none stores through it, so D_800E2874 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E28B0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E28B0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E28EC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E28EC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2928", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E2928 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2964", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E2964 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E29A0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E29A0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E29DC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E29DC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2A18", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E2A18 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2A54", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2A90 initialiser "
      "(kart_attributes.c:170); D_800E2A90 is only indexed as an rvalue (src/player_controller.c:2609,2612). "
      "Every dereference of the holder reads; none stores through it, so D_800E2A54 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2AD0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2AD0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2B0C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2B0C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2B48", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2B48 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2B84", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2B84 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2BC0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2BC0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2BFC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2BFC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2C38", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2C38 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2C74", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2EB0 initialiser "
      "(kart_attributes.c:244); D_800E2EB0 is only indexed as an rvalue (src/player_controller.c:2713). Every "
      "dereference of the holder reads; none stores through it, so D_800E2C74 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2CB0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2CB0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2CEC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2CEC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2D28", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2D28 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2D64", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2D64 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2DA0", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2DA0 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2DDC", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2DDC is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2E18", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2E18 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E2E54", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E2E90 initialiser "
      "(kart_attributes.c:241); D_800E2E90 is only indexed as an rvalue (src/player_controller.c:2671,2676). "
      "Every dereference of the holder reads; none stores through it, so D_800E2E54 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3030", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E3030 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E306C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E306C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E30A8", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E30A8 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E30E4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E30E4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3120", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E3120 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E315C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E315C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3198", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E3198 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E31D4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3210 initialiser "
      "(kart_attributes.c:310); D_800E3210 is only indexed as an rvalue (src/player_controller.c:3302,3305). "
      "Every dereference of the holder reads; none stores through it, so D_800E31D4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3230", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E3230 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E326C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E326C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E32A8", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E32A8 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E32E4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E32E4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3320", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E3320 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E335C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E335C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3398", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E3398 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E33D4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3410 initialiser "
      "(kart_attributes.c:346); D_800E3410 is only indexed as an rvalue (src/player_controller.c:3828,3831). "
      "Every dereference of the holder reads; none stores through it, so D_800E33D4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3430", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E3430 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E346C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E346C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E34A8", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E34A8 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E34E4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E34E4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3520", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E3520 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E355C", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E355C is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E3598", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E3598 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E35D4", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the D_800E3610 initialiser "
      "(kart_attributes.c:382); D_800E3610 is only indexed as an rvalue (src/player_controller.c:3657,3660). "
      "Every dereference of the holder reads; none stores through it, so D_800E35D4 is never written during an "
      "iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationBowser", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:274); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationBowser is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationDK", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:274); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationDK is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationLuigi", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:273); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationLuigi is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationMario", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:273); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationMario is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationPeach", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:274); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationPeach is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationToad", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:273); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationToad is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationWario", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:274); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationWario is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gKartAccelerationYoshi", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gKartAccelerationTables initialiser "
      "(kart_attributes.c:273); gKartAccelerationTables is only indexed as an rvalue "
      "(src/player_controller.c:3011-3097). Every dereference of the holder reads; none stores through it, so "
      "gKartAccelerationYoshi is never written during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gTopSpeed100cc", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gTopSpeedTable initialiser "
      "(kart_attributes.c:85); gTopSpeedTable is only indexed as an rvalue (src/spawn_players.c:98,106,113). "
      "Every dereference of the holder reads; none stores through it, so gTopSpeed100cc is never written during "
      "an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gTopSpeed150cc", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gTopSpeedTable initialiser "
      "(kart_attributes.c:85); gTopSpeedTable is only indexed as an rvalue (src/spawn_players.c:98,106,113). "
      "Every dereference of the holder reads; none stores through it, so gTopSpeed150cc is never written during "
      "an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gTopSpeed50cc", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gTopSpeedTable initialiser "
      "(kart_attributes.c:85); gTopSpeedTable is only indexed as an rvalue (src/spawn_players.c:98,106,113). "
      "Every dereference of the holder reads; none stores through it, so gTopSpeed50cc is never written during "
      "an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gTopSpeedBattle", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gTopSpeedTable initialiser "
      "(kart_attributes.c:85); gTopSpeedTable is only indexed as an rvalue (src/spawn_players.c:98,106,113). "
      "Every dereference of the holder reads; none stores through it, so gTopSpeedBattle is never written "
      "during an iteration. (ESCAPES-only; wave 2, E1)" },
    { "gTopSpeedExtra", "src/data/kart_attributes.c",
      "never written (every escape traced to a read): Sole escape is the gTopSpeedTable initialiser "
      "(kart_attributes.c:85); gTopSpeedTable is only indexed as an rvalue (src/spawn_players.c:98,106,113). "
      "Every dereference of the holder reads; none stores through it, so gTopSpeedExtra is never written during "
      "an iteration. (ESCAPES-only; wave 2, E1)" },
    { "D_800E5740", "src/data/some_data.c",
      "never written (every escape traced to a read): Every escape ends in func_8007675C, which only reads the "
      "three s16 (update_objects.c:1843-1845), and the whole chain starts in func_80076884, which nothing "
      "calls. Nothing stores through the table. (ESCAPES-only; wave 2, E3)" },
    { "D_800E579C", "src/data/some_data.c",
      "never written (every escape traced to a read): Every escape ends in func_8007675C, which only reads the "
      "three s16 (update_objects.c:1843-1845), and the whole chain starts in func_80076884, which nothing "
      "calls. Nothing stores through the table. (ESCAPES-only; wave 2, E3)" },
    { "D_800E57F8", "src/data/some_data.c",
      "never written (every escape traced to a read): Every escape ends in func_8007675C, which only reads the "
      "three s16 (update_objects.c:1843-1845), and the whole chain starts in func_80076884, which nothing "
      "calls. Nothing stores through the table. (ESCAPES-only; wave 2, E3)" },
    { "D_800E5988", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E5A44", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E5B08", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E5BD4", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E5C90", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E6034", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E60F0", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E61B4", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E6280", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E641C", "src/data/some_data.c",
      "never written (every escape traced to a read): Its sole holder D_800E6724[] is never read or indexed "
      "anywhere, so the address is never dereferenced, let alone stored through. (ESCAPES-only; wave 2, E3)" },
    { "D_800E64D8", "src/data/some_data.c",
      "never written (every escape traced to a read): Its sole holder D_800E6724[] is never read or indexed "
      "anywhere, so the address is never dereferenced, let alone stored through. (ESCAPES-only; wave 2, E3)" },
    { "D_800E659C", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E6668", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E67B8", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E6834", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E694C", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E69B0", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "D_800E69F4", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "boo10", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "boo6", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "boo7", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "boo8", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "boo9", "src/data/some_data.c",
      "never written (every escape traced to a read): The address only reaches Object.spline (a pointer field, "
      "hashed as null-ness); every dereference of spline/controlPoints in src is a load or a pointer increment "
      "of the Object field (code_80086E70.c). Nothing stores into SplineData. (ESCAPES-only; wave 2, E3)" },
    { "gBalloonVertexPlane1", "src/data/some_data.c",
      "never written (every escape traced to a read): Only uses are the address operands of two gSPVertex calls "
      "in the balloon render functions; the Fast3D interpreter reads vertices through a const pointer, so "
      "nothing stores through the address. (ESCAPES-only; wave 2, E3)" },
    { "gBalloonVertexPlane2", "src/data/some_data.c",
      "never written (every escape traced to a read): Only uses are the address operands of two gSPVertex calls "
      "in the balloon render functions; the Fast3D interpreter reads vertices through a const pointer, so "
      "nothing stores through the address. (ESCAPES-only; wave 2, E3)" },
    { "gKalimariDesertClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gKoopaTroopaBeachClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gLuigiRacewayClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gPlayerEightVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerFiveVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerFourVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerOneVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerSevenVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerSixVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerThreeVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gPlayerTwoVtx", "src/data/some_data.c",
      "render-only: Stores via gPlayerVtx[playerId] (render_player.c:901-950) run only from render_player -> "
      "func_80023BF0, recompute the y of 6 vertices from player->unk_DA4 / unk_DB4.unk1E (hashed), and are read "
      "only by the 4 gSPVertex sites; pure function of hashed Player state, render-only. (ESCAPES-only; wave 2, "
      "E3)" },
    { "gRoyalRacewayClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gSherbetLandClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gToadsTurnpikeRainbowRoadStars", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gWarioStadiumStars", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "gYoshiValleyMooMooFarmClouds", "src/data/some_data.c",
      "never written (every escape traced to a read): Constructors only copy the address into Props pointer "
      "members; the single dereference (Sky::InitActors, Sky.cpp:289-325) reads rotY/posY/scalePercent/subType "
      "and advances a local pointer. No store through any alias exists. (ESCAPES-only; wave 2, E3)" },
    { "D_02004E80", "src/data/textures.c",
      "render-only: Static asset descriptor; the single runtime write (width, menu_items.c:6657) is an "
      "idempotent function of asset dimensions executed inside render_menus, and the readers are the menu "
      "texture draw path." },
    { "D_802856DC", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80283D2C:936 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802856DC through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285718", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80283D2C:937 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285718 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285754", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80283EA0:941 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285754 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285784", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80283EA0:942 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285784 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802857B4", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80283F6C:958 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802857B4 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802857CC", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284068:974 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802857CC through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802857F0", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802842D8:1025 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802857F0 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285850", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802842D8:1026 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285850 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802858B0", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802842A8:1020 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802858B0 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802858C8", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802842A8:1021 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802858C8 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802858E0", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284308:1037 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802858E0 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_802858F8", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284308:1038 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_802858F8 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285910", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284154:994 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285910 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285928", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284154:995 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285928 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285940", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802841E8:1008 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285940 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285A10", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284418:1173 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285A10 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285A4C", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284418:1174 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285A4C through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285A88", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284494:1180 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285A88 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285AB8", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_80284494:1181 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285AB8 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285AE8", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802844FC:1186 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285AE8 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285B00", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802844FC:1187 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285B00 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285B18", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_8028454C:1195 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285B18 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285B54", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_8028454C:1196 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285B54 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285B90", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802845EC:1200 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285B90 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285BA8", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802845EC:1201 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285BA8 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285C38", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802846B4:1219 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285C38 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285C74", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802846B4:1220 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285C74 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285CB0", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802846E4:1226 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285CB0 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285CE0", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Every escape is a read-only pass: func_802846E4:1227 -> "
      "func_80282C40 (ceremony_and_credits.c:359-376) only reads the records and copies them into "
      "D_80287818/D_80287998. No store to D_80285CE0 through any pointer; the tree holds only decl, extern, "
      "this use. (ESCAPES-only; wave 2, E4)" },
    { "D_80285D80", "src/ending/ceremony_and_credits.c",
      "never written (every escape traced to a read): Only holder is D_80286A04 (:1389-1438), itself never "
      "written; its unk4/unk8 pointers are read at :1471-1479 and passed to func_80282C40, which only reads the "
      "records (:359-376). No store to D_80285D80 through any pointer. (ESCAPES-only; wave 2, E4)" },
    { "D_80287750", "src/ending/ceremony_and_credits.c",
      "dead (stored, never read): Stored only by the init-time zero clear (ceremony_and_credits.c:562-563) and "
      "never read by any code in src/ or courses/; the all-zero value cannot affect the simulation." },
    { "D_80284EE0", "src/ending/dl_unk_80284EE0.c",
      "never written (every escape traced to a read): Only escape is the address emitted into the master "
      "display list by func_80281D00 (code_80281C40.c:95,99); the interpreter only reads it and the in-place DL "
      "patchers (interpreter.cpp:3622, Track.cpp:59-83, collision.c:2149) never receive this array. No store "
      "anywhere. (ESCAPES-only; wave 2, E4)" },
    { "D_80284F70", "src/ending/dl_unk_80284EE0.c",
      "never written (every escape traced to a read): Only escape is the address emitted into the master "
      "display list by func_80281D00 (code_80281C40.c:93); the interpreter handler for its OTR file-path "
      "commands only reads them (interpreter.cpp:3648-3669). No in-place DL patcher can receive this array. No "
      "store anywhere. (ESCAPES-only; wave 2, E4)" },
    { "D_802874D4", "src/ending/podium_ceremony_actors.c",
      "dead (stored, never read): Written once (podium_ceremony_actors.c:447) and never read anywhere: 3 "
      "occurrences in the whole tree, all decl/extern/store. Its value is a host display-list address." },
    { "D_80287500", "src/ending/podium_ceremony_actors.c",
      "render-only: Written through an escape: guLookAtF(D_80287500, cameras[0]...) at "
      "src/ending/podium_ceremony_actors.c:393 each pass, before any read. Only readers are "
      "func_80280A28:226-234 -> AddEffectMatrix (render matrix stack). A pure function of hashed camera state, "
      "read by no sim code. (ESCAPES-only; wave 2, E4)" },
    { "initBalloon", "src/ending/podium_ceremony_actors.c",
      "never written (every escape traced to a read): Only escape is new_actor(&initBalloon) at "
      "src/ending/podium_ceremony_actors.c:335, which stores the pointer in CeremonyActor.initParams (:131); "
      "the only dereference is set_initial_position :86-97, which reads. No write through initParams anywhere "
      "in src. (ESCAPES-only; wave 2, E4)" },
    { "initBurst", "src/ending/podium_ceremony_actors.c",
      "never written (every escape traced to a read): Only escape is new_actor(&initBurst) at "
      "src/ending/podium_ceremony_actors.c:364, which stores the pointer in CeremonyActor.initParams (:131); "
      "the only dereference is set_initial_position :86-97, which reads. No write through initParams anywhere "
      "in src. (ESCAPES-only; wave 2, E4)" },
    { "initCone", "src/ending/podium_ceremony_actors.c",
      "never written (every escape traced to a read): Only escape is new_actor(&initCone) at "
      "src/ending/podium_ceremony_actors.c:207, which stores the pointer in CeremonyActor.initParams (:131); "
      "the only dereference is set_initial_position :86-97, which reads. No write through initParams anywhere "
      "in src. (ESCAPES-only; wave 2, E4)" },
    { "initDummy", "src/ending/podium_ceremony_actors.c",
      "never written (every escape traced to a read): Only escape is new_actor(&initDummy) at "
      "src/ending/podium_ceremony_actors.c:316, which stores the pointer in CeremonyActor.initParams (:131); "
      "the only dereference is set_initial_position :86-97, which reads. No write through initParams anywhere "
      "in src. (ESCAPES-only; wave 2, E4)" },
    { "freeCam", "src/enhancements/freecam/freecam_engine.c",
      "UI / freecam: Freecam-only state, unreachable in oracle sessions (guard OracleGuards.cpp:16-18, "
      "FreeCamera.cpp:41-44, main.c:358) and read by nothing but the freecam camera itself, which integrates it "
      "with the host frame delta gDeltaTime." },
    { "gDampValue", "src/enhancements/freecam/freecam_engine.c",
      "UI / freecam: A UI tunable: only the Freecam slider writes it (FreecamWindow.cpp:79); only freecam code "
      "reads it, reachable solely while the user-enabled FreeCamera is active (default off). Camera effects "
      "would appear in cameras[] anyway. (ESCAPES-only; wave 2, E5)" },
    { "gFreecamFollowFactor", "src/enhancements/freecam/freecam_engine.c",
      "UI / freecam: A UI tunable: only the Freecam slider writes it (FreecamWindow.cpp:91); only freecam code "
      "reads it, reachable solely while the user-enabled FreeCamera is active (default off). Camera effects "
      "would appear in cameras[] anyway. (ESCAPES-only; wave 2, E5)" },
    { "gRotDampValue", "src/enhancements/freecam/freecam_engine.c",
      "UI / freecam: A UI tunable: only the Freecam slider writes it (FreecamWindow.cpp:88); only freecam code "
      "reads it, reachable solely while the user-enabled FreeCamera is active (default off). Camera effects "
      "would appear in cameras[] anyway. (ESCAPES-only; wave 2, E5)" },
    { "mat_ok64_black", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_ok64_black is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_ok64_f3dlite_material_001", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_ok64_f3dlite_material_001 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_ok64_f3dlite_material_002", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_ok64_f3dlite_material_002 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_ok64_f3dlite_material_003", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_ok64_f3dlite_material_003 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_ok64_white", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_ok64_white is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_revert_ok64_black", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_revert_ok64_black is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_revert_ok64_f3dlite_material_001", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_revert_ok64_f3dlite_material_001 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_revert_ok64_f3dlite_material_002", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_revert_ok64_f3dlite_material_002 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_revert_ok64_f3dlite_material_003", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_revert_ok64_f3dlite_material_003 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "mat_revert_ok64_white", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to mat_revert_ok64_white is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_tri_0", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_tri_0 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_tri_1", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_tri_1 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_tri_2", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_tri_2 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_tri_3", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_tri_3 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_tri_4", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_tri_4 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_0", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_0 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_1", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_1 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_2", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_2 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_3", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_3 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_4", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_4 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_Plane_001_mesh_vtx_cull", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_Plane_001_mesh_vtx_cull is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "ok64_ok64_rgba16", "src/harbour_masters.c",
      "never written (every escape traced to a read): Chain ends at ok64_Plane_001_mesh "
      "(src/harbour_masters.c:2774), which nothing references: rg 'ok64' over src/courses/include finds only "
      "harbour_masters.c/.h and the oracle header; the header is included only by harbour_masters.c:3. No "
      "pointer to ok64_ok64_rgba16 is dereferenced. (ESCAPES-only; wave 2, E4)" },
    { "calculate_delta_time.prevtime", "src/main.c",
      "host-dependent: Holds the previous host-clock reading (osGetCount = std::chrono::steady_clock, "
      "os.cpp:49). Output gDeltaTime is read only by freecam, which the oracle guards refuse "
      "(OracleGuards.cpp:16-19)." },
    { "gActiveSPTask", "src/main.c",
      "libultra OS bookkeeping: N64 SP-task bookkeeping. Every writer/reader sits in the thread3_video chain, "
      "which the port never starts (main_func has no caller), so it stays NULL forever. libultra OS state, not "
      "simulation." },
    { "gAudioThread", "src/main.c",
      "libultra OS bookkeeping: libultra thread bookkeeping: the game's only stores are two NULLs in "
      "create_thread (main.c:201-202) and the stubbed osCreateThread/osStartThread never touch it; nothing "
      "reads it." },
    { "gAudioThreadStack", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: the only escape is the stack pointer given to create_thread -> "
      "osCreateThread (main.c:1039,203), an empty stub (stubs.c:52), inside thread3_video, which the port never "
      "starts (main_func has no caller, main.c:206). No store, no reader. (ESCAPES-only; wave 2, E4)" },
    { "gDisplayListHead", "src/main.c",
      "render-only: Pure render cursor: all 2300+ sites emit display-list words; no sim/menu logic reads it. "
      "Its value depends on host state (cvars, aspect ratio command counts), so hashing it would import host "
      "dependence." },
    { "gDmaMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot buffer of gDmaMesgQueue. Only escape is "
      "osCreateMesgQueue(&gDmaMesgQueue, gDmaMesgBuf, 1) main.c:249 in setup_mesg_queues (thread3_video chain, "
      "never runs); no osSendMesg/osJamMesg anywhere targets gDmaMesgQueue, so no slot is ever stored. "
      "(ESCAPES-only; wave 2, E4)" },
    { "gDmaMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping, never written: each escape is osPiStartDma(..,&q) (port impl "
      "os_pi.cpp:17-28 ignores mq) or osRecvMesg(&q,..), which returns -1 at os_mesg.cpp:40-42 because "
      "validCount stays 0 (only osCreateMesgQueue(&q) is main.c:249, dead chain; no osSendMesg/osJamMesg "
      "targets it). (ESCAPES-only; wave 2, E4)" },
    { "gGameLoopThread", "src/main.c",
      "libultra OS bookkeeping: Same proof as gAudioThread: two NULL stores in create_thread (main.c:201-202), "
      "stubbed OS calls, no reader." },
    { "gGameLoopThreadStack", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: the only escape is the stack pointer given to create_thread -> "
      "osCreateThread (main.c:1042,203), an empty stub (stubs.c:52), inside thread3_video, which the port never "
      "starts (main_func has no caller, main.c:206). No store, no reader. (ESCAPES-only; wave 2, E4)" },
    { "gGameMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot of gGameVblankQueue. Only escape is the pointer store in "
      "osCreateMesgQueue (main.c:1138). No osSendMesg targets gGameVblankQueue (set_vblank_handler(2,..) is "
      "commented out, main.c:1144) and its osRecvMesg (:509) is #ifdef TARGET_N64, never defined. "
      "(ESCAPES-only; wave 2, E4)" },
    { "gGameVblankQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: single store is osCreateMesgQueue at startup (main.c:1138 -> "
      "os_mesg.cpp:8-11; msg field = host address). Never read: its only osRecvMesg (main.c:509) is #ifdef "
      "TARGET_N64, never defined; set_vblank_handler(2,..) is commented out (:1144). (ESCAPES-only; wave 2, E4)" },
    { "gGfxMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot of gGfxVblankQueue; only escape is the pointer store in "
      "osCreateMesgQueue (main.c:1137). Slots would be written by osSendMesg(spTask->msgqueue) in "
      "handle_dp_complete:970 / handle_sp_complete:1010, which belong to the thread3_video chain the port never "
      "starts. (ESCAPES-only; wave 2, E4)" },
    { "gGfxPool", "src/main.c",
      "render-only: Selects which of the two display-list/matrix pools renders this frame (main.c:480,492); all "
      "readers build or submit Gfx/Mtx. No logic branches on it. Value is a pure function of gGlobalTimer&1, so "
      "it adds no information." },
    { "gGfxPools", "src/main.c",
      "render-only: Pool words are written only by display-list/matrix emitters (via gDisplayListHead = "
      "gGfxPool->gfxPool, main.c:482,493; mtxHud/mtxScreen/mtxObject writers listed) and read only by the "
      "Fast3D interpreter (main.c:309/1170 -> Game.cpp:58). spTask fields are read only in the dead "
      "thread3_video chain. (ESCAPES-only; wave 2, E4)" },
    { "gGfxSPTask", "src/main.c",
      "libultra OS bookkeeping: libultra OSTask descriptor for the display-list job; only "
      "create_gfx_task_structure touches it and only to fill RSP-task fields (mostly #ifdef TARGET_N64). Value "
      "is &gGfxPools[gGlobalTimer&1].spTask; no logic reads it." },
    { "gGfxVblankQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: single store is osCreateMesgQueue at startup (main.c:1137; msg "
      "field = host address). Its address is also kept in spTask->msgqueue (main.c:273) but dereferenced only "
      "in handle_dp_complete:970 / handle_sp_complete:1010 (dead thread3_video chain). (ESCAPES-only; wave 2, "
      "E4)" },
    { "gIntrMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot buffer of gIntrMesgQueue; escapes are osCreateMesgQueue at "
      "main.c:251 (setup_mesg_queues, dead chain). The one live osSendMesg to the queue (exec_display_list:438) "
      "returns -1 at os_mesg.cpp:17-19 (msgCount 0) before any slot store. (ESCAPES-only; wave 2, E4)" },
    { "gIntrMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping, never written: stays zero. "
      "osCreateMesgQueue/osViSetEvent/osSetEventMesg on it are only in setup_mesg_queues (main.c:251-254, dead "
      "chain); the lone live access, osSendMesg in exec_display_list:438, hits the validCount>=msgCount (0>=0) "
      "guard at os_mesg.cpp:17-19 and returns -1. (ESCAPES-only; wave 2, E4)" },
    { "gMainReceivedMesg", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping, never written: its only possible store is osRecvMesg's *msg at "
      "os_mesg.cpp:44, behind the validCount==0 early return (:40-42) for gDmaMesgQueue (never filled); "
      "display_and_vsync:509 is #ifdef TARGET_N64, never defined. (ESCAPES-only; wave 2, E4)" },
    { "gPIMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: only escape is osCreatePiManager(.., &gPIMesgQueue, gPIMesgBuf, "
      "32) at main.c:234 in thread1_idle (never runs), and the port's osCreatePiManager has an empty body "
      "(os_pi.cpp:6-7). Never stored, never read. (ESCAPES-only; wave 2, E4)" },
    { "gPIMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: only escape is osCreatePiManager(.., &gPIMesgQueue, ..) at "
      "main.c:234 in thread1_idle (never runs); the port's osCreatePiManager has an empty body (os_pi.cpp:6-7). "
      "Never stored, never read. (ESCAPES-only; wave 2, E4)" },
    { "gPhysicalFramebuffers", "src/main.c",
      "libultra OS bookkeeping: Only written in thread3_video (main.c:1027-1029), which never runs in the port, "
      "so all 3 entries stay NULL and the jumbotron copy functions early-return. VI framebuffer bookkeeping, no "
      "sim effect." },
    { "gPhysicalZBuffer", "src/main.c",
      "render-only: Written once at startup with the Z buffer's address; the only readers "
      "(skybox_and_splitscreen.c:186-187,207-208) put it into display-list depth/colour image commands. No "
      "simulation reader." },
    { "gPortFramebuffers", "src/main.c",
      "render-only: Written only by the renderer (G_READFB -> interpreter.cpp:4149, queued at main.c:809) and "
      "read only by the jumbotron texture refresh in the track Draw functions (WarioStadium.cpp:284, "
      "LuigiRaceway.cpp:307 -> copy_framebuffer). GPU pixels are host-dependent; no sim reader. (ESCAPES-only; "
      "wave 2, E4)" },
    { "gSIEventMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot buffer for gSIEventMesgQueue, pointer stored once by "
      "osCreateMesgQueue main.c:337. Nothing sends to the SI queue (osContStartReadData os.cpp:25-27 returns 0, "
      "osEeprom*/osPfs* ignore the queue, osRecvMesg commented main.c:397), so the slots are never stored. "
      "(ESCAPES-only; wave 2, E4)" },
    { "gSIEventMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: single store is osCreateMesgQueue at startup (main.c:337: "
      "validCount 0, first 0, msgCount 3, msg = host address). Every other escape "
      "(osContInit/osContStartReadData/osPfs*/osEeprom*) ignores the queue parameter in the port; no field is "
      "read by anyone. (ESCAPES-only; wave 2, E4)" },
    { "gSPTaskMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot buffer of gSPTaskMesgQueue; only escape is "
      "osCreateMesgQueue at main.c:250 (setup_mesg_queues). The only osSendMesg that fills it, "
      "dispatch_audio_sptask:429, is called only from thread4_audio:1212 - both in the thread chain the port "
      "never starts. (ESCAPES-only; wave 2, E4)" },
    { "gSPTaskMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: every escape (osCreateMesgQueue main.c:250, osSendMesg :429 in "
      "dispatch_audio_sptask, osRecvMesg :883 in receive_new_tasks) sits in thread4_audio / handle_vblank / "
      "setup_mesg_queues, reachable only from thread3_video, which the port never starts (main_func has no "
      "caller). (ESCAPES-only; wave 2, E4)" },
    { "gVblankHandler1", "src/main.c",
      "libultra OS bookkeeping: Set only by set_vblank_handler from thread4_audio and read only by "
      "handle_vblank; both belong to the N64 thread chain the port never starts (main_func has no caller), so "
      "it stays NULL. libultra message plumbing." },
    { "gVideoThread", "src/main.c",
      "libultra OS bookkeeping: Same proof as gAudioThread: only create_thread NULL stores (main.c:201-202) and "
      "stubbed osCreateThread/osStartThread; no reader." },
    { "gVideoThreadStack", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: the only escape is the stack pointer given to create_thread -> "
      "osCreateThread (main.c:238,203), an empty stub (stubs.c:52), inside thread1_idle, which the port never "
      "starts (main_func has no caller, main.c:206). No store, no reader. (ESCAPES-only; wave 2, E4)" },
    { "sCurrentAudioSPTask", "src/main.c",
      "libultra OS bookkeeping: Both writers (receive_new_tasks via handle_vblank, handle_sp_complete) are in "
      "the thread3_video chain that the port never starts; it stays NULL. N64 SP-task queue bookkeeping, not "
      "simulation." },
    { "sCurrentDisplaySPTask", "src/main.c",
      "libultra OS bookkeeping: Runtime value is fixed after startup: exec_display_list (main.c:435-436) sets "
      "it once; the only clear (:974) is in the unreachable handle_dp_complete. N64 SP-task bookkeeping, no "
      "simulation reader." },
    { "sNextAudioSPTask", "src/main.c",
      "libultra OS bookkeeping: Written and read only inside receive_new_tasks, reached only from handle_vblank "
      "in the thread3_video chain that the port never starts; stays NULL. libultra task queue, not simulation." },
    { "sNextDisplaySPTask", "src/main.c",
      "libultra OS bookkeeping: Stored every iteration (main.c:440) but its only reader (:899-901) is in "
      "receive_new_tasks, unreachable in the port. Value is &gGfxPools[gGlobalTimer&1].spTask anyway. SP-task "
      "bookkeeping, no simulation reader." },
    { "sSoundMesgBuf", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: slot of sSoundMesgQueue; only escape is osCreateMesgQueue at "
      "main.c:1199 in thread4_audio, created only by thread3_video:1039 (never runs). The only send into it, "
      "handle_vblank:960, is in the same dead chain. (ESCAPES-only; wave 2, E4)" },
    { "sSoundMesgQueue", "src/main.c",
      "libultra OS bookkeeping: OS bookkeeping: every touch (osCreateMesgQueue :1199, set_vblank_handler :1200, "
      "osRecvMesg :1206) is inside thread4_audio, created only by thread3_video:1039, which the port never "
      "starts; the address held by sSoundVblankHandler is dereferenced only in handle_vblank:960 (same dead "
      "chain). (ESCAPES-only; wave 2, E4)" },
    { "sSoundVblankHandler", "src/main.c",
      "libultra OS bookkeeping: Audio-thread vblank bookkeeping: written once at audio init with a queue "
      "pointer and the constant 512, read only by handle_vblank (OS message passing). No simulation reader." },
    { "gInterpolationMatrix", "src/math_util_2.c",
      "render-only: Render-only: written only inside the frame-interpolation replay "
      "(FrameInterpolation.cpp:411,415,426-589), run from ProcessGfxCommands via main.c:1189 after the "
      "iteration's hash point (main.c:1185). Its only outside reader, Matrix_GetCurrent :272/:830, has no "
      "caller. No simulation code touches it. (ESCAPES-only; wave 2, E2)" },
    { "prevObject2", "src/math_util_2.c",
      "render-only: Port-added frame-interpolation helper: written and read only inside "
      "mtxf_set_matrix_gObjectList, whose single caller rsp_set_matrix_gObjectList only chooses whether to emit "
      "a draw matrix. The source of the values (object->pos) is already hashed under group:objects." },
    { "sInterpolationMatrixStack", "src/math_util_2.c",
      "render-only: Render-only scratch: its only address holder is gInterpolationMatrix, and every store "
      "through it is in interpolate_branch / Matrix_Push (FrameInterpolation.cpp:411-589, matrix.c:63), run "
      "from ProcessGfxCommands after the hash point. No simulation reader exists. (ESCAPES-only; wave 2, E2)" },
    { "D_800E7360", "src/menu_items.c",
      "never written (every escape traced to a read): The one escape (menu_items.c:8725) passes the entry to "
      "pause_menu_item_box_cursor, which only reads arg1->column/row (:8760). No other reference; the table is "
      "never stored to. (ESCAPES-only; wave 2, E5)" },
    { "D_8018E060", "src/menu_items.c",
      "render-only: A per-frame queue: reset by func_80099A70 and drained by func_80099AEC within the same menu "
      "render function (menu_items.c:2461-2464, 2581-2600), then consumed only by replace_texture (texture-name "
      "bookkeeping). Its entries are not read by menu or race logic." },
    { "D_8018E0E8", "src/menu_items.c",
      "render-only: Texture-upload queue like D_8018E060: consumed only by replace_texture; no menu or race "
      "logic reads the entries. Left medium because the main-menu path (func_80094A64) never resets or drains "
      "it." },
    { "D_8018E768", "src/menu_items.c",
      "render-only: Display-list cache: both fields are pointers into static texture tables / a DL scratch "
      "buffer and the only reader emits gSPDisplayList. The count that indexes it (gNumD_8018E768Entries) is "
      "already hashed as scalar." },
    { "D_8018E7E8", "src/menu_items.c",
      "render-only: Snapshot of gScreenContexts rectangles (menu_items.c:4922-4923) read only by the menu "
      "fade/transition drawers that emit gDPFillRectangle; no logic reads it." },
    { "D_8018E810", "src/menu_items.c",
      "render-only: Same as D_8018E7E8: snapshot of screen sizes (menu_items.c:4924-4925) read only by the "
      "fade-rectangle drawers." },
    { "gControllerPak1FileHandle", "src/menu_items.c",
      "dead (stored, never read): Stored by osPfsInit (pak.cpp:106-108: queue address, channel, status) and "
      "never read: every other use only passes &handle to stub osPfs* functions that ignore it. Stored, never "
      "read. (ESCAPES-only; wave 2, E5)" },
    { "gControllerPak2FileHandle", "src/menu_items.c",
      "dead (stored, never read): Stored by osPfsInit (pak.cpp:106-108: queue address, channel, status) and "
      "never read: every other use only passes &handle to stub osPfs* functions that ignore it. Stored, never "
      "read. (ESCAPES-only; wave 2, E5)" },
    { "gTextDistance", "src/menu_items.c",
      "never written (every escape traced to a read): Every use passes the string to get_string_width / "
      "print_text*, which only read `*text` (menu_items.c:2015-2034, 2047-2118, 2137-2207; char_to_glyph_index "
      ":1804 and func_80092DF8/E1C/EE4 read only). No store, so the array stays at its initializer. "
      "(ESCAPES-only; wave 2, E5)" },
    { "gTextMenuAnnounceGhost", "src/menu_items.c",
      "never written (every escape traced to a read): Every use passes the string to get_string_width / "
      "print_text*, which only read `*text` (menu_items.c:2015-2034, 2047-2118, 2137-2207; char_to_glyph_index "
      ":1804 and func_80092DF8/E1C/EE4 read only). No store, so the array stays at its initializer. "
      "(ESCAPES-only; wave 2, E5)" },
    { "gTextMenuData", "src/menu_items.c",
      "never written (every escape traced to a read): Every use passes the string to get_string_width / "
      "print_text*, which only read `*text` (menu_items.c:2015-2034, 2047-2118, 2137-2207; char_to_glyph_index "
      ":1804 and func_80092DF8/E1C/EE4 read only). No store, so the array stays at its initializer. "
      "(ESCAPES-only; wave 2, E5)" },
    { "pause_menu_item_box_cursor.x2", "src/menu_items.c",
      "render-only: Written and read only inside the drawing function pause_menu_item_box_cursor: accumulates a "
      "rotation angle that goes into guRotate for a gSPMatrix (menu_items.c:8749-8762). No sim reader." },
    { "pause_menu_item_box_cursor.y2", "src/menu_items.c",
      "render-only: Written and read only inside the drawing function pause_menu_item_box_cursor: accumulates a "
      "rotation angle that goes into guRotate for a gSPMatrix (menu_items.c:8749-8762). No sim reader." },
    { "pause_menu_item_box_cursor.z2", "src/menu_items.c",
      "render-only: Written and read only inside the drawing function pause_menu_item_box_cursor: accumulates a "
      "rotation angle that goes into guRotate for a gSPMatrix (menu_items.c:8749-8762). No sim reader." },
    { "sGfxPtr", "src/menu_items.c",
      "render-only: DL write cursor in the gSomeDLBuffer pool block, render readers only; consistent with "
      "gDisplayListHead (host/CVar-dependent command counts)" },
    { "sMenuTextureList", "src/menu_items.c",
      "asset pointer: Slots only ever receive asset-name strings from MenuTexture.textureData (or NULL at "
      "clear); readers just load/measure the texture. Slot occupancy is menu-flow state already counted by "
      "sMenuTextureListIndex/Entries. Pixel edits are render-only." },
    { "cpu_forBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forBowserAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforBowser "
      "(player_controller.c:181) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDk", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forDkAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforDk "
      "(player_controller.c:172) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forLuigiAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforLuigi "
      "(player_controller.c:163) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMario", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forMarioAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforMario "
      "(player_controller.c:160) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forPeachAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforPeach "
      "(player_controller.c:178) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToad", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forToadAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforToad "
      "(player_controller.c:169) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWario", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forWarioAndYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforWario "
      "(player_controller.c:175) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Only escape is the static initializer of cpu_forPlayer "
      "(player_controller.c:45). Its sole dereference, spawn_players.c:535, copies the pointer into a local "
      "u16* arr; :536-539 only read arr[i]/arr[0]. No store, no memcpy, pointer kept nowhere. Never written. "
      "(ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndDk", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndMario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndToad", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "cpu_forYoshiAndWario", "src/player_controller.c",
      "never written (every escape traced to a read): Chain: array -> initializer of gListCPUforYoshi "
      "(player_controller.c:166) -> initializer of cpu_forTwoPlayer (:186). Sole dereference "
      "spawn_players.c:661 into a local u16* arr; :662-665 only read arr[i]/arr[0]. No store through it "
      "anywhere. Never written. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforBowser", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforDk", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforLuigi", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforMario", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforPeach", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforToad", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforWario", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gListCPUforYoshi", "src/player_controller.c",
      "never written (every escape traced to a read): Pointer table of constant addresses; sole escape is the "
      "initializer of cpu_forTwoPlayer (player_controller.c:186). Sole dereference spawn_players.c:661 loads "
      "one element and reads through it (:662-665). No store to the table. Never written; addresses are not "
      "hashable anyway. (ESCAPES-only; wave 2, E2)" },
    { "gIdentityMatrix", "src/port/interpolation/matrix.c",
      "never written (every escape traced to a read): Every use is a read: Matrix_Copy's source "
      "(matrix.c:34,44) or a gSPMatrix operand (framebuffer_effects.c:41,114; gbiMacro.c:22), which the "
      "interpreter copies out of (interpreter.cpp:1459-1485). Interpolation writes to its own map. "
      "(ESCAPES-only; wave 2, E5)" },
    { "gProfilerFrameData", "src/profiler.c",
      "host-dependent: Every store is a wall-clock timestamp from osGetTime() (std::chrono::steady_clock) and "
      "the only reader is the debug profiler overlay; nothing in the simulation reads it." },
    { "gActorHotAirBalloonItemBox", "src/racing/actors.c",
      "FINDING: dangling C view of a heap AActor (never nulled when freed, World.cpp:290,300); fits no rule; "
      "fix = null-on-clean (game change, out of 1.4c scope)" },
    { "D_802B91C8", "src/racing/math_util.c",
      "never written (every escape traced to a read): All 10 escapes pass it as the velocity argument of "
      "func_800C98B8/func_800C9D80, which only keep &velocity[i] in D_8018EFD8[].velX/Y/Z (external.c:412-414); "
      "the sole uses of those pointers are loads (external.c:449,450) and a compare (:505). No store through "
      "it. Never written. (ESCAPES-only; wave 2, E2)" },
    { "sMemoryPool", "src/racing/memory.c",
      "decision 3: its live allocations are hashed field-wise through their owners (the pool-ptr rows), "
      "pointers as pool offsets" },
    { "sPoolEnd", "src/racing/memory.c",
      "encoding anchor: the pool-ptr encoding anchor: its own offset is 0 by definition (memory.c:29, :79)" },
    { "D_802B8880", "src/racing/skybox_and_splitscreen.c",
      "never written (every escape traced to a read): Every escape is a gSPViewport display-list command (8 "
      "sites, 3 callers inline them). Its only consumer, Fast3D CalcAndSetViewport(const F3DVp_t*), reads "
      "vscale/vtrans into its own mRdp->viewport. No store through the pointer; skybox_and_splitscreen.c:88-96 "
      "write ScreenContext. (ESCAPES-only; wave 2, E2)" },
    { "D_800E45C0", "src/render_objects.c",
      "render-only: Written inside OCheepCheep::Draw's helper (func_800520C0) from an object flag and consumed "
      "by the gSPLight commands of the same draw call; no simulation read." },
    { "D_800E4620", "src/render_objects.c",
      "render-only: Direction is copied from D_80165840 right before each gSPLight in the ice-block draw "
      "functions; the only readers are those display-list commands." },
    { "common_vtx_neon", "src/render_objects.c",
      "never written (every escape traced to a read): Its only use is a gSPVertex operand "
      "(render_objects.c:3759); Fast3D reads vertices through a const pointer (interpreter.cpp:1554). No store "
      "into the Vtx data anywhere. (ESCAPES-only; wave 2, E5)" },
    { "speedometer_vtx", "src/render_objects.c",
      "never written (every escape traced to a read): Its only use is passed to func_8004A2F4, whose `vtx` "
      "parameter is used once, as a gSPVertex operand (render_objects.c:1307). Fast3D reads it via a const "
      "pointer (interpreter.cpp:1554). No store into the Vtx data. (ESCAPES-only; wave 2, E5)" },
    { "gKartBowserWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartBowserWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartDKWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartDKWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartLuigiWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartLuigiWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartMarioWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartMarioWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartPeachWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartPeachWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartShadowVertices", "src/render_player.c",
      "never written (every escape traced to a read): Only uses are the address operands of two gSPVertex calls "
      "in the shadow render functions; the interpreter reads vertices through a const pointer. (ESCAPES-only; "
      "wave 2, E3)" },
    { "gKartToadWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartToadWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartWarioWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartWarioWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartYoshiWheels0", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel0Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gKartYoshiWheels1", "src/render_player.c",
      "never written (every escape traced to a read): The array's address is stored in a table "
      "(gPaletteWheel1Offset) that nothing reads, so it is never dereferenced, let alone stored through; the "
      "values are never consumed. (ESCAPES-only; wave 2, E3)" },
    { "gPlayerPalette", "src/render_player.c",
      "render-only: Set and read only inside render_kart/render_ghost to feed gDPLoadTLUT_pal256 "
      "(render_player.c:1354,1375-1416,1469,1488). Points into gPlayerPalettesList; no logic reads it." },
    { "sKartTexture", "src/render_player.c",
      "render-only: Written and read only in render_kart/render_ghost to feed gDPLoadTextureBlock "
      "(render_player.c:1362-1364,1425,1475-1477,1497); values are asset-name strings picked from kart_dma.c "
      "tables. No sim reader; the inputs are already hashed player state." },
    { "D_80162DC4", "src/replays.c",
      "asset pointer: Only ever assigned one of three static staff-ghost arrays (never written through; the "
      "oracle table lists them ESCAPES-only) and read only to copy them. The effective state is the copy in "
      "D_802BFB80 (sCourseGhostReplay). Medium: it is a selector, so an enum hash is the safe alternative." },
    { "D_800E4470", "src/update_objects.c",
      "never written (every escape traced to a read): The address is only stored into Object.vertex "
      "(update_objects.c, Boos.cpp) and later used as a gSPVertex operand (render_objects.c:945); Fast3D reads "
      "vertices through a const pointer (interpreter.cpp:1554). No store into the Vtx data. (ESCAPES-only; wave "
      "2, E5)" },
    { "D_800E44B0", "src/update_objects.c",
      "never written (every escape traced to a read): The address is only stored into Object.vertex "
      "(update_objects.c, Boos.cpp) and later used as a gSPVertex operand (render_objects.c:945); Fast3D reads "
      "vertices through a const pointer (interpreter.cpp:1554). No store into the Vtx data. (ESCAPES-only; wave "
      "2, E5)" },
    { "sNeonBooList", "src/update_objects.c",
      "never written (every escape traced to a read): The table is only handed to init_texture_object, which "
      "stores its address in Object.textureList; every use of that field loads textureList[idx] "
      "(update_objects.c:766,772,1286). No `textureList[...] =` store exists, so the pointer table keeps its "
      "initializer. (ESCAPES-only; wave 2, E5)" },
    { "sNeonMarioList", "src/update_objects.c",
      "never written (every escape traced to a read): The table is only handed to init_texture_object, which "
      "stores its address in Object.textureList; every use of that field loads textureList[idx] "
      "(update_objects.c:766,772,1286). No `textureList[...] =` store exists, so the pointer table keeps its "
      "initializer. (ESCAPES-only; wave 2, E5)" },
    { "sNeonMushroomList", "src/update_objects.c",
      "never written (every escape traced to a read): The table is only handed to init_texture_object, which "
      "stores its address in Object.textureList; every use of that field loads textureList[idx] "
      "(update_objects.c:766,772,1286). No `textureList[...] =` store exists, so the pointer table keeps its "
      "initializer. (ESCAPES-only; wave 2, E5)" },
    // END GENERATED judged exclusions
};
#undef ORACLE_AUDIO_REASON

// `symbols` is a space-separated list; the IR class is the sweep's (ESCAPES-only, NEVER-WRITTEN, WRITTEN-init-only), or
// for a definition in the source text that the sweep does not list: IR-CONSTANT when the IR holds it as `internal
// constant` (the optimizer proved it is never written), ABSENT when the IR does not define it at all (dropped).
struct CGlobalSnapshot {
    const char* file;
    const char* irClass;
    const char* symbols;
};

inline constexpr CGlobalSnapshot kCGlobalsNotStored[] = {
    { "courses/banshee_boardwalk/course_offsets.c", "ESCAPES-only", "banshee_boardwalk_dls" },
    { "courses/bowsers_castle/course_offsets.c", "ESCAPES-only", "bowsers_castle_dls" },
    { "courses/choco_mountain/course_offsets.c", "ESCAPES-only", "choco_mountain_dls" },
    { "courses/dks_jungle_parkway/course_offsets.c", "ESCAPES-only", "d_course_dks_jungle_parkway_unknown_dl_list" },
    { "courses/frappe_snowland/course_offsets.c", "ESCAPES-only", "d_course_frappe_snowland_dl_list" },
    { "courses/harbour/ghostship_model.c", "ESCAPES-only",
      "ghostship_Plane_mesh ghostship_Plane_mesh_tri_0 ghostship_Plane_mesh_tri_1 ghostship_Plane_mesh_tri_2 "
      "ghostship_Plane_mesh_tri_3 ghostship_Plane_mesh_tri_4 ghostship_Plane_mesh_tri_5 "
      "ghostship_Plane_mesh_tri_6 ghostship_Plane_mesh_tri_7 ghostship_Plane_mesh_vtx_0 "
      "ghostship_Plane_mesh_vtx_1 ghostship_Plane_mesh_vtx_2 ghostship_Plane_mesh_vtx_3 "
      "ghostship_Plane_mesh_vtx_4 ghostship_Plane_mesh_vtx_5 ghostship_Plane_mesh_vtx_6 "
      "ghostship_Plane_mesh_vtx_7 ghostship_Plane_mesh_vtx_cull ghostship_burntfabricHoles_rgba16 "
      "ghostship_flag1_ci4 ghostship_flag1_pal_rgba16 ghostship_metal2_i8 ghostship_metallight_ci4 "
      "ghostship_metallight_pal_rgba16 ghostship_window_i8 ghostship_wood3_ci8 ghostship_wood3_pal_rgba16 "
      "ghostship_woodmoldy1_rgba16 ghostship_woodmoldy2_ci8 ghostship_woodmoldy2_pal_rgba16 mat_ghostship_boat1 "
      "mat_ghostship_boat2 mat_ghostship_boat3 mat_ghostship_dark mat_ghostship_flag mat_ghostship_glass1 "
      "mat_ghostship_metal1 mat_ghostship_sail mat_revert_ghostship_boat1 mat_revert_ghostship_boat2 "
      "mat_revert_ghostship_boat3 mat_revert_ghostship_dark mat_revert_ghostship_flag "
      "mat_revert_ghostship_glass1 mat_revert_ghostship_metal1 mat_revert_ghostship_sail" },
    { "courses/harbour/ghostship_model.c", "NEVER-WRITTEN",
      "ghostship_burntfabricHoles_rgba16_aligner ghostship_flag1_ci4_aligner ghostship_flag1_pal_rgba16_aligner "
      "ghostship_metal2_i8_aligner ghostship_metallight_ci4_aligner ghostship_metallight_pal_rgba16_aligner "
      "ghostship_window_i8_aligner ghostship_wood3_ci8_aligner ghostship_wood3_pal_rgba16_aligner "
      "ghostship_woodmoldy1_rgba16_aligner ghostship_woodmoldy2_ci8_aligner "
      "ghostship_woodmoldy2_pal_rgba16_aligner" },
    { "courses/harbour/powered.c", "ESCAPES-only",
      "hm64_Text_001_mesh_tri_0 hm64_Text_001_mesh_tri_1 hm64_Text_001_mesh_vtx_0 hm64_Text_001_mesh_vtx_1 "
      "hm64_Text_001_mesh_vtx_cull hm64_grayishtoblue_rgba16 hm64_heart1_rgba16 mat_hm64_HM mat_hm64__64 "
      "mat_powered__64_001 mat_revert_hm64_HM mat_revert_hm64__64 mat_revert_powered__64_001 "
      "powered_Text_mesh_tri_0 powered_Text_mesh_vtx_0 powered_Text_mesh_vtx_cull powered_heart1_rgba16" },
    { "courses/harbour/powered.c", "NEVER-WRITTEN",
      "hm64_Text_001_mesh hm64_grayishtoblue_rgba16_aligner hm64_heart1_rgba16_aligner powered_Text_mesh "
      "powered_heart1_rgba16_aligner" },
    { "courses/harbour/ship2_model.c", "ESCAPES-only",
      "mat_revert_ship2_f3d_material_002 mat_revert_ship2_f3d_material_003 mat_revert_ship2_sohSail "
      "mat_revert_ship2_wood1_001 mat_revert_ship2_wood2 mat_ship2_f3d_material_002 mat_ship2_f3d_material_003 "
      "mat_ship2_sohSail mat_ship2_wood1_001 mat_ship2_wood2 ship2_SoH_mesh ship2_SoH_mesh_tri_0 "
      "ship2_SoH_mesh_tri_1 ship2_SoH_mesh_tri_2 ship2_SoH_mesh_tri_3 ship2_SoH_mesh_tri_4 ship2_SoH_mesh_vtx_0 "
      "ship2_SoH_mesh_vtx_1 ship2_SoH_mesh_vtx_2 ship2_SoH_mesh_vtx_3 ship2_SoH_mesh_vtx_4 "
      "ship2_SoH_mesh_vtx_cull ship2_flag_rgba16 ship2_sail2_rgba16 ship2_window_i8 ship2_wood1_ci8 "
      "ship2_wood1_pal_rgba16 ship2_wood2_ci4 ship2_wood2_pal_rgba16" },
    { "courses/harbour/ship2_model.c", "NEVER-WRITTEN",
      "ship2_flag_rgba16_aligner ship2_sail2_rgba16_aligner ship2_window_i8_aligner ship2_wood1_ci8_aligner "
      "ship2_wood1_pal_rgba16_aligner ship2_wood2_ci4_aligner ship2_wood2_pal_rgba16_aligner" },
    { "courses/harbour/ship3_model.c", "ESCAPES-only",
      "mat_revert_ship3__2shipLogo mat_revert_ship3_f3d_material_008 mat_revert_ship3_f3dlite_material_003 "
      "mat_revert_ship3_flag1 mat_revert_ship3_sail2 mat_revert_ship3_wood2_001 mat_ship3__2shipLogo "
      "mat_ship3_f3d_material_008 mat_ship3_f3dlite_material_003 mat_ship3_flag1 mat_ship3_sail2 "
      "mat_ship3_wood2_001 ship3_2Ship_mesh ship3_2Ship_mesh_tri_0 ship3_2Ship_mesh_tri_1 "
      "ship3_2Ship_mesh_tri_2 ship3_2Ship_mesh_tri_3 ship3_2Ship_mesh_tri_4 ship3_2Ship_mesh_tri_5 "
      "ship3_2Ship_mesh_vtx_0 ship3_2Ship_mesh_vtx_1 ship3_2Ship_mesh_vtx_2 ship3_2Ship_mesh_vtx_3 "
      "ship3_2Ship_mesh_vtx_4 ship3_2Ship_mesh_vtx_5 ship3_2Ship_mesh_vtx_cull ship3__2shipBetaLogo_rgba32 "
      "ship3_flag_rgba16 ship3_sail1_rgba16 ship3_window_i8 ship3_wood1_ci4 ship3_wood1_pal_rgba16 "
      "ship3_wood2_ci8 ship3_wood2_pal_rgba16" },
    { "courses/harbour/ship3_model.c", "NEVER-WRITTEN",
      "ship3__2shipBetaLogo_rgba32_aligner ship3_flag_rgba16_aligner ship3_sail1_rgba16_aligner "
      "ship3_window_i8_aligner ship3_wood1_ci4_aligner ship3_wood1_pal_rgba16_aligner ship3_wood2_ci8_aligner "
      "ship3_wood2_pal_rgba16_aligner" },
    { "courses/harbour/ship_model.c", "ESCAPES-only",
      "mat_revert_wheels_metal1_002 mat_revert_wheels_tire1_002 mat_ship1_black mat_ship1_black_001 "
      "mat_ship1_boat1 mat_ship1_engine mat_ship1_engine2 mat_ship1_flag mat_ship1_logo mat_ship1_meat1 "
      "mat_ship1_metal1 mat_ship1_metal1_001 mat_ship1_sail mat_ship1_sauce1 mat_ship1_spag1 mat_ship1_wood2 "
      "mat_wheels_metal1_002 mat_wheels_tire1_002 ship1__32wood_rgba16 ship1_enginepiece_rgba16 "
      "ship1_flag_rgba16 ship1_hmpainted_rgba16 ship1_metal1_i8 ship1_metal1_i8_copy ship1_sail2_rgba16 "
      "ship1_sauce_rgba16 ship1_shinyengine_ci8 ship1_shinyengine_pal_rgba16 ship1_spag1_003_mesh_tri_0 "
      "ship1_spag1_003_mesh_tri_1 ship1_spag1_003_mesh_tri_10 ship1_spag1_003_mesh_tri_11 "
      "ship1_spag1_003_mesh_tri_12 ship1_spag1_003_mesh_tri_13 ship1_spag1_003_mesh_tri_2 "
      "ship1_spag1_003_mesh_tri_3 ship1_spag1_003_mesh_tri_4 ship1_spag1_003_mesh_tri_5 "
      "ship1_spag1_003_mesh_tri_6 ship1_spag1_003_mesh_tri_7 ship1_spag1_003_mesh_tri_8 "
      "ship1_spag1_003_mesh_tri_9 ship1_spag1_003_mesh_vtx_0 ship1_spag1_003_mesh_vtx_1 "
      "ship1_spag1_003_mesh_vtx_10 ship1_spag1_003_mesh_vtx_11 ship1_spag1_003_mesh_vtx_12 "
      "ship1_spag1_003_mesh_vtx_13 ship1_spag1_003_mesh_vtx_2 ship1_spag1_003_mesh_vtx_3 "
      "ship1_spag1_003_mesh_vtx_4 ship1_spag1_003_mesh_vtx_5 ship1_spag1_003_mesh_vtx_6 "
      "ship1_spag1_003_mesh_vtx_7 ship1_spag1_003_mesh_vtx_8 ship1_spag1_003_mesh_vtx_9 "
      "ship1_spag1_003_mesh_vtx_cull ship1_spag1_mesh ship1_spag1_rgba16 ship1_wood2_rgba16 ship1_wood_rgba16 "
      "wheels_Spaghetti_002_mesh wheels_Spaghetti_002_mesh_tri_0 wheels_Spaghetti_002_mesh_tri_1 "
      "wheels_Spaghetti_002_mesh_vtx_0 wheels_Spaghetti_002_mesh_vtx_1 wheels_Spaghetti_002_mesh_vtx_cull "
      "wheels_metal1_i8 wheels_tire_i8" },
    { "courses/harbour/ship_model.c", "NEVER-WRITTEN",
      "ship1__32wood_rgba16_aligner ship1_enginepiece_rgba16_aligner ship1_flag_rgba16_aligner "
      "ship1_hmpainted_rgba16_aligner ship1_metal1_i8_aligner ship1_metal1_i8_copy_aligner "
      "ship1_sail2_rgba16_aligner ship1_sauce_rgba16_aligner ship1_shinyengine_ci8_aligner "
      "ship1_shinyengine_pal_rgba16_aligner ship1_spag1_rgba16_aligner ship1_wood2_rgba16_aligner "
      "ship1_wood_rgba16_aligner wheels_metal1_i8_aligner wheels_tire_i8_aligner" },
    { "courses/harbour/starship_model.c", "ESCAPES-only",
      "mat_starship_black mat_starship_f3dlite_material_008 mat_starship_flare mat_starship_glass "
      "mat_starship_starship1 mat_starship_starship2 mat_starship_starship3 starship_Cube_mesh "
      "starship_Cube_mesh_tri_0 starship_Cube_mesh_tri_1 starship_Cube_mesh_tri_2 starship_Cube_mesh_tri_3 "
      "starship_Cube_mesh_tri_4 starship_Cube_mesh_tri_5 starship_Cube_mesh_tri_6 starship_Cube_mesh_vtx_0 "
      "starship_Cube_mesh_vtx_1 starship_Cube_mesh_vtx_2 starship_Cube_mesh_vtx_3 starship_Cube_mesh_vtx_4 "
      "starship_Cube_mesh_vtx_5 starship_Cube_mesh_vtx_6 starship_Cube_mesh_vtx_cull "
      "starship_SoHShinyBlueish_rgba16 starship_SoHShiny_rgba16 starship_flare1_ci8 starship_flare1_pal_rgba16 "
      "starship_shiplogov2_rgba32 starship_superFXBlue_rgba16 starship_superFXBlue_rgba32 starship_tire_i8" },
    { "courses/harbour/starship_model.c", "NEVER-WRITTEN",
      "starship_SoHShinyBlueish_rgba16_aligner starship_SoHShiny_rgba16_aligner starship_flare1_ci8_aligner "
      "starship_flare1_pal_rgba16_aligner starship_shiplogov2_rgba32_aligner "
      "starship_superFXBlue_rgba16_aligner starship_superFXBlue_rgba32_aligner starship_tire_i8_aligner" },
    { "courses/harbour/track.c", "ESCAPES-only",
      "bush_leaves1_rgba32 bush_map_004_mesh bush_map_004_mesh_tri_0 bush_map_004_mesh_vtx_0 "
      "bush_map_004_mesh_vtx_cull castle_bars1_rgba16 castle_brick1_rgba16 castle_door1_rgba16 "
      "castle_fence_rgba32 castle_map_002_mesh castle_map_002_mesh_tri_0 castle_map_002_mesh_tri_1 "
      "castle_map_002_mesh_tri_2 castle_map_002_mesh_tri_3 castle_map_002_mesh_tri_4 castle_map_002_mesh_tri_5 "
      "castle_map_002_mesh_vtx_0 castle_map_002_mesh_vtx_1 castle_map_002_mesh_vtx_2 castle_map_002_mesh_vtx_3 "
      "castle_map_002_mesh_vtx_4 castle_map_002_mesh_vtx_5 castle_map_002_mesh_vtx_cull "
      "castle_seamless_wood_texture_free_76_rgba16 castle_superFXSteel_rgba16 ground_door1_rgba16 "
      "ground_flag_rgba16 ground_keyboardArt_rgba16 ground_map_mesh ground_map_mesh_tri_0 ground_map_mesh_tri_1 "
      "ground_map_mesh_tri_10 ground_map_mesh_tri_11 ground_map_mesh_tri_12 ground_map_mesh_tri_13 "
      "ground_map_mesh_tri_2 ground_map_mesh_tri_3 ground_map_mesh_tri_4 ground_map_mesh_tri_5 "
      "ground_map_mesh_tri_6 ground_map_mesh_tri_7 ground_map_mesh_tri_8 ground_map_mesh_tri_9 "
      "ground_map_mesh_vtx_0 ground_map_mesh_vtx_1 ground_map_mesh_vtx_10 ground_map_mesh_vtx_11 "
      "ground_map_mesh_vtx_12 ground_map_mesh_vtx_13 ground_map_mesh_vtx_2 ground_map_mesh_vtx_3 "
      "ground_map_mesh_vtx_4 ground_map_mesh_vtx_5 ground_map_mesh_vtx_6 ground_map_mesh_vtx_7 "
      "ground_map_mesh_vtx_8 ground_map_mesh_vtx_9 ground_map_mesh_vtx_cull ground_noparking_rgba16 "
      "ground_parascode_rgba16 ground_paraseye_rgba16 ground_parking_rgba16 ground_road4_rgba16 "
      "ground_sand_rgba16 "
      "ground_seamless_green_lush_grass_texture_seamless_green_lush_grass_texture_fresh_backdrop_144751691_rgba16 "
      "ground_seamless_wood_texture_free_76_rgba16 ground_window_rgba16 ground_woodPlanks1_rgba16 "
      "mat_bush_leaves mat_castle_door1 mat_castle_fence mat_castle_grate mat_castle_roof1 mat_castle_wall1 "
      "mat_castle_window1 mat_ground_black mat_ground_dock mat_ground_door1 mat_ground_f3d_material_001 "
      "mat_ground_f3d_material_006 mat_ground_f3d_material_013 mat_ground_f3d_material_014 "
      "mat_ground_f3dlite_material_001 mat_ground_ground mat_ground_parking mat_ground_path mat_ground_roof1 "
      "mat_ground_sand mat_ground_sign mat_moon_black mat_moon_eye2 mat_moon_moon1 mat_moon_moon2 "
      "mat_moon_teeth mat_revert_bush_leaves mat_revert_castle_door1 mat_revert_castle_fence "
      "mat_revert_castle_grate mat_revert_castle_roof1 mat_revert_castle_wall1 mat_revert_castle_window1 "
      "mat_revert_ground_black mat_revert_ground_dock mat_revert_ground_door1 "
      "mat_revert_ground_f3d_material_001 mat_revert_ground_f3d_material_006 mat_revert_ground_f3d_material_013 "
      "mat_revert_ground_f3d_material_014 mat_revert_ground_f3dlite_material_001 mat_revert_ground_ground "
      "mat_revert_ground_parking mat_revert_ground_path mat_revert_ground_roof1 mat_revert_ground_sand "
      "mat_revert_ground_sign mat_revert_moon_black mat_revert_moon_eye2 mat_revert_moon_moon1 "
      "mat_revert_moon_moon2 mat_revert_moon_teeth mat_revert_road_path mat_revert_statue_black "
      "mat_revert_statue_f3dlite_material_002 mat_revert_statue_f3dlite_material_007 "
      "mat_revert_statue_f3dlite_material_009 mat_revert_statue_flowers mat_revert_trees_bark1 "
      "mat_revert_trees_leaf1 mat_revert_water_water1 mat_revert_water_water2 mat_road_path mat_statue_black "
      "mat_statue_f3dlite_material_002 mat_statue_f3dlite_material_007 mat_statue_f3dlite_material_009 "
      "mat_statue_flowers mat_trees_bark1 mat_trees_leaf1 mat_water_water1 mat_water_water2 moon_ARmosMap_ci8 "
      "moon_ARmosMap_pal_rgba16 moon_moon1_ci8 moon_moon1_pal_rgba16 moon_moon2_ci8 moon_moon2_pal_rgba16 "
      "moon_moon_mesh moon_moon_mesh_tri_0 moon_moon_mesh_tri_1 moon_moon_mesh_tri_2 moon_moon_mesh_tri_3 "
      "moon_moon_mesh_tri_4 moon_moon_mesh_vtx_0 moon_moon_mesh_vtx_1 moon_moon_mesh_vtx_2 moon_moon_mesh_vtx_3 "
      "moon_moon_mesh_vtx_4 moon_moon_mesh_vtx_cull moon_teeth1_rgba32 moon_window_rgba16 road_map_001_mesh "
      "road_map_001_mesh_tri_0 road_map_001_mesh_vtx_0 road_map_001_mesh_vtx_cull road_road4_rgba32 "
      "statue_flowers1_rgba32 statue_map_005_mesh statue_map_005_mesh_tri_0 statue_map_005_mesh_tri_1 "
      "statue_map_005_mesh_tri_2 statue_map_005_mesh_tri_3 statue_map_005_mesh_tri_4 statue_map_005_mesh_vtx_0 "
      "statue_map_005_mesh_vtx_1 statue_map_005_mesh_vtx_2 statue_map_005_mesh_vtx_3 statue_map_005_mesh_vtx_4 "
      "statue_map_005_mesh_vtx_cull statue_marble1_ci8 statue_marble1_pal_rgba16 statue_window_rgba16 "
      "trees_bark2_rgba32 trees_leaf_ci4 trees_leaf_pal_rgba16 trees_map_003_mesh trees_map_003_mesh_tri_0 "
      "trees_map_003_mesh_tri_1 trees_map_003_mesh_vtx_0 trees_map_003_mesh_vtx_1 trees_map_003_mesh_vtx_cull "
      "water_water1_mesh water_water1_mesh_tri_0 water_water1_mesh_tri_1 water_water1_mesh_vtx_0 "
      "water_water1_mesh_vtx_1 water_water1_mesh_vtx_cull water_water1_rgba32 water_water3_rgba16" },
    { "courses/harbour/track.c", "NEVER-WRITTEN",
      "bush_leaves1_rgba32_aligner castle_bars1_rgba16_aligner castle_brick1_rgba16_aligner "
      "castle_door1_rgba16_aligner castle_fence_rgba32_aligner "
      "castle_seamless_wood_texture_free_76_rgba16_aligner castle_superFXSteel_rgba16_aligner "
      "ground_door1_rgba16_aligner ground_flag_rgba16_aligner ground_keyboardArt_rgba16_aligner "
      "ground_noparking_rgba16_aligner ground_parascode_rgba16_aligner ground_paraseye_rgba16_aligner "
      "ground_parking_rgba16_aligner ground_road4_rgba16_aligner ground_sand_rgba16_aligner "
      "ground_seamless_green_lush_grass_texture_seamless_green_lush_grass_texture_fresh_backdrop_144751691_rgba16_aligner "
      "ground_seamless_wood_texture_free_76_rgba16_aligner ground_window_rgba16_aligner "
      "ground_woodPlanks1_rgba16_aligner moon_ARmosMap_ci8_aligner moon_ARmosMap_pal_rgba16_aligner "
      "moon_moon1_ci8_aligner moon_moon1_pal_rgba16_aligner moon_moon2_ci8_aligner "
      "moon_moon2_pal_rgba16_aligner moon_teeth1_rgba32_aligner moon_window_rgba16_aligner "
      "road_road4_rgba32_aligner statue_flowers1_rgba32_aligner statue_marble1_ci8_aligner "
      "statue_marble1_pal_rgba16_aligner statue_window_rgba16_aligner trees_bark2_rgba32_aligner "
      "trees_leaf_ci4_aligner trees_leaf_pal_rgba16_aligner water_water1_rgba32_aligner "
      "water_water3_rgba16_aligner" },
    { "courses/kalimari_desert/course_offsets.c", "ESCAPES-only", "kalimari_desert_dls" },
    { "courses/koopa_troopa_beach/course_offsets.c", "ESCAPES-only",
      "d_course_koopa_troopa_beach_dl_list1 d_course_koopa_troopa_beach_unk4 "
      "d_course_koopa_troopa_beach_unk_data2 d_course_koopa_troopa_beach_unk_data3 "
      "d_course_koopa_troopa_beach_unk_data4 d_course_koopa_troopa_beach_unk_data5 koopa_troopa_beach_dls2" },
    { "courses/koopa_troopa_beach/course_offsets.c", "NEVER-WRITTEN",
      "d_course_koopa_troopa_beach_unk_data1 d_course_koopa_troopa_beach_unk_data6" },
    { "courses/luigi_raceway/course_offsets.c", "ESCAPES-only", "luigi_raceway_dls" },
    { "courses/mario_raceway/course_offsets.c", "ESCAPES-only", "mario_raceway_dls" },
    { "courses/moo_moo_farm/course_offsets.c", "ESCAPES-only", "moo_moo_farm_dls" },
    { "courses/rainbow_road/course_offsets.c", "ESCAPES-only",
      "d_rainbow_road_chomp_jaw_animation d_rainbow_road_unk1 d_rainbow_road_unk2 d_rainbow_road_unk3 "
      "d_rainbow_road_unk4 rainbow_road_dls" },
    { "courses/rainbow_road/course_offsets.c", "NEVER-WRITTEN",
      "d_course_rainbow_road_unk_double d_course_rainbow_road_unk_double2 d_rainbow_road_unk5" },
    { "courses/royal_raceway/course_offsets.c", "ESCAPES-only", "royal_raceway_dls" },
    { "courses/sherbet_land/course_offsets.c", "ESCAPES-only",
      "d_course_sherbet_land_unk_data1 d_course_sherbet_land_unk_data10 d_course_sherbet_land_unk_data11 "
      "d_course_sherbet_land_unk_data2 d_course_sherbet_land_unk_data3 d_course_sherbet_land_unk_data4 "
      "d_course_sherbet_land_unk_data5 d_course_sherbet_land_unk_data6 d_course_sherbet_land_unk_data7 "
      "d_course_sherbet_land_unk_data8 d_course_sherbet_land_unk_data9 sherbet_land_dls sherbet_land_dls_2" },
    { "courses/sherbet_land/course_offsets.c", "NEVER-WRITTEN", "d_course_sherbet_land_unk_data12" },
    { "courses/staff_ghost_data.c", "ESCAPES-only",
      "d_luigi_raceway_staff_ghost d_mario_raceway_staff_ghost d_royal_raceway_staff_ghost" },
    { "courses/staff_ghost_data.c", "NEVER-WRITTEN", "D_0F4FCAD0" },
    { "courses/test_track/data.c", "ESCAPES-only", "test_track_addr test_track_path" },
    { "courses/test_track/data.c", "NEVER-WRITTEN", "test_course_data_dl" },
    { "courses/test_track/model.c", "ESCAPES-only",
      "mario_Plane_001_mesh mario_Plane_001_mesh_tri_0 mario_Plane_001_mesh_tri_1 mario_Plane_001_mesh_vtx_0 "
      "mario_Plane_001_mesh_vtx_1 mario_Plane_001_mesh_vtx_cull mario_f3dlite_material_009_lights "
      "mario_f3dlite_material_lights mat_mario_f3dlite_material mat_mario_f3dlite_material_009 "
      "mat_revert_mario_f3dlite_material mat_revert_mario_f3dlite_material_009" },
    { "courses/test_track/vtx.c", "NEVER-WRITTEN", "test_course_vertices" },
    { "courses/toads_turnpike/course_offsets.c", "ESCAPES-only", "d_course_toads_turnpike_dl_list" },
    { "courses/wario_stadium/course_offsets.c", "ESCAPES-only", "wario_stadium_dls" },
    { "courses/yoshi_valley/course_offsets.c", "ESCAPES-only",
      "d_course_yoshi_valley_dl_list d_course_yoshi_valley_unk1 d_course_yoshi_valley_unk2 "
      "d_course_yoshi_valley_unk3 d_course_yoshi_valley_unk4 d_course_yoshi_valley_unk5" },
    { "courses/yoshi_valley/course_offsets.c", "NEVER-WRITTEN", "d_course_yoshi_valley_unk6" },
    { "include/assets/textures/boo_frames.h", "WRITTEN-init-only", "gTextureGhosts" },
    { "include/assets/textures/common_data.h", "ABSENT",
      "common_texture_bomb common_texture_player_emblem common_tlut_lakitu_countdown" },
    { "include/assets/textures/common_data.h", "ESCAPES-only", "common_texture_particle_smoke" },
    { "include/assets/textures/common_data.h", "IR-CONSTANT",
      "D_0D015258 common_texture_hud_place common_texture_hud_type_C_rank_font "
      "common_texture_hud_type_C_rank_tiny_font common_texture_minimap_kart_character "
      "common_texture_particle_spark" },
    { "include/assets/textures/karts/bowser_kart.h", "ESCAPES-only", "bowser_kart_wheels" },
    { "include/assets/textures/karts/donkeykong_kart.h", "ESCAPES-only", "donkeykong_kart_wheels" },
    { "include/assets/textures/karts/luigi_kart.h", "ESCAPES-only", "luigi_kart_wheels" },
    { "include/assets/textures/karts/mario_kart.h", "ESCAPES-only", "mario_kart_wheels" },
    { "include/assets/textures/karts/peach_kart.h", "ESCAPES-only", "peach_kart_wheels" },
    { "include/assets/textures/karts/toad_kart.h", "ESCAPES-only", "toad_kart_wheels" },
    { "include/assets/textures/karts/wario_kart.h", "ESCAPES-only", "wario_kart_wheels" },
    { "include/assets/textures/karts/yoshi_kart.h", "ESCAPES-only", "yoshi_kart_wheels" },
    { "include/assets/textures/tracks/banshee_boardwalk/banshee_boardwalk_data.h", "ABSENT",
      "d_course_banshee_boardwalk_bat" },
    { "include/assets/textures/tracks/rainbow_road/rainbow_road_data.h", "ESCAPES-only",
      "d_course_rainbow_road_neon_boo_tlut_list d_course_rainbow_road_neon_mario_tlut_list "
      "d_course_rainbow_road_neon_mushroom_tlut_list d_course_rainbow_road_static_textures" },
    { "include/assets/textures/tracks/rainbow_road/rainbow_road_data.h", "IR-CONSTANT",
      "d_course_rainbow_road_static_tluts" },
    { "src/actors/piranha_plant/render.inc.c", "NEVER-WRITTEN", "sPiranhaPlantTextures" },
    { "src/audio/audio_session_presets.c", "NEVER-WRITTEN",
      "D_800EA5D8 gAudioHeapSize gAudioInitPoolSize gAudioSessionPresets gTatumsPerBeat gUnusedCount800EA5C8" },
    { "src/audio/data.c", "NEVER-WRITTEN",
      "D_803B7192 fill fill2 gAudioGlobalsEndMarker gDefaultNoteSub gDefaultPanVolume gHeadsetPanQuantization "
      "gHeadsetPanVolume gNoteFrequencies gPitchBendFrequencyScale gStereoPanVolume gUnknownData_800F6290 "
      "gWaveSamples gZeroNoteSub" },
    { "src/audio/external.c", "NEVER-WRITTEN",
      "D_800E9DA4 D_800EA0F8 D_800EA154 D_800EA188 D_800EA1A0 D_800EA1F0 external_unused_string00 "
      "external_unused_string01 external_unused_string02 external_unused_string03 external_unused_string04 "
      "external_unused_string05 external_unused_string06 external_unused_string07 external_unused_string08 "
      "external_unused_string09 external_unused_string10 external_unused_string11 external_unused_string12 "
      "external_unused_string13 external_unused_u32_0 external_unused_u32_1 external_unused_u32s" },
    { "src/audio/heap.c", "NEVER-WRITTEN",
      "gAudioUnusedBuffer heapAudioString00 heapAudioString01 heapAudioString02 heapAudioString03 "
      "heapAudioString04 heapAudioString05 heapAudioString06 heapAudioString07 heapAudioString08 "
      "heapAudioString09 heapAudioString10 heapAudioString11 heapAudioString12 heapAudioString13 "
      "heapAudioString14 heapAudioString15 heapAudioString16 heapAudioString17 heapAudioString18 "
      "heapAudioString19 heapAudioString20 heapAudioString21 heapAudioString22 heapAudioString23 "
      "heapAudioString24 heapAudioString25 heapAudioString26 heapAudioString27 heapAudioString28 "
      "heapAudioString29 heapAudioString30 heapAudioString31 heapAudioString32 heapAudioString33 "
      "heapAudioString34 heapAudioString35 heapAudioString36 heapAudioString37 heapAudioString38 sAudioHeapPad" },
    { "src/audio/load.c", "NEVER-WRITTEN",
      "gAlBankSets gAlCtlHeader gAlTbl gCtlEntries gCurrAudioFrameDmaIoMesgBufs gSeqFileHeader "
      "loadAudioString00 loadAudioString01 loadAudioString02 loadAudioString03 loadAudioString04 "
      "loadAudioString05 loadAudioString06 loadAudioString07 loadAudioString08 loadAudioString09 "
      "loadAudioString10 loadAudioString11 loadAudioString12 loadAudioString13 loadAudioString14 "
      "loadAudioString15 loadAudioString16 loadAudioString17 loadAudioString18 loadAudioString19 "
      "loadAudioString20 loadAudioString21 loadAudioString22 loadAudioString23 loadAudioString24 "
      "loadAudioString25 loadAudioString26 loadAudioString27 loadAudioString28 loadAudioString29 "
      "loadAudioString30 loadAudioString31 loadAudioString32 loadAudioString33 loadAudioString34 "
      "loadAudioString35 loadAudioString36 loadAudioString37 loadAudioString38 load_bss_pad" },
    { "src/audio/mixer.c", "ABSENT", "lower_bit" },
    { "src/audio/mixer.c", "IR-CONSTANT", "resample_table" },
    { "src/audio/port_eu.c", "ABSENT", "create_next_audio_buffer.gMaxAbiCmdCnt" },
    { "src/audio/port_eu.c", "NEVER-WRITTEN",
      "D_800EA3A8 D_800EA3AC D_800EA3B0 D_800EA3B4 port_eu_unused_string0 port_eu_unused_string1 "
      "port_eu_unused_string2 port_eu_unused_string3 port_eu_unused_string4 port_eu_unused_string5 "
      "port_eu_unused_string6 port_eu_unused_string7" },
    { "src/audio/seqplayer.c", "NEVER-WRITTEN",
      "seqplayer_unused_string00 seqplayer_unused_string01 seqplayer_unused_string02 seqplayer_unused_string03 "
      "seqplayer_unused_string04 seqplayer_unused_string05 seqplayer_unused_string06 seqplayer_unused_string07 "
      "seqplayer_unused_string08 seqplayer_unused_string09 seqplayer_unused_string10 seqplayer_unused_string11 "
      "seqplayer_unused_string12 seqplayer_unused_string13 seqplayer_unused_string14 seqplayer_unused_string15 "
      "seqplayer_unused_string16 seqplayer_unused_string17 seqplayer_unused_string18 seqplayer_unused_string19 "
      "seqplayer_unused_string20 seqplayer_unused_string21 seqplayer_unused_string22 seqplayer_unused_string23 "
      "seqplayer_unused_string24 seqplayer_unused_string25" },
    { "src/audio/synthesis.c", "NEVER-WRITTEN",
      "sAudioSynthesisPad synthesisAudioString0 synthesisAudioString1 synthesisAudioString2 "
      "synthesisAudioString3 synthesisAudioString4" },
    { "src/buffers/gfx_output_buffer.c", "NEVER-WRITTEN", "gGfxSPTaskOutputBuffer gGfxSPTaskOutputBufferSize" },
    { "src/buffers/memory_pool.c", "NEVER-WRITTEN", "sMemoryPool" },
    { "src/buffers/random.c", "NEVER-WRITTEN", "randomSeedPadding" },
    { "src/buffers/trig_tables.c", "NEVER-WRITTEN", "gArctanTable gSineTable" },
    { "src/buffers/trig_tables_bss.c", "NEVER-WRITTEN", "trigTablesPadding" },
    { "src/camera.c", "NEVER-WRITTEN", "D_800DDB30 D_801649D0 camera1 camera2 camera3 camera4 gFreecamCamera" },
    { "src/code_800029B0.c", "NEVER-WRITTEN",
      "D_800DC5CC D_800DC5D0 D_800DC5D4 D_800DC5D8 D_8015F6F8 D_8015F718 code_800029B0_bss_pad pAppNmiBuffer "
      "pad_800029B0" },
    { "src/code_800029B0.c", "WRITTEN-init-only", "gHeapEndPtr" },
    { "src/code_80005FD0.c", "NEVER-WRITTEN",
      "D_800EB710 D_800EB728 D_800EB74C D_800EB780 D_800EB7BC D_800EB800 D_800EB824 D_800EB84C D_800EB87C "
      "D_800EB8A0 D_800EB8C4 D_800EB8E8 D_800EB8FC D_800EB910 D_800EB934 D_800EB940 D_800EB964 D_800EB970 "
      "D_800EB9B4 D_800EB9DC D_800EBA20 D_800EBA38 D_800EBA58 D_800EBA94 D_800EBAE4 D_800EBB60 D_800EBBDC "
      "D_800EBC24 D_800EBC50 D_800EBC60 D_800EBC74 D_800EBC80 D_800EBC98 D_800EBCB0 D_800EBCD8 D_800EBD00 "
      "D_800EBD28 D_800EBD54 D_800EBD68 D_800EBD80 D_800EBD90 D_800EBDB4 D_800EBDB8 D_800EBDE0 D_800EBE08 "
      "D_800EBE30 D_800EBE58 D_800EBE80 D_800EBEA8 D_800EBEAC D_800EBECC D_800EBED8 D_800EBEE8 D_800EBF08 "
      "D_800EBF34 D_800EBF5C D_800EBF84 D_800EBFA8 D_800EBFC8 D_800EBFEC D_800EC01C D_800EC044 D_800EC068 "
      "D_800EC088 D_800EC0B0 D_800EC0D4 D_800EC0FC D_800EC124 D_800EC150 D_800EC164 D_800EC184 D_800EC1A0 "
      "D_800EC1BC D_800EC1D4 D_800EC1EC D_800EC21C D_800EC25C D_800EC27C D_800EC298 D_800EC2C0 D_800EC2E4 "
      "D_800EC300 D_800EC32C D_800EC344 D_800EC350 D_800EC360 D_800EC374 D_800EC384 D_800EC394 D_800EC3A0 "
      "D_800EC3BC D_800EC3E4 D_800EC3FC D_800EC414 D_800EC434 D_800EC454 D_800EC474 D_800EC494 D_800EC4B4 "
      "D_800EC4D0 D_800EC4F4 D_800EC514 D_800EC53C D_800EC568 D_800EC594 D_800EC5C0 D_800EC5F8 D_800EC628 "
      "D_800EC65C D_800EC670 D_800EC684 D_800EC694 D_800EC6A0 D_800EC6D0 D_800EC708 D_800EC71C D_800EC730 "
      "D_800EC740 D_800EC74C D_800EC76C D_800EC798 D_800EC7D8 D_800EC7EC D_800EC800 D_800EC810 D_800EC81C "
      "D_800EC858 D_800EC86C D_800EC880 D_800EC890 D_800EC89C D_800EC8CC D_800EC8F4 D_800EC920 D_800EC954 "
      "D_800EC968 D_800EC97C D_800EC98C D_800EC998 D_800EC9D8 D_800EC9EC D_800ECA00 D_800ECA10 D_800ECA1C "
      "D_800ECA34 D_800ECA70 D_800ECA98 D_800ECAC4 D_800ECAF8 D_800ECB0C D_800ECB20 D_800ECB30 D_800ECB3C "
      "D_800ECB78 D_800ECBB8 D_800ECBCC D_800ECBE0 D_800ECBF0 D_800ECBFC D_800ECC14 D_800ECC44 D_800ECC70 "
      "D_800ECCA0 D_800ECCD8 D_800ECCEC D_800ECCFC D_800ECD0C D_800ECD34 D_800ECD5C D_800ECD90 D_800ECDA4 "
      "D_800ECDB8 D_800ECDC8 D_800ECDD4 D_800ECE00 D_800ECE38 D_800ECE4C D_800ECE60 D_800ECE70 D_800ECE7C "
      "D_800ECE8C D_800ECE98 D_800ECEB4 D_800ECEDC D_800ECEF0 D_800ECF14 D_800ECF24 D_800ECF30 D_800ECF3C "
      "D_800ECF4C D_800ECF60 D_80162F08 D_80162F90 D_801646A4 D_801646A8 D_801646AC D_801646B0 D_801646B4 "
      "D_801646B8 D_801646BC unk_code_80005FD0_pad" },
    { "src/code_80005FD0.c", "WRITTEN-init-only",
      "gBombKartCollision gBombKarts gBoxTruckList gCarList gFerrySmokeTimer gSchoolBusList gTankerTruckList "
      "gTrainList gTrainSmokeTimer isCrossingTriggeredByIndex" },
    { "src/code_80057C60.c", "ABSENT", "func_8005EA94.test func_8005ED48.test setup_tyre_particles.test" },
    { "src/code_80057C60.c", "NEVER-WRITTEN",
      "D_800E4730 D_800E4770 D_800E47A0 D_800E47DC D_800E480C D_801655A0 D_801655A8 D_801655B0 D_801655B8 "
      "D_801655D0 D_801655DC D_801655EC D_801655F4 D_80165600 D_80165610 D_80165620 D_80165630 D_80165640 "
      "D_80165650 D_80165680 D_801656B8 D_801656C8 D_801656D8 D_801656E8 D_801656F8 D_8016570C D_80165710 "
      "D_80165734 D_80165738 D_8016573C D_80165740 D_80165744 D_80165748 D_8016574C D_80165768 D_80165778 "
      "D_80165788 D_8016578E D_80165792 D_80165798 D_801657A0 D_801657A4 D_801657A8 D_801657AF D_801657B1 "
      "D_801657B3 D_801657B4 D_801657C8 D_801657DA D_801657E0 D_801657EC D_801657F4 D_80165824 D_8016583A "
      "D_80165848 D_80165864 D_80165868 D_80165870 D_80165884 D_8016588C D_80165894 D_8016589C D_801658A0 "
      "D_801658B0 D_801658C0 D_801658C4 D_801658C8 D_801658CC D_801658D0 D_801658D4 D_801658D8 D_801658E0 "
      "D_801658E8 D_801658F0 D_801658F8 D_801658FC D_80165904 D_80165910 D_80165AA0 D_80165C14 D_80183D58 "
      "D_80183DF8 D_80183E60 D_80183F20 D_8018C5F0 D_8018CF24 gIndexObjectBombKart gNumActiveThwomps "
      "gThowmpSpawnList sRandomItemIndex some_unused_data" },
    { "src/code_80057C60.c", "WRITTEN-init-only", "D_8018CC80" },
    { "src/code_80057C60_var.c", "NEVER-WRITTEN",
      "D_8018CFFC D_8018D004 D_8018D014 D_8018D118 D_8018D128 D_8018D148 D_8018D154 D_8018D15C D_8018D164 "
      "D_8018D198 D_8018D1A8 D_8018D1B8 D_8018D1C8 D_8018D1D0 D_8018D1D8 D_8018D1E4 D_8018D1E8 D_8018D1F4 "
      "D_8018D200 D_8018D208 D_8018D210 D_8018D218 D_8018D22C D_8018D238 D_8018D248 D_8018D2A8 D_8018D2E4 "
      "D_8018D2EC D_8018D2F4 D_8018D2FC D_8018D31C D_8018D328 D_8018D390 D_8018D3C8 D_8018D404 D_8018D408 "
      "D_8018D418 D_8018D47C gCloudTexture gMinimapPlayerScale gMinimapPlayerX gMinimapPlayerY gMinimapX "
      "gMinimapY" },
    { "src/code_80057C60_var.c", "WRITTEN-init-only", "D_8018D1EC" },
    { "src/course_metadata.c", "NEVER-WRITTEN", "CurveTargetSpeed D_0D0096B8 NormalTargetSpeed OffTrackTargetSpeed" },
    { "src/crash_screen.c", "NEVER-WRITTEN",
      "D_80162790 D_80162D40 D_80162D58 gDebugThreadStack sButtonSequenceIndex sCounter" },
    { "src/crash_screen.c", "WRITTEN-init-only", "pFramebuffer" },
    { "src/data/kart_attributes.c", "NEVER-WRITTEN",
      "D_800E2400 D_800E24B4 D_800E2568 D_800E2670 D_800E2A90 D_800E2AB0 D_800E2E90 D_800E2EB0 D_800E3210 "
      "D_800E3410 D_800E3610 D_800E3710 D_800E3730 D_800E3750 D_800E3770 D_800E3790 D_800E37B0 D_800E37D0 "
      "D_800E37F0 gKartAccelerationTables gKartBoundingBoxSizeTable gKartFrictionTable gKartHandlingTable "
      "gKartHopJerkTable gKartTable800E3650 gKartTopSpeedTable gKartTripleABoost gKartTurnSpeedReductionTable0 "
      "gKartTurnSpeedReductionTable1 gTopSpeedTable" },
    { "src/data/path_spawn_metadata.c", "NEVER-WRITTEN", "D_800DDB24 gAIDistances nullPath some_data" },
    { "src/data/path_spawn_metadata.c", "WRITTEN-init-only", "D_800DDB20" },
    { "src/data/some_data.c", "NEVER-WRITTEN",
      "D_800E51D0 D_800E5210 D_800E5520 D_800E55A0 D_800E55B0 D_800E55D0 D_800E55F8 D_800E5618 D_800E5628 "
      "D_800E5D78 D_800E5D9C D_800E5DB0 D_800E5DB4 D_800E5FD0 D_800E634C D_800E6724 D_800E672C D_800E6734 "
      "D_800E68E0 D_800E6F30 D_800E6F48 gCourseOutlineTextures gCrabSpawns gFireBreathsSpawns gFlagpoleSpawns "
      "gHedgehogPatrolPoints gHedgehogSpawns gMoleSpawns gSnowmanSpawns gThomwpSpawns150CC gThomwpSpawns50CC "
      "gThwompSpawns100CCExtra gTorchSpawns minimapDimensions" },
    { "src/data/some_data.c", "WRITTEN-init-only", "D_800E5D54" },
    { "src/data/textures.c", "ESCAPES-only",
      "D_02000000 D_02000028 D_02000050 D_02000078 D_020000A0 D_020000C8 D_020000F0 D_02000118 D_02000140 "
      "D_02000168 D_02000190 D_020001B8 D_020001E0 D_02000208 D_02000230 D_02000258 D_02000280 D_020002A8 "
      "D_020002D0 D_020002F8 D_02000320 D_02000348 D_02000370 D_02000398 D_020003C0 D_020003E8 D_02000410 "
      "D_02000438 D_02000460 D_02000488 D_020004B0 D_020004D8 D_02000500 D_02000528 D_02000550 D_02000578 "
      "D_020005A0 D_020005C8 D_020005F0 D_02000618 D_02000640 D_02000668 D_02000690 D_020006B8 D_020006E0 "
      "D_02000708 D_02000730 D_02000758 D_02000780 D_020007A8 D_020007D0 D_020007F8 D_02000820 D_02000848 "
      "D_02000870 D_02000898 D_020008C0 D_020008E8 D_02000910 D_02000938 D_02000960 D_02000988 D_020009B0 "
      "D_020009D8 D_02000A00 D_02000A28 D_02000A50 D_02000A78 D_02000AA0 D_02000AC8 D_02000AF0 D_02000B18 "
      "D_02000B40 D_02000B68 D_02000B90 D_02000BB8 D_02000BE0 D_02000C08 D_02000C30 D_02000C58 D_02000C80 "
      "D_02000CA8 D_02000CD0 D_02000CF8 D_02000D20 D_02000D48 D_02000D70 D_02000D98 D_02000DC0 D_02000DE8 "
      "D_02000E10 D_02000E38 D_02000E60 D_02000E88 D_02000EB0 D_02000ED8 D_02000F00 D_02000F28 D_02000F50 "
      "D_02000F78 D_02000FA0 D_02000FC8 D_02000FF0 D_02001018 D_02001040 D_02001068 D_02001090 D_020010B8 "
      "D_020010E0 D_02001108 D_02001130 D_02001158 D_02001180 D_020011A8 D_020011D0 D_020011F8 D_02001220 "
      "D_02001248 D_02001270 D_02001298 D_020012C0 D_020012E8 D_02001310 D_02001338 D_02001360 D_02001388 "
      "D_020013B0 D_020013D8 D_02001400 D_02001428 D_02001450 D_02001478 D_020014A0 D_020014C8 D_02001540 "
      "D_0200157C D_020015A4 D_020015CC D_02001630 D_02001658 D_020016BC D_020016E4 D_0200170C D_02001734 "
      "D_0200175C D_02001784 D_020017AC D_020017D4 D_020017FC D_02001824 D_0200184C D_02001874 D_0200189C "
      "D_020018C4 D_020018EC D_02001914 D_0200193C D_02001964 D_0200198C D_0200199C D_020019AC D_020019BC "
      "D_020019CC D_020019DC D_020019EC D_02001A14 D_02001A3C D_02001A64 D_02001A8C D_02001AB4 D_02001ADC "
      "D_02001B04 D_02001B54 D_02001B7C D_02001BA4 D_02001BCC D_02001BF4 D_02001C1C D_02001C44 D_02001C6C "
      "D_02001C94 D_02001CBC D_02001CE4 D_02001D0C D_02001D34 D_02001D5C D_02001D84 D_02001DAC D_02001DD4 "
      "D_02001DFC D_02001E24 D_02001E64 D_02001E74 D_02001E84 D_02001E94 D_02001EA4 D_02001EB4 D_02001EC4 "
      "D_02001ED4 D_02001EE4 D_02001EF4 D_02001F04 D_02001F14 D_02001F24 D_02001F34 D_02001F44 D_02001F54 "
      "D_02001F64 D_02001F74 D_02001F84 D_02001F94 D_02001FA4 D_02001FCC D_02001FF4 D_0200201C D_02002044 "
      "D_0200206C D_02002094 D_020020BC D_020020CC D_020020DC D_020020EC D_020020FC D_0200210C D_02002824 "
      "D_0200284C D_02002874 D_0200289C D_020028C4 D_020028EC D_02002914 D_0200293C D_02002964 D_0200298C "
      "D_020029B4 D_020029DC D_02002A04 D_02002A2C D_02002A54 D_02002A7C D_02002AA4 D_02002ACC D_02002AF4 "
      "D_02002B1C D_02002B44 D_02002B6C D_02002B94 D_02002BBC D_02002BE4 D_02002C0C D_02002C34 D_02002C5C "
      "D_02002C84 D_02002CAC D_02002CD4 D_02002CFC D_02002D24 D_02002D4C D_02002D74 D_02002D9C D_02002DC4 "
      "D_02002DEC D_02002E14 D_02002E3C D_02002E64 D_02002E8C D_02002EB4 D_02002EDC D_02002F04 D_02002F2C "
      "D_02002F54 D_02002F7C D_02002FA4 D_02002FCC D_02002FF4 D_0200301C D_02003044 D_0200306C D_02003094 "
      "D_020030BC D_020030E4 D_0200310C D_02003134 D_0200315C D_02003184 D_020031AC D_020031D4 D_020031FC "
      "D_02003224 D_0200324C D_02003274 D_0200329C D_020032C4 D_020032EC D_02003314 D_0200333C D_02003364 "
      "D_0200338C D_020033B4 D_020033DC D_02003404 D_0200342C D_02003454 D_0200347C D_020034A4 D_020034CC "
      "D_020034F4 D_0200351C D_02003544 D_0200356C D_02003594 D_020035BC D_020035E4 D_0200360C D_02003634 "
      "D_0200365C D_02003684 D_020036AC D_020036D4 D_020036FC D_02003724 D_0200374C D_02003774 D_0200379C "
      "D_020037C4 D_020037EC D_02003814 D_0200383C D_02003864 D_0200388C D_020038B4 D_020038DC D_02003904 "
      "D_0200392C D_02003954 D_0200397C D_020039A4 D_020039CC D_020039F4 D_02003A1C D_02003A44 D_02003A6C "
      "D_02003A94 D_02003ABC D_02003AE4 D_02003B0C D_02003B34 D_02003B5C D_02003B84 D_02003BAC D_02003BD4 "
      "D_02003BFC D_02003C24 D_02003C4C D_02003C74 D_02003C9C D_02003CC4 D_02003CEC D_02003D14 D_02003D3C "
      "D_02003D64 D_02003D8C D_02003DB4 D_02003DDC D_02003E04 D_02003E2C D_02003E54 D_02003E7C D_02003EA4 "
      "D_02003ECC D_02003EF4 D_02003F1C D_02003F44 D_02003F6C D_02003F94 D_02003FBC D_02003FE4 D_0200400C "
      "D_02004034 D_0200405C D_02004084 D_020040AC D_020040D4 D_020040FC D_02004124 D_0200414C D_02004174 "
      "D_0200419C D_020041C4 D_020041EC D_02004214 D_0200423C D_02004264 D_0200428C D_020042B4 D_020042DC "
      "D_02004304 D_0200432C D_02004354 D_0200437C D_020043A4 D_020043CC D_020043F4 D_0200441C D_02004444 "
      "D_0200446C D_02004494 D_020044BC D_020044E4 D_0200450C D_0200455C D_02004638 D_020047DC D_02004804 "
      "D_0200482C D_02004854 D_0200487C D_020049BC D_020049E4 D_02004A0C D_02004B4C D_02004B74 D_02004EF8 "
      "D_02004F20 D_02004F48 D_02004F70 D_02004F98 D_02004FC0 D_02004FE8 D_02005010 D_02005038 D_02005060 "
      "D_02005088 D_020050B0 D_020050D8 D_02005100 D_02005128 D_02005150 D_02005178 D_020051A0 D_020051C8 "
      "D_020051F0 D_02005218 D_02005240 D_02005268 D_02005290 D_020052B8 D_020052E0 D_02005308 D_02005330 "
      "D_02005358 D_02005380 D_020053A8 D_020053D0 D_020053F8 D_02005420 D_02005448 D_02005470 D_02005498 "
      "D_020054C0 D_020054E8 D_02005510 D_02005538 D_02005560 D_02005588 D_020055B0 D_020055D8 D_02005600 "
      "D_02005628 D_02005650 D_02005678 D_020056A0 D_020056C8 D_020056F0 D_02005718 D_02005740 D_02005768 "
      "D_02005790 D_020057B8 D_020057E0 D_02005808 D_02005830 D_02005858 D_02005880 D_020058A8 D_020058D0 "
      "D_020058F8 D_02005920 D_02005948 D_02005970 D_02005998 D_020059C0 D_020059E8 D_02005A10 D_02005A38 "
      "D_02005A60 D_02005A88 D_02005AB0 D_02005AD8 D_02005B00 D_02005B28 D_02005B50 D_02005B78 D_02005BA0 "
      "D_02005BC8 D_02005BF0 D_02005C18 D_02005C40 D_02005C68 D_02005C90 D_02005CB8 D_02005CE0 D_02005D08 "
      "D_02005D30 D_02005D58 D_02005D80 D_02005DA8 D_02005DD0 D_02005DF8 D_02005E20 D_02005E48 D_02005E70 "
      "D_02005E98 D_02005EC0 D_02005EE8 D_02005F10 D_02005F38 D_02005F60 D_02005F88 D_02005FB0 D_02005FD8 "
      "D_02006000 D_02006028 D_02006050 D_02006078 D_020060A0 D_020060C8 D_020060F0 D_02006118 D_02006140 "
      "D_02006168 D_02006190 D_020061B8 D_020061E0 D_02006208 D_02006230 D_02006258 D_02006280 D_020062A8 "
      "D_020062D0 D_020062F8 D_02006320 D_02006348 D_02006370 D_02006398 D_020063C0 D_020063E8 D_02006410 "
      "D_02006438 D_02006460 D_02006488 D_020064B0 D_020064D8 D_02006500 D_02006528 D_02006550 D_02006578 "
      "D_020065A0 D_020065C8 D_020065F0 D_02006618 D_02006640 D_02006668 D_02006690 D_020066B8 D_020066E0 "
      "D_02006708 D_02006718 D_02006778 D_02006788 D_020067E0 D_02006838 D_020068E0 D_020068F0 D_02006900 "
      "D_02006960 D_02006970 D_020069C8 D_02006A20 D_02006AC8 D_02006AD8 D_02006AE8 D_02006B48 D_02006B58 "
      "D_02006BB0 D_02006C08 D_02006CB0 D_02006CC0 D_02006CD0 D_02006D30 D_02006D40 D_02006D98 D_02006DF0 "
      "D_02006E98 D_02006EA8 D_02006EB8 D_02006F18 D_02006F28 D_02006F80 D_02006FD8 D_02007080 D_02007090 "
      "D_020070A0 D_02007100 D_02007110 D_02007170 D_020071C8 D_02007270 D_02007280 D_02007290 D_020072F0 "
      "D_02007300 D_02007358 D_020073B0 D_02007458 D_02007468 D_02007478 D_020074D8 D_020074E8 D_02007540 "
      "D_02007598 D_02007640 gTextureFontCC gTextureFontComma gTextureFontDot gTextureFontDoubleQuote "
      "gTextureFontEmpty gTextureFontExclamationMark gTextureFontFourDote gTextureFontInterogationMark "
      "gTextureFontMinus gTextureFontNumberEight gTextureFontNumberFive gTextureFontNumberFour "
      "gTextureFontNumberNine gTextureFontNumberOne gTextureFontNumberSeven gTextureFontNumberSix "
      "gTextureFontNumberThree gTextureFontNumberTwo gTextureFontNumberZero gTextureFontPlus "
      "gTextureFontSimpleQuote seg2_100_CC_texture seg2_150_CC_texture seg2_50_CC_texture "
      "seg2_P1_border_texture seg2_P2_border_texture seg2_P3_border_texture seg2_P4_border_texture "
      "seg2_battle_title_texture seg2_blue_sky_background_texture seg2_choco_mountain_title_texture "
      "seg2_copyright_1996_texture seg2_data_texture seg2_extra_CC_texture seg2_flower_cup_texture "
      "seg2_flower_cup_title_texture seg2_game_select_texture seg2_mario_raceway_preview_small_texture "
      "seg2_mario_raceway_preview_texture seg2_mario_raceway_title_texture seg2_menu_1p_column "
      "seg2_menu_2p_column seg2_menu_3p_column seg2_menu_4p_column seg2_menu_no_item_texture "
      "seg2_menu_select_texture seg2_mushroom_cup_texture seg2_mushroom_cup_title_texture "
      "seg2_push_start_button_texture seg2_special_cup_texture seg2_special_cup_title_texture "
      "seg2_star_cup_texture seg2_star_cup_title_texture seg2_sunset_background_texture seg2_textureFontLetterA "
      "seg2_textureFontLetterB seg2_textureFontLetterC seg2_textureFontLetterD seg2_textureFontLetterE "
      "seg2_textureFontLetterF seg2_textureFontLetterG seg2_textureFontLetterH seg2_textureFontLetterI "
      "seg2_textureFontLetterJ seg2_textureFontLetterK seg2_textureFontLetterL seg2_textureFontLetterM "
      "seg2_textureFontLetterN seg2_textureFontLetterO seg2_textureFontLetterP seg2_textureFontLetterQ "
      "seg2_textureFontLetterR seg2_textureFontLetterS seg2_textureFontLetterT seg2_textureFontLetterU "
      "seg2_textureFontLetterV seg2_textureFontLetterW seg2_textureFontLetterX seg2_textureFontLetterY "
      "seg2_textureFontLetterZ seg2_textureMenuLOption seg2_textureMenuRData" },
    { "src/data/textures.c", "NEVER-WRITTEN", "D_02001E4C" },
    { "src/ending/ceremony_and_credits.c", "NEVER-WRITTEN",
      "D_802856C8 D_80285D58 D_80286A04 D_80286B34 D_80286B5C D_802876DC gCutScene" },
    { "src/ending/code_80281780.c", "NEVER-WRITTEN", "defaultCharacterIds" },
    { "src/ending/code_80281C40.c", "NEVER-WRITTEN", "D_80281C40_pad" },
    { "src/ending/code_80281C40.c", "WRITTEN-init-only", "D_80287560" },
    { "src/ending/credits.c", "NEVER-WRITTEN", "gCreditsText gTextCreditsRenderInfo" },
    { "src/ending/podium_ceremony_actors.c", "NEVER-WRITTEN", "fireworkColour fireworkConeColour sUpdate" },
    { "src/harbour_masters.c", "NEVER-WRITTEN", "ok64_Plane_001_mesh ok64_ok64_rgba16_aligner" },
    { "src/kart_dma.c", "NEVER-WRITTEN",
      "D_800DDEB0 gKartPalettes gKartTextureTable0 gKartTextureTable1 gKartTextureTumbles" },
    { "src/main.c", "NEVER-WRITTEN",
      "D_800DC528 D_800DC534 D_800DC550 D_800DC554 D_800DC558 D_800DC564 D_800DC590 D_800FD850 D_8014F010 "
      "D_8014F014 D_80150128 D_8015012C D_80150140 D_80150144 D_80150154 D_801502A0 D_801502A4 D_801502B8 "
      "D_801502BC D_8015680_Stack D_80156820 D_8015CD30 D_8015CD30_Stack gControllerEight gControllerFive "
      "gControllerFour gControllerOne gControllerSeven gControllerSix gControllerThree gControllerTwo "
      "gGameVblankHandler gGfxSPTaskStack gGfxSPTaskYieldBuffer gPlayerEight gPlayerFive gPlayerFour gPlayerOne "
      "gPlayerSeven gPlayerSix gPlayerThree gPlayerTwo gTickGame gTickVisuals gfxPool_padding padding "
      "paddingThread unused_gMsgQueue" },
    { "src/main.c", "WRITTEN-init-only", "gD_80150158 gDmaIoMesg gIdleThread gIdleThreadStack gVblankHandler2" },
    { "src/menu_items.c", "ABSENT",
      "pause_menu_item_box_cursor.x1 pause_menu_item_box_cursor.y1 pause_menu_item_box_cursor.z1" },
    { "src/menu_items.c", "NEVER-WRITTEN",
      "D_800E70A0 D_800E70E8 D_800E7108 D_800E7148 D_800E7168 D_800E7188 D_800E7208 D_800E7248 D_800E7258 "
      "D_800E7268 D_800E7278 D_800E72F8 D_800E7300 D_800E7380 D_800E7390 D_800E73C0 D_800E73D0 D_800E73E0 "
      "D_800E7410 D_800E7420 D_800E7430 D_800E7458 D_800E7480 D_800E74A8 D_800E74D0 D_800E76A8 D_800E76CC "
      "D_800E76DC D_800E7744 D_800E7778 D_800E7840 D_800E7860 D_800E7878 D_800E7884 D_800E7890 D_800E78BC "
      "D_800E78D0 D_800E7900 D_800E7918 D_800E7920 D_800E7928 D_800E7930 D_800E7938 D_800E7940 D_800E7980 "
      "D_800E798C D_800E7A34 D_800E7A3C D_800E7A44 D_800E7A48 D_800E7A54 D_800E7A60 D_800E7A6C D_800E7A74 "
      "D_800E7A80 D_800E7A88 D_800E7A98 D_800E7A9C D_800E7AC8 D_800E7AE8 D_800E7AF8 D_800E7D0C D_800E7D34 "
      "D_800E7D54 D_800E7D74 D_800E7DC4 D_800E7E14 D_800E7E34 D_800E8234 D_800E8254 D_800E8294 D_800E82A4 "
      "D_800E82AC D_800E82F4 D_800E8320 D_800E8340 D_800E8360 D_800E83A0 D_800E8440 D_800E8460 D_800E8480 "
      "D_800E84A0 D_800E84C0 D_800E8538 D_800E8540 D_800E85C0 D_800E8600 D_8018E83C D_8018E864_pad "
      "gBackgroundColor gBestTimeText gCharacterCelebrateAnimation gCharacterDefeatAnimation "
      "gCharacterDeselectAnimation gCharacterDoubleBlinkAnimation gCharacterSingleBlinkAnimation gCupNames "
      "gCupText gDebugCCModeNames gDebugCharacterNames gDebugScreenModeNames gDebugSoundModeNames "
      "gEraseBestGhostText gGlyphTextureLUT gLapTimeText gMenuTexturesBackground gMenuTexturesBorderPlayer "
      "gMenuTexturesTrackSelection gPlaceText gPrefixTimeText gSoundModeNames gTextBattleIntroduction "
      "gTextMenuOption gTextNoController gTextOptionMenu gTextPauseButton gWinLoseText" },
    { "src/menus.c", "NEVER-WRITTEN", "sVIGammaOffDitherOn unref_D_800E86D4 unref_D_800E86E0" },
    { "src/os/guRotateF.c", "ABSENT", "guRotateF.D_80365D70" },
    { "src/player_controller.c", "NEVER-WRITTEN", "cpu_forPlayer cpu_forTwoPlayer" },
    { "src/port/interpolation/matrix.c", "NEVER-WRITTEN",
      "gCalcMatrix gGfxMatrix gIdentityMtx gMainMatrixStack sCalcMatrixStack sGfxMatrixStack" },
    { "src/port/interpolation/matrix.c", "WRITTEN-init-only", "gGfxMtx" },
    { "src/racing/actors.c", "NEVER-WRITTEN",
      "D_802B8808 D_802B8810 gTextureBlueshell texture_green_shell texture_red_shell" },
    { "src/racing/math_util.c", "NEVER-WRITTEN", "D_802B91C0" },
    { "src/racing/memory.c", "NEVER-WRITTEN",
      "D_802B8CD4 D_802B8CE4 dlSegEnd memoryPadding sGfxSeekPosition sPackedSeekPosition testaaa texSegEnd "
      "texSegSize vtxSegEnd" },
    { "src/racing/memory.c", "WRITTEN-init-only", "gMainPoolState sPoolFreeSpace sPoolListHeadL sPoolListHeadR" },
    { "src/racing/race_logic.c", "NEVER-WRITTEN", "D_802BA03C" },
    { "src/racing/render_courses.c", "NEVER-WRITTEN", "D_802B87B8 D_802B87C0" },
    { "src/racing/skybox_and_splitscreen.c", "NEVER-WRITTEN",
      "D_802B8890 D_802B8910 D_802B8990 D_802B8A10 D_802B8A90" },
    { "src/racing/skybox_and_splitscreen.c", "WRITTEN-init-only", "gIdentityMatrix2" },
    { "src/render_objects.c", "NEVER-WRITTEN",
      "D_800E46B0 D_800E46C8 D_800E46E0 D_800E471CA common_texture_minimap_progress" },
    { "src/render_player.c", "NEVER-WRITTEN",
      "D_800DDB5C D_800DDE74 D_80164AF4 gCharacterSize gPaletteWheel0Offset gPaletteWheel1Offset gPlayerVtx "
      "gPlayerWhiteEffect gRenderingFramebufferByPlayer junk wheelPtr wheel_offset" },
    { "src/replays.c", "NEVER-WRITTEN", "gReplayGhostCompressed sReplayGhostEncoded" },
    { "src/save.c", "NEVER-WRITTEN", "gCompanyCode gGameCode" },
    { "src/spawn_players.c", "NEVER-WRITTEN", "D_800E43A0 D_800E43A4 D_80165250 D_80165348" },
    { "src/spawn_players.c", "WRITTEN-init-only", "D_800E43A8 D_801653C0" },
    { "src/stubs.c", "NEVER-WRITTEN", "fileIndex openFile osResetType osTvType" },
    { "src/update_objects.c", "NEVER-WRITTEN",
      "D_800E43B0 D_800E43F0 devaddr2 gHudLapTextures gItemWindowTLUTs gItemWindowTextures gPortraitTLUTs "
      "gPortraitTextures" },
};

// File-scope macro statements whose definitions only the compiler sees (token pasting, `var##Group0`): the 1.4b rule
// is a named row with its reason, never a wider parser. Each row pins its invocation count, so a new invocation fails
// the critic; the IR sweep is the method that names what they define.
struct CGlobalMacroRow {
    const char* file;
    const char* macro;
    int uses;
    const char* reason;
};

#define ORACLE_KART_DMA_REASON                                                                                    \
    "asset pointer tables (kart_dma.c:24-27, 50-525: arrays of texture pointers per kart frame); the IR at "      \
    "ff029646c defines 2480 of them, 8 karts x (289 frames + 18 groups + Table0 + Table1 + Tumble), all "         \
    "ESCAPES-only, none stored"
inline constexpr CGlobalMacroRow kCGlobalMacros[] = {
    { "src/kart_dma.c", "DECLARE_KART_TEXTURES", 8, ORACLE_KART_DMA_REASON },
    { "src/kart_dma.c", "DECLARE_KART_GROUP", 8, ORACLE_KART_DMA_REASON },
    { "src/kart_dma.c", "DECLARE_KART_TABLE0", 8, ORACLE_KART_DMA_REASON },
    { "src/kart_dma.c", "DECLARE_KART_TABLE1", 8, ORACLE_KART_DMA_REASON },
    { "src/kart_dma.c", "DECLARE_KART_TUMBLE_TABLE", 8, ORACLE_KART_DMA_REASON },
};
#undef ORACLE_KART_DMA_REASON

// FASE 1.4c.1 (e): the stored C++ globals that 1.4b's statics critic (kStaticsHashed / kStaticsExcluded) does not
// name, from the IR sweep (implement/fase-1-artifacts/1.4c/cpp/cpp_uncovered.tsv). GENERATED by gen_cpp_rows.py.
// Source text cannot list C++ globals the way test_cglobal_coverage lists C ones, so oracle_tests only checks that no
// row is stale (test_cpp_global_rows): a NEW C++ global outside src/engine is caught by re-running the IR sweep, not by
// a unit test — a named limit of 1.4c. `symbol` is the IR's demangled name ("<function>::<name> (.N)" for a local).
struct CppGlobalRow {
    const char* file;
    const char* symbol;
    const char* rule; // kCppGlobalsHashed: one of kCGlobalRules; kCppGlobalsExcluded: the reason
};

inline constexpr CppGlobalRow kCppGlobalsHashed[] = {
    // BEGIN GENERATED C++ hashed (1.4c/gen_cpp_rows.py)
    { "src/port/Game.cpp", "gTrophyIndex",
      "scalar" },
    // END GENERATED C++ hashed
};

inline constexpr CppGlobalRow kCppGlobalsExcluded[] = {
    // BEGIN GENERATED C++ excluded (1.4c/gen_cpp_rows.py)
    { "src/engine/RaceManager.cpp", "mirroredVtxCache",
      "address-keyed cache of mirror-mode vertex copies (RaceManager.cpp:19-60): the keys are host addresses; "
      "the copies are the collision mesh's vertices, EXCLUDED as asset in OracleStatePut.h (a named limit of "
      "1.4c)" },
    { "src/engine/editor/Collision.cpp", "TrackEditor::gDebugObjVtxCache",
      "track editor (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/engine/editor/EditorMath.cpp", "EditorMatrix",
      "track editor (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/engine/objects/Thwomp.cpp", "D_800E4638",
      "render-only: a Lights1 written from D_80165840 and read only by gSPLight (Thwomp.cpp:782-792)" },
    { "src/engine/sky/Sky.cpp", "Sky::mSkyboxScreenOne",
      "render-only: skybox vertices, written and read only by Sky::Draw (Sky.cpp:172, 250)" },
    { "src/enhancements/freecam/freecam.cpp", "FreecamKeyDown(int)::prevKeyState",
      "freecam (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/enhancements/freecam/freecam.cpp", "bFreecamUseController",
      "freecam (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/enhancements/freecam/freecam.cpp", "fController",
      "freecam (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/enhancements/freecam/freecam.cpp", "fRankIndex",
      "freecam (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/enhancements/freecam/freecam.cpp", "fTargetPlayer",
      "freecam (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/port/Engine.cpp", "GameEngine::ProcessGfxCommands(Gfx*)::last_fps",
      "host: the renderer's frame timing / alt-asset toggle (GameEngine::ProcessGfxCommands, "
      "Engine.cpp:422-423)" },
    { "src/port/Engine.cpp", "GameEngine::ProcessGfxCommands(Gfx*)::last_update_rate",
      "host: the renderer's frame timing / alt-asset toggle (GameEngine::ProcessGfxCommands, "
      "Engine.cpp:422-423)" },
    { "src/port/Engine.cpp", "GameEngine::ProcessGfxCommands(Gfx*)::time",
      "host: the renderer's frame timing / alt-asset toggle (GameEngine::ProcessGfxCommands, "
      "Engine.cpp:422-423)" },
    { "src/port/Engine.cpp", "audio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/Engine.cpp", "prevAltAssets",
      "host: the renderer's frame timing / alt-asset toggle (GameEngine::ProcessGfxCommands, "
      "Engine.cpp:422-423)" },
    { "src/port/Game.cpp", "bCleanWorld",
      "track editor (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2); set "
      "only from the editor's Tools window (ui/Tools.cpp:236), consumed at Game.cpp:815-817" },
    { "src/port/Game.cpp", "gActorRegistry",
      "registry filled at static init (_GLOBAL__sub_I_Game.cpp) and by mods; the mod list is pinned by the "
      "header's modListHash" },
    { "src/port/Game.cpp", "gEditor",
      "track editor (UI): refused by the session guards (Oracle_ApplySessionGuards, main.c:1173; FASE 1.2)" },
    { "src/port/Game.cpp", "gItemRegistry",
      "registry filled at static init (_GLOBAL__sub_I_Game.cpp) and by mods; the mod list is pinned by the "
      "header's modListHash" },
    { "src/port/Game.cpp", "gItemTableRegistry",
      "registry filled at static init (_GLOBAL__sub_I_Game.cpp) and by mods; the mod list is pinned by the "
      "header's modListHash" },
    { "src/port/Game.cpp", "gTrackRegistry",
      "registry filled at static init (_GLOBAL__sub_I_Game.cpp) and by mods; the mod list is pinned by the "
      "header's modListHash" },
    { "src/port/Game.cpp", "sResetRequested",
      "UI latch: set from the UI thread (CM_RequestReset, Game.cpp:926) and consumed at Game.cpp:935; the reset "
      "it triggers changes hashed state" },
    { "src/port/GameExtractor.cpp", "mGameList",
      "host: a resource importer's table, filled at static init and never written again" },
    { "src/port/audio/HMAS.cpp", "g_DeviceTrackingInitCounter_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_DeviceTrackingInitLock_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_DeviceTrackingMutex_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_TrackedDeviceCap_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_TrackedDeviceCount_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_maLCG (.0)",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "g_ppTrackedDevices_CoreAudio",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/audio/HMAS.cpp", "ma_dr_flac__gIsNEONSupported",
      "audio thread / host audio device: the miniaudio backend (src/port/audio/HMAS.cpp) and "
      "GameEngine::HandleAudioThread (Engine.cpp); 1.4's straddle probe covers the audio thread's writes into "
      "hashed state" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::camera_epoch",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::current_path",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::current_recording",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::frame_had_camera_cut",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::is_recording",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::previous_camera_epoch",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "(anonymous namespace)::previous_recording",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/interpolation/FrameInterpolation.cpp", "camera_interpolation",
      "render-only: frame interpolation's recording of draw transforms "
      "(src/port/interpolation/FrameInterpolation.cpp), read only to draw interpolated frames" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sCursor",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sHashMicrosMax",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sHashMicrosTotal",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sHeader",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sIteration",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sLive",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sOpened",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sReplayEnded",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sReplayLoaded",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/Oracle.cpp", "(anonymous namespace)::sStraddle",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/oracle/OracleStateHash.cpp", "(anonymous namespace)::sLastCost",
      "oracle bookkeeping: the oracle's own state (FASE 1.4c.3 gotcha 3: hashing it makes the hash depend on "
      "the hasher)" },
    { "src/port/resource/importers/AudioSequenceFactory.cpp", "extension",
      "host: a resource importer's table, filled at static init and never written again" },
    { "src/port/resource/importers/BetterTextureFactory.cpp", "MK64::extension",
      "host: a resource importer's table, filled at static init and never written again" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::ContentBrowserWindow::TrainWindow()::numCarriages",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::ContentBrowserWindow::TrainWindow()::pathIndex",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::ContentBrowserWindow::TrainWindow()::pathPoint",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::ContentBrowserWindow::TrainWindow()::spawnMode",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::ContentBrowserWindow::TrainWindow()::tender",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "TrackEditor::bIsTrainWindowOpen",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ContentBrowser.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::Freecam::controlType",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "MenuInit::GetInitFuncs()::menuInitFuncs",
      "port UI (structural): an inline function's local (linkonce_odr) (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/FreecamWindow.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ImguiUI.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "MenuInit::GetUpdateFuncs()::menuUpdateFuncs",
      "port UI (structural): an inline function's local (linkonce_odr) (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "Ship::disabledTooltip",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "Ship::disabledValue",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Menu.cpp", "windowTypeSizes",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::PortMenu::AddSettings()::isFullscreen",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::PortMenu::AddSettings()::maxFps",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/PortMenu.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Properties.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::aspectRatioPresetLabels",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::aspectRatioX",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::aspectRatioY",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::item_aspectRatio",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::item_pixelCount",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::update",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "GameUI::verticalPixelCount",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/ResolutionEditor.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/SceneExplorer.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "TrackEditor::ToolsWindow::DrawElement()::selectedTool",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "TrackEditor::ToolsWindow::DrawElement()::toggleBoundary",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "TrackEditor::ToolsWindow::DrawElement()::toggleGroundSnap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/Tools.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::introBehaviourOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::logLevels",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::menuExtentOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::menuThemeOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::motionBlurOptions",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "GameUI::textureFilteringMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "TrackEditor::TrackPropertiesWindow::DrawResourceNameEdit()::initialized",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "TrackEditor::TrackPropertiesWindow::DrawResourceNameEdit()::oldResourceName",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "TrackEditor::TrackPropertiesWindow::DrawResourceNameEdit()::resourceNameBuffer",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "TrackEditor::TrackPropertiesWindow::SelectedKeyframe",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "TrackEditor::TrackPropertiesWindow::SelectedShot",
      "port UI (structural): no reference outside src/port/ui (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "audioBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/TrackProperties.cpp", "windowBackendsMap",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    { "src/port/ui/UIWidgets.cpp", "UIWidgets::ColorValues",
      "port UI (structural): internal linkage (cpp/ui_evidence.tsv)" },
    // END GENERATED C++ excluded
};

// Decision 3's liveness gates: the ONE place that says when a pool owner's contents are hashed. A content owner is
// LIVE only in the gamestates whose setup allocates it after that setup's own pool reset; update_gamestate runs the
// setup in the iteration that enters the state, before the hash (main.c:1175-1178). In any other state the owner still
// names a pool address, but the bytes there belong to whatever the pool holds now: the menus reset the cursor
// (menu_items.c:1407-1409) and build a display list holding host addresses in the reused bytes (:4477, :5960). So
// there only the owner's offset is hashed. `states` is a mask of defines.h gamestates (OracleCGlobalsHash.cpp asserts
// the bits against the macros); a content owner with no row is Oracle_Abort, and oracle_tests' test_pool_gates fails on
// an owner without a row or a row without an owner.
inline constexpr unsigned kGateMenus = 1u << 0 | 1u << 1 | 1u << 2 | 1u << 3; // the four *_MENU_FROM_QUIT states
inline constexpr unsigned kGateRacing = 1u << 4;
inline constexpr unsigned kGateEnding = 1u << 5;
inline constexpr unsigned kGateCredits = 1u << 9;

struct PoolGate {
    const char* owner;
    unsigned states;
    const char* setup; // the allocation that makes it live in each of those states
};

#define ORACLE_PATH_SETUP "init_course_path_point: spawn_players.c:881 (RACING), code_80281780.c:165 (ENDING)"
#define ORACLE_RESULTS_SETUP                                                                                           \
    "func_80091FA4: code_800029B0.c:240, :317 (RACING); func_80093E60: code_80281780.c:168 (ENDING), "             \
    "code_80280000.c:204 (CREDITS)"
inline constexpr PoolGate kPoolGates[] = {
    { "gCollisionIndices", kGateRacing | kGateEnding,
      "func_80295C6C (render_courses.c:266): every Track::Load (RACING), code_80281780.c:155 (ENDING)" },
    { "gCollisionMesh", kGateRacing | kGateEnding | kGateCredits,
      "Track.cpp:455 (RACING), code_80281780.c:135 (ENDING), code_80280000.c:191 (CREDITS)" },
    { "gPathExpectedRotation", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gTrackConsecutiveCurveCounts", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gTrackLeftPaths", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gTrackPaths", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gTrackRightPaths", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gTrackSectionTypes", kGateRacing | kGateEnding, ORACLE_PATH_SETUP },
    { "gVehicle2DPathPoint", kGateRacing,
      "init_course_path_point (spawn_players.c:881), filled only by generate_train_path / generate_ferry_path "
      "(code_80005FD0.c:4335-4366, called from KalimariDesert.cpp:175, DKJungle.cpp:175)" },
    { "sPodiumActorList", kGateEnding, "balloons_and_fireworks_init: code_80281780.c:166" },
    { "sGPPointsCopy", kGateRacing | kGateEnding | kGateCredits, ORACLE_RESULTS_SETUP },
    { "sTKMK00_LowResBuffer", kGateMenus | kGateRacing | kGateEnding | kGateCredits,
      "func_80091B78: main.c:1066-1084 (the four menus); " ORACLE_RESULTS_SETUP },
};
#undef ORACLE_PATH_SETUP
#undef ORACLE_RESULTS_SETUP

} // namespace oracle
