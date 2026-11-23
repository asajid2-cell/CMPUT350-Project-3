#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <cassert>
#include <iostream>

namespace ECSEngine {

// A two-dimensional point stored in engine world coordinates.
struct Point2D {
    float x, y;

    // Builds a point from floating-point coordinates.
    Point2D(float x = 0, float y = 0) : x(x), y(y) {}

    // Builds a point by converting integer coordinates to floats.
    Point2D(int x, int y) : x(static_cast<float>(x)), y(static_cast<float>(y)) {}

    // Adds other to this point and returns this point.
    inline Point2D& operator+=(const Point2D& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // Returns the component-wise sum of this point and other.
    inline Point2D operator+(const Point2D& other) const {
        return Point2D{x + other.x, y + other.y};
    }

    // Returns this point scaled by f.
    inline Point2D operator*(const float& f) const {
        return {x * f, y * f};
    }

    // Returns the component-wise difference between this point and other.
    inline Point2D operator-(const Point2D& other) const {
        return {x - other.x, y - other.y};
    }

    // Returns this point divided by non-zero f.
    inline Point2D operator/(const float& f) const {
        assert(f != 0.0f);
        return {x / f, y / f};
    }
};

// Writes p as (x, y) and returns os.
[[maybe_unused]] inline std::ostream& operator<<(std::ostream& os, const Point2D& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

// Returns rhs scaled by number.
[[maybe_unused]] inline Point2D operator*(float number, const Point2D& rhs) {
    return Point2D(number * rhs.x, number * rhs.y);
}

// A line segment between p1 and p2.
struct Line {
    Point2D p1, p2;

    // Builds a line segment from two points or zero-valued defaults.
    Line(Point2D p1 = {0, 0}, Point2D p2 = {0, 0}) : p1(p1), p2(p2) {}

    // Builds a line segment from its endpoint coordinates.
    Line(float x1, float y1, float x2, float y2) : p1(x1, y1), p2(x2, y2) {}
};

// Writes l in a compact diagnostic form and returns os.
[[maybe_unused]] inline std::ostream& operator<<(std::ostream& os, const Line& l) {
    os << "L{" << l.p1 << ", " << l.p2 << "}";
    return os;
}

// A circle described by its center and radius.
struct Circle {
    Point2D center;
    float radius;

    // Builds a circle from a center and radius or zero-valued defaults.
    Circle(Point2D c = {0, 0}, float r = 0) : center(c), radius(r) {}

    // Builds a circle from center coordinates and radius.
    Circle(float x, float y, float r) : center(x, y), radius(r) {}
};

// An axis-aligned rectangle described by its top-left corner, width, and height.
struct Rect {
    Point2D topLeft;
    float width, height;

    // Returns the geometric center of this rectangle.
    inline Point2D center() const {
        return {topLeft.x + 0.5f * width, topLeft.y + 0.5f * height};
    }
};

// Writes r in a compact diagnostic form and returns os.
[[maybe_unused]] inline std::ostream& operator<<(std::ostream& os, const Rect& r) {
    os << "R{" << r.topLeft << ", " << r.width << "x" << r.height << "}";
    return os;
}

}  // namespace ECSEngine

#endif  // MATHUTIL_H
