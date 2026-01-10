// src/test/test_fixtures.cpp
// Common Test Fixtures Implementation
// Task 18a - Test Framework Foundation

#include "test/test_fixtures.h"
#include "platform.h"

//=============================================================================
// PlatformFixture
//=============================================================================

void PlatformFixture::SetUp() {
    initialized_ = (Platform_Init() == 0);
}

void PlatformFixture::TearDown() {
    if (initialized_) {
        Platform_Shutdown();
        initialized_ = false;
    }
}

//=============================================================================
// GraphicsFixture
//=============================================================================

void GraphicsFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    graphics_initialized_ = (Platform_Graphics_Init() == 0);

    if (graphics_initialized_) {
        uint8_t* buffer;
        int32_t w, h, p;
        if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) == 0) {
            width_ = w;
            height_ = h;
            pitch_ = p;
        }
    }
}

void GraphicsFixture::TearDown() {
    if (graphics_initialized_) {
        Platform_Graphics_Shutdown();
        graphics_initialized_ = false;
    }
    PlatformFixture::TearDown();
}

uint8_t* GraphicsFixture::GetBackBuffer() {
    uint8_t* buffer = nullptr;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
    return buffer;
}

void GraphicsFixture::ClearBackBuffer(uint8_t color) {
    uint8_t* buffer = GetBackBuffer();
    if (buffer) {
        for (int y = 0; y < height_; y++) {
            memset(buffer + y * pitch_, color, width_);
        }
    }
}

void GraphicsFixture::RenderFrame() {
    Platform_Graphics_Flip();
}

//=============================================================================
// AudioFixture
//=============================================================================

void AudioFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    AudioConfig config;
    config.sample_rate = 22050;
    config.channels = 2;
    config.bits_per_sample = 16;
    config.buffer_size = 1024;

    audio_initialized_ = (Platform_Audio_Init(&config) == 0);
}

void AudioFixture::TearDown() {
    if (audio_initialized_) {
        Platform_Audio_Shutdown();
        audio_initialized_ = false;
    }
    PlatformFixture::TearDown();
}

void AudioFixture::WaitMs(uint32_t ms) {
    Platform_Timer_Delay(ms);
}

//=============================================================================
// AssetFixture
//=============================================================================

void AssetFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    // Initialize MixManager and load essential MIX files
    // In real implementation:
    // MixManager::Instance().Initialize();
    // assets_loaded_ = MixManager::Instance().LoadMix("REDALERT.MIX");

    assets_loaded_ = true;  // Placeholder
}

void AssetFixture::TearDown() {
    if (assets_loaded_) {
        // MixManager::Instance().Shutdown();
        assets_loaded_ = false;
    }
    PlatformFixture::TearDown();
}

bool AssetFixture::IsMixLoaded(const char* mix_name) const {
    (void)mix_name;  // Suppress unused parameter warning
    // return MixManager::Instance().IsMixLoaded(mix_name);
    return assets_loaded_;  // Placeholder
}

//=============================================================================
// GameFixture
//=============================================================================

void GameFixture::SetUp() {
    // In real implementation:
    // game_initialized_ = Game_Init();

    game_initialized_ = true;  // Placeholder
}

void GameFixture::TearDown() {
    if (game_initialized_) {
        // Game_Shutdown();
        game_initialized_ = false;
    }
}

void GameFixture::RunFrames(int count) {
    for (int i = 0; i < count; i++) {
        PumpEvents();
        // Game_Update();
        // Game_Render();
        Platform_Timer_Delay(16);  // ~60fps
    }
}

void GameFixture::PumpEvents() {
    Platform_PollEvents();
    Platform_Input_Update();
}
