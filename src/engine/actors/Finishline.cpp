#include <libultraship.h>
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash
#include <libultra/gbi.h>
#include "engine/CoreMath.h"

#include "Finishline.h"
#include "engine/Actor.h"
#include "engine/World.h"
#include "assets/models/common_data.h"
#include "src/port/Game.h"
#include "port/interpolation/FrameInterpolation.h"

extern "C" {
#include "macros.h"
#include "common_structs.h"
#include "racing/math_util.h"
#include "actor_types.h"
#include "racing/actors.h"
extern f32 gKartHopInitialVelocityTable[];
extern f32 gKartGravityTable[];
}

size_t AFinishline::_count = 0;

AFinishline::AFinishline(const SpawnParams& params) : AActor(params) {
    Name = "Finishline";
    ResourceName = "mk:finishline";
    _idx = _count;

    if (params.Location.has_value()) {
        FVector pos = params.Location.value_or(FVector(0, 0, 0));
        // Set spawn point to the provided position
        Pos[0] = D_8015F8D0[0] = pos.x;
        Pos[1] = D_8015F8D0[1] = pos.y - 15;
        Pos[2] = D_8015F8D0[2] = pos.z;
    } else {
        // Set spawn point to the tracks first path point.
        Pos[0] = D_8015F8D0[0] = gCurrentTrackPath->x;
        Pos[1] = D_8015F8D0[1] = (f32) (gCurrentTrackPath->y - 15);
        Pos[2] = D_8015F8D0[2] = gCurrentTrackPath->z;
    }

    IRotator rot = params.Rotation.value_or(IRotator(0, 0, 0));

    Rot[0] = rot.pitch;
    Rot[1] = rot.yaw;
    Rot[2] = rot.roll;

    Flags = -0x8000 | 0x4000;

    BoundingBoxSize = 0.0f;

    _count +=1;
}

void AFinishline::BeginPlay() {
    // Prevent collision mesh from being generated extra times.
    if (Editor_IsEnabled()) {
        if (Triangles.size() == 0) {
            TrackEditor::GenerateCollisionMesh(this, (Gfx*)LOAD_ASSET_RAW(D_0D001B90), 1.0f);
        }
    }
}

void AFinishline::Tick() {
}

void AFinishline::Draw(Camera *camera) {
    Mat4 mtx;
    s16 temp = Pos[2];
    s32 maxObjectsReached;

    if (gGamestate == CREDITS_SEQUENCE) {
        return;
    }

    FrameInterpolation_RecordOpenChild("finishline", (_idx << 4) | (camera->cameraId));

    mtxf_pos_rotation_xyz(mtx, Pos, Rot);

    maxObjectsReached = render_set_position(mtx, 0) == 0;
    if (maxObjectsReached) {
        FrameInterpolation_RecordCloseChild();
        return;
    }

    if (temp < camera->pos[2]) {
        if (bFog) {

            gDPSetFogColor(gDisplayListHead++, gFogColour.r, gFogColour.g, gFogColour.b, 0xFF);
            gSPDisplayList(gDisplayListHead++, (Gfx*) D_0D001C20);
        } else {
            gSPDisplayList(gDisplayListHead++, (Gfx*) D_0D001B90);
        }
    } else if (bFog) {
        gDPSetFogColor(gDisplayListHead++, gFogColour.r, gFogColour.g, gFogColour.b, 0xFF);
        gSPDisplayList(gDisplayListHead++, (Gfx*) D_0D001C88);
    } else {
        gSPDisplayList(gDisplayListHead++, (Gfx*) D_0D001BD8);
    }

    FrameInterpolation_RecordCloseChild();
}

void AFinishline::Collision(Player* player, AActor* actor) {
}

bool AFinishline::IsMod() {
    return true;
}

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void AFinishline::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "bIsFinishline", bIsFinishline);
    put(h, "_idx", (uint64_t) _idx);
    put(h, "PickedUp", PickedUp);
    put(h, "Timer", Timer);
    oracle::PutPlayerPtr(h, "_player", _player, "AFinishline::_player");
    put(h, "Hop", Hop);
    put(h, "Gravity", Gravity);
    put(h, "OldHop", OldHop);
    put(h, "OldGravity", OldGravity);
}

// MK64 Online FASE 1.4b: the class statics, hashed once per iteration by World::HashState (OracleCoverage.h).
void AFinishline::HashStatics(oracle::Hasher& h) {
    using oracle::put;
    put(h, "_count", (uint64_t) _count);
}
