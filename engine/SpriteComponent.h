#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// How an entity is drawn. The draw rect is relative to the entity's LocationComponent, which the
// sprite system adds in before drawing. The entity needs a LocationComponent as well; the sprite
// system asserts that.
class SpriteComponent {
public:
    SpriteID textureID;  // sprite registered with SpriteManager
    Rect bounds;         // draw rect relative to the entity location
    bool worldSpace;     // true = moved by the camera; false = drawn in screen space
};

}  // namespace ECSEngine
