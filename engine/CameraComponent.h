#pragma once

#include <unordered_map>

#include "Entity.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// The camera's state in world coordinates. CameraSystem reads it, applies the three-zone follow,
// adds any shake and writes the result to WindowManager.
struct CameraComponent {
    Point2D position;          // world point drawn at the window centre
    float worldUnitsPerPixel;  // zoom; 1 means one world unit per pixel

    // Optional bounds on where the camera may go. Not used by the platformer yet; they exist so a
    // level can clamp the view without changing the follow logic.
    Rect hardExclusion;  // screen-space area the camera must stay out of
    Rect softExclusion;  // screen-space area the camera may enter but scrolls out of
    float speed;         // follow multiplier; 1 is the normal track
};

// Ties a camera to the entity it follows. The system looks the target up by id each frame, so the
// camera never holds a pointer that a removal could dangle.
struct CameraFollower {
    EntityID entityID;  // the entity being tracked
};

// A short camera kick used for hit feedback. The collision system writes it on the followed entity;
// CameraSystem adds offset to the camera for frameLimit frames and then lets it settle back.
struct CameraShake {
    int frameLimit;   // frames of shake left
    Point2D offset;   // world-space offset added to the camera each frame
};

}  // namespace ECSEngine
