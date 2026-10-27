// The ONE walk over the simulation state (FASE 1.4.2): ten groups + `all`, field by field (`world`: FASE 1.4b). Game
// target only. What each class contributes, member by member, is OracleCoverage.h (oracle_tests enforces it).
#include <libultraship.h>

#include "OracleStatePut.h"
#include "OracleStateHash.h"
#include "OracleIdentityHash.h"
#include "OracleState.h" // Oracle_Abort

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>
#include <vector>

#include "engine/World.h"
#include "engine/Actor.h"
#include "engine/objects/Object.h"
#include "engine/particles/ParticleEmitter.h"
#include "engine/mods/ModMetadata.h"
#include "port/Game.h" // gTrackRegistry / gActorRegistry / gItemRegistry, GetWorld()
#include "ship/resource/archive/Archive.h"

extern "C" {
#include "main.h"           // gPlayers[] :128, gControllers[] :118, gTickLogic :162, gGamestate :203, gRaceState :205,
                            // gGamestateNext :210, gGlobalTimer :220, gCourseTimer :227, gTickCounter :228
#include "buffers/random.h" // gRandomSeed16 :6
#include "code_800029B0.h"  // gIsInQuitToMenuTransition :55, gIsGamePaused :73
#include "code_80057C60.h"  // objectListSize :374, gRaceFrameCounter :566
#include "defines.h"        // NUM_PLAYERS, RACE_INIT, RACE_EXIT
// The headers declare these unsized; the bounds below are the DEFINITIONS' (each checked at its definition):
extern Object gObjectList[OBJECT_LIST_SIZE];                                   // src/code_80057C60.c:223
extern Camera cameras[NUM_CAMERAS];                                            // src/camera.c:29
extern u16 gWrongDirectionCounter[12], gIsPlayerWrongDirection[12];            // src/code_80005FD0.c:105-106
extern s32 gPreviousLapProgressScore[10];                                      // :107
extern u16 gCurrentCPUBehaviourId[12], cpu_BehaviourState[12];                 // :109, :111
extern u16 gPlayersTrackSectionId[12], gSpeedCPUBehaviour[12];                 // :113, :118
extern s16 gGPCurrentRacePlayerIdByRank[12];                                   // :170
extern s32 gLapCountByPlayerId[10], gGPCurrentRaceRankByPlayerId[10];          // :172-173
extern s32 gPreviousGPCurrentRaceRankByPlayerId[10];                           // :174
extern u16 gNearestPathPointByPlayerId[12];                                    // :177
extern s32 gNumPathPointsTraversed[10];                                        // :178
extern f32 gLapCompletionPercentByPlayerId[10], gCourseCompletionPercentByPlayerId[10]; // :183-184
extern u16 gPathIndexByPlayerId[12];                                           // :194
}

// ModManager.cpp:22 — not declared in any header.
extern std::vector<std::tuple<ModMetadata, std::shared_ptr<Ship::Archive>>> Mods;

static_assert(RACE_INIT == 0 && RACE_EXIT == 7, "gRaceState has 8 values RACE_INIT 0 … RACE_EXIT 7 (T2 c02)");
static_assert(NUM_PLAYERS == 8, "gPlayers / gControllers hold NUM_PLAYERS == 8 entries (include/defines.h)");

using namespace oracle;

