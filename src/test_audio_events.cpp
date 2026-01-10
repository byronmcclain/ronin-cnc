// src/test_audio_events.cpp
// Tests for Game Audio Events
// Task 17f - Game Audio Events

#include "game/audio/audio_events.h"
#include "game/audio/event_rate_limiter.h"
#include "game/audio/audio_system.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Test Utilities
//=============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(name) \
    do { \
        printf("Test: %s... ", #name); \
        if (Test_##name()) { \
            printf("PASSED\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } while(0)

//=============================================================================
// Rate Limiter Tests
//=============================================================================

bool Test_RateLimiter_Global() {
    EventRateLimiter limiter;

    limiter.SetGlobalCooldown(1, 100);  // 100ms cooldown for event type 1

    // First call should pass
    TEST_ASSERT(limiter.CanFireGlobal(1), "First event should fire");

    // Immediate second call should fail
    TEST_ASSERT(!limiter.CanFireGlobal(1), "Immediate repeat should be rate-limited");

    // After cooldown, should pass
    Platform_Timer_Delay(110);
    TEST_ASSERT(limiter.CanFireGlobal(1), "After cooldown should fire");

    return true;
}

bool Test_RateLimiter_Position() {
    EventRateLimiter limiter;

    limiter.SetPositionCooldown(2, 100);  // 100ms cooldown for event type 2

    // Same position: first should pass, second should fail
    TEST_ASSERT(limiter.CanFireAtPosition(2, 10, 10), "First at (10,10) should fire");
    TEST_ASSERT(!limiter.CanFireAtPosition(2, 10, 10), "Second at (10,10) should be limited");

    // Different position should pass
    TEST_ASSERT(limiter.CanFireAtPosition(2, 20, 20), "Different position should fire");

    return true;
}

bool Test_RateLimiter_Object() {
    EventRateLimiter limiter;

    limiter.SetObjectCooldown(3, 100);  // 100ms cooldown for event type 3

    // Same object: first should pass, second should fail
    TEST_ASSERT(limiter.CanFireForObject(3, 100), "First for object 100 should fire");
    TEST_ASSERT(!limiter.CanFireForObject(3, 100), "Second for object 100 should be limited");

    // Different object should pass
    TEST_ASSERT(limiter.CanFireForObject(3, 200), "Different object should fire");

    return true;
}

bool Test_RateLimiter_Combined() {
    EventRateLimiter limiter;

    limiter.SetGlobalCooldown(4, 50);
    limiter.SetPositionCooldown(4, 100);

    // First call should pass both
    TEST_ASSERT(limiter.CanFireGlobalAndPosition(4, 5, 5), "First should fire");

    // Immediate call at different position should fail (global limit)
    TEST_ASSERT(!limiter.CanFireGlobalAndPosition(4, 6, 6), "Global should block different pos");

    // After global cooldown but within position cooldown
    Platform_Timer_Delay(60);
    TEST_ASSERT(!limiter.CanFireGlobalAndPosition(4, 5, 5), "Position should still block");

    // Different position after global cooldown should pass
    TEST_ASSERT(limiter.CanFireGlobalAndPosition(4, 15, 15), "New position should fire");

    return true;
}

bool Test_RateLimiter_Cleanup() {
    EventRateLimiter limiter;

    limiter.SetPositionCooldown(5, 50);

    // Add some entries
    for (int i = 0; i < 100; i++) {
        limiter.CanFireAtPosition(5, i, i);
    }

    TEST_ASSERT(limiter.GetTrackedCount() >= 100, "Should have entries");

    // Cleanup shouldn't remove recent entries
    limiter.Cleanup();
    TEST_ASSERT(limiter.GetTrackedCount() >= 100, "Recent entries should remain");

    // Reset should clear all
    limiter.Reset();
    TEST_ASSERT(limiter.GetTrackedCount() == 0, "Reset should clear all");

    return true;
}

//=============================================================================
// Audio Events Tests
//=============================================================================

bool Test_AudioEvents_Init() {
    AudioEvents_Shutdown();  // Ensure clean state

    AudioEvents_Init();

    // Should be able to init twice without error
    AudioEvents_Init();

    TEST_ASSERT(AudioEvents_GetTotalTriggered() == 0, "Fresh init should have 0 triggered");

    return true;
}

bool Test_AudioEvents_UIClick() {
    AudioEvents_Init();

    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_UIClick();

    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "UI click should trigger");

    // Rapid clicks should be rate-limited
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();

    AudioEvent_UIClick();
    AudioEvent_UIClick();
    AudioEvent_UIClick();

    uint32_t limited_after = AudioEvents_GetTotalRateLimited();
    TEST_ASSERT(limited_after > limited_before, "Rapid clicks should be rate-limited");

    return true;
}

bool Test_AudioEvents_UnitSelection() {
    AudioEvents_Init();

    // Create fake objects (just use addresses)
    int fake_unit1 = 1;
    int fake_unit2 = 2;

    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit1));

    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Unit select should trigger");

    // Same unit immediately should be rate-limited
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();
    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit1));
    TEST_ASSERT(AudioEvents_GetTotalRateLimited() > limited_before, "Same unit should be limited");

    // Different unit should fire (after short delay for global cooldown)
    Platform_Timer_Delay(150);
    before = AudioEvents_GetTotalTriggered();
    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit2));
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Different unit should fire");

    return true;
}

