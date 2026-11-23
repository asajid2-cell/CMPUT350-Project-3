#pragma once

#include <algorithm>

#include "CollisionComponent.h"
#include "Keyboard.h"
#include "SoundManager.h"
#include "SoundNames.h"

namespace ECSEngine {

// Speeds are in world units per frame at the fixed step the systems use, not per second.
//
// Horizontal movement is a ramp, not a snap: the player accelerates toward kMaxRunVelocity and the
// system only clamps the result. These are the tuning constants the assignment lets us choose, so
// they sit together in one block instead of being scattered through the system body.
const float kMaxRunVelocity = 15;
// Slower than the run, and slower than the dash. Capping the air speed separately is what keeps a
// wide gap out of jump range: the jump arc is unchanged but it does not carry as far.
const float kMaxAirVelocity = 8;
const float kDashVelocity = 50;
const float kJumpVelocity = 15;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 30;  // frames; the game steps at 30 fps, so this is once a second

// How many frames one dash lasts. At the dash speed this carries about six tiles, which is enough
// for the wide spike corridor but further than any jump reaches.
const int kDashFrames = 8;

// A wall jump throws the player up and away from the wall at the same time. The sideways part is
// what makes it possible to climb between two facing walls, which the first spike corridor needs.
const float kWallJumpPushX = 12.0f;

// Releasing jump while still rising cuts the rise to this fraction, so a tap is a hop and a hold is
// a full jump. Not zero: a dead stop feels like the game ate the input.
const float kJumpCutMultiplier = 0.35f;

// One frame of the player's intent, read out of the input bitset. The system builds this so the
// behaviour reads in words instead of bit indices.
struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool jump = false;
    bool dash = false;
};

// Stand-in for an entity that has no collision component, so the rest of the system can read the
// contact flags without a branch at every use.
inline const CollisionComponent sNoCollision{};

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;                        // skip the gaps removals leave
            if (!entities.template HasComponent<InputComponent>(id)) continue;
            if (!entities.template HasComponent<MovementComponent>(id)) continue;

            auto& input = entities.template GetComponent<InputComponent>(id);
            auto& movement = entities.template GetComponent<MovementComponent>(id);

            const InputState state = ReadInput(input);

            const auto& collision = entities.template HasComponent<CollisionComponent>(id)
                                        ? entities.template GetComponent<CollisionComponent>(id)
                                        : sNoCollision;
            const bool onGround = collision.onGround;
            const float accel = onGround ? kGroundRunAcceleration : kAirRunAcceleration;

            // Touching the ground clears the no-repeat wall-jump rule, so the next wall is fair game.
            if (onGround) input.lastWallSide = 0;

            float& vx = movement.velocity.x;

            if (input.dashTimer > 0) {
                // Mid-dash: hold the burst speed and ignore the arrow keys. Gravity is switched off
                // for the same frames, so the dash is a straight lunge rather than an arc.
                --input.dashTimer;
                vx = input.dashDir * kDashVelocity;
                ++input.lastDash;
            } else {
                // Arrow keys drive horizontal velocity whether or not we are airborne. Letting go
                // bleeds the speed off instead of stopping dead, which is what makes the movement
                // feel like a ramp rather than a toggle. Air speed is capped lower than ground
                // speed, so a jump covers less ground than the run that launched it.
                const float maxSpeed = onGround ? kMaxRunVelocity : kMaxAirVelocity;
                if (state.left && !state.right) {
                    vx = std::max(vx - accel, -maxSpeed);
                } else if (state.right && !state.left) {
                    vx = std::min(vx + accel, maxSpeed);
                } else if (vx > 0) {
                    vx = std::max(vx - accel, 0.0f);
                } else if (vx < 0) {
                    vx = std::min(vx + accel, 0.0f);
                }

                // Dash is edge triggered -- the key must go down this frame and not last frame --
                // and rate limited so it can only fire once per kDashInterval frames.
                const bool dashPressed = state.dash && !input.lastKeydown.test(Key::C);
                if (dashPressed && input.lastDash >= kDashInterval) {
                    input.dashDir = vx < 0.0f ? -1.0f : 1.0f;
                    input.dashTimer = kDashFrames;
                    vx = input.dashDir * kDashVelocity;
                    movement.velocity.y = 0.0f;
                    if (entities.template HasComponent<GravityComponent>(id))
                        entities.template GetComponent<GravityComponent>(id).disabledTimer = kDashFrames;
                    input.lastDash = 0;
                    Play(engine, SoundName::kDash);
                } else {
                    ++input.lastDash;
                }
            }

            // Jump is edge triggered. Three cases, all reading last frame's contact flags because
            // this frame's are not decided until the collision system runs:
            //   on the ground              -> a normal jump
            //   against a wall, airborne   -> a wall jump, up and away from that wall
            //   in the air, nothing there  -> no jump at all
            const bool jumpPressed = state.jump && !input.lastKeydown.test(Key::Z);
            if (jumpPressed && onGround) {
                movement.velocity.y = -kJumpVelocity;
                input.lastWallSide = 0;
                Play(engine, SoundName::kJump);
            } else if (jumpPressed && (collision.onLeftWall || collision.onRightWall)) {
                // Push away from whichever side we are touching. If both flags are set the shaft is
                // narrow enough that we touch both walls at once; the arrow key the player holds
                // picks the direction out, and with neither held we push off the opposite wall from
                // the last jump so mashing still alternates instead of climbing one side.
                float away = 0.0f;
                if (collision.onLeftWall && collision.onRightWall) {
                    if (state.right) away = 1.0f;
                    else if (state.left) away = -1.0f;
                    else away = input.lastWallSide == 0 ? -1.0f : -static_cast<float>(input.lastWallSide);
                } else if (collision.onLeftWall) {
                    away = 1.0f;
                } else {
                    away = -1.0f;
                }

                // The wall being pushed off: pushing right means the wall was on the left.
                const int side = (away > 0.0f) ? -1 : 1;
                // A wall jump has to alternate surfaces (or follow a landing). Otherwise one wall can
                // be climbed by mashing jump, which makes the dash-only corridor passable by jumping.
                if (input.lastWallSide != side) {
                    movement.velocity.y = -kJumpVelocity;
                    movement.velocity.x = away * kWallJumpPushX;
                    input.lastWallSide = side;
                    Play(engine, SoundName::kJump);
                }
            }

            // Jump-cut: letting go of jump while still rising kills most of the rise. This is what
            // turns one jump button into a range of jump heights -- a tap clears a tile, a hold
            // clears four.
            const bool jumpReleased = !state.jump && input.lastKeydown.test(Key::Z);
            if (jumpReleased && movement.velocity.y < 0) {
                movement.velocity.y *= kJumpCutMultiplier;
            }

            input.lastKeydown = input.keydown;
        }

        return kContinue;
    }

private:
    // Play a registered sound if the game set it up. Guarding on HasSound keeps the engine usable
    // without audio: a test harness that never registers sounds still runs, it just stays silent.
    static void Play(EngineView<Components...>* engine, const char* name) {
        auto& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) sounds.GetSound(name).play();
    }

    static InputState ReadInput(const InputComponent& input) {
        InputState s;
        s.left  = input.keydown.test(Key::Left);
        s.right = input.keydown.test(Key::Right);
        s.up    = input.keydown.test(Key::Up);
        s.down  = input.keydown.test(Key::Down);
        s.jump  = input.keydown.test(Key::Z);
        s.dash  = input.keydown.test(Key::C);
        return s;
    }
};

}  // namespace ECSEngine
