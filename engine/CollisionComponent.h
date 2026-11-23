#pragma once

#include <bitset>
#include <cstdint>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// A collider and the contacts it had last frame.
//
// The box is stored relative to the entity's LocationComponent, so the world box is the box plus
// the location. The four side flags are written by CollisionSystem during a resolve and read by the
// systems that run earlier in the next frame (input, gravity), which is why they describe the
// previous frame rather than the current one.
struct CollisionComponent {
    uint8_t staticEntity : 1;  // 1 = never moves; other colliders are resolved against it
    uint8_t doesDamage : 1;    // reserved; the platformer has no damage mechanic
    uint8_t onGround : 1;      // touched a top face last frame
    uint8_t onCeiling : 1;     // touched a bottom face last frame
    uint8_t onLeftWall : 1;    // touched a wall on the left last frame
    uint8_t onRightWall : 1;   // touched a wall on the right last frame
    Rect collisionBox;         // local box, relative to the entity location
    Rect previousCollisionBox; // world box recorded at the start of the frame
};

}  // namespace ECSEngine