namespace {

template <typename T> void PutN(Hasher& h, const char* n, const T* a, size_t count) {
    Hasher::Scope s(h, n);
    for (size_t i = 0; i < count; i++) {
        Hasher::Scope e(h, i);
        put(h, "", a[i]);
    }
}

void HashRng(Hasher& h) {
    put(h, "gRandomSeed16", gRandomSeed16);
}

void HashRace(Hasher& h) {
    static bool sLoggedRaceState = false;
    if ((gRaceState < RACE_INIT || gRaceState > RACE_EXIT) && !sLoggedRaceState) {
        SPDLOG_WARN("oracle: gRaceState={} is outside RACE_INIT..RACE_EXIT (hashed as is)", gRaceState);
        sLoggedRaceState = true;
    }
    put(h, "gRaceState", gRaceState);
    put(h, "gGamestate", gGamestate);
    put(h, "gGamestateNext", gGamestateNext);
    put(h, "gTickCounter", (uint64_t) gTickCounter);
    put(h, "gCourseTimer", gCourseTimer);
    put(h, "gRaceFrameCounter", gRaceFrameCounter);
    put(h, "gIsGamePaused", gIsGamePaused);
    put(h, "gIsInQuitToMenuTransition", gIsInQuitToMenuTransition);
    put(h, "gGlobalTimer", gGlobalTimer);
    put(h, "gTickLogic", gTickLogic);
    put(h, "configHash", ConfigHash()); // a CVar toggled mid-session (ImGui) shows up here, on that iteration
}

void HashPlayers(Hasher& h) {
    PutN(h, "gPlayers", gPlayers, NUM_PLAYERS); // includes the four particle pools (a split later would be MK64HASH 2)
}

void HashControllers(Hasher& h) {
    PutN(h, "gControllers", gControllers, NUM_PLAYERS);
}

void HashObjects(Hasher& h) {
    put(h, "objectListSize", objectListSize);
    put(h, "gObjectList", gObjectList);
}

// Deques are walked BY INDEX: insertion order is simulation state (T4 C09). Never iterate World::Lakitus (unordered_map).
void HashActors(Hasher& h) {
    World* w = GetWorld();
    put(h, "world", w != nullptr);
    if (w == nullptr) {
        return;
    }
    const auto& v = w->Actors;
    put(h, "count", (uint64_t) v.size());
    Hasher::Scope s(h, "Actors");
    for (size_t i = 0; i < v.size(); i++) {
        Hasher::Scope e(h, i);
        put(h, "present", v[i] != nullptr);
        if (v[i] == nullptr) {
            continue;
        }
        // Not hashed: the vptr, Model / Name / ResourceName (pointers), uuid (no writer anywhere), Triangles.
        const AActor& a = *v[i];
        put(h, "Type", a.Type);
        put(h, "Flags", a.Flags);
        put(h, "Unk_04", a.Unk_04);
        put(h, "State", a.State);
        put(h, "Unk_08", a.Unk_08);
        put(h, "BoundingBoxSize", a.BoundingBoxSize);
        put(h, "Rot", a.Rot);
        put(h, "Unk_16", a.Unk_16);
        put(h, "Pos", a.Pos);
        put(h, "Velocity", a.Velocity);
        put(h, "Unk30", a.Unk30);
        put(h, "SpawnPos", a.SpawnPos);
        put(h, "SpawnRot", a.SpawnRot);
        put(h, "SpawnScale", a.SpawnScale);
        put(h, "Scale", a.Scale);
        put(h, "Speed", a.Speed);
        put(h, "bPendingDestroy", a.bPendingDestroy);
        // FASE 1.4b: subclass state. Under its own path segment: ATree re-declares Flags / State / BoundingBoxSize /
        // Unk_08, so the base's and the subclass's members would otherwise share a dump path.
        Hasher::Scope sub(h, "sub");
        a.HashState(h);
    }
}

void HashOObjects(Hasher& h) {
    World* w = GetWorld();
    put(h, "world", w != nullptr);
    if (w == nullptr) {
        return;
    }
    const auto& v = w->Objects;
    put(h, "count", (uint64_t) v.size());
    put(h, "lakitus", (uint64_t) w->Lakitus.size());
    Hasher::Scope s(h, "Objects");
    for (size_t i = 0; i < v.size(); i++) {
        Hasher::Scope e(h, i);
        put(h, "present", v[i] != nullptr);
        if (v[i] == nullptr) {
            continue;
        }
        // Not hashed: the vptr, uuid, the dead `o` (T4 C10: no reader or writer), Name / ResourceName / Model, Triangles.
        const OObject& o = *v[i];
        put(h, "_objectIndex", o._objectIndex);
        put(h, "bPendingDestroy", o.bPendingDestroy);
        put(h, "SpawnPos", o.SpawnPos);
        put(h, "SpawnRot", o.SpawnRot);
        put(h, "SpawnScale", o.SpawnScale);
        put(h, "Speed", o.Speed);
        o.HashState(h); // FASE 1.4.3: subclass state (Lakitu, Boos, MoleGroup, Thwomp); no-op for the others
    }
}

void HashCameras(Hasher& h) {
    World* w = GetWorld();
    put(h, "count", (uint64_t) (w != nullptr ? w->Cameras.size() : 0));
    put(h, "cameras", cameras); // all 16, stale slots included (T4 C13: identical in both runs)
}

// FASE 1.4b: World-level state outside the Actors / Objects deques and the class statics (World::HashState, World.cpp).
void HashWorld(Hasher& h) {
    World* w = GetWorld();
    put(h, "world", w != nullptr);
    if (w != nullptr) {
        w->HashState(h);
    }
}

void HashRaceArrays(Hasher& h) {
#define ORACLE_RA(a) put(h, #a, a);
    ORACLE_RA(gWrongDirectionCounter)
    ORACLE_RA(gIsPlayerWrongDirection)
    ORACLE_RA(gPreviousLapProgressScore)
    ORACLE_RA(gCurrentCPUBehaviourId)
    ORACLE_RA(cpu_BehaviourState)
    ORACLE_RA(gPlayersTrackSectionId)
    ORACLE_RA(gSpeedCPUBehaviour)
    ORACLE_RA(gGPCurrentRacePlayerIdByRank)
    ORACLE_RA(gLapCountByPlayerId)
    ORACLE_RA(gGPCurrentRaceRankByPlayerId)
    ORACLE_RA(gPreviousGPCurrentRaceRankByPlayerId)
    ORACLE_RA(gNearestPathPointByPlayerId)
    ORACLE_RA(gNumPathPointsTraversed)
    ORACLE_RA(gLapCompletionPercentByPlayerId)
    ORACLE_RA(gCourseCompletionPercentByPlayerId)
    ORACLE_RA(gPathIndexByPlayerId)
#undef ORACLE_RA
}

HashCost sLastCost{};

template <typename TInfo, typename... TArgs>
std::vector<RegistryEntry> RegistryEntries(const Registry<TInfo, TArgs...>& r) {
    std::vector<RegistryEntry> out;
    for (const TInfo* info : r.GetAllInfo()) {
        out.push_back(RegistryEntry{ info->Id, info->ResourceName });
    }
    return out;
}

} // namespace

