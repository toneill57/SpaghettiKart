#pragma once
// Field-wise `put` overloads for the game structs the state hash walks (FASE 1.4.2). Game target only.
// Every list is in DECLARATION order and was checked against the fork's headers field by field (implement/fase-1.md §1.4:
// the clang record-layout dump comparison). Never a whole struct as raw memory: padding, vptrs and pointers would make two
// peers with identical simulation state hash differently.
#include "OracleHash.h"
#include "engine/CoreMath.h" // FVector (x,y,z), IRotator (pitch,yaw,roll), IPathSpan (Start,End), FVector2D, IVector2D, RGBA8

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" {
#include "common_structs.h"
#include "objects.h"
#include "camera.h"
#include "vehicles.h"                 // TrainCarStuff (FASE 1.4b)
#include "data/path_spawn_metadata.h" // TrackPathSizes (FASE 1.4b)
#include "code_800029B0.h"             // ScreenContext (FASE 1.4b: SkyActor::mScreen)
}

class OObject;
class ParticleEmitter;

#define ORACLE_F(f) put(h, #f, s.f);
#define ORACLE_STRUCT(Type, FIELDS)                                  \
    inline void put(Hasher& h, const char* n, const Type& s) {       \
        Hasher::Scope sc(h, n);                                      \
        FIELDS(ORACLE_F)                                             \
    }

// include/common_structs.h
#define ORACLE_COLLISION_FIELDS(X) \
    X(unk30) X(unk32) X(unk34) X(meshIndexYX) X(meshIndexZY) X(meshIndexZX) X(surfaceDistance) X(unk48) X(unk54) \
    X(orientationVector) X(unk6C)
#define ORACLE_TYRE_FIELDS(X) X(pos) X(surfaceType) X(surfaceFlags) X(collisionMeshIndex) X(baseHeight) X(unk_14)
#define ORACLE_PARTICLE_FIELDS(X)                                                                                        \
    X(pos) X(scale) X(unk_010) X(type) X(surfaceType) X(unk_018) X(isAlive) X(unk_01E) X(unk_020) X(unk_022) X(unk_024) \
    X(unk_028) X(unk_02C) X(unk_02E) X(unk_030) X(unk_032) X(unk_034) X(unk_036) X(red) X(green) X(blue) X(alpha)       \
    X(unk_040) X(unk_042) X(unk_044) X(unk_046)
#define ORACLE_PLAYER_INNER_FIELDS(X) \
    X(unk0) X(unk2) X(unk4) X(unk8) X(unkC) X(unk10) X(unk14) X(unk18) X(unk1A) X(unk1C) X(unk1E) X(unk20)
#define ORACLE_PLAYER_FIELDS(X)                                                                                             \
    X(type) X(unk_002) X(currentRank) X(unk_006) X(lapCount) X(unk_00A) X(triggers) X(currentItemCopy) X(unk_012) X(pos)    \
    X(oldPos) X(rotation) X(unk_032) X(velocity) X(unk_040) X(unk_042) X(kartProps) X(unk_046) X(unk_048) X(unk_050)       \
    X(unk_058) X(unk_05C) X(unk_060) X(unk_064) X(boundingBoxSize) X(unk_074) X(unk_078) X(hopFrameCounter) X(unk_07C)     \
    X(boostPower) X(unk_084) X(unk_088) X(kartPropulsionStrength) X(unk_090) X(speed) X(unk_098) X(currentSpeed)           \
    X(unk_0A0) X(unk_0A4) X(unk_0A8) X(unk_0AA) X(unk_0AC) X(unk_0AE) X(unk_0B0) X(unk_0B2) X(unk_0B4) X(kartGraphics)    \
    X(unk_0B8) X(effects) X(unk_0C0) X(unk_0C2) X(slopeAccel) X(alpha) X(unk_0C8) X(lakituProps) X(unk_0CC) X(unk_0D4)    \
    X(boostTimer) X(oobProps) X(unk_0E0) X(unk_0E2) X(unk_0E4) X(unk_0E8) X(kartHopVelocity) X(kartHopJerk)              \
    X(kartHopAcceleration) X(surfaceType) X(unk_0FA) X(kartFriction) X(kartGravity) X(unk_104) X(hopVerticalOffset)       \
    X(unk_10C) X(unk_10E) X(collision) X(unk_150) X(orientationMatrix) X(tyres) X(unk_1F8) X(unk_1FC) X(unk_200)          \
    X(driftDuration) X(unk_206) X(unk_208) X(unk_20C) X(unk_210) X(topSpeed) X(unk_218) X(unk_21C) X(nearestPathPointId)  \
    X(unk_222) X(size) X(unk_228) X(driftState) X(previousSpeed) X(unk_230) X(unk_234) X(unk_236) X(unk_238) X(unk_23A)   \
    X(unk_23C) X(tyreSpeed) X(animFrameSelector) X(animGroupSelector) X(characterId) X(unk_256) X(particlePool0)          \
    X(particlePool1) X(particlePool2) X(particlePool3) X(unk_D98) X(unk_D9A) X(unk_D9C) X(unk_DA0) X(unk_DA4) X(unk_DA6)  \
    X(unk_DA8) X(unk_DAC) X(unk_DB0) X(unk_DB4) X(nControlFlags) X(nCharacter) X(nStartingRank) X(nHasAuthority)
