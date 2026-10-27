// The `cglobals` group (FASE 1.4c): every kCGlobalsHashed row of OracleCGlobals.h that no older group hashes, in the
// table's order (file, then symbol), each under its symbol name. GENERATED with the table from one decision list by
// docs implement/fase-1-artifacts/1.4c/gen_hash_tu.py; oracle_tests' test_cglobal_walk fails when the two disagree.
// Every declaration comes from the game's own headers, each one included by its defining file, so the C compiler has
// checked it against the definition — none is re-declared here.
#include <libultraship.h>

#include "OracleStatePut.h"
#include "OracleStateHash.h"
#include "OracleState.h" // Oracle_Abort

#include <string>

// C++-aware game headers first, with C++ linkage (render_courses.h reaches engine/tracks/Track.h, whose __cplusplus
// half holds templates), as OracleStateHash.cpp does; their include guards then keep them out of the block below.
#include "port/Game.h"

extern "C" {
#include "defines.h" // NUM_PLAYERS & co.: several headers below use them without including it
#include "include/kart_attributes.h"
#include "include/objects.h"
#include "include/save_data.h"
#include "include/stubs.h"
#include "include/waypoints.h"
#include "src/animation.h"
#include "src/buffers.h"
#include "src/camera.h"
#include "src/code_800029B0.h"
#include "src/code_80005FD0.h"
#include "src/code_80057C60.h"
#include "src/code_800AF9B0.h"
#include "src/data/path_spawn_metadata.h"
#include "src/effects.h"
#include "src/ending/ceremony_and_credits.h"
#include "src/ending/code_80280000.h"
#include "src/ending/code_80281C40.h"
#include "src/ending/podium_ceremony_actors.h"
#include "src/main.h"
#include "src/menu_items.h"
#include "src/menus.h"
#include "src/profiler.h"
#include "src/racing/actors.h"
#include "src/racing/collision.h"
#include "src/racing/framebuffer_effects.h"
#include "src/racing/memory.h"
#include "src/racing/race_logic.h"
#include "src/racing/render_courses.h"
#include "src/render_objects.h"
#include "src/render_player.h"
#include "src/replays.h"
#include "src/save.h"
#include "src/spawn_players.h"
#include "src/update_objects.h"
}

#define ORACLE_CG(g) put(h, #g, g);

#include "OracleCGlobals.h"    // kPoolGates
#include "OracleCGlobalsPut.h" // FASE 1.4c: the field lists of the structs below

#include <cstring>

