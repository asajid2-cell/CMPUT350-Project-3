#include "WindowManager.h"

#include <cstdint>

#include "SfHelper.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    auto windowSize = SfHelper::PntSf(mWindow->getSize()); 
    return {{0.0f,0.0f}, windowSize.x, windowSize.y}; 
}

// Both of these take a rect in WORLD coordinates -- the point of a debug draw is to show where the
// engine thinks something is, and colliders live in world space. The rect is converted to window
// space and drawn immediately, so call them from a system after the sprite pass or they will be
// covered by whatever draws next.
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    const Rect onScreen = WorldToWindow(r);

    sf::RectangleShape shape({onScreen.width, onScreen.height});
    shape.setPosition({onScreen.topLeft.x, onScreen.topLeft.y});
    shape.setFillColor(sf::Color(static_cast<uint8_t>(red), static_cast<uint8_t>(green),
                                 static_cast<uint8_t>(blue), static_cast<uint8_t>(alpha)));
    mWindow->draw(shape);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    const Rect onScreen = WorldToWindow(r);

    sf::RectangleShape shape({onScreen.width, onScreen.height});
    shape.setPosition({onScreen.topLeft.x, onScreen.topLeft.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(static_cast<uint8_t>(red), static_cast<uint8_t>(green),
                                    static_cast<uint8_t>(blue), static_cast<uint8_t>(alpha)));
    shape.setOutlineThickness(1.0f);
    mWindow->draw(shape);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const { 
    return {WindowToWorld(rect.topLeft), WindowToWorld(rect.width), WindowToWorld(rect.height)}; }

Point2D WindowManager::WindowToWorld(const Point2D& pt) const { 
    return mViewCenterWorld - ((windowSize() * 0.5f) * mWorldUnitsPerPixel) +
               (pt * mWorldUnitsPerPixel); }

Rect WindowManager::WorldToWindow(const Rect& rect) const { 
    return {WorldToWindow(rect.topLeft), WorldToWindow(rect.width), WorldToWindow(rect.height)}; }

Point2D WindowManager::WorldToWindow(const Point2D& pt) const { return (pt - (mViewCenterWorld - ((windowSize() * 0.5f) * mWorldUnitsPerPixel))) /
               mWorldUnitsPerPixel; }

void WindowManager::SetCamera(const Point2D& pWorld, const Point2D& pWindow) {
    mViewCenterWorld =
            pWorld - pWindow * mWorldUnitsPerPixel + windowSize() * 0.5f * mWorldUnitsPerPixel;
}

Point2D WindowManager::GetWorldCenter() const { return mViewCenterWorld; }

float WindowManager::GetWorldScale() const { return mWorldUnitsPerPixel; }

void WindowManager::SetWorldCenter(const Point2D& worldCenter) { mViewCenterWorld = worldCenter;}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    // Map the rect's width onto the window's width, then put its centre at the window centre.
    const Vector2D size = windowSize();
    if (size.x > 0.0f) mWorldUnitsPerPixel = worldCameraRect.width / size.x;
    mViewCenterWorld = worldCameraRect.center();
}

float WindowManager::WindowToWorld(float x) const { return x * mWorldUnitsPerPixel; }

float WindowManager::WorldToWindow(float x) const { return x / mWorldUnitsPerPixel; }

Vector2D WindowManager::windowSize() const {
    return {GetWindowBounds().width, GetWindowBounds().height};
}

}  // namespace ECSEngine
