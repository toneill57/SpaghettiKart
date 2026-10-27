#pragma once
// MK64 Online FASE 1.4c: field-wise `put` overloads for the game structs the `cglobals` group hashes (the 1.4b rule:
// POD structs through an ORACLE_*_FIELDS list, never as raw memory). Included only by OracleCGlobalsHash.cpp, after
// the game headers that declare these types.
//
// Every list is checked by the compiler, not by eye: ORACLE_CG_LAYOUT asserts that the bytes of the listed fields, plus
// the fields deliberately left out, plus the padding counted by hand, add up to sizeof(the struct). A field added,
// removed or resized anywhere in the struct breaks that sum. Each `padding` below is derived in its comment.
#include "OracleStatePut.h"

#include <cstddef>

#define ORACLE_CG_SIZEOF(f) +sizeof(ORACLE_CG_T::f)
#define ORACLE_CG_LAYOUT(Type, FIELDS, leftOut, padding)                                                               \
    static_assert(sizeof(Type) == (0 FIELDS(ORACLE_CG_SIZEOF)) + (leftOut) + (padding),                              \
                  #Type " changed: re-check its ORACLE_*_FIELDS list")

namespace oracle {

// An index into one fixed global array, -1 for null; anywhere else is Oracle_Abort (1.4b's pointer rule).
template <typename T, size_t N>
void PutArrayIndex(Hasher& h, const char* n, const T* p, const T (&a)[N], const char* what) {
    int32_t i = -1;
    if (p != nullptr) {
        if (p < a || p >= a + N) {
            Oracle_Abort((std::string("state hash: ") + what + " points outside its array").c_str());
        }
        i = (int32_t) (p - a);
    }
    put(h, n, i);
}

// src/code_800029B0.h. Pointers by what they name (skybox_and_splitscreen.c:736-739, 801-804; the cameras come from
// GameCamera's ctor, GameCamera.cpp:18,27, or gFreecamCamera = &cameras[4], camera.c:34). `viewport` is all s16 data.
#define ORACLE_SCREENCONTEXT_FIELDS(F)                                                                                  \
    F(pad) F(cameraHeight) F(unk) F(screenWidth) F(screenHeight) F(screenStartX) F(screenStartY) F(unk3C)             \
        F(playerDirection) F(pathCounter) F(unk42) F(pad2)
#define ORACLE_CG_T ScreenContext
// left out: the 8 pointers (hashed below as indices) and the Vp (hashed below as its two s16[4]); padding: none
ORACLE_CG_LAYOUT(ScreenContext, ORACLE_SCREENCONTEXT_FIELDS, 8 * sizeof(void*) + sizeof(Vp), 0);
#undef ORACLE_CG_T
inline void put(Hasher& h, const char* n, const ScreenContext& s) {
    Hasher::Scope sc(h, n);
    PutArrayIndex(h, "controllers", s.controllers, gControllers, "ScreenContext::controllers");
    PutCameraPtr(h, "camera", s.camera, "ScreenContext::camera");
    PutCameraPtr(h, "raceCamera", s.raceCamera, "ScreenContext::raceCamera");
    PutCameraPtr(h, "lookBehindCamera", s.lookBehindCamera, "ScreenContext::lookBehindCamera");
    PutCameraPtr(h, "freeCamera", s.freeCamera, "ScreenContext::freeCamera");
    PutCameraPtr(h, "pendingCamera", s.pendingCamera, "ScreenContext::pendingCamera");
    PutPlayerPtr(h, "player", s.player, "ScreenContext::player");
    PutArrayIndex(h, "unkC", s.unkC, D_8015F790, "ScreenContext::unkC");
    put(h, "viewport.vscale", s.viewport.vp.vscale);
    put(h, "viewport.vtrans", s.viewport.vp.vtrans);
    ORACLE_SCREENCONTEXT_FIELDS(ORACLE_F)
}

// src/code_80005FD0.h
#define ORACLE_UNKSTRUCT46D0_FIELDS(F) F(unk0) F(unk2) F(unk4) F(unk6)
#define ORACLE_CPUITEMSTRATEGYDATA_FIELDS(F)                                                                            \
    F(branch) F(actorIndex) F(timer) F(numItemUse) F(numDroppedBananaBunch) F(unk_0A) F(unk_0C) F(timeBeforeThrow)
#define ORACLE_TRACKPOSITIONFACTORINSTRUCTION_FIELDS(F) F(current) F(target) F(step) F(unkC)
#define ORACLE_UNEXPIREDACTORS_FIELDS(F) F(unk0) F(unk4) F(unk8) F(unkC) F(actorIndex) F(unk10) F(unk14) F(unk18)
#define ORACLE_CG_T UnkStruct_46D0
ORACLE_CG_LAYOUT(UnkStruct_46D0, ORACLE_UNKSTRUCT46D0_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_CG_T CpuItemStrategyData
ORACLE_CG_LAYOUT(CpuItemStrategyData, ORACLE_CPUITEMSTRATEGYDATA_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_CG_T TrackPositionFactorInstruction
ORACLE_CG_LAYOUT(TrackPositionFactorInstruction, ORACLE_TRACKPOSITIONFACTORINSTRUCTION_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_CG_T unexpiredActors
ORACLE_CG_LAYOUT(struct unexpiredActors, ORACLE_UNEXPIREDACTORS_FIELDS, 0, 2); // 2: after the s16 unk10, before u32
#undef ORACLE_CG_T
ORACLE_STRUCT(UnkStruct_46D0, ORACLE_UNKSTRUCT46D0_FIELDS)
ORACLE_STRUCT(CpuItemStrategyData, ORACLE_CPUITEMSTRATEGYDATA_FIELDS)
ORACLE_STRUCT(TrackPositionFactorInstruction, ORACLE_TRACKPOSITIONFACTORINSTRUCTION_FIELDS)
ORACLE_STRUCT(struct unexpiredActors, ORACLE_UNEXPIREDACTORS_FIELDS)

// src/code_80057C60.h. hud_player's three anonymous unions are hashed through their array member.
#define ORACLE_D8018CE10_FIELDS(F) F(unk_00) F(unk_04) F(unk_10) F(objectIndex) F(unk_18)
#define ORACLE_HUDPLAYER_FIELDS(F)                                                                                      \
    F(unknownScaling) F(rankScaling) F(someTimer) F(someTimer1) F(timeLastTouchedFinishLine) F(lapCompletionTimes)     \
        F(lapDurations) F(posXInt) F(posYInt) F(posZInt) F(unk_38) F(blinkTimer) F(speedometerX) F(speedometerY)       \
            F(itemBoxX) F(itemBoxY) F(slideItemBoxX) F(slideItemBoxY) F(unk_4A) F(unk_4C) F(timerX)                    \
                F(lapCompletionTimeXs) F(totalTimeX) F(timerY) F(lapX) F(lapAfterImage1X) F(lapAfterImage2X) F(lapY)   \
                    F(rankX) F(rankY) F(slideRankX) F(slideRankY) F(stagingPosition) F(unk_6C) F(unk_6E)               \
                        F(raceCompleteBool) F(lapCount) F(alsoLapCount) F(blinkState) F(unk_74) F(unk_75)              \
                            F(itemOverride) F(unk_77) F(unk_78) F(unk_79) F(unk_7A) F(unk_7B) F(unk_7C) F(unk_7D)      \
                                F(unk_7E) F(unk_7F) F(unk_80) F(unk_81) F(unk_82) F(unk_83)
#define ORACLE_CG_T struct_D_8018CE10
ORACLE_CG_LAYOUT(struct_D_8018CE10, ORACLE_D8018CE10_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_CG_T hud_player
ORACLE_CG_LAYOUT(hud_player, ORACLE_HUDPLAYER_FIELDS, 0, 0);
#undef ORACLE_CG_T
ORACLE_STRUCT(struct_D_8018CE10, ORACLE_D8018CE10_FIELDS)
ORACLE_STRUCT(hud_player, ORACLE_HUDPLAYER_FIELDS)

// src/ending/ceremony_and_credits.h, src/ending/podium_ceremony_actors.h
#define ORACLE_CINEMATICCAMERA_FIELDS(F)                                                                                \
    F(pos) F(lookAt) F(unk18) F(cutscene) F(unk20) F(unk24) F(unk30) F(unk3C) F(unk48) F(unk4E) F(unk54) F(unk5A)     \
        F(unk60) F(unk62) F(unk64) F(unk68) F(unk6C) F(unk6E)
#define ORACLE_STRUCT80283430_FIELDS(F) F(unk0) F(unk1) F(unk2) F(unk4) F(unk5) F(unk6)
#define ORACLE_D802874D8_FIELDS(F) F(unk0) F(unk4) F(actorTimer) F(unk) F(unk1C) F(unk1D) F(unk1E)
#define ORACLE_CG_T CinematicCamera
ORACLE_CG_LAYOUT(CinematicCamera, ORACLE_CINEMATICCAMERA_FIELDS, 0, 3); // 3: after the u8 cutscene, before f32 unk20
#undef ORACLE_CG_T
#define ORACLE_CG_T struct_80283430
ORACLE_CG_LAYOUT(struct struct_80283430, ORACLE_STRUCT80283430_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_CG_T struct_D_802874D8
ORACLE_CG_LAYOUT(struct_D_802874D8, ORACLE_D802874D8_FIELDS, 0, 1); // 1: 31 bytes of fields rounded up to s32's 4
#undef ORACLE_CG_T
ORACLE_STRUCT(CinematicCamera, ORACLE_CINEMATICCAMERA_FIELDS)
ORACLE_STRUCT(struct struct_80283430, ORACLE_STRUCT80283430_FIELDS)
ORACLE_STRUCT(struct_D_802874D8, ORACLE_D802874D8_FIELDS)

// src/main.h. A cell's `triangle` (an index into gCollisionIndices) is meaningful only when numTriangles != 0:
// func_80295C6C resets only numTriangles (collision.c:1951) and writes `triangle` for the cells it fills (:1987), so an
// empty cell keeps a value from an earlier track, and every reader looks at it only for a non-empty cell.
#define ORACLE_COLLISIONGRID_FIELDS(F) F(triangle) F(numTriangles)
#define ORACLE_CG_T CollisionGrid
ORACLE_CG_LAYOUT(CollisionGrid, ORACLE_COLLISIONGRID_FIELDS, 0, 0);
#undef ORACLE_CG_T
inline void put(Hasher& h, const char* n, const CollisionGrid& s) {
    Hasher::Scope sc(h, n);
    put(h, "numTriangles", s.numTriangles);
    if (s.numTriangles != 0) {
        put(h, "triangle", s.triangle);
    }
}

// libultra controller.h: written once at startup by osContInit (os.cpp:16), read by the menus (menu_items.c:1453).
#define ORACLE_OSCONTSTATUS_FIELDS(F) F(type) F(status) F(err_no)
#define ORACLE_CG_T OSContStatus
ORACLE_CG_LAYOUT(OSContStatus, ORACLE_OSCONTSTATUS_FIELDS, 0, 0);
#undef ORACLE_CG_T
ORACLE_STRUCT(OSContStatus, ORACLE_OSCONTSTATUS_FIELDS)

// libultra controller.h. Only the five fields the recorder keeps (Oracle.cpp:354-358) and the replay restores
// (OracleReplay.h:46-50): err_no, gyro_x and gyro_y are written from the live host devices by ControlDeck::WriteToPad,
// are in no recording, and no game code reads them.
#define ORACLE_OSCONTPAD_FIELDS(F) F(button) F(stick_x) F(stick_y) F(right_stick_x) F(right_stick_y)
#define ORACLE_CG_T OSContPad
// padding 5: 3 after the u8 err_no (gyro_x is a float at 8), 2 after right_stick_y (size rounds to float's 4)
ORACLE_CG_LAYOUT(OSContPad, ORACLE_OSCONTPAD_FIELDS, sizeof(uint8_t) + 2 * sizeof(float), 5);
#undef ORACLE_CG_T
ORACLE_STRUCT(OSContPad, ORACLE_OSCONTPAD_FIELDS)

// src/menu_items.h. As 1.4b's Object (OracleStatePut.h): which static asset a pointer names is render state, so only
// whether it is null is hashed. textureSequence is only ever a static MkAnimation table (menu_items.c:5979, 6056-6100,
// 6190-6240, 9990, 10453-10540, 11702-11713, 12683-12693) and nothing writes through it; TextureMap::textureData holds
// a texture NAME cast to u64* (load_texture, :3648-3657).
#define ORACLE_D8018DEE0_FIELDS(F) F(sequenceIndex) F(frameCountDown) F(visible) F(menuTextureIndex) F(unk14)
#define ORACLE_MENUITEM_FIELDS(F)                                                                                       \
    F(type) F(state) F(subState) F(column) F(row) F(priority) F(visible) F(unk16) F(D_8018DEE0_index) F(param1)        \
        F(param2) F(paramf)
#define ORACLE_CG_T struct_8018DEE0_entry
// padding 4: 28 bytes after the pointer, rounded up to the pointer's 8
ORACLE_CG_LAYOUT(struct_8018DEE0_entry, ORACLE_D8018DEE0_FIELDS, sizeof(void*), 4);
#undef ORACLE_CG_T
#define ORACLE_CG_T MenuItem
ORACLE_CG_LAYOUT(MenuItem, ORACLE_MENUITEM_FIELDS, 0, 0);
#undef ORACLE_CG_T
#define ORACLE_TEXTUREMAP_FIELDS(F) F(offset)
#define ORACLE_CG_T TextureMap
ORACLE_CG_LAYOUT(TextureMap, ORACLE_TEXTUREMAP_FIELDS, sizeof(void*), 4); // padding 4: the s32 rounded up to the pointer's 8
#undef ORACLE_CG_T
inline void put(Hasher& h, const char* n, const struct_8018DEE0_entry& s) {
    Hasher::Scope sc(h, n);
    put(h, "hasTextureSequence", s.textureSequence != nullptr);
    ORACLE_D8018DEE0_FIELDS(ORACLE_F)
}
ORACLE_STRUCT(MenuItem, ORACLE_MENUITEM_FIELDS)
inline void put(Hasher& h, const char* n, const TextureMap& s) {
    Hasher::Scope sc(h, n);
    put(h, "hasTextureData", s.textureData != nullptr);
    put(h, "offset", s.offset);
}

// include/common_structs.h. pad_43 is data, not padding: the 0x100-byte controller-pak read fills it (save.c:927-928).
#define ORACLE_D8018EE10_FIELDS(F)                                                                                      \
    F(unk_00) F(ghostDataSaved) F(trackIndex) F(characterId) F(unk_07) F(pad_43) F(checksum)
#define ORACLE_CG_T struct_8018EE10_entry
ORACLE_CG_LAYOUT(struct_8018EE10_entry, ORACLE_D8018EE10_FIELDS, 0, 0);
#undef ORACLE_CG_T
ORACLE_STRUCT(struct_8018EE10_entry, ORACLE_D8018EE10_FIELDS)

// libultra pfs.h: the controller-pak directory, read from the host's controllerPak_*.sav files (pak.cpp:248-256).
#define ORACLE_OSPFSSTATE_FIELDS(F) F(file_size) F(game_code) F(company_code) F(ext_name) F(game_name)
#define ORACLE_CG_T OSPfsState
ORACLE_CG_LAYOUT(OSPfsState, ORACLE_OSPFSSTATE_FIELDS, 0, 2); // padding 2: after the u16 company_code
#undef ORACLE_CG_T
ORACLE_STRUCT(OSPfsState, ORACLE_OSPFSSTATE_FIELDS)

// include/save_data.h: hashed with the `bytes` rule. Every member is a u8 (or an array or struct of them), so the
// struct's alignment is 1 and it can hold no padding and no pointer.
static_assert(alignof(SaveData) == 1 && sizeof(SaveData) == 0x200, "SaveData changed: re-check the bytes rule");

// podium_ceremony_actors.h — initParams is NOT in the list: it is hashed as which of the four ActorInitParams it names
// (new_actor's only callers, podium_ceremony_actors.c:207, 316, 335, 364). A Firework (same file) overlays the same 0x50
// bytes with the same pointer at 0x28 and data elsewhere, so the list covers every byte but the pointer.
#define ORACLE_CEREMONYACTOR_FIELDS(F)                                                                                  \
    F(isActive) F(type) F(unk6) F(unk8) F(unk9) F(unkA) F(unkC) F(unkE) F(pos) F(unk1C) F(unk20) F(unk24) F(unk2C)    \
    F(unk2E) F(unk30) F(unk32) F(unk34) F(unk36) F(unk38) F(timer) F(unk40) F(unk44) F(unk48)
static_assert(offsetof(CeremonyActor, initParams) == 0x28 && offsetof(Firework, initParams) == 0x28 &&
                  sizeof(CeremonyActor) == sizeof(Firework) &&
                  sizeof(CeremonyActor) == offsetof(CeremonyActor, unk48) + sizeof(s32),
              "CeremonyActor changed: re-check ORACLE_CEREMONYACTOR_FIELDS");

inline void put(Hasher& h, const char* n, const CeremonyActor& s) {
    Hasher::Scope sc(h, n);
    ORACLE_CEREMONYACTOR_FIELDS(ORACLE_F)
    const ActorInitParams* const known[] = { &initBalloon, &initBurst, &initCone, &initDummy };
    int32_t which = -1;
    for (int32_t i = 0; i < 4; i++) {
        if (s.initParams == known[i]) {
            which = i;
        }
    }
    if (s.initParams != nullptr && which < 0) {
        Oracle_Abort("state hash: CeremonyActor::initParams names no ActorInitParams");
    }
    put(h, "initParams", which);
}

} // namespace oracle

#undef ORACLE_CG_LAYOUT
#undef ORACLE_CG_SIZEOF
