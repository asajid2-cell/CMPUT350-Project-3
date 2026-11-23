#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include "MathUtil.h"

namespace SfHelper {
    template<typename T>
    inline sf::Vector2<T>SfPnt(const ECSEngine::Point2D& pnt) {
        return {static_cast<T>(pnt.x), static_cast<T>(pnt.y)};
    }

    template <typename T>
    inline sf::Vector2<T> operator/(const sf::Vector2<T>& l,const sf::Vector2<T>& r) {
        return {l.x / r.x, l.y / r.y};
    }

    

    template <typename T>
    inline sf::Rect<T> SfRect(const ECSEngine::Rect& rect) {
        return {
            {static_cast<T>(rect.topLeft.x), static_cast<T>(rect.topLeft.y)}, 
            {static_cast<T>(rect.width), static_cast<T>(rect.height)}
        };
    }

    template <typename T>
    inline ECSEngine::Point2D PntSf(const sf::Vector2<T>& pnt) {
        return {static_cast<float>(pnt.x), static_cast<float>(pnt.y)};
    }

    template <typename T>
    inline ECSEngine::Rect RectSf(const sf::Rect<T> rec) {
        return {PntSf(rec.position), static_cast<float>(rec.size.x), static_cast<float>(rec.size.y)};
    }

};
