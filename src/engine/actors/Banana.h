#pragma once

#include <libultraship.h>
#include "engine/Actor.h"

class ABanana : public AActor {
public:

    uint16_t PlayerId;

    // Constructor
    ABanana(const SpawnParams& params);
    virtual ~ABanana() override = default;

    // Virtual functions to be overridden by derived classes
    virtual void Tick() override;
    virtual void Draw(Camera*) override;
    virtual void Collision(Player*, AActor*) override;
    virtual void Destroy() override;
  public:
    // MK64 Online FASE 1.4b: the oracle state hash (src/port/oracle/OracleCoverage.h).
    void HashState(oracle::Hasher& h) const override;
};
