#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"

// Points awarded for collecting one star.
constexpr int kScorePoints = 1;

namespace ECSEngine {

// The running score and the sprites that display it. The entity carrying this component is the HUD
// holder: it has a LocationComponent for the top-left of the number and no collider, so the
// collision system never mistakes it for a star. ScoreSystem writes the digits' sprites.
class ScoreComponent {
public:
    int score;                  // points collected so far
    SpriteID digitTextureID[10]; // sprite for each digit 0..9
    EntityID digitID[3];        // the three digit entities, most significant first
};

}  // namespace ECSEngine
