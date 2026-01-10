// src/graphics/sprite_batch.h
// Sprite Batching for Efficient Rendering
// Task 18h - Performance Optimization

#ifndef SPRITE_BATCH_H
#define SPRITE_BATCH_H

#include <vector>
#include <cstdint>

//=============================================================================
// Sprite Vertex
//=============================================================================

struct SpriteVertex {
    float x, y;           // Position
    float u, v;           // Texture coordinates
    uint32_t color;       // RGBA packed color
};

//=============================================================================
// Sprite Instance
//=============================================================================

struct SpriteInstance {
    float x, y;           // Position
    float width, height;  // Size
    float u0, v0, u1, v1; // Texture rect (normalized)
    uint32_t color;       // Tint color
    float rotation;       // Rotation in radians
    uint32_t texture_id;  // Texture to use
    int layer;            // Sort layer (higher = on top)
};

//=============================================================================
// Sprite Batch
//=============================================================================

class SpriteBatch {
public:
    SpriteBatch(size_t max_sprites = 10000);
    ~SpriteBatch();

    // Begin/end batch
    void begin();
    void end();

    // Add sprite to batch
    void draw(const SpriteInstance& sprite);

    // Convenience methods
    void draw(uint32_t texture_id, float x, float y, float w, float h);
    void draw(uint32_t texture_id, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1);
    void draw(uint32_t texture_id, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              uint32_t color, float rotation);

    // Flush current batch to GPU
    void flush();

    // Statistics
    int get_draw_calls() const { return draw_calls_; }
    int get_sprite_count() const { return sprite_count_; }
    int get_batch_count() const { return batch_count_; }

    // Settings
    void set_sort_enabled(bool enabled) { sort_enabled_ = enabled; }

private:
    struct Batch {
        uint32_t texture_id;
        int start_index;
        int count;
    };

    void sort_sprites();
    void build_batches();
    void render_batch(const Batch& batch);

    std::vector<SpriteInstance> sprites_;
    std::vector<SpriteVertex> vertices_;
    std::vector<uint16_t> indices_;
    std::vector<Batch> batches_;

    size_t max_sprites_;
    bool in_batch_ = false;
    bool sort_enabled_ = true;

    // Stats
    int draw_calls_ = 0;
    int sprite_count_ = 0;
    int batch_count_ = 0;

    // GPU resources (would be Metal buffers)
    void* vertex_buffer_ = nullptr;
    void* index_buffer_ = nullptr;
};

//=============================================================================
// Batch Renderer (Higher-level interface)
//=============================================================================

class BatchRenderer {
public:
    static BatchRenderer& instance();

    void begin_frame();
    void end_frame();

    // Layer management
    void set_layer(int layer);
    int get_layer() const { return current_layer_; }

    // Draw calls
    void draw_sprite(uint32_t texture_id, float x, float y, float w, float h);
    void draw_sprite_rotated(uint32_t texture_id, float x, float y, float w, float h,
                             float rotation);
    void draw_sprite_tinted(uint32_t texture_id, float x, float y, float w, float h,
                            uint32_t color);

    // Frame statistics
    int get_total_sprites() const;
    int get_total_draw_calls() const;
    int get_batches_rendered() const;

private:
    BatchRenderer();

    SpriteBatch batch_;
    int current_layer_ = 0;
};

#endif // SPRITE_BATCH_H
