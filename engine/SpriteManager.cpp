#include "SpriteManager.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "SfHelper.h"

namespace ECSEngine {

namespace {
// The atlas starts at a power of two and grows by doubling its height when the rows run out.
constexpr unsigned kAtlasStartWidth = 1024;
constexpr unsigned kAtlasStartHeight = 1024;

// Source rects are floats in the map format; the atlas needs whole texels.
int Texels(float value) { return static_cast<int>(value + 0.5f); }
}  // namespace

SpriteManager::SpriteManager() {
    mAtlasImage = sf::Image({kAtlasStartWidth, kAtlasStartHeight}, sf::Color::Transparent);
}

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    const int width = Texels(sourceRect.width);
    const int height = Texels(sourceRect.height);
    assert(width > 0 && height > 0 && "SpriteManager::RegisterTexture got an empty source rect");

    const int sourceX = Texels(sourceRect.topLeft.x);
    const int sourceY = Texels(sourceRect.topLeft.y);

    // The same region of the same sheet is only ever copied once.
    const std::string key = texturePath + "|" + std::to_string(sourceX) + "," +
                            std::to_string(sourceY) + "," + std::to_string(width) + "," +
                            std::to_string(height);
    const auto packed = mPacked.find(key);
    if (packed != mPacked.end()) return packed->second;

    // Decode the sheet on first use and keep it, so a map with 30 tiles from one sheet loads it once.
    auto source = mSourceImages.find(texturePath);
    if (source == mSourceImages.end()) {
        sf::Image image;
        const bool loaded = image.loadFromFile(texturePath);
        assert(loaded && "SpriteManager::RegisterTexture could not load the file");
        (void)loaded;
        source = mSourceImages.emplace(texturePath, std::move(image)).first;
    }

    // Copy this rectangle into the atlas. sf::Image::copy clips the source rect to the image, so a
    // rect that runs past the sheet edge packs what exists rather than reading out of bounds.
    const sf::IntRect slot = Reserve(width, height);
    const sf::IntRect sourceRectTexels({sourceX, sourceY}, {width, height});
    const sf::Vector2u dest(static_cast<unsigned>(slot.position.x),
                            static_cast<unsigned>(slot.position.y));
    const bool copied = mAtlasImage.copy(source->second, dest, sourceRectTexels);
    assert(copied && "SpriteManager::RegisterTexture could not blit into the atlas");
    (void)copied;

    // The sprite points at the atlas and at just this slot, so callers keep using it exactly as they
    // did when each sprite had its own texture.
    mSprites.emplace_back(mTexture, slot);
    mAtlasRects.push_back(slot);

    const SpriteID id = mSprites.size() - 1;
    mPacked.emplace(key, id);

    // Refreshed on the next read rather than here, so registering a whole map uploads once.
    mTextureDirty = true;
    return id;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    SyncTexture();
    assert(id < mSprites.size() && "SpriteManager::GetSprite: id was never registered");
    return mSprites[id];
}

sf::Texture& SpriteManager::GetTexture() {
    SyncTexture();
    return mTexture;
}

const sf::IntRect& SpriteManager::GetAtlasRect(SpriteID id) const {
    assert(id < mAtlasRects.size() && "SpriteManager::GetAtlasRect: id was never registered");
    return mAtlasRects[id];
}

sf::IntRect SpriteManager::Reserve(int width, int height) {
    const sf::Vector2u atlasSize = mAtlasImage.getSize();
    const int atlasWidth = static_cast<int>(atlasSize.x);
    const int atlasHeight = static_cast<int>(atlasSize.y);

    // A sprite wider than the atlas cannot be packed; widen the atlas to fit it and start over.
    if (width > atlasWidth) {
        sf::Image wider({static_cast<unsigned>(width), static_cast<unsigned>(atlasHeight)},
                        sf::Color::Transparent);
        const bool kept = wider.copy(mAtlasImage, {0, 0});
        assert(kept && "SpriteManager::Reserve could not carry the atlas across a resize");
        (void)kept;
        mAtlasImage = std::move(wider);
        mCursorX = 0;
        mCursorY = 0;
        mRowHeight = 0;
        return Reserve(width, height);
    }

    // No room left in the current row: start a new one under the tallest sprite in this row.
    if (mCursorX + width > atlasWidth) {
        mCursorX = 0;
        mCursorY += mRowHeight;
        mRowHeight = 0;
    }

    // Rows have run past the bottom: double the height and try again.
    if (mCursorY + height > atlasHeight) {
        sf::Image taller({atlasSize.x, static_cast<unsigned>(atlasHeight * 2)},
                         sf::Color::Transparent);
        const bool kept = taller.copy(mAtlasImage, {0, 0});
        assert(kept && "SpriteManager::Reserve could not carry the atlas across a resize");
        (void)kept;
        mAtlasImage = std::move(taller);
        return Reserve(width, height);
    }

    const sf::IntRect slot({mCursorX, mCursorY}, {width, height});
    mCursorX += width;
    mRowHeight = std::max(mRowHeight, height);
    return slot;
}

void SpriteManager::SyncTexture() {
    if (!mTextureDirty) return;

    // loadFromImage sizes the texture to the atlas and uploads it in one go. Only called when
    // something new was packed, so a frame that only draws does not pay for it.
    const bool uploaded = mTexture.loadFromImage(mAtlasImage);
    assert(uploaded && "SpriteManager::SyncTexture could not upload the atlas");
    (void)uploaded;

    // The sprites were built against this same texture object, so its contents changing is enough --
    // there is nothing per-sprite to rebuild.
    mTextureDirty = false;
}

}  // namespace ECSEngine
