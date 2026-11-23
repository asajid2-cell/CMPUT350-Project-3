#pragma once

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {
    using Vector2D = Point2D;

// Owns the render window and the world/screen mapping.
//
// The view is a world-space centre plus a scale in world units per pixel. Every world point maps to
// the window through that pair, which is how the camera works: moving the centre scrolls the world,
// changing the scale zooms it. Screen-space drawing (the HUD) skips the conversion.
class WindowManager {
public:
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    ~WindowManager() = default;

    // The window this manager owns. The shared_ptr may be copied but the manager keeps ownership.
    std::shared_ptr<sf::RenderWindow> GetWindow() const;

    // The window's size in pixels, with its top-left at (0, 0).
    Rect GetWindowBounds() const;

    // Puts worldPt at screenPt by moving the view centre.
    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);

    // Sets the world point that is drawn at the centre of the window.
    void SetWorldCenter(const Point2D& worldCenter);

    // Fits worldCameraRect to the window: its centre becomes the view centre and its width sets the
    // scale. Height follows the window's aspect ratio.
    void SetWorldRect(const Rect& worldCameraRect);

    // Sets how many world units one pixel covers (1 = no zoom).
    void SetWorldScale(float worldUnitsPerPixel);

    Point2D GetWorldCenter() const;
    float GetWorldScale() const;

    // Conversions between world and window coordinates. The scalar forms scale a length; the point
    // and rect forms also apply the camera centre.
    float WindowToWorld(float x) const;
    float WorldToWindow(float x) const;
    Rect WindowToWorld(const Rect& rect) const;
    Rect WorldToWindow(const Rect& rect) const;
    Point2D WorldToWindow(const Point2D& pt) const;
    Point2D WindowToWorld(const Point2D& pt) const;

    // Debug helpers. Both take a world-space rect and draw it immediately, so call them after the
    // sprite pass or the sprites will be drawn over them.
    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    Vector2D windowSize() const;  // size of the window in screen pixels (window coordinates)
    Vector2D mViewCenterWorld;    // world point drawn at the centre of the window
    float mWorldUnitsPerPixel;
};

}  // namespace ECSEngine
