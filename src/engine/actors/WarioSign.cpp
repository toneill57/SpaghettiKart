#include "WarioSign.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash

#include <libultra/gbi.h>
#include <assets/models/tracks/wario_stadium/wario_stadium_data.h>
#include "port/interpolation/FrameInterpolation.h"

extern "C" {
#include "common_structs.h"
#include "racing/math_util.h"
#include "main.h"
#include "actor_types.h"
#include "code_800029B0.h"
#include "racing/collision.h"
}

size_t AWarioSign::_count = 0;

AWarioSign::AWarioSign(const SpawnParams& params) : AActor(params) {
    Type = ACTOR_WARIO_SIGN;
    Name = "Wario Sign";
    ResourceName = "mk:wario_sign";
    Model = d_course_wario_stadium_dl_sign;

    Speed = params.Speed.value_or(182);
    _idx = _count;

    FVector pos = params.Location.value_or(FVector(0, 0, 0));
    Pos[0] = pos.x * gTrackDirection;
    Pos[1] = pos.y;
    Pos[2] = pos.z;

    IRotator rot = params.Rotation.value_or(IRotator(0, 0, 0));
    Rot[0] = rot.pitch;
    Rot[1] = rot.yaw;
    Rot[2] = rot.roll;

    Scale = params.Scale.value_or(FVector(1.0f, 1.0f, 1.0f));

    func_802AAAAC(&Unk30);
    Flags = -0x8000;

    _count += 1;
}

bool AWarioSign::IsMod() {
    return true;
}

void AWarioSign::Tick() {
    Rot[1] += Speed;
}

void AWarioSign::Draw(Camera *camera) {
    Mat4 sp38;
    f32 unk =
        is_within_render_distance(camera->pos, Pos, camera->rot[1], 0, camera->fieldOfView, 16000000.0f);

    if (CVarGetInteger("gNoCulling", 0) == 1) {
        unk = MAX(unk, 0.0f);
    }
    if (!(unk < 0.0f)) {
        FrameInterpolation_RecordOpenChild("mk:mario_sign", TAG_OBJECT((_idx << 4) | camera->cameraId));
        gSPSetGeometryMode(gDisplayListHead++, G_SHADING_SMOOTH);
        gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING);

        mtxf_pos_rotation_xyz(sp38, Pos, Rot);
        if (render_set_position(sp38, 0) != 0) {

            gSPDisplayList(gDisplayListHead++, (Gfx*)d_course_wario_stadium_dl_sign);
        }
        FrameInterpolation_RecordCloseChild();
    }
}

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void AWarioSign::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "_idx", (uint64_t) _idx);
}

// MK64 Online FASE 1.4b: the class statics, hashed once per iteration by World::HashState (OracleCoverage.h).
void AWarioSign::HashStatics(oracle::Hasher& h) {
    using oracle::put;
    put(h, "_count", (uint64_t) _count);
}
