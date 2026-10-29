#include "Tree.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash

#include <libultra/gbi.h>

extern "C" {
#include "common_structs.h"
#include "racing/math_util.h"
#include "main.h"
#include "racing/actors.h"
}

ATree::ATree(Vec3f pos, Gfx* displaylist, f32 drawDistance, f32 minDrawDistance, const char* tlut = nullptr) {
    Name = "Tree";
    ResourceName = "mk:tree";
    Pos[0] = pos[0];
    Pos[1] = pos[1];
    Pos[2] = pos[2];

    Displaylist = displaylist;
    Tlut = tlut;
    DrawDistance = drawDistance;
    MinDrawDistance = minDrawDistance;
}

void ATree::Tick() {
    if (((Flags & 0x800) == 0) && ((Flags & 0x400) != 0)) {
        Pos[1] = Pos[1] + 4.0f;
        if (Pos[1] > 800.0f) {
            Flags |= 0x800;
        }
    }
}

void ATree::Draw(Camera* camera) {
    f32 dist;

    if ((Flags & 0x800) != 0) {
        return;
    }

    dist = is_within_render_distance(camera->pos, Pos, camera->rot[1], 0, camera->fieldOfView,
                                        DrawDistance);

    if (CVarGetInteger("gNoCulling", 0) == 1) {
        dist = MAX(dist, 0.0f);
    }
 
    if (dist < 0.0f) {
        return;
    }

    if (((Flags & 0x400) == 0) && (dist < MinDrawDistance)) {
        func_8029794C(Pos, Rot, 3.0f);
    }
    sBillBoardMtx[3][0] = Pos[0];
    sBillBoardMtx[3][1] = Pos[1];
    sBillBoardMtx[3][2] = Pos[2];

    if (render_set_position(sBillBoardMtx, 0) != 0) {
        if (Tlut) {
            gDPLoadTLUT_pal256(gDisplayListHead++, Tlut);
        }
        gSPDisplayList(gDisplayListHead++, Displaylist);
    }
}

void ATree::Collision(Player* player, AActor*) { }
void ATree::Destroy() { }

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void ATree::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "DrawDistance", DrawDistance);
    put(h, "MinDrawDistance", MinDrawDistance);
    put(h, "Flags", Flags);
    put(h, "State", State);
    put(h, "BoundingBoxSize", BoundingBoxSize);
    put(h, "Unk_08", Unk_08);
}
