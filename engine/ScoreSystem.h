#pragma once

#include "EntityManager.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {

// Turns the running score into three digit sprites in the top-left corner. The HUD entity owns the
// ScoreComponent and the three digit ids; the system only ever writes their SpriteComponent.
//
// Every pass rewrites all three digits from scratch, which is cheap (three sprite ids and a value)
// and means there is no state to get out of sync.
const float kScoreDigitSize = 32.0f;
const float kScoreDigitSpacing = 30.0f;

template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<ScoreComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;

            auto& score = entities.template GetComponent<ScoreComponent>(id);
            auto& loc = entities.template GetComponent<LocationComponent>(id);

            int value = score.score;
            for (int slot = 2; slot >= 0; --slot) {
                const int digit = value % 10;
                value /= 10;

                const EntityID digitID = score.digitID[slot];
                if (!entities.IsValid(digitID)) continue;
                if (!entities.template HasComponent<SpriteComponent>(digitID)) continue;

                auto& sprite = entities.template GetComponent<SpriteComponent>(digitID);
                sprite.textureID = score.digitTextureID[digit];
                sprite.worldSpace = false;   // screen space, so the camera transform is skipped
                sprite.bounds = {{loc.loc.x + static_cast<float>(slot) * kScoreDigitSpacing,
                                  loc.loc.y},
                                 kScoreDigitSize, kScoreDigitSize};
            }
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
