#pragma once

#include "CameraComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "WindowManager.h"

namespace ECSEngine {

// The dead zone, as fractions of the window width.
const float kCameraDeadZoneInner = 0.30f;
const float kCameraDeadZoneOuter = 0.10f;
const float kCameraTween = 0.05f;

// Fixed in y, follows in x. Three zones, straight from the assignment:

//   - player in the middle 40%          -> camera holds still
//   - player in the next 20% either side -> camera eases, so it feels like a slow track
//   - player in the outer 10%           -> camera follows directly, so they never reach the edge

// Shake is added on top and decays over its frame budget.

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CameraSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        auto& window = engine->GetWindowManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CameraComponent>(id)) continue;
            if (!entities.template HasComponent<CameraFollower>(id)) continue;

            auto& camera = entities.template GetComponent<CameraComponent>(id);
            const EntityID target = entities.template GetComponent<CameraFollower>(id).entityID;

            if (entities.IsValid(target) && entities.template HasComponent<LocationComponent>(target)) {
                const float playerX = entities.template GetComponent<LocationComponent>(target).loc.x;
                // The bounds are pixels; the camera works in world units. Convert before mixing them,
                // so a world scale other than 1 does not silently break the zones.
                const float width = window.GetWindowBounds().width * window.GetWorldScale();
                const float centre = camera.position.x;

                // Where the player sits across the window as a fraction, 0 = left edge.
                //   centre band  [0.30, 0.70]  -> no camera movement
                //   tween band   [0.10, 0.30) and (0.70, 0.90] -> ease toward the player
                //   outer band   < 0.10 or > 0.90 -> follow directly
                const float relative = (playerX - centre) / width + 0.5f;

                if (relative < kCameraDeadZoneOuter || relative > 1.0f - kCameraDeadZoneOuter) {
                    // Outer band: put the player exactly on the outer boundary so they never
                    // reach the edge of the window.
                    camera.position.x = relative < 0.5f
                                            ? playerX + (0.5f - kCameraDeadZoneOuter) * width
                                            : playerX - (0.5f - kCameraDeadZoneOuter) * width;
                } else if (relative < kCameraDeadZoneInner || relative > 1.0f - kCameraDeadZoneInner) {
                    // Tween band: drift toward the player instead of snapping.
                    camera.position.x += (playerX - centre) * kCameraTween;
                }
            }

            // Shake rides on top of the settled position and never feeds back into it, so it decays
            // to exactly the tracked centre. The shake lives on whatever the collision system hit,
            // which is the followed entity, not the camera itself.
            Point2D shaken = camera.position;
            if (entities.IsValid(target) && entities.template HasComponent<CameraShake>(target)) {
                auto& shake = entities.template GetComponent<CameraShake>(target);
                if (shake.frameLimit > 0) {
                    --shake.frameLimit;
                    shaken += shake.offset;
                }
            }

            window.SetWorldCenter(shaken);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
