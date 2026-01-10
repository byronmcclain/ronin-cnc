// include/test/test_fixtures.h
// Common Test Fixtures
// Task 18a - Test Framework Foundation

#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

#include "test/test_framework.h"
#include <cstdint>
#include <cstring>

//=============================================================================
// Base Test Fixture
//=============================================================================

class TestFixture {
public:
    virtual ~TestFixture() = default;

    /// Called before each test
    virtual void SetUp() {}

    /// Called after each test (even on failure)
    virtual void TearDown() {}

protected:
    TestFixture() = default;
};

//=============================================================================
// Platform Fixture - Initializes Platform Layer
//=============================================================================

class PlatformFixture : public TestFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if platform initialized successfully
    bool IsInitialized() const { return initialized_; }

protected:
    bool initialized_ = false;
};

//=============================================================================
// Graphics Fixture - Platform + Graphics Subsystem
//=============================================================================

class GraphicsFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Get backbuffer for pixel testing
    uint8_t* GetBackBuffer();
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetPitch() const { return pitch_; }

    /// Helper to clear backbuffer
    void ClearBackBuffer(uint8_t color = 0);

    /// Helper to render a frame
    void RenderFrame();

protected:
    int width_ = 0;
    int height_ = 0;
    int pitch_ = 0;
    bool graphics_initialized_ = false;
};

//=============================================================================
// Audio Fixture - Platform + Audio Subsystem
//=============================================================================

class AudioFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if audio initialized successfully
    bool IsAudioInitialized() const { return audio_initialized_; }

    /// Wait for a duration (for audio playback tests)
    void WaitMs(uint32_t ms);

protected:
    bool audio_initialized_ = false;
};

//=============================================================================
// Asset Fixture - Platform + MIX Loading
//=============================================================================

class AssetFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if assets loaded successfully
    bool AreAssetsLoaded() const { return assets_loaded_; }

    /// Check if a specific MIX file is loaded
    bool IsMixLoaded(const char* mix_name) const;

protected:
    bool assets_loaded_ = false;
};

//=============================================================================
// Full Game Fixture - All Systems
//=============================================================================

class GameFixture : public TestFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if game initialized
    bool IsInitialized() const { return game_initialized_; }

    /// Run N frames of game loop
    void RunFrames(int count);

    /// Process input events
    void PumpEvents();

protected:
    bool game_initialized_ = false;
};

//=============================================================================
// Fixture Usage Macro
//=============================================================================

/// Define a test that uses a fixture
#define TEST_WITH_FIXTURE(fixture_class, test_name, test_category) \
    static void TestFunc_##test_name(fixture_class& fixture); \
    static void TestWrapper_##test_name() { \
        fixture_class fixture; \
        fixture.SetUp(); \
        try { \
            TestFunc_##test_name(fixture); \
        } catch (...) { \
            fixture.TearDown(); \
            throw; \
        } \
        fixture.TearDown(); \
    } \
    static struct TestRegistrar_##test_name { \
        TestRegistrar_##test_name() { \
            TestCaseInfo info; \
            info.name = #test_name; \
            info.category = test_category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = 0; \
            TestRegistry::Instance().RegisterTest(info, TestWrapper_##test_name); \
        } \
    } g_test_registrar_##test_name; \
    static void TestFunc_##test_name(fixture_class& fixture)

#endif // TEST_FIXTURES_H
