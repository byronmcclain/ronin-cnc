// src/graphics/dirty_rect.h
// Dirty Rectangle Tracking
// Task 18h - Performance Optimization

#ifndef DIRTY_RECT_H
#define DIRTY_RECT_H

#include <vector>
#include <cstdint>

//=============================================================================
// Rectangle Structure
//=============================================================================

struct Rect {
    int x, y;
    int width, height;

    int right() const { return x + width; }
    int bottom() const { return y + height; }

    bool intersects(const Rect& other) const {
        return !(x >= other.right() || right() <= other.x ||
                 y >= other.bottom() || bottom() <= other.y);
    }

    bool contains(int px, int py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    Rect merged_with(const Rect& other) const {
        int nx = (x < other.x) ? x : other.x;
        int ny = (y < other.y) ? y : other.y;
        int nr = (right() > other.right()) ? right() : other.right();
        int nb = (bottom() > other.bottom()) ? bottom() : other.bottom();
        return {nx, ny, nr - nx, nb - ny};
    }

    int area() const { return width * height; }
};

//=============================================================================
// Dirty Rectangle Tracker
//=============================================================================

class DirtyRectTracker {
public:
    DirtyRectTracker(int screen_width, int screen_height);

    // Mark region as dirty
    void mark_dirty(int x, int y, int width, int height);
    void mark_dirty(const Rect& rect);

    // Mark entire screen dirty
    void mark_all_dirty();

    // Clear dirty regions (call after rendering)
    void clear();

    // Get optimized dirty rectangles for rendering
    const std::vector<Rect>& get_dirty_rects() const { return merged_rects_; }

    // Check if anything is dirty
    bool has_dirty_rects() const { return !dirty_rects_.empty(); }

    // Check if using full redraw (optimization for many dirty rects)
    bool is_full_redraw() const { return full_redraw_; }

    // Optimization settings
    void set_merge_threshold(float threshold) { merge_threshold_ = threshold; }
    void set_max_rects(int max) { max_rects_ = max; }

    // Statistics
    int get_dirty_rect_count() const { return static_cast<int>(dirty_rects_.size()); }
    int get_merged_rect_count() const { return static_cast<int>(merged_rects_.size()); }
    int get_dirty_pixel_count() const;

private:
    void merge_rects();
    void optimize_rects();

    int screen_width_;
    int screen_height_;
    std::vector<Rect> dirty_rects_;
    std::vector<Rect> merged_rects_;
    bool full_redraw_ = false;

    float merge_threshold_ = 0.5f;
    int max_rects_ = 20;
};

//=============================================================================
// Double-Buffered Dirty Tracking
//=============================================================================

class DoubleBufferedDirtyTracker {
public:
    DoubleBufferedDirtyTracker(int width, int height);

    // Mark dirty in current frame
    void mark_dirty(const Rect& rect);
    void mark_dirty(int x, int y, int w, int h);

    // Swap buffers at frame boundary
    void swap_buffers();

    // Get rects that need redrawing (union of both buffers for double-buffering)
    std::vector<Rect> get_redraw_rects() const;

    // Clear both buffers (e.g., on resize)
    void clear_all();

private:
    DirtyRectTracker front_;  // Current frame
    DirtyRectTracker back_;   // Previous frame
};

#endif // DIRTY_RECT_H
