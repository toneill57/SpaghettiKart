#include <libultraship.h>
#include "World.h"
#include "Cup.h"
#include "tracks/Track.h"
#include "TrainCrossing.h"
#include <memory>
#include "objects/Object.h"
#include "port/Game.h"
#include "engine/sky/Sky.h"

extern "C" {
#include "objects.h"
#include "main.h"
#include "defines.h"
#include "audio/external.h"
#include "menus.h"
#include "code_800029B0.h"
}

#include "engine/cameras/GameCamera.h"
// MK64 Online FASE 1.4b: the oracle's `world` group (World::HashState) reaches every class with statics.
#include "engine/AllActors.h"
#include "engine/objects/MoleGroup.h"
#include "engine/PlayerBombKart.h"
#include "engine/sky/SkyCloud.h"
#include "engine/sky/SkySnow.h"
#include "engine/sky/SkyStar.h"
#include "engine/TrackBrowser.h"
#include "port/oracle/OracleStatePut.h"
#include "port/oracle/OracleState.h"
extern "C" {
#include "code_80057C60.h" // D_80165898, D_801658CE, D_801658DC
}

World* World::Instance;
std::unique_ptr<Track> mTrack;
Cup* CurrentCup;

World::World() {
    Instance = this;
    RaceManagerInstance = std::make_unique<RaceManager>(*this);
}

World::~World() {
    CleanWorld();
}

void World::AddCup(Cup* cup) {
    Cups.push_back(cup);
}

void World::SetCurrentTrack(std::unique_ptr<Track> track) {
    if (mTrack == track) {
        return;
    }
    mTrack = std::move(track);
}

TrainCrossing* World::AddCrossing(Vec3f position, u32 waypointMin, u32 waypointMax, f32 approachRadius,
                                  f32 exitRadius) {
    auto crossing = std::make_shared<TrainCrossing>(position, waypointMin, waypointMax, approachRadius, exitRadius);
    Crossings.push_back(crossing);
    return crossing.get();
}

u32 World::GetCupIndex() {
    return this->CupIndex;
}

u32 World::NextCup() {
    s32 hack = 1;

    // Do not display battle mode on GP, TT, or VS
    if (gModeSelection != BATTLE) {
        hack = 2;
    }

    if (CupIndex < Cups.size() - hack) {
        CupIndex++;
        CurrentCup = Cups[CupIndex];
        CurrentCup->CursorPosition = 0;
        reset_cycle_flash_menu();
        play_sound2(SOUND_MENU_CURSOR_MOVE);
        return CupIndex;
    }
    return Cups.size() - hack;
}

u32 World::PreviousCup() {
    if (CupIndex > 0) {
        CupIndex--;
        CurrentCup = Cups[CupIndex];
        CurrentCup->CursorPosition = 0;
        reset_cycle_flash_menu();
        play_sound2(SOUND_MENU_CURSOR_MOVE);
        return CupIndex;
    }
    return 0;
}

void World::SetCupIndex(size_t index) {
    CupIndex = index;
}

void World::SetCurrentCup(Cup* cup) {
    if (cup) {
        CurrentCup = cup;
        CurrentCup->CursorPosition = 0;
    }
}

void World::TickCameras() {

    for (size_t i = 0; i < 4; i++) {
        ScreenContext* screen = &gScreenContexts[i];
        if (NULL == screen->pendingCamera) { continue; }
        if (screen->pendingCamera != screen->camera) {
            screen->camera = screen->pendingCamera;
            screen->pendingCamera = nullptr;
        }
    }

    for (auto& camera : Cameras) {
        if (camera->IsActive()) {
            camera->Tick();
        }
    }
}

AActor* World::AddActor(std::unique_ptr<AActor> actor) {
    Actors.push_back(std::move(actor));
    Actors.back()->BeginPlay();
    return Actors.back().get();
}

struct Actor* World::AddBaseActor() {
    Actors.push_back(std::make_unique<AActor>());

    AActor* actor = Actors.back().get();

    // Skip C++ vtable to access variables in C
    return reinterpret_cast<struct Actor*>(reinterpret_cast<char*>(actor) + sizeof(void*));
}

void World::ActorBeginPlay(Actor* actor) {
    AActor* act = ConvertActorToAActor(actor);
    act->BeginPlay();
}

/**
 * Converts a C struct Actor* to its C++ AActor class
 */
AActor* World::ConvertActorToAActor(Actor* actor) {
    // Move the ptr back so that it points at the vtable.
    // Which is the initial item in the class, or in other words
    // Point to the class.
    return reinterpret_cast<AActor*>((char*) actor - sizeof(void*));
}

/**
 * Converts a C++ AActor class to a C Actor* struct.
 */
Actor* World::ConvertAActorToActor(AActor* actor) {
    // Move the ptr forward past the vtable.
    // This allows C to access the class variables like a normal Actor* struct.
    return reinterpret_cast<Actor*>((char*) actor + sizeof(void*));
}

AActor* World::GetActor(size_t index) {
    return Actors[index].get();
}

