#include "StaticMeshActor.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash
#include <libultra/gbi.h>
#include "Matrix.h"

extern "C" {
#include "main.h"
#include "racing/math_util.h"
#include "math_util_2.h"
}

StaticMeshActor::StaticMeshActor(std::string name, FVector pos, IRotator rot, FVector scale, std::string model, int32_t* collision) : Name(name), Pos(pos), Rot(rot), Scale(scale), Model(""), CollisionMesh(collision) {
    Name = "StaticMesh";
    ResourceName = "hm:static_mesh";

}

void StaticMeshActor::Draw() {
    Mat4 mtx;
    gSPSetGeometryMode(gDisplayListHead++, G_SHADING_SMOOTH);
    gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING);
    if (!Model.empty()) {
        ApplyMatrixTransformations(mtx, Pos, Rot, Scale);
        if (render_set_position(mtx, 0) != 0) {
            gSPDisplayList(gDisplayListHead++, (Gfx*)Model.c_str());
        }
    }
}

void StaticMeshActor::Destroy() {
    bPendingDestroy = true;
}

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void StaticMeshActor::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "Name", Name);
    put(h, "ResourceName", ResourceName);
    put(h, "Pos", Pos);
    put(h, "Rot", Rot);
    put(h, "Scale", Scale);
    put(h, "Model", Model);
    put(h, "bPendingDestroy", bPendingDestroy);
}
