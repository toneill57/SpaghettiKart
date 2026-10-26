#include "SpaghettiShip.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash

#include <libultra/gbi.h>
#include "engine/Matrix.h"

extern "C" {
#include "common_structs.h"
#include "racing/math_util.h"
#include "main.h"
#include "courses/harbour/ship_model.h"
}

ASpaghettiShip::ASpaghettiShip(const SpawnParams& params) : AActor(params) {
    Name = "Spaghetti Ship";
    ResourceName = "hm:spaghetti_ship";
    BoundingBoxSize = 3.0f;

    FVector pos = params.Location.value_or(FVector(0, 0, 0));
    Pos[0] = pos.x;
    Pos[1] = pos.y;
    Pos[2] = pos.z;

    Scale = params.Scale.value_or(FVector(0.4f, 0.4f, 0.4f));

    IRotator rot = params.Rotation.value_or(IRotator(0, 0, 0));
    Rot[0] = rot.pitch;
    Rot[1] = rot.yaw;
    Rot[2] = rot.roll;
}

// MK64 Online FASE 1.4b: was `static float angle` inside ASpaghettiShip::Tick — hoisted (pure rename, same zero initial
// value) so the oracle can hash it. Shared by every instance, exactly as the function-static was.
static float sSpaghettiShipAngle = 0.0f; // Keeps track of the ship's rotation around the circle

void ASpaghettiShip::Tick() {
    float& angle = sSpaghettiShipAngle;
    float radius = 150.0f;      // The radius of the circular path
    float speed = 0.01f;       // Speed of rotation

    angle += speed; // Increment the angle to move in a circle

    // Update the position based on a circular path
    FVector spawn = SpawnPos;
    Pos[0] = spawn.x + radius * cosf(angle);
    Pos[2] = spawn.z + radius * sinf(angle);

    // Rotate to face forward along the circle
    Rot[1] = -static_cast<int16_t>(angle * (32768.0f / M_PI / 2.0f));

    WheelRot.pitch += 500;
}

void ASpaghettiShip::Draw(Camera *camera) {
    Mat4 shipMtx;
    Mat4 objectMtx;
    Mat4 resultMtx;
    Vec3f hullPos = {Pos[0], Pos[1], Pos[2]};
    Vec3s hullRot = {Rot[0], Rot[1], Rot[2]};
    Vec3s rot = {(s16) WheelRot.pitch, (s16) WheelRot.yaw, (s16) WheelRot.roll};

    gSPSetGeometryMode(gDisplayListHead++, G_SHADING_SMOOTH);
    gSPClearGeometryMode(gDisplayListHead++, G_LIGHTING);

    ApplyMatrixTransformations(shipMtx, *(FVector*)Pos, *(IRotator*)Rot, Scale);
    if (render_set_position(shipMtx, 0) != 0) {}

    // Render the ships hull
    ApplyMatrixTransformations(objectMtx, {0, 0, 0}, {0, 0, 0}, {1, 1, 1});
    mtxf_multiplication(resultMtx, shipMtx, objectMtx);
    if (render_set_position(resultMtx, 3) != 0) {
        gSPDisplayList(gDisplayListHead++, ship1_spag1_mesh);
    }

    // Front tyre
    ApplyMatrixTransformations(objectMtx, FVector(0, 0, 110), IRotator(0, 0, 0), FVector(1, 1, 1));
    mtxf_identity(shipMtx);
    AddLocalRotation(shipMtx, WheelRot);
    mtxf_multiplication(resultMtx, shipMtx, objectMtx);
    if (render_set_position(resultMtx, 3) != 0) {
        gSPDisplayList(gDisplayListHead++, wheels_Spaghetti_002_mesh);
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    }
    
    // Back tyre
    AddLocalRotation(shipMtx, WheelRot);
    ApplyMatrixTransformations(objectMtx, FVector(0, 0, -165), {0, 0, 0}, {1, 1, 1});
    mtxf_multiplication(resultMtx, shipMtx, objectMtx);
    if (render_set_position(resultMtx, 3) != 0) {
        gSPDisplayList(gDisplayListHead++, wheels_Spaghetti_002_mesh);
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    }
}

bool ASpaghettiShip::IsMod() { return true; }

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void ASpaghettiShip::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "WheelRot", WheelRot);
}

// MK64 Online FASE 1.4b: the oracle hashes the class statics, once per iteration (World::HashState).
void ASpaghettiShip::HashStatics(oracle::Hasher& h) {
    oracle::put(h, "angle", sSpaghettiShipAngle); // the hoisted function-static
}
