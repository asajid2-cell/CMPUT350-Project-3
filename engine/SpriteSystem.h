#pragma once

#include <bitset>
#include <cassert>
#include <unordered_map>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "ECSEngine.h"
#include "SfHelper.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "LocationComponent.h"
#include "WindowManager.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

namespace ECSEngine {

// Draws a red box around every sprite. A debugging aid only -- scaffolding, so it is off in the
// submitted build and flipped on from here when a sprite lands somewhere unexpected.
constexpr bool kDebugSpriteBounds = false;

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {

      auto& entities = engine->GetEntityManager();

      const WindowManager& windowManager = engine->GetWindowManager();
      auto& spriteManager = engine->GetSpriteManager();

      for (const Entity& entity : entities) {
        if (!entities.template HasComponent<SpriteComponent>(entity.GetID())) continue; 

        SpriteComponent& spriteComp = entities.template GetComponent<SpriteComponent>(entity.GetID());

        sf::Sprite& sprite = spriteManager.GetSprite(spriteComp.textureID);
        sf::Vector2f finalPosition{};
        sf::Vector2f finalScale{};

        assert(entities.template HasComponent<LocationComponent>(entity.GetID()));
        LocationComponent& curLocComp = entities.template GetComponent<LocationComponent>(entity.GetID());

        if (spriteComp.worldSpace) { // spriteComp.bounds is in world space
          auto finalRectS = spriteComp.bounds;
          finalRectS.topLeft += curLocComp.loc;
          finalRectS = windowManager.WorldToWindow(finalRectS);

          using namespace SfHelper;
          finalPosition = SfHelper::SfPnt<float>(finalRectS.topLeft);
          finalScale = sf::Vector2f{finalRectS.width, finalRectS.height} / static_cast<sf::Vector2f>(sprite.getTextureRect().size);




        } else {
          finalPosition = SfHelper::SfPnt<float>(spriteComp.bounds.topLeft + curLocComp.loc);
          using namespace SfHelper;
          finalScale = sf::Vector2{spriteComp.bounds.width, spriteComp.bounds.height} / static_cast<sf::Vector2f>(sprite.getTextureRect().size);

        }
        
        sprite.setPosition(finalPosition);
        sprite.setScale(finalScale);

        windowManager.GetWindow()->draw(sprite);

        // Debug aid: outline every sprite with the box the engine thinks it occupies. Off by
        // default because it is scaffolding, not part of the game -- flip it on when a sprite is
        // drawn somewhere you did not expect.
        if constexpr (kDebugSpriteBounds) {
          const sf::FloatRect rec = sprite.getGlobalBounds();
          sf::RectangleShape outline(rec.size);
          outline.setPosition(rec.position);
          outline.setFillColor(sf::Color::Transparent);
          outline.setOutlineColor(sf::Color::Red);
          outline.setOutlineThickness(5.0f);
          windowManager.GetWindow()->draw(outline);
        }
      }

      windowManager.GetWindow()->display();

      return kContinue; 
    }
};



}  // namespace ECSEngine
