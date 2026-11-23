#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// Index into the sprite vector. An id stays valid as long as the manager lives; a sprite is never
// unregistered.
using SpriteID = size_t;

// Owns the game's textures and the sprites cut from them.
//
// Textures are cached by file path, so a sheet is decoded once no matter how many sprites use it;
// each registered sprite is an sf::Sprite that references its texture and a rectangle of it.
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    // Loads the texture at texturePath if it is not cached yet, then registers a sprite cut from
    // sourceRect of it. Returns the sprite's id. The texture stays alive for the manager's lifetime.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // The sprite for this id. The reference points into the sprite vector, so it is valid only until
    // the next RegisterTexture, which can grow (and move) the vector. Use it immediately; the id
    // itself is what is safe to keep.
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::vector<sf::Sprite> mSprites;
    using TextMap = std::unordered_map<std::string, sf::Texture>;
    TextMap mPathToTexture;
};

}  // namespace ECSEngine