#define ORACLE_CONTROLLER_FIELDS(X)                                                                                  \
    X(rawStickX) X(rawStickY) X(rightRawStickX) X(rightRawStickY) X(button) X(buttonPressed) X(buttonDepressed)     \
    X(stickDirection) X(stickPressed) X(stickDepressed)
// include/objects.h — the 8 pointer fields (activeTLUT, activeTexture, tlutList, textureList, model, vertex, controlPoints,
// spline) are NOT in the list; `put(Object)` hashes their null-ness as one derived byte (ptrMask).
#define ORACLE_OBJECT_FIELDS(X)                                                                                         \
    X(sizeScaling) X(pos) X(origin_pos) X(unk_01C) X(offset) X(unk_034) X(velocity) X(surfaceHeight) X(unk_048)        \
    X(unk_04C) X(timer) X(status) X(unk_058) X(unk_05C) X(unk_078) X(unk_084) X(animationTimer) X(unk_09A) X(unk_09C)  \
    X(unk_09E) X(primAlpha) X(unk_0A2) X(type) X(state) X(unk_0A8) X(unk_0AA) X(unk_0AC) X(unk_0AE) X(unk_0B0)         \
    X(orientation) X(unk_0B8) X(direction_angle) X(unk_0C4) X(unk_0C6) X(boundingBoxSize) X(unk_0CA) X(isTimerActive)   \
    X(unk_0CC) X(unk_0CD) X(unk_0CE) X(unk_0CF) X(unk_0D0) X(unk_0D1) X(textureListIndex) X(unk_0D3) X(unk_0D4)         \
    X(unk_0D5) X(unk_0D6) X(unk_0D7) X(unk_0D8) X(textureWidth) X(textureHeight) X(unk_0DB) X(unk_0DC) X(unk_0DD)       \
    X(unk_0DE) X(unk_0DF)
// src/camera.h — the two trailing Mtx* (perspectiveMatrix, lookAtMatrix) are NOT hashed
#define ORACLE_CAMERA_INNER_FIELDS(X) X(unk_0) X(unk_4) X(unk_6) X(unk_8)
#define ORACLE_CAMERA_FIELDS(X)                                                                                    \
    X(pos) X(lookAt) X(up) X(rot) X(fieldOfView) X(someBitFlags) X(unk_2C) X(unk_2E) X(unk_30) X(unk_3C) X(unk_48) \
    X(unk_4C) X(unk_50) X(collision) X(unk_94) X(unk_A0) X(unk_A4) X(unk_A8) X(unk_AC) X(playerId) X(unk_B0)       \
    X(unk_B2) X(unk_B4) X(cameraId) X(mode) X(renderMode)
