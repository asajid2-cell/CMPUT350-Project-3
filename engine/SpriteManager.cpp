#include "SpriteManager.h"
#include "SfHelper.h"
#include <cassert>
#include <unordered_map>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    
    auto getTexture = [&texturePath](TextMap& mPathToTexture) -> sf::Texture& {
        if (mPathToTexture.find(texturePath) == mPathToTexture.end()) {
            sf::Texture& texture = mPathToTexture[texturePath] = sf::Texture();
            assert(texture.loadFromFile(texturePath));
            return texture;
        } else return mPathToTexture[texturePath];
    };


    mSprites.push_back(sf::Sprite{getTexture(mPathToTexture), SfHelper::SfRect<int>(sourceRect)});
    return mSprites.size() - 1;
    
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    return mSprites[id];
}

}  // namespace ECSEngine
