// src/graphics/sprite_batch.cpp
// Sprite Batching Implementation
// Task 18h - Performance Optimization

#include "graphics/sprite_batch.h"
#include "platform/profiler.h"
#include <algorithm>
#include <cstring>
#include <cmath>

//=============================================================================
// SpriteBatch Implementation
//=============================================================================

SpriteBatch::SpriteBatch(size_t max_sprites)
    : max_sprites_(max_sprites) {
    sprites_.reserve(max_sprites);
    vertices_.reserve(max_sprites * 4);  // 4 vertices per sprite
    indices_.reserve(max_sprites * 6);   // 6 indices per sprite (2 triangles)
    batches_.reserve(100);

    // Pre-generate indices (they're the same pattern for all sprites)
    for (size_t i = 0; i < max_sprites; i++) {
        uint16_t base = static_cast<uint16_t>(i * 4);
        indices_.push_back(base + 0);
        indices_.push_back(base + 1);
        indices_.push_back(base + 2);
        indices_.push_back(base + 2);
        indices_.push_back(base + 3);
        indices_.push_back(base + 0);
    }
}

SpriteBatch::~SpriteBatch() {
    // Would release GPU buffers here
}

void SpriteBatch::begin() {
    sprites_.clear();
    batches_.clear();
    in_batch_ = true;
    draw_calls_ = 0;
    sprite_count_ = 0;
    batch_count_ = 0;
}

void SpriteBatch::end() {
    if (!in_batch_) return;

    PROFILE_SCOPE("SpriteBatch::end");

    if (sort_enabled_) {
        sort_sprites();
    }
    build_batches();
    flush();

    in_batch_ = false;
}

void SpriteBatch::draw(const SpriteInstance& sprite) {
    if (!in_batch_) return;
    if (sprites_.size() >= max_sprites_) {
        // Batch full, flush and continue
        end();
        begin();
    }

    sprites_.push_back(sprite);
    sprite_count_++;
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h) {
    draw(texture_id, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1) {
    draw(texture_id, x, y, w, h, u0, v0, u1, v1, 0xFFFFFFFF, 0.0f);
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1,
                       uint32_t color, float rotation) {
    SpriteInstance sprite;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = u0;
    sprite.v0 = v0;
    sprite.u1 = u1;
    sprite.v1 = v1;
    sprite.color = color;
    sprite.rotation = rotation;
    sprite.texture_id = texture_id;
    sprite.layer = 0;

    draw(sprite);
}

void SpriteBatch::sort_sprites() {
    PROFILE_SCOPE("SpriteBatch::sort");

    // Sort by texture to minimize state changes, then by layer
    std::sort(sprites_.begin(), sprites_.end(),
        [](const SpriteInstance& a, const SpriteInstance& b) {
            if (a.layer != b.layer) return a.layer < b.layer;
            return a.texture_id < b.texture_id;
        });
}

void SpriteBatch::build_batches() {
    PROFILE_SCOPE("SpriteBatch::build_batches");

    if (sprites_.empty()) return;

    vertices_.clear();
    batches_.clear();

    uint32_t current_texture = sprites_[0].texture_id;
    int batch_start = 0;
    int count = 0;

    for (size_t i = 0; i < sprites_.size(); i++) {
        const SpriteInstance& s = sprites_[i];

        // Check for texture change
        if (s.texture_id != current_texture) {
            // End current batch
            Batch batch;
            batch.texture_id = current_texture;
            batch.start_index = batch_start * 6;
            batch.count = count * 6;
            batches_.push_back(batch);

            // Start new batch
            current_texture = s.texture_id;
            batch_start = static_cast<int>(i);
            count = 0;
        }

        // Generate vertices for this sprite
        float x0 = s.x;
        float y0 = s.y;
        float x1 = s.x + s.width;
        float y1 = s.y + s.height;

        // Handle rotation
        if (s.rotation != 0.0f) {
            float cx = s.x + s.width * 0.5f;
            float cy = s.y + s.height * 0.5f;
            float cos_r = std::cos(s.rotation);
            float sin_r = std::sin(s.rotation);

            // Rotate corners around center
            auto rotate = [cx, cy, cos_r, sin_r](float& px, float& py) {
                float dx = px - cx;
                float dy = py - cy;
                px = cx + dx * cos_r - dy * sin_r;
                py = cy + dx * sin_r + dy * cos_r;
            };

            float corners[4][2] = {
                {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}
            };
            for (int j = 0; j < 4; j++) {
                rotate(corners[j][0], corners[j][1]);
            }

            vertices_.push_back({corners[0][0], corners[0][1], s.u0, s.v0, s.color});
            vertices_.push_back({corners[1][0], corners[1][1], s.u1, s.v0, s.color});
            vertices_.push_back({corners[2][0], corners[2][1], s.u1, s.v1, s.color});
            vertices_.push_back({corners[3][0], corners[3][1], s.u0, s.v1, s.color});
        } else {
            // No rotation - fast path
            vertices_.push_back({x0, y0, s.u0, s.v0, s.color});
            vertices_.push_back({x1, y0, s.u1, s.v0, s.color});
            vertices_.push_back({x1, y1, s.u1, s.v1, s.color});
            vertices_.push_back({x0, y1, s.u0, s.v1, s.color});
        }

        count++;
    }

    // Final batch
    if (count > 0) {
        Batch batch;
        batch.texture_id = current_texture;
        batch.start_index = batch_start * 6;
        batch.count = count * 6;
        batches_.push_back(batch);
    }

    batch_count_ = static_cast<int>(batches_.size());
}

void SpriteBatch::flush() {
    PROFILE_SCOPE("SpriteBatch::flush");

    if (vertices_.empty()) return;

    // Would upload vertices to GPU here

    for (const Batch& batch : batches_) {
        render_batch(batch);
    }
}

void SpriteBatch::render_batch(const Batch& batch) {
    // Would bind texture and issue draw call here
    draw_calls_++;
    PROFILE_COUNTER("DrawCalls");

    (void)batch;  // Suppress unused warning in stub
}

//=============================================================================
// BatchRenderer Implementation
//=============================================================================

BatchRenderer& BatchRenderer::instance() {
    static BatchRenderer instance;
    return instance;
}

BatchRenderer::BatchRenderer() : batch_(10000) {}

void BatchRenderer::begin_frame() {
    batch_.begin();
    current_layer_ = 0;
}

void BatchRenderer::end_frame() {
    batch_.end();
}

void BatchRenderer::set_layer(int layer) {
    current_layer_ = layer;
}

void BatchRenderer::draw_sprite(uint32_t texture_id, float x, float y, float w, float h) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = 0xFFFFFFFF;
    sprite.rotation = 0.0f;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

void BatchRenderer::draw_sprite_rotated(uint32_t texture_id, float x, float y,
                                        float w, float h, float rotation) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = 0xFFFFFFFF;
    sprite.rotation = rotation;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

void BatchRenderer::draw_sprite_tinted(uint32_t texture_id, float x, float y,
                                       float w, float h, uint32_t color) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = color;
    sprite.rotation = 0.0f;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

int BatchRenderer::get_total_sprites() const {
    return batch_.get_sprite_count();
}

int BatchRenderer::get_total_draw_calls() const {
    return batch_.get_draw_calls();
}

int BatchRenderer::get_batches_rendered() const {
    return batch_.get_batch_count();
}