// src/engine/CoreMath.h
#define ORACLE_FVECTOR_FIELDS(X) X(x) X(y) X(z)
#define ORACLE_IROTATOR_FIELDS(X) X(pitch) X(yaw) X(roll)
#define ORACLE_IPATHSPAN_FIELDS(X) X(Start) X(End)
// FASE 1.4b — the POD member types of the classes in OracleCoverage.h. Each list is checked against the compiler's record
// layout by implement/fase-1-artifacts/1.4b/inventory.py (the 1.4 field-coverage method). The parameter is F, not X:
// IVector2D has a member named X.
#define ORACLE_FVECTOR2D_FIELDS(F) F(x) F(z)
#define ORACLE_IVECTOR2D_FIELDS(F) F(X) F(Y)
#define ORACLE_RGBA8_FIELDS(F) F(r) F(g) F(b) F(a)
#define ORACLE_RGB8_FIELDS(F) F(r) F(g) F(b)                                   // include/common_structs.h
#define ORACLE_TRAINCARSTUFF_FIELDS(F) \
    F(isActive) F(compilerPadding) F(position) F(velocity) F(waypointIndex) F(actorIndex) F(unused) // include/vehicles.h
#define ORACLE_TRACKPATHSIZES_FIELDS(F) F(unk0) F(unk2) F(unk4) F(unk6) F(unk8) F(padA) // src/data/path_spawn_metadata.h
#define ORACLE_VTX_T_FIELDS(F) F(ob) F(flag) F(tc) F(cn)                       // libultraship gbi.h (Vtx = union { Vtx_t v; … })

