#pragma once
// Identity hashes that are not file hashes (FASE 1.4.5 probes 2-4): the simulation-relevant CVars (`configHash`, T3 P11) and
// the mod load order + registry ids (`modListHash`, T3 P13/P14). Std-only: the tables and the hashing rules are unit-tested in
// oracle_tests; the game side (OracleStateHash.cpp) only reads the live values and calls these functions.
#include "OracleHash.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace oracle {

enum class CVarKind : uint8_t { Integer = 0, Float = 1 };

struct CVarSpec {
    const char* name;
    CVarKind kind;
    float defaultValue; // the default every game call site passes (an integer default is exact in a float)
};

// Every CVar that game code reads (`CVarGetInteger/Float("<literal>"` in src/, src/port/ui excluded — the FASE sweep) whose
// value can change state the 1.4 walk hashes. Fixed order: it IS the hash input order. Sweep + classification measured
// 2026-10-25 at netplay 98d07383f (62 names); oracle_tests re-runs the sweep and fails on any unclassified or stale name.
inline constexpr CVarSpec kSimCVars[] = {
    // Simulation logic (racing, items, CPUs, spawns)
    { "gHarderCPU", CVarKind::Integer, 0 },
    { "gDisableRubberbanding", CVarKind::Integer, 0 },
    { "gEnableMoonJump", CVarKind::Integer, 0 },
    { "gNoWallColision", CVarKind::Integer, 0 },
    { "gMinHeight", CVarKind::Float, 0.0f },
    { "gShellsShootStraight", CVarKind::Integer, 0 },
    { "gDisableItemboxes", CVarKind::Integer, 0 },
    { "gAllThwompsAreMarty", CVarKind::Integer, 0 },
    { "gAllBombKartsChase", CVarKind::Integer, 0 },
    { "gGoFish", CVarKind::Integer, 0 },
    { "gEnableCustomCC", CVarKind::Integer, 0 },
    { "gCustomCC", CVarKind::Float, 150.0f },
    { "gMultiplayerNoFeatureCuts", CVarKind::Integer, 0 },
    { "gNumTrucks", CVarKind::Integer, 7 },
    { "gNumBuses", CVarKind::Integer, 7 },
    { "gNumTankerTrucks", CVarKind::Integer, 7 },
    { "gNumCars", CVarKind::Integer, 7 },
    { "gNumTrains", CVarKind::Integer, 2 },
    { "gNumCarriages", CVarKind::Integer, 5 },
    { "gHasTender", CVarKind::Integer, 1 },
    // Menu flow (the recorded pads drive the menus, so a different flow is a different simulation)
    { "gSkipIntro", CVarKind::Integer, 0 },
    { "gEnableDebugMode", CVarKind::Integer, 0 },
    { "gUniqueCharacterSelections", CVarKind::Integer, 1 },
    { "gControllerPakScreen", CVarKind::Integer, 0 },
    // Track load: collision mesh choice (MarioRaceway::Load) and asset set
    { "gDisableLod", CVarKind::Integer, 1 },
    { "gEnhancements.Mods.AlternateAssets", CVarKind::Integer, 0 },
    // Camera state (hashed group `cameras`)
    { "gLookBehind", CVarKind::Integer, 0 },
    // Game-thread render paths. Measured: render code WRITES hashed state (OSeagull/OCrab/… ::Draw → func_8008A364 →
    // gObjectList flags; render_players → Player.slopeAccel / unk_050 / unk_002), and gNoCulling also gates
    // set_vehicle_render_distance_flags, which the train-crossing AI reads. So a draw toggle can change hashed state.
    { "gNoCulling", CVarKind::Integer, 0 },
    { "gDrawTrackGeometry", CVarKind::Integer, 1 },
    { "gDrawCActors", CVarKind::Integer, 1 },
    { "gDrawCPPActors", CVarKind::Integer, 1 },
    { "gDrawStaticMeshActors", CVarKind::Integer, 1 },
    { "gDrawObjects", CVarKind::Integer, 1 },
    { "gDrawPlayers", CVarKind::Integer, 1 },
    { "gDrawTransparentTrack", CVarKind::Integer, 1 },
    { "gDrawParticles", CVarKind::Integer, 1 },
    { "gDrawItemBoxes", CVarKind::Integer, 1 },
    { "gDrawHUD", CVarKind::Integer, 1 },
    { "gDrawSky", CVarKind::Integer, 1 },
    { "gDrawSkyActors", CVarKind::Integer, 1 },
    { "gRenderCollisionMesh", CVarKind::Integer, 0 },
    { "gFixVisuals", CVarKind::Integer, 0 },
    { "gEnableDigitalSpeedometer", CVarKind::Integer, 0 },
    { "gBetterResultPortraits", CVarKind::Integer, 0 },
    { "gShowSpaghettiVersion", CVarKind::Integer, 1 },
    // Actor code (AText::ValidateString)
    { "gIsEditorEnabled", CVarKind::Integer, 0 },
};