void World::TickActors() {
    // This only ticks modded actors
    for (auto& actor : Actors) {
        if (actor->IsMod()) { // Do not tick unported C actors
            actor->Tick();
        }
    }
}

void World::TickActors60fps() {
    for (auto& actor: Actors) {
        if (actor->IsMod()) {
            actor->Tick60fps();
        }
    }
}

StaticMeshActor* World::AddStaticMeshActor(const std::string& name, FVector pos, IRotator rot, FVector scale, const std::string& model, int32_t* collision) {
    StaticMeshActors.push_back(std::make_unique<StaticMeshActor>(name, pos, rot, scale, model, collision));
    auto* actor = StaticMeshActors.back().get();
    return actor;
}

void World::DrawStaticMeshActors() {
    for (const auto& actor : StaticMeshActors) {
        actor->Draw();
    }
}

// OObject* World::AddObject(OObject object) {
//     Objects.push_back(std::make_unique<OObject>(object));

//     // This is an example of how to get the C object.
//     // However, nothing is being done with it, so it's been commented out.
//     // if (object->_objectIndex != -1) {
//     //     Object* cObj = &gObjectList[object->_objectIndex];
//     // }

//     return Objects.back().get();
// }

OObject* World::AddObject(std::unique_ptr<OObject> object) {
    Objects.push_back(std::move(object));

    // This is an example of how to get the C object.
    // However, nothing is being done with it, so it's been commented out.
    // if (object->_objectIndex != -1) {
    //     Object* cObj = &gObjectList[object->_objectIndex];
    // }

    return Objects.back().get();
}

void World::TickObjects() {
    for (auto& object : Objects) {
        object->Tick();
    }
}

// Some objects such as lakitu are ticked in process_game_tick.
// This is a fallback to support those objects. Probably don't use this.
void World::TickObjects60fps() {
    for (auto& object : Objects) {
        object->Tick60fps();
    }
}

ParticleEmitter* World::AddEmitter(std::unique_ptr<ParticleEmitter> emitter) {
    Emitters.push_back(std::move(emitter));
    return Emitters.back().get();
}

void World::DrawObjects(s32 cameraId) {
    for (auto& object : Objects) {
        object->Draw(cameraId);
    }
}

void World::TickParticles() {
    for (const auto& emitter : Emitters) {
        emitter->Tick();
    }
}

void World::DrawParticles(s32 cameraId) {
    for (const auto& emitter : Emitters) {
        emitter->Draw(cameraId);
    }
}

// Sets OObjects or AActors static member variables back to default values
void World::Reset() {
    for (auto& object : Objects) {
        object->Reset(); // Used for OPenguin
    }
}

Object* World::GetObjectByIndex(size_t index) {
    // if (index < this->Objects.size()) {
    //  Assuming GameActor::a is accessible, use reinterpret_cast if needed
    //    return reinterpret_cast<Object*>(&this->Objects[index]->o);
    //}
    return nullptr; // Or handle the error as needed
}

// Deletes all objects from the world
void World::CleanWorld(void) {
    printf("[Game.cpp] Clean World\n");

    World::Reset(); // Reset OObjects

    for (size_t i = 0; i < ARRAY_COUNT(mPlayerBombKart); i++) {
        mPlayerBombKart[i].state = PlayerBombKart::PlayerBombKartState::DISABLED;
        mPlayerBombKart[i]._primAlpha = 0;
    }

    gEditor.ClearObjects();
    Actors.clear();
    StaticMeshActors.clear();
    Objects.clear();
    Emitters.clear();
    Lakitus.clear();
    Sky::Instance->GetSkyActors().clear();
}

void World::CleanActors(void) {
    World::Reset();
    Actors.clear();
    Lakitus.clear();
    Objects.clear();
}