GroupHashes oracle::HashState(FieldSink* sink) {
    using Fn = void (*)(Hasher&);
    static constexpr Fn kFns[kGroupCount] = { HashRng,    HashRace,     HashPlayers,    HashControllers,
                                              HashObjects, HashActors,  HashOObjects,   HashCameras,
                                              HashRaceArrays, HashWorld, HashCGlobals };
    GroupHashes out{};
    for (int g = 0; g < kGroupCount; g++) {
        Hasher h(sink);
        Hasher::Scope s(h, kGroupNames[g]);
        kFns[g](h);
        out.g[g] = h.value();
        sLastCost.bytes[g] = h.byteCount();
    }
    out.all = Combine(out.g);
    sLastCost.footprint[kPlayers] = sizeof(Player) * NUM_PLAYERS;
    sLastCost.footprint[kControllers] = sizeof(struct Controller) * NUM_PLAYERS;
    sLastCost.footprint[kObjects] = sizeof(objectListSize) + sizeof(gObjectList);
    sLastCost.footprint[kCameras] = sizeof(cameras);
    return out;
}

// Pointer rules (OracleCoverage.h): what a pointer NAMES, never its address. A target outside the array it must point
// into is a hasher bug or memory corruption — Oracle_Abort, never a guess.
void oracle::PutPlayerPtr(Hasher& h, const char* n, const Player* p, const char* what) {
    int32_t idx = -1;
    if (p != nullptr) {
        const ptrdiff_t d = p - gPlayers;
        if (d < 0 || d >= NUM_PLAYERS) {
            Oracle_Abort((std::string("state hash: ") + what + " points outside gPlayers[0..8)").c_str());
        }
        idx = (int32_t) d;
    }
    put(h, n, idx);
}

void oracle::PutCameraPtr(Hasher& h, const char* n, const Camera* c, const char* what) {
    int32_t idx = -1;
    if (c != nullptr) {
        const ptrdiff_t d = c - cameras;
        if (d < 0 || d >= NUM_CAMERAS) {
            Oracle_Abort((std::string("state hash: ") + what + " points outside cameras[0..16)").c_str());
        }
        idx = (int32_t) d;
    }
    put(h, n, idx);
}

void oracle::PutScreenPtr(Hasher& h, const char* n, const ScreenContext* s, const char* what) {
    constexpr ptrdiff_t kScreens = sizeof(gScreenContexts) / sizeof(gScreenContexts[0]);
    int32_t idx = -1;
    if (s != nullptr) {
        const ptrdiff_t d = s - gScreenContexts;
        if (d < 0 || d >= kScreens) {
            Oracle_Abort((std::string("state hash: ") + what + " points outside gScreenContexts").c_str());
        }
        idx = (int32_t) d;
    }
    put(h, n, idx);
}

void oracle::PutObjectPtr(Hasher& h, const char* n, const OObject* o) {
    put(h, n, o != nullptr ? o->_objectIndex : -2); // the MoleGroup precedent (FASE 1.4)
}

void oracle::PutEmitterPtr(Hasher& h, const char* n, const ParticleEmitter* e, const char* what) {
    int32_t idx = -1;
    if (e != nullptr) {
        const World* w = GetWorld();
        if (w != nullptr) {
            for (size_t i = 0; i < w->Emitters.size(); i++) {
                if (w->Emitters[i].get() == e) {
                    idx = (int32_t) i;
                }
            }
        }
        if (idx < 0) {
            Oracle_Abort((std::string("state hash: ") + what + " points at no World::Emitters entry").c_str());
        }
    }
    put(h, n, idx);
}

HashCost oracle::LastHashCost() {
    return sLastCost;
}

uint64_t oracle::ConfigHash() {
    constexpr size_t kN = sizeof(kSimCVars) / sizeof(kSimCVars[0]);
    CVarValue values[kN];
    for (size_t i = 0; i < kN; i++) {
        const CVarSpec& s = kSimCVars[i];
        values[i] = CVarValue{};
        if (s.kind == CVarKind::Float) {
            values[i].f = CVarGetFloat(s.name, s.defaultValue);
        } else {
            values[i].i = CVarGetInteger(s.name, (int32_t) s.defaultValue);
        }
    }
    return HashConfig(kSimCVars, values, kN);
}

uint64_t oracle::ModListHash(size_t* entries) {
    ModListInput in;
    for (const auto& mod : Mods) {
        in.modsInLoadOrder.push_back(std::get<0>(mod).name);
    }
    in.tracks = RegistryEntries(gTrackRegistry);
    in.actors = RegistryEntries(gActorRegistry);
    in.items = RegistryEntries(gItemRegistry);
    if (entries != nullptr) {
        *entries = in.modsInLoadOrder.size() + in.tracks.size() + in.actors.size() + in.items.size();
    }
    return HashModList(in);
}
