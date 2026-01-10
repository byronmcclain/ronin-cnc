// src/graphics/dirty_rect.cpp
// Dirty Rectangle Implementation
// Task 18h - Performance Optimization

#include "graphics/dirty_rect.h"
#include <algorithm>
#include <utility>

//=============================================================================
// DirtyRectTracker Implementation
//=============================================================================

DirtyRectTracker::DirtyRectTracker(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height) {
    dirty_rects_.reserve(100);
    merged_rects_.reserve(max_rects_);
}

void DirtyRectTracker::mark_dirty(int x, int y, int width, int height) {
    mark_dirty(Rect{x, y, width, height});
}

void DirtyRectTracker::mark_dirty(const Rect& rect) {
    // Clamp to screen bounds
    Rect clamped;
    clamped.x = (rect.x < 0) ? 0 : rect.x;
    clamped.y = (rect.y < 0) ? 0 : rect.y;
    clamped.width = rect.width;
    clamped.height = rect.height;

    if (clamped.right() > screen_width_) {
        clamped.width = screen_width_ - clamped.x;
    }
    if (clamped.bottom() > screen_height_) {
        clamped.height = screen_height_ - clamped.y;
    }

    if (clamped.width <= 0 || clamped.height <= 0) {
        return;  // Off screen or empty
    }

    dirty_rects_.push_back(clamped);

    // Check if we should switch to full redraw
    if (static_cast<int>(dirty_rects_.size()) > max_rects_ * 2) {
        full_redraw_ = true;
    }
}

void DirtyRectTracker::mark_all_dirty() {
    full_redraw_ = true;
    dirty_rects_.clear();
    dirty_rects_.push_back({0, 0, screen_width_, screen_height_});
}

void DirtyRectTracker::clear() {
    dirty_rects_.clear();
    merged_rects_.clear();
    full_redraw_ = false;
}

void DirtyRectTracker::merge_rects() {
    merged_rects_.clear();

    if (dirty_rects_.empty()) return;

    if (full_redraw_) {
        merged_rects_.push_back({0, 0, screen_width_, screen_height_});
        return;
    }

    // Simple greedy merge algorithm
    std::vector<bool> merged(dirty_rects_.size(), false);

    for (size_t i = 0; i < dirty_rects_.size(); i++) {
        if (merged[i]) continue;

        Rect current = dirty_rects_[i];

        // Try to merge with subsequent rects
        bool did_merge;
        do {
            did_merge = false;
            for (size_t j = i + 1; j < dirty_rects_.size(); j++) {
                if (merged[j]) continue;

                const Rect& other = dirty_rects_[j];

                // Check if merging would be beneficial
                Rect combined = current.merged_with(other);
                int original_area = current.area() + other.area();
                int combined_area = combined.area();

                // Merge if overlap or combined isn't much larger
                if (current.intersects(other) ||
                    combined_area < static_cast<int>(original_area * (1.0f + merge_threshold_))) {
                    current = combined;
                    merged[j] = true;
                    did_merge = true;
                }
            }
        } while (did_merge);

        merged_rects_.push_back(current);
    }

    // If still too many rects, fall back to full redraw
    if (static_cast<int>(merged_rects_.size()) > max_rects_) {
        full_redraw_ = true;
        merged_rects_.clear();
        merged_rects_.push_back({0, 0, screen_width_, screen_height_});
    }
}

void DirtyRectTracker::optimize_rects() {
    merge_rects();
}

int DirtyRectTracker::get_dirty_pixel_count() const {
    int total = 0;
    for (const Rect& r : merged_rects_) {
        total += r.area();
    }
    return total;
}

//=============================================================================
// DoubleBufferedDirtyTracker Implementation
//=============================================================================

DoubleBufferedDirtyTracker::DoubleBufferedDirtyTracker(int width, int height)
    : front_(width, height)
    , back_(width, height) {
}

void DoubleBufferedDirtyTracker::mark_dirty(const Rect& rect) {
    front_.mark_dirty(rect);
}

void DoubleBufferedDirtyTracker::mark_dirty(int x, int y, int w, int h) {
    front_.mark_dirty(x, y, w, h);
}

void DoubleBufferedDirtyTracker::swap_buffers() {
    // Swap front and back
    std::swap(front_, back_);
    front_.clear();
}

std::vector<Rect> DoubleBufferedDirtyTracker::get_redraw_rects() const {
    // For double-buffered rendering, need union of both frames
    std::vector<Rect> result;

    const auto& front_rects = front_.get_dirty_rects();
    const auto& back_rects = back_.get_dirty_rects();

    result.reserve(front_rects.size() + back_rects.size());
    result.insert(result.end(), front_rects.begin(), front_rects.end());
    result.insert(result.end(), back_rects.begin(), back_rects.end());

    return result;
}

void DoubleBufferedDirtyTracker::clear_all() {
    front_.clear();
    back_.clear();
}