// MK64 Online FASE 1.4b: the oracle's `world` group (OracleStateHash.cpp HashWorld). The order is fixed — it IS the
// hash input order: track, cups, cameras, static meshes, emitters, battle bomb karts, train crossings, race manager,
// sky, the track browser, the class statics (alphabetical), the C globals subclass members point at. Containers by
// index (T4 C09). Every member's classification is src/port/oracle/OracleCoverage.h.
void World::HashState(oracle::Hasher& h) const {
    using oracle::put;
    {
        oracle::Hasher::Scope s(h, "mTrack");
        put(h, "present", mTrack != nullptr);
        if (mTrack != nullptr) {
            mTrack->HashState(h);
        }
    }
    put(h, "CupIndex", (uint64_t) CupIndex);
    {
        int32_t cup = -1; // cup-ptr: CurrentCup by its index in Cups
        for (size_t i = 0; i < Cups.size(); i++) {
            if (Cups[i] == CurrentCup) {
                cup = (int32_t) i;
            }
        }
        if (CurrentCup != nullptr && cup < 0) {
            Oracle_Abort("state hash: World::CurrentCup is not in World::Cups");
        }
        put(h, "CurrentCup", cup);
    }
    {
        oracle::Hasher::Scope s(h, "Cups");
        put(h, "size", (uint64_t) Cups.size());
        for (size_t i = 0; i < Cups.size(); i++) {
            oracle::Hasher::Scope e(h, i);
            put(h, "present", Cups[i] != nullptr);
            if (Cups[i] != nullptr) {
                Cups[i]->HashState(h);
            }
        }
    }
    {
        oracle::Hasher::Scope s(h, "Cameras");
        put(h, "size", (uint64_t) Cameras.size());
        for (size_t i = 0; i < Cameras.size(); i++) {
            oracle::Hasher::Scope e(h, i);
            put(h, "present", Cameras[i] != nullptr);
            if (Cameras[i] != nullptr) {
                Cameras[i]->HashState(h);
            }
        }
    }
    {
        oracle::Hasher::Scope s(h, "StaticMeshActors");
        put(h, "size", (uint64_t) StaticMeshActors.size());
        for (size_t i = 0; i < StaticMeshActors.size(); i++) {
            oracle::Hasher::Scope e(h, i);
            put(h, "present", StaticMeshActors[i] != nullptr);
            if (StaticMeshActors[i] != nullptr) {
                StaticMeshActors[i]->HashState(h);
            }
        }
    }
    {
        oracle::Hasher::Scope s(h, "Emitters");
        put(h, "size", (uint64_t) Emitters.size());
        for (size_t i = 0; i < Emitters.size(); i++) {
            oracle::Hasher::Scope e(h, i);
            put(h, "present", Emitters[i] != nullptr);
            if (Emitters[i] != nullptr) {
                Emitters[i]->HashState(h);
            }
        }
    }
    {
        oracle::Hasher::Scope s(h, "mPlayerBombKart");
        for (size_t i = 0; i < 4; i++) {
            oracle::Hasher::Scope e(h, i);
            mPlayerBombKart[i].HashState(h);
        }
    }
    {
        oracle::Hasher::Scope s(h, "Crossings");
        put(h, "size", (uint64_t) Crossings.size());
        for (size_t i = 0; i < Crossings.size(); i++) {
            oracle::Hasher::Scope e(h, i);
            put(h, "present", Crossings[i] != nullptr);
            if (Crossings[i] != nullptr) {
                Crossings[i]->HashState(h);
            }
        }
    }
    {
        oracle::Hasher::Scope s(h, "RaceManagerInstance");
        put(h, "present", RaceManagerInstance != nullptr);
        if (RaceManagerInstance != nullptr) {
            RaceManagerInstance->HashState(h);
        }
    }
    {
        oracle::Hasher::Scope s(h, "Sky");
        put(h, "present", Sky::Instance != nullptr);
        if (Sky::Instance != nullptr) {
            Sky::Instance->HashState(h);
        }
    }
    {
        oracle::Hasher::Scope s(h, "TrackBrowser");
        put(h, "present", TrackBrowser::Instance != nullptr);
        if (TrackBrowser::Instance != nullptr) {
            TrackBrowser::Instance->HashState(h);
        }
    }
    {
        oracle::Hasher::Scope s(h, "statics");
        {
            oracle::Hasher::Scope c(h, "ABoat");
            ABoat::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ABowserStatue");
            ABowserStatue::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ABus");
            ABus::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ACar");
            ACar::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AFallingRock");
            AFallingRock::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AFinishline");
            AFinishline::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AMarioSign");
            AMarioSign::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ASpaghettiShip");
            ASpaghettiShip::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AStarship");
            AStarship::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ATankerTruck");
            ATankerTruck::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AText");
            AText::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ATrain");
            ATrain::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "ATruck");
            ATruck::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "AWarioSign");
            AWarioSign::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "GameCamera");
            GameCamera::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OBat");
            OBat::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OBombKart");
            OBombKart::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OBoos");
            OBoos::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OChainChomp");
            OChainChomp::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OCrab");
            OCrab::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OFlagpole");
            OFlagpole::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OGrandPrixBalloons");
            OGrandPrixBalloons::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OHedgehog");
            OHedgehog::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OLakitu");
            OLakitu::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OMole");
            OMole::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OMoleGroup");
            OMoleGroup::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OPenguin");
            OPenguin::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OSeagull");
            OSeagull::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OSnowman");
            OSnowman::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "OThwomp");
            OThwomp::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "PlayerBombKart");
            PlayerBombKart::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "SkyCloud");
            SkyCloud::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "SkySnow");
            SkySnow::HashStatics(h);
        }
        {
            oracle::Hasher::Scope c(h, "SkyStar");
            SkyStar::HashStatics(h);
        }
    }
    {
        // The C globals that subclass members point at or read directly (OTrophy, OHotAirBalloon). Every other mutable
        // C global is the separate C-globals task (decision 2, 2026-10-26).
        oracle::Hasher::Scope s(h, "cglobals");
        put(h, "D_80165898", D_80165898);
        put(h, "D_801658CE", D_801658CE);
        put(h, "D_801658DC", D_801658DC);
    }
}