struct CVarExclusion {
    const char* name;
    const char* reason;
};

// Read by game code but provably unable to change hashed state in an oracle session.
inline constexpr CVarExclusion kExcludedCVars[] = {
    { "gGameMasterVolume", "audio thread only (Engine.cpp HandleAudioThread)" },
    { "gMainMusicVolume", "audio state only (audio_init)" },
    { "gSFXMusicVolume", "audio state only (audio_init)" },
    { "gEnvironmentVolume", "audio state only (audio_init, note_set_vel_pan_reverb)" },
    { "gInterpolationFPS", "engine frame pacing (GetInterpolationFPS; no game-code caller)" },
    { "gInterpolationTargetFPS", "engine frame pacing (ProcessGfxCommands)" },
    { "gModifyInterpolationTargetFPS", "engine frame pacing (ProcessGfxCommands)" },
    { "gMatchRefreshRate", "engine frame pacing (GetInterpolationFPS)" },
    { "gVsyncEnabled", "engine frame pacing (GetInterpolationFPS)" },
    { "gDeveloperTools.LogLevel", "logging (GameEngine constructor)" },
    { "eGizmoMode", "track editor only (refused in oracle mode)" },
    { "gEditorSnapToGround", "track editor only (refused in oracle mode)" },
    { "gEditorBoundary", "track editor only (refused in oracle mode)" },
    { "gEditorEnabled", "refused by the session guards (FASE 1.2): always 0 in a session" },
    { "gFreecam", "refused by the session guards (FASE 1.2): always 0 in a session" },
    { "gFarFrustrum", "projection matrix only (guPerspectiveF writes its output matrix)" },
};

// The value exactly as the game reads it. "Unset" and "set to the default" read the same at every call site because each
// CVar has ONE default across all of them (measured; oracle_tests enforces it), so whether it is set is not hashed — that
// would only add noise (a menu that registers a CVar would change the hash without changing what the game sees).
struct CVarValue {
    int32_t i; // CVarGetInteger(name, default) — Integer kind
    float f;   // CVarGetFloat(name, default)   — Float kind
};

// FNV-1a-64 over, per spec in table order: name + '\0', then the value (S32, or F32 by bit pattern). `values` is parallel
// to `specs`.
inline uint64_t HashConfig(const CVarSpec* specs, const CVarValue* values, size_t n) {
    Hasher h;
    for (size_t i = 0; i < n; i++) {
        h.bytes(specs[i].name, std::strlen(specs[i].name) + 1);
        if (specs[i].kind == CVarKind::Float) {
            h.WriteF32(values[i].f);
        } else {
            h.WriteS32(values[i].i);
        }
    }
    return h.value();
}

struct RegistryEntry {
    uint32_t id;
    std::string resourceName;
};

struct ModListInput {
    std::vector<std::string> modsInLoadOrder; // ModManager.cpp `Mods` after SortModsByDependencies
    std::vector<RegistryEntry> tracks;        // gTrackRegistry.GetAllInfo()
    std::vector<RegistryEntry> actors;        // gActorRegistry.GetAllInfo()
    std::vector<RegistryEntry> items;         // gItemRegistry.GetAllInfo()
};

// T3 P14: the mod LOAD ORDER is identity (later archives override earlier ones), so mods are hashed in order.
// T3 P13: registry ids come from `mCounter++` in registration order and ARE identity; the registry's unordered_map
// iteration order is not, so entries are sorted by (id, name) before hashing.
inline uint64_t HashModList(const ModListInput& in) {
    Hasher h;
    h.WriteU64(in.modsInLoadOrder.size());
    for (const std::string& m : in.modsInLoadOrder) {
        h.bytes(m.c_str(), m.size() + 1);
    }
    const std::vector<RegistryEntry>* registries[] = { &in.tracks, &in.actors, &in.items };
    for (const std::vector<RegistryEntry>* r : registries) {
        std::vector<RegistryEntry> sorted = *r;
        std::sort(sorted.begin(), sorted.end(), [](const RegistryEntry& a, const RegistryEntry& b) {
            return a.id != b.id ? a.id < b.id : a.resourceName < b.resourceName;
        });
        h.WriteU64(sorted.size());
        for (const RegistryEntry& e : sorted) {
            h.WriteU32(e.id);
            h.bytes(e.resourceName.c_str(), e.resourceName.size() + 1);
        }
    }
    return h.value();
}

} // namespace oracle
