#ifndef SKY_H
#define SKY_H

#ifdef __cplusplus

#include <libultraship/libultraship.h>
#include <libultraship/libultra/gbi.h>

#include "SkyActor.h"

#include "defines.h"

EXTERN_C_START
#include "code_800029B0.h"
EXTERN_C_END

namespace oracle { // MK64 Online FASE 1.4b
class Hasher;
}

class Sky {
public:
    static Sky* Instance;
    Sky();
    virtual void Draw(ScreenContext* screen);
    virtual void DrawFloor(ScreenContext* screen);
    Sky* GetSky();
    void SetColours(Vtx* skybox);
    void InitActors(ScreenContext* screen);
    std::vector<std::unique_ptr<SkyActor>>& GetSkyActors();
private:
    Mtx mSkyboxMatrix;
    static Vtx mSkyboxScreenOne[8];
    static Vtx mSkyboxScreenTwo[8];
    static Vtx mSkyboxScreenThree[8];
    static Vtx mSkyboxScreenFour[8];

    std::vector<std::unique_ptr<SkyActor>> mSkyActors;
  public:
    // MK64 Online FASE 1.4b: the oracle state hash (src/port/oracle/OracleCoverage.h).
    void HashState(oracle::Hasher& h) const;
};
#endif // __cplusplus

/** C Compatible functions **/
EXTERN_C void InitSkyActors(ScreenContext* screen);
EXTERN_C void TickSkyActors();
EXTERN_C void DrawSkyActors(ScreenContext* screen, s32 arg0);

#endif // SKY_H
