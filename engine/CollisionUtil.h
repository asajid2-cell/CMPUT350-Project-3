#pragma once

#include <cmath>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"

namespace ECSEngine {

// Box helpers shared by the collision system. Everything here works in world space: a
// CollisionComponent stores its box relative to the entity's LocationComponent, so a world box is
// the local box pushed through the location.
namespace CollisionUtil {

// The collider's box in world space: the local box plus the entity location.
inline Rect WorldBox(const CollisionComponent& collision, const LocationComponent& location) {
    return {collision.collisionBox.topLeft + location.loc,
            collision.collisionBox.width, collision.collisionBox.height};
}

// True when the boxes touch or overlap. The comparisons are inclusive, so a shared edge counts as a
// collision; that is what keeps a resting collider grounded instead of sinking and being pushed out
// every frame.
inline bool Overlaps(const Rect& a, const Rect& b) {
    return a.topLeft.x <= b.topLeft.x + b.width && b.topLeft.x <= a.topLeft.x + a.width &&
           a.topLeft.y <= b.topLeft.y + b.height && b.topLeft.y <= a.topLeft.y + a.height;
}

// The signed overlap of a and b on each axis: how far a must move to leave b. The sign is the
// direction a moves (negative = left/up). Its magnitude is the amount of overlap, so the smaller
// component is the cheapest way out. The world box is pushed out by this value.
inline Point2D Penetration(const Rect& a, const Rect& b) {
    const float aCenterX = a.topLeft.x + a.width * 0.5f;
    const float aCenterY = a.topLeft.y + a.height * 0.5f;
    const float bCenterX = b.topLeft.x + b.width * 0.5f;
    const float bCenterY = b.topLeft.y + b.height * 0.5f;

    return {(aCenterX < bCenterX ? -1.0f : 1.0f) * (a.width * 0.5f + b.width * 0.5f -
                                                    std::abs(aCenterX - bCenterX)),
            (aCenterY < bCenterY ? -1.0f : 1.0f) * (a.height * 0.5f + b.height * 0.5f -
                                                    std::abs(aCenterY - bCenterY))};
}

}  // namespace CollisionUtil

}  // namespace ECSEngine
