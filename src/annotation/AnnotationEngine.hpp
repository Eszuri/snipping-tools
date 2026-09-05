#pragma once
#include "../core/Common.hpp"
#include "Shapes.hpp"
#include "../core/AppConfig.hpp"

class AnnotationEngine {
private:
    std::vector<std::unique_ptr<AnnotationShape>> shapes;
    std::vector<std::unique_ptr<AnnotationShape>> redoStack;
    std::unique_ptr<AnnotationShape> activeShape;

public:
    AnnotationEngine() = default;

    void SetActiveShape(std::unique_ptr<AnnotationShape> shape) {
        activeShape = std::move(shape);
    }

    AnnotationShape* GetActiveShape() {
        return activeShape.get();
    }

    void CommitActiveShape() {
        if (activeShape) {
            shapes.push_back(std::move(activeShape));
            redoStack.clear(); // Clear redo on new action
        }
    }

    void CancelActiveShape() {
        activeShape.reset();
    }

    void AddShape(std::unique_ptr<AnnotationShape> shape) {
        if (shape) {
            shapes.push_back(std::move(shape));
            redoStack.clear();
        }
    }

    bool CanUndo() const {
        return !shapes.empty();
    }

    bool CanRedo() const {
        return !redoStack.empty();
    }

    void Undo() {
        if (!shapes.empty()) {
            redoStack.push_back(std::move(shapes.back()));
            shapes.pop_back();
        }
    }

    void Redo() {
        if (!redoStack.empty()) {
            shapes.push_back(std::move(redoStack.back()));
            redoStack.pop_back();
        }
    }

    void Clear() {
        shapes.clear();
        redoStack.clear();
        activeShape.reset();
    }

    void RenderAll(Gdiplus::Graphics& g, Gdiplus::Bitmap* baseImage = nullptr) {
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        for (const auto& shape : shapes) {
            if (shape) {
                shape->Draw(g, baseImage);
            }
        }

        if (activeShape) {
            activeShape->Draw(g, baseImage);
        }
    }

    size_t GetShapeCount() const {
        return shapes.size();
    }
};
