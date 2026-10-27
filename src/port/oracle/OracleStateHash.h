#pragma once
// The ONE walk over the simulation state (FASE 1.4.2). Game target only. `sink == nullptr` hashes; FASE 1.6's
// --oracle-dump-at passes a sink and receives every field of the same walk.
#include "OracleHash.h"

namespace oracle {

GroupHashes HashState(FieldSink* sink);

// The `cglobals` group (FASE 1.4c, OracleCGlobalsHash.cpp): every kCGlobalsHashed row of OracleCGlobals.h that no older
// group hashes, in the table's order.
void HashCGlobals(Hasher& h);

// FNV-1a-64 of the simulation-relevant CVars (kSimCVars, OracleIdentityHash.h) as the game reads them right now.
// Recorded once in the header (`configHash`) and re-hashed every iteration inside the `race` group.
uint64_t ConfigHash();

// FNV-1a-64 of the mod load order + the three registries' (id, resource name) sets (T3 P13/P14). Header field `modListHash`.
// `entries` (optional) receives the number of hashed entries (mods + tracks + actors + items) for the identity log line.
uint64_t ModListHash(size_t* entries = nullptr);

// Bytes fed to the hasher per group by the last HashState() call (the per-group cost, logged once), next to the raw memory
// footprint of the fixed-size groups (sizeof × count; 0 for heap groups). hashed < footprint by exactly the skipped pointers
// and the padding — anything larger means a field is missing from a list.
struct HashCost {
    uint64_t bytes[kGroupCount];
    uint64_t footprint[kGroupCount];
};
HashCost LastHashCost();

} // namespace oracle
