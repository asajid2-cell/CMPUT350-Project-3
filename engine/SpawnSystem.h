#pragma once

#include <cmath>
#include <random>
#include <vector>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SpawnComponent.h"

namespace ECSEngine {

// A spawned star: 32 units across, worth one point, thrown at a fixed speed in a random direction.
const float kStarSize = 32.0f;
const float kStarSpeed = 6.0f;

// One shared generator. Seeded from random_device so runs differ, which is fine here: nothing about
// the game's correctness depends on the star directions.
inline std::mt19937& StarRng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

// Walks every spawner, counts its timer down, and once it fires builds a star entity. New entities
// are created after the scan, never during it, because CreateEntity can reallocate the entity
// storage while we are iterating it.
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        struct PendingSpawn {
            Point2D at;
            SpriteID textureID;
            Point2D velocity;
        };
        std::vector<PendingSpawn> pending;

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<SpawnComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            auto& spawner = entities.template GetComponent<SpawnComponent>(id);
            const auto& loc = entities.template GetComponent<LocationComponent>(id);

            if (spawner.totalSpawns == 0) continue;  // exhausted

            if (++spawner.timeToNextSpawn >= spawner.timeBetweenSpawns) {
                spawner.timeToNextSpawn = 0;
                if (spawner.totalSpawns > 0) --spawner.totalSpawns;  // -1 means unlimited

                std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
                const float theta = angle(StarRng());
                pending.push_back({{loc.loc.x, loc.loc.y},
                                   spawner.spawnTextureID,
                                   {std::cos(theta) * kStarSpeed, std::sin(theta) * kStarSpeed}});
            }
        }

        for (const PendingSpawn& spawn : pending) {
            const EntityID star = entities.CreateEntity("star");

            LocationComponent starLoc{spawn.at};
            MovementComponent starMove{spawn.velocity};
            // The star's sprite reuses the spawner's texture and is drawn at the star's own box.
            SpriteComponent starSprite{spawn.textureID,
                                       {{0.0f, 0.0f}, kStarSize, kStarSize}, true};
            CollisionComponent starCollision{};
            starCollision.collisionBox = {{0.0f, 0.0f}, kStarSize, kStarSize};

            entities.template AddComponent<LocationComponent>(star, starLoc);
            entities.template AddComponent<MovementComponent>(star, starMove);
            entities.template AddComponent<SpriteComponent>(star, starSprite);
            entities.template AddComponent<CollisionComponent>(star, starCollision);
        }

        CullStars(engine, entities);

        return kContinue;
    }

private:
    // A star lives until it is collected or leaves the field. Without this, a star that bounces out
    // past the map edges is never removed, so the entity list and the O(n^2) star-vs-star pass grow
    // for the whole session. "Left the field" is measured from the camera: once a star is more than a
    // full screen away it cannot be seen, so dropping it costs nothing.
    static void CullStars(EngineView<Components...>* engine,
                          EntityManager<Components...>& entities) {
        const auto& window = engine->GetWindowManager();
        const Point2D centre = window.GetWorldCenter();
        const float scale = window.GetWorldScale();
        const Rect view = window.GetWindowBounds();
        const float reachX = view.width * scale;
        const float reachY = view.height * scale;

        std::vector<EntityID> gone;
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CollisionComponent>(id)) continue;
            if (entities.template HasComponent<InputComponent>(id)) continue;  // the player
            if (!entities.template HasComponent<MovementComponent>(id)) continue;
            if (entities.template GetComponent<CollisionComponent>(id).staticEntity) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            const Point2D loc = entities.template GetComponent<LocationComponent>(id).loc;
            if (std::abs(loc.x - centre.x) > reachX || std::abs(loc.y - centre.y) > reachY)
                gone.push_back(id);
        }

        // Removed after the scan, for the same reason spawning is: RemoveEntity can hand the slot
        // straight back to the next caller.
        for (EntityID id : gone) entities.RemoveEntity(id);
    }
};

}  // namespace ECSEngine