bool Test_AudioEvents_Explosions() {
    AudioEvents_Init();

    // Reset rate limiters
    GetEventRateLimiter().Reset();
    AudioEvents_Init();  // Re-init to reset rate limiter config

    uint32_t before = AudioEvents_GetTotalTriggered();

    // First explosion should fire
    AudioEvent_Explosion(100, 100, 2);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "First explosion should fire");

    // Different position should fire
    Platform_Timer_Delay(150);
    before = AudioEvents_GetTotalTriggered();
    AudioEvent_Explosion(500, 500, 1);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Different position should fire");

    return true;
}

bool Test_AudioEvents_EVA() {
    AudioEvents_Init();

    // Reset to ensure clean state
    GetEventRateLimiter().Reset();
    AudioEvents_Init();

    uint32_t before = AudioEvents_GetTotalTriggered();

    // First EVA should fire
    AudioEvent_InsufficientFunds(reinterpret_cast<HouseClass*>(1));
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "First EVA should fire");

    // Immediate second should be limited (5 second cooldown)
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();
    AudioEvent_InsufficientFunds(reinterpret_cast<HouseClass*>(1));
    TEST_ASSERT(AudioEvents_GetTotalRateLimited() > limited_before, "EVA should be rate-limited");

    return true;
}

bool Test_AudioEvents_Stats() {
    AudioEvents_Init();

    // Trigger some events
    AudioEvent_UIClick();
    Platform_Timer_Delay(60);
    AudioEvent_UIClick();

    uint32_t triggered = AudioEvents_GetTotalTriggered();

    TEST_ASSERT(triggered > 0, "Should have triggered events");

    // Print stats (visual check)
    printf("\n  ");
    AudioEvents_PrintStats();
    printf("  ");

    return true;
}

bool Test_AudioEvents_GameState() {
    AudioEvents_Init();

    // These should not crash
    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_EnterMainMenu();
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "EnterMainMenu should trigger");

    before = AudioEvents_GetTotalTriggered();
    AudioEvent_MissionStart();
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "MissionStart should trigger");

    return true;
}

bool Test_AudioEvents_BuildingEvents() {
    AudioEvents_Init();
    GetEventRateLimiter().Reset();
    AudioEvents_Init();

    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_BuildingPlaced(nullptr);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "BuildingPlaced should trigger");

    Platform_Timer_Delay(250);
    before = AudioEvents_GetTotalTriggered();
    AudioEvent_BuildingSold(nullptr);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "BuildingSold should trigger");

    return true;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Audio Events Tests (Task 17f) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    Platform_Init();

    // Initialize audio system (needed for event handlers)
    Audio_Init();

    // Rate limiter tests
    printf("--- Rate Limiter Tests ---\n");
    RUN_TEST(RateLimiter_Global);
    RUN_TEST(RateLimiter_Position);
    RUN_TEST(RateLimiter_Object);
    RUN_TEST(RateLimiter_Combined);
    RUN_TEST(RateLimiter_Cleanup);

    // Audio event tests
    printf("\n--- Audio Event Tests ---\n");
    RUN_TEST(AudioEvents_Init);
    RUN_TEST(AudioEvents_UIClick);
    RUN_TEST(AudioEvents_UnitSelection);
    RUN_TEST(AudioEvents_Explosions);
    RUN_TEST(AudioEvents_EVA);
    RUN_TEST(AudioEvents_GameState);
    RUN_TEST(AudioEvents_BuildingEvents);
    RUN_TEST(AudioEvents_Stats);

    AudioEvents_Shutdown();
    Audio_Shutdown();
    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
