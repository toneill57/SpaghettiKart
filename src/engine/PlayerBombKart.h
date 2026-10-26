#pragma once

#include <libultraship.h>
#include "CoreMath.h"

namespace oracle { // MK64 Online FASE 1.4b
class Hasher;
}

class PlayerBombKart {
public:

    enum PlayerBombKartState {
        DISABLED,
        ACTIVE,
    };

    PlayerBombKart();

    ~PlayerBombKart() {
        _count--;
    }

    static size_t GetCount() {
        return _count;
    }

    s16 state = PlayerBombKartState::DISABLED;
    FVector pos = {0, 0, 0};
    f32 surfaceHeight = 0;
    s32 _primAlpha = 0;

    void Draw(size_t playerId, s32 cameraId);
    void func_800563DC(s32 cameraId, s32 arg2);
    void func_800562E4(s32 cameraId, s32 arg0, s32 arg1, s32 arg2, s32 id);
    void func_8005669C(s32 cameraId, s32 arg2);
    void LoadMtx(s32 cameraId);
private:
    static u32 vec[3][3];
    static size_t _count;
    size_t _idx;
  public:
    // MK64 Online FASE 1.4b: the oracle state hash (src/port/oracle/OracleCoverage.h).
    virtual void HashState(oracle::Hasher& h) const;
    static void HashStatics(oracle::Hasher& h);
};
