#pragma once

namespace ECSEngine {

// Straight integration: position moves by velocity. No collision here -- the collision system runs
// after this and pushes anything that ended up inside a wall back out.
template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;
            if (!entities.template HasComponent<MovementComponent>(id)) continue;

            auto& location = entities.template GetComponent<LocationComponent>(id);
            const auto& movement = entities.template GetComponent<MovementComponent>(id);

            location.loc += movement.velocity;
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
