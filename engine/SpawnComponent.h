#pragma once

#include <string>
#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine {

// A spawner. SpawnSystem counts it down and, when it fires, creates an entity at this entity's
// location with the given texture. The platformer's S tile is the only spawner and it spawns stars.
class SpawnComponent {
public:
    EntityID entityID;       // the entity this spawner is attached to
    std::string spawnType;   // what to create; "star" in this game
    SpriteID spawnTextureID; // texture the spawned entity is drawn with
    int timeToNextSpawn;     // frames until the next spawn
    int timeBetweenSpawns;   // frames between spawns
    int totalSpawns;         // spawns left; -1 means forever
};

}  // namespace ECSEngine
