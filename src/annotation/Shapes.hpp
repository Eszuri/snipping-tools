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

// 3. Arrow
class ArrowShape : public AnnotationShape {
public:
    int x1, y1, x2, y2;
    Gdiplus::Color color;
    float strokeWidth;

    ArrowShape(int x1, int y1, int x2, int y2, Gdiplus::Color color, float strokeWidth)
        : x1(x1), y1(y1), x2(x2), y2(y2), color(color), strokeWidth(strokeWidth) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        float dx = (float)(x2 - x1);
        float dy = (float)(y2 - y1);
        float length = std::sqrt(dx * dx + dy * dy);
        if (length < 2.0f) return;

        Gdiplus::Pen pen(color, strokeWidth);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        g.DrawLine(&pen, (INT)x1, (INT)y1, (INT)x2, (INT)y2);

        // Arrow head
        float headLength = (std::max)(12.0f, strokeWidth * 4.0f);
        float headAngle = 0.5f; // ~28 degrees

        float angle = std::atan2(dy, dx);
        float ax1 = x2 - headLength * std::cos(angle - headAngle);
        float ay1 = y2 - headLength * std::sin(angle - headAngle);
        float ax2 = x2 - headLength * std::cos(angle + headAngle);
        float ay2 = y2 - headLength * std::sin(angle + headAngle);

        Gdiplus::PointF points[3] = {
            Gdiplus::PointF((float)x2, (float)y2),
            Gdiplus::PointF(ax1, ay1),
            Gdiplus::PointF(ax2, ay2)
        };

        Gdiplus::SolidBrush brush(color);
        g.FillPolygon(&brush, points, 3);
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

// 5. Highlighter
class HighlighterShape : public AnnotationShape {
public:
    std::vector<Gdiplus::Point> points;
    Gdiplus::Color color;
    float strokeWidth;

    HighlighterShape(Gdiplus::Color color, float strokeWidth)
        : color(Gdiplus::Color(100, color.GetR(), color.GetG(), color.GetB())), strokeWidth(strokeWidth * 3.5f) {}

    void AddPoint(int x, int y) {
        points.emplace_back(x, y);
    }

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        if (points.size() < 2) return;

        Gdiplus::Pen pen(color, strokeWidth);
        pen.SetStartCap(Gdiplus::LineCapSquare);
        pen.SetEndCap(Gdiplus::LineCapSquare);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        for (size_t i = 0; i < points.size() - 1; ++i) {
            g.DrawLine(&pen, points[i], points[i + 1]);
        }
    }
};

// 6. Step Badge (1, 2, 3...)
class StepBadgeShape : public AnnotationShape {
public:
    int cx, cy;
    int stepNumber;
    Gdiplus::Color bgColor;
    float radius;

    StepBadgeShape(int cx, int cy, int stepNumber, Gdiplus::Color bgColor, float radius = 14.0f)
        : cx(cx), cy(cy), stepNumber(stepNumber), bgColor(bgColor), radius(radius) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        Gdiplus::SolidBrush brush(bgColor);
        g.FillEllipse(&brush, cx - radius, cy - radius, radius * 2, radius * 2);

        // White border
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
        g.DrawEllipse(&borderPen, cx - radius, cy - radius, radius * 2, radius * 2);

        // Text inside badge
        std::wstring text = std::to_wstring(stepNumber);
        Gdiplus::Font font(L"Segoe UI", radius * 0.95f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        Gdiplus::RectF layoutRect((float)(cx - radius), (float)(cy - radius), radius * 2.0f, radius * 2.0f);
        g.DrawString(text.c_str(), -1, &font, layoutRect, &sf, &textBrush);
    }
};

// 7. Pixelate / Blur Filter (Mosaic Redaction)
class PixelateShape : public AnnotationShape {
public:
    int x1, y1, x2, y2;
    int blockSize;

    PixelateShape(int x1, int y1, int x2, int y2, int blockSize = 10)
        : x1(x1), y1(y1), x2(x2), y2(y2), blockSize(blockSize) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* baseImage = nullptr) override {
        if (!baseImage) return;

        int rx = (std::min)(x1, x2);
        int ry = (std::min)(y1, y2);
        int rw = std::abs(x2 - x1);
        int rh = std::abs(y2 - y1);
        if (rw <= 0 || rh <= 0) return;

        int imgW = baseImage->GetWidth();
        int imgH = baseImage->GetHeight();

        rx = (std::max)(0, (std::min)(rx, imgW - 1));
        ry = (std::max)(0, (std::min)(ry, imgH - 1));
        rw = (std::min)(rw, imgW - rx);
        rh = (std::min)(rh, imgH - ry);

        if (rw <= 0 || rh <= 0) return;

        Gdiplus::Rect rect(rx, ry, rw, rh);
        Gdiplus::BitmapData bmpData;
        if (baseImage->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmpData) != Gdiplus::Ok) {
            return;
        }

        int bSize = (std::max)(4, blockSize);

        for (int by = 0; by < rh; by += bSize) {
            for (int bx = 0; bx < rw; bx += bSize) {
                int bw = (std::min)(bSize, rw - bx);
                int bh = (std::min)(bSize, rh - by);

                // Sample center pixel
                int sampleX = bx + bw / 2;
                int sampleY = by + bh / 2;

                BYTE* samplePixel = (BYTE*)bmpData.Scan0 + sampleY * bmpData.Stride + sampleX * 4;
                BYTE b = samplePixel[0];
                BYTE gVal = samplePixel[1];
                BYTE r = samplePixel[2];
                BYTE a = samplePixel[3];

                // Fill block with sample
                for (int y = by; y < by + bh; ++y) {
                    BYTE* row = (BYTE*)bmpData.Scan0 + y * bmpData.Stride;
                    for (int x = bx; x < bx + bw; ++x) {
                        row[x * 4 + 0] = b;
                        row[x * 4 + 1] = gVal;
                        row[x * 4 + 2] = r;
                        row[x * 4 + 3] = a;
                    }
                }
            }
        }

        baseImage->UnlockBits(&bmpData);

        // Draw the pixelated section
        g.DrawImage(baseImage, rx, ry, rx, ry, rw, rh, Gdiplus::UnitPixel);
    }
};

// 8. Text Shape
class TextShape : public AnnotationShape {
public:
    int x, y;
    std::wstring text;
    Gdiplus::Color color;
    float fontSize;

    TextShape(int x, int y, const std::wstring& text, Gdiplus::Color color, float fontSize = 16.0f)
        : x(x), y(y), text(text), color(color), fontSize(fontSize) {}

    void Draw(Gdiplus::Graphics& g, Gdiplus::Bitmap* = nullptr) override {
        if (text.empty()) return;

        Gdiplus::Font font(L"Segoe UI", fontSize, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush textBrush(color);
        Gdiplus::PointF origin((float)x, (float)y);
        g.DrawString(text.c_str(), -1, &font, origin, &textBrush);
    }
};
