#pragma once

#include <libultraship.h>

extern "C" {
#include "common_structs.h"
}

namespace oracle { // MK64 Online FASE 1.4b
class Hasher;
}

/**
 * @brief TrainCrossing class
 * @arg Vec3f position of the crossing
 * @arg u32 waypointMin
 * @arg u32 waypointMax
 * 
 * Min/Max are used to determine the range for AI to stop for the crossing.
 * 
 */
class TrainCrossing {
public:
    TrainCrossing(Vec3f position, u32 waypointMin, u32 waypointMax, f32 approachRadius, f32 exitRadius);

    Vec3f Position;
    s32 OnTriggered = 0;
    u32 Timer;
    u32 WaypointMin;
    u32 WaypointMax;

    f32 ApproachRadius;
    f32 ExitRadius;

    virtual void CrossingTrigger();
    virtual void AICrossingBehaviour(s32 playerId);
    f32 Distance(Vec3f a, Vec3f b);
    f32 DynamicRadius(Vec3f trainPos, Vec3f trainVelocity, Vec3f crossingPos);
  public:
    // MK64 Online FASE 1.4b: the oracle state hash (src/port/oracle/OracleCoverage.h).
    virtual void HashState(oracle::Hasher& h) const;
};