namespace oracle {

// Order matters: a struct's overload must be declared before any struct that contains it (non-template lookup).
ORACLE_STRUCT(FVector, ORACLE_FVECTOR_FIELDS)
ORACLE_STRUCT(IRotator, ORACLE_IROTATOR_FIELDS)
ORACLE_STRUCT(IPathSpan, ORACLE_IPATHSPAN_FIELDS)
ORACLE_STRUCT(struct Collision, ORACLE_COLLISION_FIELDS)
ORACLE_STRUCT(KartTyre, ORACLE_TYRE_FIELDS)
ORACLE_STRUCT(UnkPlayerStruct258, ORACLE_PARTICLE_FIELDS)
ORACLE_STRUCT(struct UnkPlayerInner, ORACLE_PLAYER_INNER_FIELDS)
ORACLE_STRUCT(Player, ORACLE_PLAYER_FIELDS)
ORACLE_STRUCT(struct Controller, ORACLE_CONTROLLER_FIELDS)
ORACLE_STRUCT(UnkCameraInner, ORACLE_CAMERA_INNER_FIELDS)
ORACLE_STRUCT(Camera, ORACLE_CAMERA_FIELDS)
ORACLE_STRUCT(FVector2D, ORACLE_FVECTOR2D_FIELDS)
ORACLE_STRUCT(IVector2D, ORACLE_IVECTOR2D_FIELDS)
ORACLE_STRUCT(RGBA8, ORACLE_RGBA8_FIELDS)
ORACLE_STRUCT(RGB8, ORACLE_RGB8_FIELDS)
ORACLE_STRUCT(TrainCarStuff, ORACLE_TRAINCARSTUFF_FIELDS)
ORACLE_STRUCT(TrackPathSizes, ORACLE_TRACKPATHSIZES_FIELDS)
ORACLE_STRUCT(Vtx_t, ORACLE_VTX_T_FIELDS)

inline void put(Hasher& h, const char* n, const Vtx& s) { // the colour view: every Vtx this hash reaches is built as Vtx_t
    put(h, n, s.v);
}

// Standard containers (FASE 1.4b rules, OracleCoverage.h): a size, then the elements by index; ordered containers by
// key; unordered ones SORTED first (their iteration order is not state, T4 C09). Strings: size, then the bytes.
inline void put(Hasher& h, const char* n, const std::string& s) {
    Hasher::Scope sc(h, n);
    put(h, "size", (uint64_t) s.size());
    h.bytes(s.data(), s.size());
}

template <typename T> void put(Hasher& h, const char* n, const std::vector<T>& v) {
    Hasher::Scope sc(h, n);
    put(h, "size", (uint64_t) v.size());
    for (size_t i = 0; i < v.size(); i++) {
        Hasher::Scope e(h, i);
        put(h, "", v[i]);
    }
}

template <typename T> void put(Hasher& h, const char* n, const std::optional<T>& o) {
    Hasher::Scope sc(h, n);
    put(h, "has_value", o.has_value());
    if (o.has_value()) {
        put(h, "value", *o);
    }
}

template <typename K, typename V> void put(Hasher& h, const char* n, const std::map<K, V>& m) {
    Hasher::Scope sc(h, n);
    put(h, "size", (uint64_t) m.size());
    size_t i = 0;
    for (const auto& kv : m) { // std::map iterates in key order
        Hasher::Scope e(h, i++);
        put(h, "key", kv.first);
        put(h, "value", kv.second);
    }
}

template <typename K, typename V> void put(Hasher& h, const char* n, const std::unordered_map<K, V>& m) {
    std::vector<const std::pair<const K, V>*> sorted;
    for (const auto& kv : m) {
        sorted.push_back(&kv);
    }
    std::sort(sorted.begin(), sorted.end(), [](auto* a, auto* b) { return a->first < b->first; });
    Hasher::Scope sc(h, n);
    put(h, "size", (uint64_t) sorted.size());
    for (size_t i = 0; i < sorted.size(); i++) {
        Hasher::Scope e(h, i);
        put(h, "key", sorted[i]->first);
        put(h, "value", sorted[i]->second);
    }
}

template <typename T> void put(Hasher& h, const char* n, const std::unordered_set<T>& s) {
    std::vector<T> sorted(s.begin(), s.end());
    std::sort(sorted.begin(), sorted.end());
    put(h, n, sorted);
}

// Pointers are hashed as WHAT they name, never as an address (OracleCoverage.h rules). Defined in OracleStateHash.cpp.
// `what` names the class and member for the Oracle_Abort message.
void PutPlayerPtr(Hasher& h, const char* n, const Player* p, const char* what);          // index in gPlayers, -1 null
void PutCameraPtr(Hasher& h, const char* n, const Camera* c, const char* what);          // index in cameras[], -1 null
void PutScreenPtr(Hasher& h, const char* n, const ScreenContext* s, const char* what);   // gScreenContexts[], -1 null
void PutObjectPtr(Hasher& h, const char* n, const OObject* o);                            // _objectIndex, -2 null
void PutEmitterPtr(Hasher& h, const char* n, const ParticleEmitter* e, const char* what); // World::Emitters, -1 null

inline void put(Hasher& h, const char* n, const Object& s) {
    Hasher::Scope sc(h, n);
    ORACLE_OBJECT_FIELDS(ORACLE_F)
    // Derived: only whether each pointer is null (which asset a sprite points at is render state; whether it has one is
    // decided by the simulation). Never the address.
    h.WriteU8("ptrMask", uint8_t((s.activeTLUT != nullptr) | (s.activeTexture != nullptr) << 1 | (s.tlutList != nullptr) << 2 |
                            (s.textureList != nullptr) << 3 | (s.model != nullptr) << 4 | (s.vertex != nullptr) << 5 |
                            (s.controlPoints != nullptr) << 6 | (s.spline != nullptr) << 7));
}

} // namespace oracle

// A field appended after the last listed one must extend its list (the record-layout comparison covers the middle).
static_assert(sizeof(Player) - offsetof(Player, nHasAuthority) == sizeof(u32),
              "Player gained a field after nHasAuthority: extend ORACLE_PLAYER_FIELDS");
static_assert(sizeof(Object) - offsetof(Object, unk_0DF) <= alignof(Object),
              "Object gained a trailing field: extend ORACLE_OBJECT_FIELDS");
static_assert(sizeof(Camera) - offsetof(Camera, lookAtMatrix) == sizeof(void*),
              "Camera gained a field after lookAtMatrix: extend ORACLE_CAMERA_FIELDS");
static_assert(sizeof(TrainCarStuff) == 0x24 && offsetof(TrainCarStuff, unused) == 0x20,
              "TrainCarStuff changed: re-check ORACLE_TRAINCARSTUFF_FIELDS against the record layout");
static_assert(sizeof(TrackPathSizes) == 0x10, "TrackPathSizes changed: re-check ORACLE_TRACKPATHSIZES_FIELDS");
