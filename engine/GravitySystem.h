#pragma once

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "SoundManager.h"
#include "SoundNames.h"

namespace ECSEngine
{

// Applied when the player is pressed against a wall and falling. Slower than the normal pull, which
// is what produces the wall slide instead of a free fall down the face.
const float kWallSlideAcceleration = 1.0f;

// Frames between wall-slide blips while the player keeps scraping down the same wall.
const int kSlideSoundCooldown = 18;

// Gravity multiplier while the Down key is held in the air. 2.5 is enough to read as a deliberate
// dive without making the fall feel out of control.
const float kFastFallMultiplier = 2.5f;

// Falling pulls harder than rising. This is the GMTK platformer trick: the eye reads a jump as
// floaty if the arc is symmetric, and games feel responsive when the descent is quicker than the
// ascent. ApexGravityMultiplier is applied between the two.
const float kFallGravityMultiplier = 1.9f;
const float kApexGravityMultiplier = 0.8f;

// Applies acceleration to anything with both a gravity and a movement component. Gravity is not a
// constant here: the collision flags from last frame change it, which is what gives the wall slide
// and lets us carry a separate, weaker pull while sliding.
template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }

	tSystemReturn Run(EngineView<Components...> *engine) override
	{
		auto& entities = engine->GetEntityManager();

		for (const Entity& entity : entities) {
			const EntityID id = entity.GetID();
			if (!entities.IsValid(id)) continue;
			if (!entities.template HasComponent<GravityComponent>(id)) continue;
			if (!entities.template HasComponent<MovementComponent>(id)) continue;

			auto& gravity = entities.template GetComponent<GravityComponent>(id);
			auto& movement = entities.template GetComponent<MovementComponent>(id);

			// A gravity component with a timer is switched off until the timer runs out. Dash uses
			// that so it can skim horizontally without being dragged down mid-burst.
			if (gravity.disabledTimer > 0) {
				--gravity.disabledTimer;
				continue;
			}

			float acceleration = gravity.acceleration.y;

			// Asymmetric arc: hang slightly at the top, then come down harder than we went up.
			// |vy| under a small threshold is the apex, so the easing brackets the turning point.
			if (movement.velocity.y < -1.0f) {
				acceleration *= kApexGravityMultiplier;   // still rising
			} else if (movement.velocity.y > 1.0f) {
				acceleration *= kFallGravityMultiplier;   // falling
			}

			// Holding Down pulls the player down harder while airborne. The assignment asks for it
			// explicitly, and it is the fast-fall half of the jump-tuning pair.
			if (entities.template HasComponent<InputComponent>(id) &&
			    entities.template HasComponent<CollisionComponent>(id) &&
			    !entities.template GetComponent<CollisionComponent>(id).onGround) {
				const auto& input = entities.template GetComponent<InputComponent>(id);
				if (input.keydown.test(Key::Down)) acceleration *= kFastFallMultiplier;
			}

			// Wall contact while falling bleeds the fall off. Reading the flag from the previous
			// frame is deliberate: this frame's flags are not decided until CollisionSystem runs.
			if (gravity.wallSlideTimer > 0) --gravity.wallSlideTimer;

			bool hasCollision = entities.template HasComponent<CollisionComponent>(id);

			if (hasCollision) {
				const auto& collision = entities.template GetComponent<CollisionComponent>(id);
				const bool pushingWall = collision.onLeftWall || collision.onRightWall;
				if (pushingWall && movement.velocity.y > 0) {
					acceleration = kWallSlideAcceleration;

					// One blip per slide, not one per frame. The timer is the cooldown; holding the
					// wall longer than kSlideSoundCooldown repeats it, which reads as scraping.
					if (gravity.wallSlideTimer == 0) {
						auto& sounds = engine->GetSoundManager();
						if (sounds.HasSound(SoundName::kWallSlide)) {
							sounds.GetSound(SoundName::kWallSlide).play();
						}
						gravity.wallSlideTimer = kSlideSoundCooldown;
					}
				}
			}

			// A grounded entity does not accumulate fall speed. That is what keeps a resting collider
			// exactly on the floor instead of sinking and being pushed back out every frame.
			if (!hasCollision) movement.velocity.y += acceleration;
			else {
				const auto& col = entities.template GetComponent<CollisionComponent>(id);
				if (!col.onGround) movement.velocity.y += acceleration;
			}
		}

		return kContinue;
	}
};

}
