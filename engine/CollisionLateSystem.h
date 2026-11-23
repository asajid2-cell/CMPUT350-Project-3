#pragma once

#include "CollisionUtil.h"
#include "EntityManager.h"
#include "LocationComponent.h"

namespace ECSEngine {

// Runs before anything moves. It records each collider's world box as it stands at the start of
// this frame, which is where it ended last frame. CollisionSystem resolves against the new box; this
// copy is the previous one, kept so anything that needs the box a collider came from has it.
template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CollisionComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            auto& collision = entities.template GetComponent<CollisionComponent>(id);
            const auto& location = entities.template GetComponent<LocationComponent>(id);

            collision.previousCollisionBox = CollisionUtil::WorldBox(collision, location);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
