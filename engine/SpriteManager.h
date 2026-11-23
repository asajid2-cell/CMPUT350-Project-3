#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// Index into the sprite vector. An id stays valid as long as the manager lives; a sprite is never
// unregistered.
using SpriteID = size_t;

// Owns the game's sprites, packed into ONE atlas texture.
//
// Every RegisterTexture copies its rectangle out of the source sheet and blits it into a single
// internal atlas image, then uploads that image to one sf::Texture. The atlas is what later lets the
// sprite system build a single vertex array and draw the whole scene in one call: a vertex array can
// only carry one texture, so every sprite has to live in the same one.
//
// Packing is a simple shelf: left to right, then a new row when the current one is full, and the
// atlas grows taller when the rows run out. This game registers a few dozen small sprites, so a
// smarter packer would save memory nobody needs; the shelf is short and predictable.
//
// Lifetime contract:
//   - GetTexture returns the atlas, owned by this manager; it stays valid until the manager is
//     destroyed. RegisterTexture refreshes it in place, so a reference already handed out stays
//     valid.
//   - GetSprite returns a reference into the sprite vector, valid only until the next
//     RegisterTexture, which can grow the vector. Use it immediately; the id is what is safe to keep.
//   - A SpriteID's rectangle inside the atlas never moves once assigned, because packing only
//     appends.
class SpriteManager {
public:
    SpriteManager();
    ~SpriteManager() = default;

    // Copies sourceRect out of the texture at texturePath into the atlas and returns the id of the
    // new sprite. Registering the same file and rectangle twice returns the existing id instead of
    // copying it again.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // The sprite for this id. Its texture is the atlas and its texture rect is the region this id
    // was packed into, so it can be positioned and drawn exactly as before.
    sf::Sprite& GetSprite(SpriteID id);

    // The one texture every sprite lives in. Handed to the sprite system so it can draw them all in
    // a single batched call.
    sf::Texture& GetTexture();

    // This sprite's rectangle inside the atlas, in texels. What a vertex-array quad maps its UVs to.
    const sf::IntRect& GetAtlasRect(SpriteID id) const;

private:
    // The atlas on the CPU side. Pixels are blitted here first because sf::Image can copy
    // rectangle-to-rectangle; the GPU copy is refreshed from it after each register.
    sf::Image mAtlasImage;
    sf::Texture mTexture;
    bool mTextureDirty = false;

    int mCursorX = 0;    // next free x in the current shelf row
    int mCursorY = 0;    // top of the current shelf row
    int mRowHeight = 0;  // tallest sprite in the current row, so the next row clears it

    std::vector<sf::Sprite> mSprites;
    std::vector<sf::IntRect> mAtlasRects;

    // Source sheets stay loaded so a later RegisterTexture from the same file does not decode it
    // again; the blit reads their pixels.
    std::unordered_map<std::string, sf::Image> mSourceImages;

    // Same file + same rect already packed: the id it got. Keeps the atlas from growing when a map
    // uses one tile ten thousand times.
    std::unordered_map<std::string, SpriteID> mPacked;

    // Places a width x height rectangle in the atlas and returns where it landed, growing the atlas
    // first if the current row is full or the sheet has no room left.
    sf::IntRect Reserve(int width, int height);

    // Uploads mAtlasImage to mTexture if anything changed since the last upload.
    void SyncTexture();
};

}  // namespace ECSEngine
