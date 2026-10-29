#pragma once

#include <libultraship.h>
#include "engine/CoreMath.h"

extern "C" {
#include "camera.h"
}

namespace oracle { // MK64 Online FASE 1.4b
class Hasher;
}

class GameCamera {
public:
    GameCamera();
    GameCamera(FVector spawn, s16 rot, u32 mode);
    virtual ~GameCamera() {
        _count--;
    }

    enum class ProjectionMode {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    ProjectionMode ProjMode;

    virtual void Tick();
    virtual void SetViewProjection();
    virtual void SetActive(bool state);

    void SetProjectionMode(GameCamera::ProjectionMode mode);
    bool IsActive();
    Camera* Get();

    Mtx* GetPerspMatrix();
    Mtx* GetLookAtMatrix();

protected:
    Mtx PerspectiveMatrix;
    Mtx LookAtMatrix;
    bool bActive;
    Camera* _camera;
    static size_t _count;
  public:
    // MK64 Online FASE 1.4b: the oracle state hash (src/port/oracle/OracleCoverage.h).
    virtual void HashState(oracle::Hasher& h) const;
    static void HashStatics(oracle::Hasher& h);
};
