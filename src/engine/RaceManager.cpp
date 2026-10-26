#include "RaceManager.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash
#include "port/oracle/OracleState.h"    // MK64 Online FASE 1.4b: Oracle_Abort

#include "AllActors.h"
#include "World.h"
#include "port/Game.h"
#include "engine/editor/Editor.h"
#include "engine/editor/SceneManager.h"
#include "engine/RandomItemTable.h"

extern "C" {
#include "racing/render_courses.h"
}

RaceManager::RaceManager(World& world) : WorldContext(world) {
}

std::unordered_map<uintptr_t, std::shared_ptr<Vtx>> mirroredVtxCache;

// Populates a collision mesh for mirror mode
extern "C" void add_triangle_to_collision_mesh(Vtx* vtx1, Vtx* vtx2, Vtx* vtx3, Vtx** outVtx1, Vtx** outVtx2, Vtx** outVtx3) {
    if (gIsMirrorMode != 0) {
        auto getOrCreateMirrored = [](Vtx* original) -> Vtx* {
            uintptr_t key = reinterpret_cast<uintptr_t>(original);
            
            auto it = mirroredVtxCache.find(key);
            if (it != mirroredVtxCache.end()) {
                return it->second.get();
            }

            auto newVtx = std::make_shared<Vtx>(*original);
            newVtx->v.ob[0] = -newVtx->v.ob[0];

            mirroredVtxCache[key] = newVtx;

            return newVtx.get();
        };

        Vtx* m1 = getOrCreateMirrored(vtx1);
        Vtx* m2 = getOrCreateMirrored(vtx2);
        Vtx* m3 = getOrCreateMirrored(vtx3);

        // don't invert winding here, already done in the gfx
        *outVtx1 = m1;
        *outVtx2 = m2;
        *outVtx3 = m3;

    } else {
        // Pas de miroir, on passe les originaux
        *outVtx1 = vtx1;
        *outVtx2 = vtx2;
        *outVtx3 = vtx3;
    }
}

void RaceManager::Load() {
    auto* track = GetWorld()->GetTrack();
    if (track) {
        mirroredVtxCache.clear();
        track->Load();
    } else {
        printf("[RaceManager] [Load] Track was nullptr\n");
    }
}

void RaceManager::PreInit() {
    // Ruleset options
    if (CVarGetInteger("gDisableItemboxes", false) == true) {
        gPlaceItemBoxes = false;
    } else {
        gPlaceItemBoxes = true;
    }

    RaceManager::SetItemTables();
}

void RaceManager::BeginPlay() {
    auto track = WorldContext.GetTrack();

    if (track) {
        // Do not spawn finishline in credits or battle mode. And if bSpawnFinishline.
        if ((gGamestate != CREDITS_SEQUENCE) && (gModeSelection != BATTLE)) {
            if (track->bSpawnFinishline) {
                if (track->FinishlineSpawnPoint.has_value()) {
                    AFinishline* finishline = AFinishline::Spawn(track->FinishlineSpawnPoint.value(), IRotator(0, 0, 0));
                    finishline->bIsFinishline = true;
                } else {
                    AFinishline* finishline = AFinishline::Spawn();
                    finishline->bIsFinishline = true;
                }
            }
        }
        gEditor.AddLight("Sun", nullptr, D_800DC610[1].l->l.dir);

        track->BeginPlay();
    }
}

void RaceManager::PostInit() {
    // Ruleset options
    if (CVarGetInteger("gAllThwompsAreMarty", false) == true) {
        for (auto& object : GetWorld()->Objects) {
            if (OThwomp* thwomp = dynamic_cast<OThwomp*>(object.get())) {
                gObjectList[thwomp->_objectIndex].unk_0D5 = OThwomp::States::JAILED; // Sets all the thwomp behaviour flags to marty
                thwomp->Behaviour = OThwomp::States::JAILED;
            }
        }
    }

    if (CVarGetInteger("gAllBombKartsChase", false) == true) {
        for (auto& object : GetWorld()->Objects) {
            if (OBombKart* kart = dynamic_cast<OBombKart*>(object.get())) {
                kart->Behaviour = OBombKart::States::CHASE;
            }
        }
    }

    if (CVarGetInteger("gGoFish", false) == true) {
        OTrophy::Spawn(FVector(0,0,0), OTrophy::TrophyType::GOLD, OTrophy::Behaviour::GO_FISH);
    }
}

