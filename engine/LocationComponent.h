#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// The entity's base position in world space. Every other offset stored on the entity (sprite draw
// rects, collision boxes) is relative to this point, so moving this one value moves everything the
// entity owns.
struct LocationComponent {
    Point2D loc;  // top-left of the entity in world units
};

// The entity's velocity. MovementSystem adds it to the location once per frame, so the units are
// world units per frame at the game's fixed 30 frames per second, not per second.
struct MovementComponent {
    Point2D velocity;
};

// The pull on an entity. GravitySystem adds acceleration to velocity every frame, so the units are
// world units per frame squared; the game tunes it in those terms.
struct GravityComponent {
    // Defaulted so the sparse set can clear a removed slot with GravityComponent{}.
    GravityComponent(Point2D acceleration = {}, int disabledTimer = 0)
        : acceleration(acceleration), disabledTimer(disabledTimer) {}
    Point2D acceleration;  // per frame squared
    int disabledTimer;     // frames left with gravity switched off (a dash sets this)
    int wallSlideTimer = 0;  // frames left before the wall-slide sound may play again
};

// Where the entity should reappear after it dies. Kept on the player; the respawn mechanic is not
// used by this game yet.
struct RespawnComponent {
    Point2D loc;
};

}  // namespace ECSEngine
