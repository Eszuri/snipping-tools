#pragma once
#include "../core/Common.hpp"

class AnnotationShape {
public:
    virtual ~AnnotationShape() = default;
    virtual void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* baseImage = nullptr) = 0;
};

// 1. Rectangle
class RectShape : public AnnotationShape {
public:
    int x1, y1, x2, y2;
    Gdiplus::Color color;
    float strokeWidth;

    RectShape(int x1, int y1, int x2, int y2, Gdiplus::Color color, float strokeWidth)
        : x1(x1), y1(y1), x2(x2), y2(y2), color(color), strokeWidth(strokeWidth) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        int rx = (std::min)(x1, x2);
        int ry = (std::min)(y1, y2);
        int rw = std::abs(x2 - x1);
        int rh = std::abs(y2 - y1);
        if (rw <= 0 || rh <= 0) return;

        Gdiplus::Pen pen(color, strokeWidth);
        pen.SetAlignment(Gdiplus::PenAlignmentCenter);
        g.DrawRectangle(&pen, rx, ry, rw, rh);
    }
};

// 2. Ellipse
class EllipseShape : public AnnotationShape {
public:
    int x1, y1, x2, y2;
    Gdiplus::Color color;
    float strokeWidth;

    EllipseShape(int x1, int y1, int x2, int y2, Gdiplus::Color color, float strokeWidth)
        : x1(x1), y1(y1), x2(x2), y2(y2), color(color), strokeWidth(strokeWidth) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        int rx = (std::min)(x1, x2);
        int ry = (std::min)(y1, y2);
        int rw = std::abs(x2 - x1);
        int rh = std::abs(y2 - y1);
        if (rw <= 0 || rh <= 0) return;

        Gdiplus::Pen pen(color, strokeWidth);
        g.DrawEllipse(&pen, rx, ry, rw, rh);
    }
};

// 4. Pen (Freehand)
class PenShape : public AnnotationShape {
public:
    std::vector<Gdiplus::Point> points;
    Gdiplus::Color color;
    float strokeWidth;

    PenShape(Gdiplus::Color color, float strokeWidth)
        : color(color), strokeWidth(strokeWidth) {}

    void AddPoint(int x, int y) {
        points.emplace_back(x, y);
    }

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        if (points.size() < 2) return;

        Gdiplus::Pen pen(color, strokeWidth);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        for (size_t i = 0; i < points.size() - 1; ++i) {
            g.DrawLine(&pen, points[i], points[i + 1]);
        }
    }
};