void RaceManager::SetItemTables() {
    std::optional<std::string> humanTableName;
    std::optional<std::string> cpuTableName;

    switch(gModeSelection) {
        case GRAND_PRIX:
            if (CVarGetInteger("gHarderCPU", false) == true) {
                humanTableName = "mk:hard_cpu_grand_prix";
                cpuTableName = "mk:hard_cpu_grand_prix";
            } else { // normal gameplay
                humanTableName = "mk:human_grand_prix";
                cpuTableName = "mk:cpu_grand_prix";
            }
            break;
        case VERSUS:
            switch (gPlayerCountSelection1) {
                case TWO_PLAYERS_SELECTED:
                    humanTableName = "mk:versus_2p";
                    cpuTableName = "mk:versus_2p"; // Required for demo mode to work
                    break;
                case THREE_PLAYERS_SELECTED:
                    humanTableName = "mk:versus_3p";
                    cpuTableName = "mk:versus_3p";
                    break;
                case FOUR_PLAYERS_SELECTED:
                    humanTableName = "mk:versus_4p";
                    cpuTableName = "mk:versus_4p";
                    break;
            }
            break;
        case BATTLE:
            humanTableName = "mk:battle";
            break;
    }
    if (humanTableName.has_value()) {
        mHumanItemTable = gItemTableRegistry.Get(humanTableName.value());
    } else {
        mHumanItemTable = nullptr;
    }
    if (cpuTableName.has_value()) {
        mCPUItemTable = gItemTableRegistry.Get(cpuTableName.value());
    } else {
        mCPUItemTable = nullptr;
    }
    printf("[RaceManager] Selected human item probability table %s\n", humanTableName.value_or("none").c_str());
    printf("[RaceManager] Selected cpu item probability table %s\n", cpuTableName.value_or("none").c_str());
}

extern "C" int16_t RaceManager_GetRandomHumanItem(uint32_t rank) {
    auto& raceManager = GetWorld()->GetRaceManager();

    auto* table = raceManager.GetHumanItemTable();
    if (nullptr == table) {
        printf("[RaceManager_GetRandomHumanItem] Item table nullptr, giving player a none item\n");
        return ITEM_NONE;
    }

    return table->Roll(rank);
}

extern "C" int16_t RaceManager_GetRandomCPUItem(uint32_t rank) {
    auto& raceManager = GetWorld()->GetRaceManager();
    auto* table = raceManager.GetCPUItemTable();
    if (nullptr == table) {
        printf("[RaceManager_GetRandomCPUItem] Item table nullptr, giving player a none item\n");
        return ITEM_NONE;
    }

    return table->Roll(rank);
}

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void RaceManager::HashState(oracle::Hasher& h) const {
    using oracle::put;
    // table-key: each active table by the name that registers it in gItemTableRegistry (the tables live there,
    // measured: RaceManager.cpp SetItemTables `gItemTableRegistry.Get(…)`; mItemTables is dead), then its contents.
    auto table = [&](const char* n, const RandomItemTable* t) {
        oracle::Hasher::Scope s(h, n);
        std::string key;
        bool found = t == nullptr;
        gItemTableRegistry.ForEach([&](const std::string& k, const RandomItemTable& v) {
            if (&v == t) {
                key = k;
                found = true;
            }
        });
        if (!found) {
            const std::string why =
                std::string("state hash: RaceManager::") + n + " points at no gItemTableRegistry entry";
            Oracle_Abort(why.c_str());
        }
        put(h, "present", t != nullptr);
        put(h, "key", key);
        if (t != nullptr) {
            t->HashState(h);
        }
    };
    table("mHumanItemTable", mHumanItemTable);
    table("mCPUItemTable", mCPUItemTable);
}