namespace oracle {

namespace {

// An array of Player* (1.4b's player-ptr rule, element by element), under the array's name like PutN.
template <size_t N> void PutPlayerPtrs(Hasher& h, const char* n, Player* const (&a)[N]) {
    Hasher::Scope s(h, n);
    for (size_t i = 0; i < N; i++) {
        Hasher::Scope e(h, i);
        PutPlayerPtr(h, "", a[i], n);
    }
}

// array-index: a pointer into one fixed global array, hashed as its BYTE offset from the array's first byte, -1 for
// null; anywhere else is Oracle_Abort (1.4b's pointer rule: what it names, never its address).
template <typename A> void PutOffsetPtr(Hasher& h, const char* n, const void* p, const A& base, const char* what) {
    int64_t off = -1;
    if (p != nullptr) {
        const uintptr_t b = reinterpret_cast<uintptr_t>(&base);
        const uintptr_t q = reinterpret_cast<uintptr_t>(p);
        if (q < b || q >= b + sizeof(A)) {
            Oracle_Abort((std::string("state hash: ") + what + " points outside its array").c_str());
        }
        off = (int64_t) (q - b);
    }
    put(h, n, off);
}

// track-ptr (FINDING, 1.4c): a uintptr_t holding the heap address of the C++ Track, which the game only compares with
// the current one (CM_GetTrack, Game.cpp:648): -1 null, 0 the current track, 1 any other (stale).
void PutTrackPtr(Hasher& h, const char* n, uintptr_t v) {
    put(h, n, (int32_t) (v == 0 ? -1 : v == CM_GetTrack() ? 0 : 1));
}

// bytes: an object whose every byte is data (no padding, no pointer), hashed as its bytes.
template <typename T> void PutBytes(Hasher& h, const char* n, const T& v) {
    put(h, n, reinterpret_cast<const uint8_t(&)[sizeof(T)]>(v));
}

// pool-ptr (decision 3): a pointer into racing/memory.c's bump pool, or an address held in a uintptr_t, as its offset
// from the pool's end (sPoolEnd, fixed by initialize_memory_pool), -1 for null; anywhere else is Oracle_Abort. The end
// itself is in range: a cursor that has used up the pool equals it. `bytes` more must fit after q.
int64_t PoolOffset(uintptr_t q, size_t bytes, const char* what) {
    if (q < sPoolEnd - MEMORY_POOL_SIZE || q > sPoolEnd || bytes > sPoolEnd - q) {
        Oracle_Abort((std::string("state hash: ") + what + " points outside the memory pool").c_str());
    }
    return (int64_t) (sPoolEnd - q);
}

void PutPoolPtr(Hasher& h, const char* n, uintptr_t q) {
    put(h, n, q == 0 ? (int64_t) -1 : PoolOffset(q, 0, n));
}

static_assert(kGateMenus == (1u << START_MENU_FROM_QUIT | 1u << MAIN_MENU_FROM_QUIT | 1u << PLAYER_SELECT_MENU_FROM_QUIT |
                            1u << COURSE_SELECT_MENU_FROM_QUIT) &&
                  kGateRacing == 1u << RACING && kGateEnding == 1u << ENDING && kGateCredits == 1u << CREDITS_SEQUENCE,
              "kPoolGates' gamestate bits drifted from defines.h");

// Whether `owner`'s contents are live in the current gamestate: its kPoolGates row (OracleCGlobals.h), the one table.
bool Live(const char* owner) {
    for (const PoolGate& g : kPoolGates) {
        if (std::strcmp(g.owner, owner) == 0) {
            return gGamestate >= 0 && gGamestate < 32 && ((g.states >> gGamestate) & 1u) != 0;
        }
    }
    Oracle_Abort((std::string("state hash: pool owner ") + owner + " has no kPoolGates row").c_str());
    return false;
}

// Decision 3's contents: the `count` elements a live owner names, field-wise, under "*<owner>", after the count.
template <typename T> void PutPoolContents(Hasher& h, const char* n, const T* p, size_t count) {
    const std::string name = std::string("*") + n;
    Hasher::Scope s(h, name.c_str());
    put(h, "count", (uint64_t) count);
    if (p == nullptr) {
        return;
    }
    PoolOffset(reinterpret_cast<uintptr_t>(p), count * sizeof(T), n);
    for (size_t i = 0; i < count; i++) {
        Hasher::Scope e(h, i);
        put(h, "", p[i]);
    }
}

// A pool owner: its offset, then (when live) its contents.
template <typename T> void PutPoolOwner(Hasher& h, const char* n, const T* p, size_t count, bool live) {
    PutPoolPtr(h, n, reinterpret_cast<uintptr_t>(p));
    if (live) {
        PutPoolContents(h, n, p, count);
    }
}

// A content owner whose gate is its kPoolGates row.
template <typename T> void PutPoolOwner(Hasher& h, const char* n, const T* p, size_t count) {
    PutPoolOwner(h, n, p, count, Live(n));
}

// The six per-path arrays (gTrackPaths & co.). init_course_path_point allocates path k with D_80163368[k] elements and
// loads it only when D_80163368[k] >= 2 (code_80005FD0.c:3426-3469); a loaded path holds gPathCountByPathIndex[k]
// points, set by load_track_path. An unloaded path's count is left over from an earlier track, so it holds none.
template <typename T> void PutPathOwners(Hasher& h, const char* n, T* const (&a)[4]) {
    const bool live = Live(n);
    Hasher::Scope s(h, n);
    for (size_t k = 0; k < 4; k++) {
        Hasher::Scope e(h, k);
        const size_t count = D_80163368[k] >= 2 ? gPathCountByPathIndex[k] : 0;
        if (live && count > (size_t) D_80163368[k]) {
            Oracle_Abort("state hash: a path holds more points than init_course_path_point allocated");
        }
        PutPoolOwner(h, "", a[k], count, live);
    }
}

// The vehicle path: init_course_path_point allocates PathSizes.unk8 points (code_80005FD0.c:3431-3433, 1 on every track
// without a train or ferry) and generate_train_path / generate_ferry_path fill gVehiclePathSize of them (:4335-4366).
// The size is only ever set there, so it is live only when it fits the current track's allocation.
void PutVehiclePathOwner(Hasher& h, const char* n, const TrackPathPoint* p) {
    const size_t count = (size_t) gVehiclePathSize;
    const bool live = Live(n) && count > 0 && count <= (size_t) CM_GetProps()->PathSizes.unk8;
    PutPoolOwner(h, n, p, count, live);
}

} // namespace

void HashCGlobals(Hasher& h) {
    // src/animation.c
    ORACLE_CG(isNotTheFirst)
    ORACLE_CG(sMatrixShouldNotPop)
    ORACLE_CG(sMatrixStackSize)
    ORACLE_CG(sOriginalPosAnimation)
    // src/buffers/buffers.c
    PutBytes(h, "D_802BFB80", D_802BFB80);
    // src/camera.c
    ORACLE_CG(D_801649D8)
    ORACLE_CG(D_801649E8)
    ORACLE_CG(D_801649F8)
    ORACLE_CG(D_80164A08)
    ORACLE_CG(D_80164A18)
    ORACLE_CG(D_80164A28)
    ORACLE_CG(D_80164A2C)
    ORACLE_CG(D_80164A30)
    ORACLE_CG(D_80164A38)
    ORACLE_CG(D_80164A48)
    ORACLE_CG(D_80164A78)
    ORACLE_CG(D_80164A88)
    ORACLE_CG(D_80164A89)
    ORACLE_CG(D_80164A90)
    ORACLE_CG(D_80164AA0)
    ORACLE_CG(sStagingTimer)
    // src/code_800029B0.c
    ORACLE_CG(D_800DC5A8)
    ORACLE_CG(D_800DC5AC)
    ORACLE_CG(D_800DC5B0)
    ORACLE_CG(D_800DC5B8)
    ORACLE_CG(D_800DC5C8)
    ORACLE_CG(D_800DC5DC)
    ORACLE_CG(D_800DC5E0)
    ORACLE_CG(D_800DC5E4)
    ORACLE_CG(D_8015F58C)
    ORACLE_CG(D_8015F59C)
    ORACLE_CG(D_8015F5A0)
    ORACLE_CG(D_8015F5A4)
    ORACLE_CG(D_8015F6F4)
    ORACLE_CG(D_8015F6F6)
    ORACLE_CG(D_8015F6FA)
    ORACLE_CG(D_8015F6FC)
    ORACLE_CG(D_8015F700)
    ORACLE_CG(D_8015F702)
    ORACLE_CG(D_8015F704)
    ORACLE_CG(D_8015F708)
    ORACLE_CG(D_8015F738)
    ORACLE_CG(D_8015F748)
    ORACLE_CG(D_8015F758)
    ORACLE_CG(D_8015F768)
    ORACLE_CG(D_8015F778)
    ORACLE_CG(D_8015F890)
    ORACLE_CG(D_8015F892)
    ORACLE_CG(D_8015F894)
    ORACLE_CG(D_8015F8D0)
    ORACLE_CG(D_8015F8DC)
    ORACLE_CG(D_8015F8E0)
    ORACLE_CG(bDrawSkybox)
    ORACLE_CG(bFog)
    PutPoolOwner(h, "gCollisionIndices", gCollisionIndices, gNumCollisionTriangles);
    PutPoolOwner(h, "gCollisionMesh", gCollisionMesh, gCollisionMeshCount);
    ORACLE_CG(gCollisionMeshCount)
    ORACLE_CG(gCreditsCourseId)
    ORACLE_CG(gCurrentCourseId)
    PutTrackPtr(h, "gCurrentlyLoadedTrackAddr", gCurrentlyLoadedTrackAddr);
    ORACLE_CG(gDebugPathCount)
    PutPoolPtr(h, "gFreeMemoryCourseAnchor", gFreeMemoryCourseAnchor);
    PutPoolPtr(h, "gFreeMemoryResetAnchor", gFreeMemoryResetAnchor);
    put(h, "gFreeMemorySize", (uint64_t) gFreeMemorySize);
    ORACLE_CG(gIsMirrorMode)
    PutPoolPtr(h, "gNextFreeMemoryAddress", gNextFreeMemoryAddress);
    PutOffsetPtr(h, "gNmiUnknown1", gNmiUnknown1, osAppNmiBuffer, "gNmiUnknown1");
    PutOffsetPtr(h, "gNmiUnknown2", gNmiUnknown2, osAppNmiBuffer, "gNmiUnknown2");
    PutOffsetPtr(h, "gNmiUnknown3", gNmiUnknown3, osAppNmiBuffer, "gNmiUnknown3");
    PutOffsetPtr(h, "gNmiUnknown4", gNmiUnknown4, osAppNmiBuffer, "gNmiUnknown4");
    PutOffsetPtr(h, "gNmiUnknown5", gNmiUnknown5, osAppNmiBuffer, "gNmiUnknown5");
    PutOffsetPtr(h, "gNmiUnknown6", gNmiUnknown6, osAppNmiBuffer, "gNmiUnknown6");
    ORACLE_CG(gNumCollisionTriangles)
    ORACLE_CG(gNumPermanentActors)
    ORACLE_CG(gNumScreens)
    ORACLE_CG(gNumSpawnedShells)
    ORACLE_CG(gPlaceItemBoxes)
    ORACLE_CG(gPlayerPositionLUT)
    ORACLE_CG(gPlayerWinningIndex)
    ORACLE_CG(gQuitToMenuTransitionCounter)
    ORACLE_CG(gScreenContexts)
    ORACLE_CG(gTimePlayerLastTouchedFinishLine)
    ORACLE_CG(gTrackDirection)
    ORACLE_CG(gTrackMaxX)
    ORACLE_CG(gTrackMaxY)
    ORACLE_CG(gTrackMaxZ)
    ORACLE_CG(gTrackMinX)
    ORACLE_CG(gTrackMinY)
    ORACLE_CG(gTrackMinZ)
    ORACLE_CG(gWaterLevel)
    ORACLE_CG(gWaterVelocity)
    ORACLE_CG(sIsController1Unplugged)
    // src/code_80005FD0.c
    ORACLE_CG(D_80162E70)
    ORACLE_CG(D_80162EB0)
    ORACLE_CG(D_80162EB2)
    ORACLE_CG(D_80162F10)
    ORACLE_CG(D_80162F50)
    ORACLE_CG(D_80162FB0)
    ORACLE_CG(D_80162FC0)
    ORACLE_CG(D_80162FD0)
    ORACLE_CG(D_80162FF8)
    ORACLE_CG(D_80163010)
    ORACLE_CG(D_80163090)
    ORACLE_CG(D_801630E8)
    ORACLE_CG(D_80163100)
    ORACLE_CG(D_80163128)
    ORACLE_CG(D_80163150)
    ORACLE_CG(D_801631CC)
    ORACLE_CG(D_801631E0)
    ORACLE_CG(D_801631F8)
    ORACLE_CG(D_80163238)
    ORACLE_CG(D_80163240)
    ORACLE_CG(D_80163330)
    ORACLE_CG(D_80163344)
    ORACLE_CG(D_80163348)
    ORACLE_CG(D_8016334C)
    ORACLE_CG(D_80163368)
    ORACLE_CG(D_80163398)
    ORACLE_CG(D_801633B0)
    ORACLE_CG(D_801633C8)
    ORACLE_CG(D_801633E0)
    ORACLE_CG(D_801633F8)
    ORACLE_CG(D_80163410)
    ORACLE_CG(D_80163418)
    ORACLE_CG(D_80163428)
    ORACLE_CG(D_80163438)
    ORACLE_CG(D_8016347C)
    ORACLE_CG(D_8016347E)
    ORACLE_CG(D_80163480)
    ORACLE_CG(D_80163484)
    ORACLE_CG(D_80163488)
    ORACLE_CG(D_8016348C)
    ORACLE_CG(D_801634C0)
    ORACLE_CG(D_801634EC)
    ORACLE_CG(D_801634F0)
    ORACLE_CG(D_801634F4)
    ORACLE_CG(D_80163DD8)
    ORACLE_CG(D_80164358)
    ORACLE_CG(D_8016435A)
    ORACLE_CG(D_8016435C)
    ORACLE_CG(D_80164378)
    ORACLE_CG(D_8016448C)
    ORACLE_CG(D_80164498)
    ORACLE_CG(D_80164538)
    ORACLE_CG(D_801645D0)
    ORACLE_CG(D_801645E8)
    ORACLE_CG(D_801645F8)
    ORACLE_CG(D_80164608)
    ORACLE_CG(D_80164618)
    ORACLE_CG(D_80164628)
    ORACLE_CG(D_80164638)
    ORACLE_CG(D_80164648)
    ORACLE_CG(D_80164658)
    ORACLE_CG(D_80164670)
    ORACLE_CG(D_80164678)
    ORACLE_CG(D_80164680)
    ORACLE_CG(D_80164688)
    ORACLE_CG(D_80164698)
    ORACLE_CG(D_8016469C)
    ORACLE_CG(D_801646A0)
    ORACLE_CG(D_801646C0)
    ORACLE_CG(D_801646C8)
    ORACLE_CG(D_801646CC)
    ORACLE_CG(D_801646D0)
    ORACLE_CG(bInMultiPathSection)
    ORACLE_CG(bStopAICrossing)
    ORACLE_CG(cpu_ItemStrategy)
    ORACLE_CG(cpu_TargetSpeed)
    ORACLE_CG(cpu_enteringPathIntersection)
    ORACLE_CG(cpu_exitingPathIntersection)
    ORACLE_CG(gBestRankedHumanPlayer)
    ORACLE_CG(gCourseCompletionPercentByRank)
    ORACLE_CG(gCurrentCpuTargetSpeed)
    ORACLE_CG(gCurrentNearestPathPoint)
    PutPoolPtr(h, "gCurrentPathPointExpectedRotationPath", (uintptr_t) gCurrentPathPointExpectedRotationPath);
    ORACLE_CG(gCurrentPlayerLookAhead)
    PutPoolPtr(h, "gCurrentTrackConsecutiveCurveCountsPath", (uintptr_t) gCurrentTrackConsecutiveCurveCountsPath);
    PutPoolPtr(h, "gCurrentTrackPath", (uintptr_t) gCurrentTrackPath);
    PutPoolPtr(h, "gCurrentTrackSectionTypesPath", (uintptr_t) gCurrentTrackSectionTypesPath);
    ORACLE_CG(gGPCurrentRaceRankByPlayerIdDup)
    ORACLE_CG(gGetPlayerByCharacterId)
    ORACLE_CG(gIncrementUpdatePlayer)
    ORACLE_CG(gIsInExtra)
    ORACLE_CG(gIsPlayerInCurve)
    ORACLE_CG(gIsPlayerNewPathPoint)
    ORACLE_CG(gLookBehindActive)
    ORACLE_CG(gNearestPathPointByCameraId)
    ORACLE_CG(gOffsetPosition)
    ORACLE_CG(gPathCountByPathIndex)
    PutPathOwners(h, "gPathExpectedRotation", gPathExpectedRotation);
    ORACLE_CG(gPathStartZ)
    ORACLE_CG(gPlayerPathIndex)
    ORACLE_CG(gPlayerPathY)
    ORACLE_CG(gPlayerTrackPositionFactorInstruction)
    ORACLE_CG(gPreviousAngleSteering)
    ORACLE_CG(gPreviousCPUBehaviourId)
    ORACLE_CG(gPreviousCpuTargetSpeed)
    ORACLE_CG(gPreviousPlayerAiOffsetX)
    ORACLE_CG(gPreviousPlayerAiOffsetZ)
    ORACLE_CG(gPreviousPlayerZ)
    ORACLE_CG(gSelectedPathCount)
    PutPathOwners(h, "gTrackConsecutiveCurveCounts", gTrackConsecutiveCurveCounts);
    PutPathOwners(h, "gTrackLeftPaths", gTrackLeftPaths);
    PutPathOwners(h, "gTrackPaths", gTrackPaths);
    ORACLE_CG(gTrackPositionFactor)
    PutPathOwners(h, "gTrackRightPaths", gTrackRightPaths);
    PutPathOwners(h, "gTrackSectionTypes", gTrackSectionTypes);
    ORACLE_CG(gUnexpiredActorsList)
    PutVehiclePathOwner(h, "gVehicle2DPathPoint", gVehicle2DPathPoint);
    ORACLE_CG(gVehicle2DPathSize)
    PutPoolPtr(h, "gVehiclePath", (uintptr_t) gVehiclePath); // == gVehicle2DPathPoint (:3433, 4348, 4364)
    put(h, "gVehiclePathSize", (uint64_t) gVehiclePathSize);
    ORACLE_CG(sCrossingActiveTimer)
    ORACLE_CG(sPlayerAngle)
    ORACLE_CG(sSomeNearestPathPoint)
    ORACLE_CG(sVehicleSoundRenderCounter)
    // src/code_80057C60.c
    ORACLE_CG(D_80165590)
    ORACLE_CG(D_80165594)
    ORACLE_CG(D_80165598)
    ORACLE_CG(D_8016559C)
    ORACLE_CG(D_801655A4)
    ORACLE_CG(D_801655AC)
    ORACLE_CG(D_801655B4)
    ORACLE_CG(D_801655BC)
    ORACLE_CG(D_801655C0)
    ORACLE_CG(D_801655C4)
    ORACLE_CG(D_801655CC)
    ORACLE_CG(D_801655D8)
    ORACLE_CG(D_801655E8)
    ORACLE_CG(D_801655F8)
    ORACLE_CG(D_80165608)
    ORACLE_CG(D_80165618)
    ORACLE_CG(D_80165628)
    ORACLE_CG(D_80165638)
    ORACLE_CG(D_80165648)
    ORACLE_CG(D_80165658)
    ORACLE_CG(D_801656B0)
    ORACLE_CG(D_801656C0)
    ORACLE_CG(D_801656D0)
    ORACLE_CG(D_801656E0)
    ORACLE_CG(D_801656F0)
    ORACLE_CG(D_80165708)
    ORACLE_CG(D_80165730)
    ORACLE_CG(D_80165754)
    ORACLE_CG(D_80165760)
    ORACLE_CG(D_8016576A)
    ORACLE_CG(D_80165770)
    ORACLE_CG(D_80165780)
    ORACLE_CG(D_8016578C)
    ORACLE_CG(D_80165790)
    ORACLE_CG(D_80165794)
    ORACLE_CG(D_8016579C)
    ORACLE_CG(D_8016579E)
    ORACLE_CG(D_801657A2)
    ORACLE_CG(D_801657AE)
    ORACLE_CG(D_801657B2)
    ORACLE_CG(D_801657B8)
    ORACLE_CG(D_801657D0)
    ORACLE_CG(D_801657D8)
    ORACLE_CG(D_801657E1)
    ORACLE_CG(D_801657E2)
    ORACLE_CG(D_801657E3)
    ORACLE_CG(D_801657E5)
    ORACLE_CG(D_801657E6)
    ORACLE_CG(D_801657E7)
    ORACLE_CG(D_801657E8)
    ORACLE_CG(D_801657F0)
    ORACLE_CG(D_801657F8)
    ORACLE_CG(D_801657FC)
    ORACLE_CG(D_80165800)
    ORACLE_CG(D_80165804)
    ORACLE_CG(D_80165808)
    ORACLE_CG(D_8016580C)
    ORACLE_CG(D_80165810)
    ORACLE_CG(D_80165814)
    ORACLE_CG(D_80165818)
    ORACLE_CG(D_8016581C)
    ORACLE_CG(D_80165820)
    ORACLE_CG(D_80165828)
    ORACLE_CG(D_8016582C)
    ORACLE_CG(D_80165832)
    ORACLE_CG(D_80165834)
    ORACLE_CG(D_80165840)
    ORACLE_CG(D_80165860)
    ORACLE_CG(D_8016586C)
    ORACLE_CG(D_80165878)
    ORACLE_CG(D_8016587C)
    ORACLE_CG(D_80165888)
    ORACLE_CG(D_80165890)
    ORACLE_CG(D_801658A8)
    ORACLE_CG(D_801658BC)
    ORACLE_CG(D_801658C6)
    ORACLE_CG(D_801658D6)
    ORACLE_CG(D_801658E4)
    ORACLE_CG(D_801658EC)
    ORACLE_CG(D_801658F4)
    ORACLE_CG(D_801658FE)
    ORACLE_CG(D_80165900)
    ORACLE_CG(D_80165908)
    ORACLE_CG(D_80165A90)
    ORACLE_CG(D_80183DA8)
    ORACLE_CG(D_80183DC8)
    ORACLE_CG(D_80183E40)
    ORACLE_CG(D_80183E50)
    ORACLE_CG(D_80183E70)
    ORACLE_CG(D_80183E80)
    ORACLE_CG(D_80183E98)
    ORACLE_CG(D_8018C3B0)
    ORACLE_CG(D_8018CE10)
    ORACLE_CG(D_8018CF10)
    PutCameraPtr(h, "D_8018CF14", D_8018CF14, "D_8018CF14");
    ORACLE_CG(D_8018CF18)
    PutPlayerPtr(h, "D_8018CF1C", D_8018CF1C, "D_8018CF1C");
    ORACLE_CG(D_8018CF20)
    ORACLE_CG(D_8018CF48)
    ORACLE_CG(D_8018CF50)
    ORACLE_CG(D_8018CF60)
    ORACLE_CG(D_8018CF68)
    ORACLE_CG(D_8018CF78)
    ORACLE_CG(D_8018CF90)
    ORACLE_CG(D_8018CF98)
    ORACLE_CG(D_8018CFA8)
    ORACLE_CG(D_8018CFAC)
    ORACLE_CG(D_8018CFB0)
    ORACLE_CG(D_8018CFB4)
    ORACLE_CG(D_8018CFB8)
    ORACLE_CG(D_8018CFBC)
    ORACLE_CG(D_8018CFC0)
    ORACLE_CG(D_8018CFC4)
    ORACLE_CG(D_8018CFC8)
    ORACLE_CG(D_8018CFCC)
    ORACLE_CG(D_8018CFD0)
    ORACLE_CG(D_8018CFD4)
    ORACLE_CG(D_8018CFD8)
    ORACLE_CG(gControllerRandom)
    ORACLE_CG(gGPCurrentRaceCharacterIdByRank)
    ORACLE_CG(gHUDDisable)
    ORACLE_CG(gHUDModes)
    ORACLE_CG(gIndexLakituList)
    ORACLE_CG(gItemWindowObjectByPlayerId)
    PutOffsetPtr(h, "gLakituTexturePtr", gLakituTexturePtr, gLakituTextureBuffer, "gLakituTexturePtr");
    ORACLE_CG(gLeafParticle)
    ORACLE_CG(gNextFreeLeafParticle)
    ORACLE_CG(gNextFreeObjectParticle1)
    ORACLE_CG(gNextFreeObjectParticle2)
    ORACLE_CG(gNextFreeObjectParticle3)
    ORACLE_CG(gNextFreeObjectParticle4)
    ORACLE_CG(gObjectParticle1)
    ORACLE_CG(gObjectParticle2)
    ORACLE_CG(gObjectParticle3)
    ORACLE_CG(gObjectParticle4)
    ORACLE_CG(indexObjectList1)
    ORACLE_CG(indexObjectList2)
    ORACLE_CG(indexObjectList3)
    ORACLE_CG(indexObjectList4)
    ORACLE_CG(playerHUD)
    // src/code_80057C60_var.c
    ORACLE_CG(D_8018CFE0)
    ORACLE_CG(D_8018CFE4)
    ORACLE_CG(D_8018CFE8)
    ORACLE_CG(D_8018CFEC)
    ORACLE_CG(D_8018CFF0)
    ORACLE_CG(D_8018CFF4)
    ORACLE_CG(D_8018CFF8)
    ORACLE_CG(D_8018D000)
    ORACLE_CG(D_8018D008)
    ORACLE_CG(D_8018D00C)
    ORACLE_CG(D_8018D010)
    ORACLE_CG(D_8018D018)
    ORACLE_CG(D_8018D020)
    ORACLE_CG(D_8018D028)
    ORACLE_CG(D_8018D048)
    ORACLE_CG(D_8018D050)
    ORACLE_CG(D_8018D070)
    ORACLE_CG(D_8018D078)
    ORACLE_CG(D_8018D098)
    ORACLE_CG(D_8018D0A0)
    ORACLE_CG(D_8018D0C0)
    ORACLE_CG(D_8018D0C8)
    ORACLE_CG(D_8018D0E8)
    ORACLE_CG(D_8018D0F0)
    ORACLE_CG(D_8018D110)
    ORACLE_CG(D_8018D114)
    ORACLE_CG(D_8018D140)
    ORACLE_CG(D_8018D150)
    ORACLE_CG(D_8018D158)
    ORACLE_CG(D_8018D160)
    ORACLE_CG(D_8018D168)
    ORACLE_CG(D_8018D16C)
    ORACLE_CG(D_8018D170)
    ORACLE_CG(D_8018D174)
    ORACLE_CG(D_8018D178)
    ORACLE_CG(D_8018D17C)
    ORACLE_CG(D_8018D180)
    ORACLE_CG(D_8018D184)
    ORACLE_CG(D_8018D18C)
    ORACLE_CG(D_8018D190)
    ORACLE_CG(D_8018D1A0)
    ORACLE_CG(D_8018D1B4)
    ORACLE_CG(D_8018D1C4)
    ORACLE_CG(D_8018D1CC)
    ORACLE_CG(D_8018D1D4)
    ORACLE_CG(D_8018D1DC)
    ORACLE_CG(D_8018D1F0)
    ORACLE_CG(D_8018D1F8)
    ORACLE_CG(D_8018D1FC)
    ORACLE_CG(D_8018D204)
    ORACLE_CG(D_8018D20C)
    ORACLE_CG(D_8018D214)
    ORACLE_CG(D_8018D21C)
    ORACLE_CG(D_8018D224)
    ORACLE_CG(D_8018D228)
    ORACLE_CG(D_8018D230)
    ORACLE_CG(D_8018D2A4)
    ORACLE_CG(D_8018D2AC)
    ORACLE_CG(D_8018D2B4)
    ORACLE_CG(D_8018D2BC)
    ORACLE_CG(D_8018D2C8)
    ORACLE_CG(D_8018D314)
    ORACLE_CG(D_8018D320)
    ORACLE_CG(D_8018D380)
    ORACLE_CG(D_8018D384)
    ORACLE_CG(D_8018D388)
    ORACLE_CG(D_8018D3BC)
    ORACLE_CG(D_8018D3C0)
    ORACLE_CG(D_8018D3C4)
    ORACLE_CG(D_8018D3D4)
    ORACLE_CG(D_8018D3D8)
    ORACLE_CG(D_8018D3DC)
    ORACLE_CG(D_8018D3E0)
    ORACLE_CG(D_8018D3E4)
    ORACLE_CG(D_8018D3E8)
    ORACLE_CG(D_8018D3EC)
    ORACLE_CG(D_8018D3F0)
    ORACLE_CG(D_8018D3F4)
    ORACLE_CG(D_8018D3F8)
    ORACLE_CG(D_8018D400)
    ORACLE_CG(D_8018D40C)
    ORACLE_CG(D_8018D410)
    ORACLE_CG(D_8018D620)
    ORACLE_CG(D_8018D650)
    ORACLE_CG(D_8018D6B0)
    ORACLE_CG(D_8018D710)
    ORACLE_CG(D_8018D770)
    ORACLE_CG(D_8018D7A0)
    ORACLE_CG(D_8018D7D0)
    ORACLE_CG(D_8018D800)
    ORACLE_CG(D_8018D830)
    ORACLE_CG(D_8018D860)
    ORACLE_CG(D_8018D890)
    ORACLE_CG(gIsHUDVisible)
    ORACLE_CG(gMatrixHudCount)
    ORACLE_CG(gMinimapHeight)
    ORACLE_CG(gMinimapWidth)
    ORACLE_CG(gPlayerBalloonCount)
    ORACLE_CG(gPlayerBalloonDepartingTimer)
    ORACLE_CG(gPlayerBalloonPosX)
    ORACLE_CG(gPlayerBalloonPosY)
    ORACLE_CG(gPlayerBalloonPosZ)
    ORACLE_CG(gPlayerBalloonStatus)
    ORACLE_CG(xOrientation)
    // src/code_800AF9B0.c
    ORACLE_CG(D_8018EDB0)
    ORACLE_CG(D_8018EDB2)
    ORACLE_CG(D_8018EDB4)
    // src/data/kart_attributes.c
    ORACLE_CG(gKartGravityTable)
    ORACLE_CG(gKartHopInitialVelocityTable)
    // src/data/path_spawn_metadata.c
    ORACLE_CG(gMarioRacewayAIDistances)
    // src/effects.c
    ORACLE_CG(D_8018D900)
    ORACLE_CG(D_8018D920)
    ORACLE_CG(D_8018D990)
    ORACLE_CG(gPlayerBooEffectStartTime)
    ORACLE_CG(gPlayerOtherScreensAlpha)
    ORACLE_CG(gPlayerStarEffectStartTime)
    // src/ending/ceremony_and_credits.c
    ORACLE_CG(D_802856B0)
    ORACLE_CG(D_802856B4)
    ORACLE_CG(D_802856C4)
    ORACLE_CG(D_802876D4)
    ORACLE_CG(D_802876D8)
    ORACLE_CG(D_802876E0)
    ORACLE_CG(D_80287818)
    ORACLE_CG(D_80287998)
    ORACLE_CG(D_80287B1E)
    ORACLE_CG(D_80287B20)
    ORACLE_CG(gCutsceneShotTimer)
    ORACLE_CG(gOrderedSizeSlidingBorders)
    ORACLE_CG(gSizeSlidingBorders)
    ORACLE_CG(sCutsceneShot)
    ORACLE_CG(sCutsceneSplineSegment)
    ORACLE_CG(sCutsceneSplineSegmentProgress)
    // src/ending/code_80280000.c
    ORACLE_CG(D_802874A0)
    // src/ending/code_80281C40.c
    ORACLE_CG(D_80287554)
    ORACLE_CG(gGotoMenu)
    // src/ending/podium_ceremony_actors.c
    ORACLE_CG(D_802874B0)
    ORACLE_CG(D_802874D8)
    ORACLE_CG(D_802874FC)
    ORACLE_CG(D_80287540)
    PutPoolOwner(h, "sPodiumActorList", sPodiumActorList, 200); // 200: the allocation, :314
    ORACLE_CG(sRandomSeed16)
    // src/main.c
    ORACLE_CG(D_800DC514)
    ORACLE_CG(D_800DC540)
    ORACLE_CG(D_800DC544)
    ORACLE_CG(D_800DC568)
    ORACLE_CG(D_800DC56C)
    ORACLE_CG(D_80150118)
    ORACLE_CG(D_8015011E)
    ORACLE_CG(D_80150120)
    ORACLE_CG(D_8015014C)
    ORACLE_CG(D_80150150)
    ORACLE_CG(D_80152308)
    ORACLE_CG(gActiveScreenMode)
    ORACLE_CG(gCCSelection)
    ORACLE_CG(gCollisionGrid)
    ORACLE_CG(gControllerBits)
    ORACLE_CG(gControllerPads)
    ORACLE_CG(gControllerStatuses)
    ORACLE_CG(gDeltaTime)
    ORACLE_CG(gDemoMode)
    ORACLE_CG(gEnableDebugMode)
    ORACLE_CG(gGotoMode)
    ORACLE_CG(gMatrixObjectCount)
    ORACLE_CG(gModeSelection)
    ORACLE_CG(gNumActors)
    ORACLE_CG(gPlayerCountSelection1)
    ORACLE_CG(gScreenAspect)
    ORACLE_CG(gScreenModeSelection)
    ORACLE_CG(gTourComplete)
    ORACLE_CG(gVBlankTimer)
    ORACLE_CG(sBillBoardMtx)
    ORACLE_CG(sNumVBlanks)
    ORACLE_CG(sRenderedFramebuffer)
    ORACLE_CG(sRenderingFramebuffer)
    ORACLE_CG(wasSoftReset)
    // src/menu_items.c
    ORACLE_CG(D_800E852C)
    ORACLE_CG(D_8018D9D8)
    ORACLE_CG(D_8018D9D9)
    ORACLE_CG(D_8018DEE0)
    ORACLE_CG(D_8018E838)
    ORACLE_CG(D_8018E840)
    ORACLE_CG(D_8018E850)
    ORACLE_CG(D_8018E858)
    ORACLE_CG(D_8018ED90)
    ORACLE_CG(D_8018ED91)
    ORACLE_CG(gCharacterIdByGPOverallRank)
    ORACLE_CG(gControllerPak1FileNote)
    ORACLE_CG(gControllerPak1MaxWriteableFiles)
    ORACLE_CG(gControllerPak1NumFilesUsed)
    ORACLE_CG(gControllerPak1NumPagesFree)
    ORACLE_CG(gControllerPak2FileNote)
    ORACLE_CG(gCurrentTransitionTime)
    ORACLE_CG(gCycleFlashMenu)
    ORACLE_CG(gGPPointsByCharacterId)
    ORACLE_CG(gMenuItems)
    ORACLE_CG(gNumD_8018E768Entries)
    PutBytes(h, "gSaveData", gSaveData);
    PutPoolPtr(h, "gSomeDLBuffer", (uintptr_t) gSomeDLBuffer); // contents EXCLUDED: ADJUDICATION.md
    ORACLE_CG(gTextColor)
    ORACLE_CG(gTransitionDuration)
    ORACLE_CG(gTransitionType)
    ORACLE_CG(pfsError)
    ORACLE_CG(pfsState)
    PutPoolOwner(h, "sGPPointsCopy", sGPPointsCopy, 4);
    ORACLE_CG(sIntroLogoTimer)
    ORACLE_CG(sIntroModelMotionSpeed)
    ORACLE_CG(sIntroModelSpeed)
    ORACLE_CG(sMenuTextureEntries)
    ORACLE_CG(sMenuTextureListIndex)
    ORACLE_CG(sMenuTextureMap)
    PutPoolOwner(h, "sTKMK00_LowResBuffer", sTKMK00_LowResBuffer, 0x4B0); // the flag grid, :5603-5734
    // src/menus.c
    ORACLE_CG(gBattleResultCursorSelection)
    ORACLE_CG(gCharacterGridIsSelected)
    ORACLE_CG(gCharacterGridSelections)
    ORACLE_CG(gCharacterSelections)
    ORACLE_CG(gControllerPakMenuSelection)
    ORACLE_CG(gControllerPakScrollDirection)
    ORACLE_CG(gControllerPakSelectedTableRow)
    ORACLE_CG(gControllerPakVisibleTableRows)
    ORACLE_CG(gCourseIndexInCup)
    ORACLE_CG(gCourseRecordsMenuSelection)
    ORACLE_CG(gCourseRecordsSubMenuSelection)
    ORACLE_CG(gCupSelection)
    ORACLE_CG(gDebugGotoScene)
    ORACLE_CG(gDebugMenuSelection)
    ORACLE_CG(gDemoUseController)
    ORACLE_CG(gFadeModeSelection)
    ORACLE_CG(gGameModeMenuColumn)
    ORACLE_CG(gGameModeSubMenuColumn)
    ORACLE_CG(gGhostPlayerInit)
    ORACLE_CG(gIntroModelPosX)
    ORACLE_CG(gIntroModelPosY)
    ORACLE_CG(gIntroModelPosZ)
    ORACLE_CG(gIntroModelRotX)
    ORACLE_CG(gIntroModelRotY)
    ORACLE_CG(gIntroModelRotZ)
    ORACLE_CG(gIntroModelScale)
    ORACLE_CG(gIntroModelZEye)
    ORACLE_CG(gMainMenuSelection)
    ORACLE_CG(gMenuDelayTimer)
    ORACLE_CG(gMenuFadeType)
    ORACLE_CG(gMenuSelection)
    ORACLE_CG(gMenuTimingCounter)
    ORACLE_CG(gNextDemoId)
    ORACLE_CG(gPlayerCount)
    ORACLE_CG(gPlayerSelectMenuSelection)
    ORACLE_CG(gScreenModeListIndex)
    ORACLE_CG(gSoundMode)
    ORACLE_CG(gSubMenuSelection)
    ORACLE_CG(gTimeTrialDataCourseIndex)
    ORACLE_CG(gTimeTrialsResultCursorSelection)
    ORACLE_CG(gTrackMapInit)
    ORACLE_CG(gVersusResultCursorSelection)
    ORACLE_CG(sTempCupSelection)
    ORACLE_CG(unref_D_8018EE0C)
    // src/profiler.c
    ORACLE_CG(D_800DC664)
    ORACLE_CG(D_800DC668)
    ORACLE_CG(D_800DC66C)
    ORACLE_CG(gEnableResourceMeters)
    // src/racing/actors.c
    ORACLE_CG(D_802BA260)
    ORACLE_CG(gTLUTRedShell)
    // src/racing/collision.c
    ORACLE_CG(numTimes)
    // src/racing/framebuffer_effects.c
    ORACLE_CG(gN64ResFrameBuffer)
    ORACLE_CG(gReusableFrameBuffer)
    // src/racing/race_logic.c
    ORACLE_CG(D_802BA030)
    ORACLE_CG(D_802BA032)
    ORACLE_CG(D_802BA034)
    ORACLE_CG(D_802BA038)
    ORACLE_CG(D_802BA040)
    ORACLE_CG(D_802BA048)
    // src/racing/render_courses.c
    ORACLE_CG(D_802B87BC)
    ORACLE_CG(D_802B87C4)
    ORACLE_CG(D_802B87C8)
    ORACLE_CG(D_802B87CC)
    ORACLE_CG(D_802B87D0)
    ORACLE_CG(D_802B87D4)
    ORACLE_CG(currentScreenSection)
    ORACLE_CG(gFogMax)
    ORACLE_CG(gFogMin)
    // src/render_player.c
    ORACLE_CG(D_80164B80)
    ORACLE_CG(D_80165020)
    ORACLE_CG(D_80165150)
    ORACLE_CG(D_80165190)
    ORACLE_CG(D_801651D0)
    ORACLE_CG(gLastAnimFrameSelector)
    ORACLE_CG(gLastAnimGroupSelector)
    ORACLE_CG(gMatrixEffectCount)
    ORACLE_CG(gPlayerBlueEffect)
    ORACLE_CG(gPlayerCyanEffect)
    ORACLE_CG(gPlayerGreenEffect)
    ORACLE_CG(gPlayerLastVelocity)
    ORACLE_CG(gPlayerMagentaEffect)
    ORACLE_CG(gPlayerRedEffect)
    ORACLE_CG(gPlayerYellowEffect)
    ORACLE_CG(gPlayersToRenderCount)
    PutPlayerPtrs(h, "gPlayersToRenderPlayer", gPlayersToRenderPlayer);
    ORACLE_CG(gPlayersToRenderPlayerId)
    ORACLE_CG(gPlayersToRenderScreenId)
    // src/replays.c
    ORACLE_CG(D_80162D86)
    ORACLE_CG(D_80162DC8)
    ORACLE_CG(D_80162DCC)
    ORACLE_CG(D_80162DD0)
    ORACLE_CG(D_80162DD8)
    ORACLE_CG(D_80162DDC)
    ORACLE_CG(D_80162DE0)
    ORACLE_CG(D_80162DE4)
    ORACLE_CG(D_80162DE8)
    ORACLE_CG(D_80162DF4)
    ORACLE_CG(D_80162DFC)
    ORACLE_CG(D_80162E00)
    ORACLE_CG(bCourseGhostDisabled)
    ORACLE_CG(bPlayerGhostDisabled)
    ORACLE_CG(gPauseTriggered)
    ORACLE_CG(gPostTimeTrialReplayCannotSave)
    ORACLE_CG(sButtonsPrevCourseGhost)
    ORACLE_CG(sCourseGhostFramesRemaining)
    PutOffsetPtr(h, "sCourseGhostReplay", sCourseGhostReplay, D_802BFB80, "sCourseGhostReplay");
    ORACLE_CG(sCourseGhostReplayIdx)
    ORACLE_CG(sPlayerGhostButtonsPrev)
    ORACLE_CG(sPlayerGhostFramesRemaining)
    PutOffsetPtr(h, "sPlayerGhostReplay", sPlayerGhostReplay, D_802BFB80, "sPlayerGhostReplay");
    ORACLE_CG(sPlayerGhostReplayIdx)
    ORACLE_CG(sPlayerInputIdx)
    PutOffsetPtr(h, "sPlayerInputs", sPlayerInputs, D_802BFB80, "sPlayerInputs");
    ORACLE_CG(sPostTTButtonsPrev)
    ORACLE_CG(sPostTTFramesRemaining)
    PutOffsetPtr(h, "sPostTTReplay", sPostTTReplay, D_802BFB80, "sPostTTReplay");
    ORACLE_CG(sPostTTReplayIdx)
    PutOffsetPtr(h, "sReplayGhostBuffer", sReplayGhostBuffer, D_802BFB80, "sReplayGhostBuffer");
    put(h, "sReplayGhostBufferSize", (uint64_t) sReplayGhostBufferSize);
    ORACLE_CG(sUnusedReplayCounter)
    PutTrackPtr(h, "staff_ghost_track_ptr", staff_ghost_track_ptr);
    // src/save.c
    ORACLE_CG(D_8018EE10)
    ORACLE_CG(gControllerPak1State)
    ORACLE_CG(sControllerPak2State)
    // src/spawn_players.c
    ORACLE_CG(D_80165210)
    ORACLE_CG(D_80165230)
    ORACLE_CG(D_80165270)
    ORACLE_CG(D_801652C0)
    ORACLE_CG(D_801652E0)
    ORACLE_CG(D_80165300)
    ORACLE_CG(D_80165330)
    ORACLE_CG(D_80165340)
    ORACLE_CG(D_8016556E)
    ORACLE_CG(D_80165570)
    ORACLE_CG(D_80165572)
    ORACLE_CG(D_80165574)
    ORACLE_CG(D_80165576)
    ORACLE_CG(D_80165578)
    ORACLE_CG(D_8016557A)
    ORACLE_CG(D_8016557C)
    ORACLE_CG(D_8016557E)
    ORACLE_CG(D_80165580)
    ORACLE_CG(D_80165582)
    ORACLE_CG(chooseCPUPlayers)
    ORACLE_CG(gCopyNearestWaypointByPlayerId)
    ORACLE_CG(gCopyPathIndexByPlayerId)
    ORACLE_CG(gCountASwitch)
    ORACLE_CG(gCountBChangement)
    ORACLE_CG(gFrameSinceLastACombo)
    ORACLE_CG(gFrameSinceLastBCombo)
    ORACLE_CG(gIsPlayerTripleAButtonCombo)
    ORACLE_CG(gIsPlayerTripleBButtonCombo)
    ORACLE_CG(gPlayerAButtonComboActiveThisFrame)
    ORACLE_CG(gPlayerBButtonComboActiveThisFrame)
    ORACLE_CG(gPlayerCurrentSpeed)
    ORACLE_CG(gPlayerIsBrakeActive)
    ORACLE_CG(gPlayerIsThrottleActive)
    ORACLE_CG(gPlayerWaterLevel)
    ORACLE_CG(gTimerBoostTripleACombo)
    ORACLE_CG(gTimerBoostTripleBCombo)
    // src/stubs.c
    ORACLE_CG(osAppNmiBuffer)
    // C++ globals (kCppGlobalsHashed, OracleCGlobals.h; FASE 1.4c.1 e)
    put(h, "gTrophyIndex", gTrophyIndex); // Game.cpp:78; OTrophy's ctor, Trophy.cpp:39; ceremony_and_credits.c:1001
}

} // namespace oracle
