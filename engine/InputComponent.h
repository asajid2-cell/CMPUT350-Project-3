#pragma once

#include <bitset>
#include <limits>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

// The player's input state. The two bitsets are written by the engine's event pump and read by
// InputSystem; the other fields are memory the input system needs from one frame to the next (dash
// cooldown, last wall jumped, dash burst).
struct InputComponent {
    // Defaulted so the sparse set can clear a removed slot with InputComponent{}.
    InputComponent(EntityID whichEntity = std::numeric_limits<EntityID>::max())
        : entity(whichEntity), lastDash(0), lastWallSide(0), dashTimer(0), dashDir(1.0f) {}
    EntityID entity;
    int lastDash;  // frames since the last dash, counted up to kDashInterval
    // Which wall the last wall jump came off: -1 left, +1 right, 0 none. A new wall jump has to use a
    // different wall (or come after touching the ground), so one wall cannot be climbed by mashing.
    int lastWallSide;
    // Frames left in the current dash, and the direction it is holding. While this is running the
    // horizontal speed is held at the dash speed and gravity is switched off, so the dash covers a
    // gap that a jump cannot reach.
    int dashTimer;
    float dashDir;
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
