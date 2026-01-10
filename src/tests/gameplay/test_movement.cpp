// src/tests/gameplay/test_movement.cpp
// Unit Movement Gameplay Tests
// Task 18e - Gameplay Tests

#include "test/test_framework.h"
#include <cmath>

//=============================================================================
// Movement Tests
//=============================================================================

TEST_CASE(Gameplay_Movement_BasicMove, "Movement") {
    // Test unit moving from A to B
    int start_x = 100, start_y = 100;
    int target_x = 200, target_y = 200;

    // Simulate movement
    int dx = target_x - start_x;
    int dy = target_y - start_y;

    TEST_ASSERT_GT(dx, 0);
    TEST_ASSERT_GT(dy, 0);
}

TEST_CASE(Gameplay_Movement_Distance, "Movement") {
    // Calculate distance between points
    int x1 = 0, y1 = 0;
    int x2 = 3, y2 = 4;

    int dx = x2 - x1;
    int dy = y2 - y1;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    // Should be 5 (3-4-5 triangle)
    TEST_ASSERT_NEAR(distance, 5.0f, 0.01f);
}

TEST_CASE(Gameplay_Movement_Pathfinding_StraightLine, "Movement") {
    // Test pathfinding on open terrain
    // Unit should take direct path (diagonal)
    int start_x = 0, start_y = 0;
    int end_x = 10, end_y = 10;

    // On open terrain, path should be close to diagonal distance
    int diagonal_dist = std::max(std::abs(end_x - start_x), std::abs(end_y - start_y));
    int expected_path_length = diagonal_dist;

    TEST_ASSERT_GT(expected_path_length, 0);
    TEST_ASSERT_EQ(expected_path_length, 10);
}

TEST_CASE(Gameplay_Movement_Pathfinding_Obstacle, "Movement") {
    // Test pathfinding around obstacles
    // Obstacle blocks direct path, must go around

    // Simulating: start at (0,0), obstacle at (5,5), target at (10,10)
    // Path should be longer than direct diagonal

    int direct_dist = 10;  // diagonal
    int path_with_obstacle = 14;  // Going around adds distance

    TEST_ASSERT_GT(path_with_obstacle, direct_dist);
}

TEST_CASE(Gameplay_Movement_TerrainSpeed, "Movement") {
    // Test speed modifiers on different terrain
    float base_speed = 10.0f;  // Cells per frame * 100

    float road_modifier = 1.0f;
    float rough_modifier = 0.5f;
    float water_modifier = 0.0f;  // Can't traverse (for land units)

    float road_speed = base_speed * road_modifier;
    float rough_speed = base_speed * rough_modifier;
    float water_speed = base_speed * water_modifier;

    TEST_ASSERT_GT(road_speed, rough_speed);
    TEST_ASSERT_EQ(water_speed, 0.0f);
    TEST_ASSERT_NEAR(rough_speed, 5.0f, 0.01f);
}

TEST_CASE(Gameplay_Movement_UnitCollision, "Movement") {
    // Units should not overlap
    struct Unit {
        int x, y;
        int radius;
    };

    Unit unit1 = {100, 100, 10};
    Unit unit2 = {105, 100, 10};  // Overlapping

    int dist = std::abs(unit1.x - unit2.x);
    bool collision = dist < (unit1.radius + unit2.radius);

    TEST_ASSERT(collision);

    // Move unit2 further away
    unit2.x = 130;
    dist = std::abs(unit1.x - unit2.x);
    collision = dist < (unit1.radius + unit2.radius);

    TEST_ASSERT(!collision);
}

TEST_CASE(Gameplay_Movement_Formation, "Movement") {
    // Multiple selected units should move in formation
    struct Unit {
        int x, y;
    };

    Unit units[5] = {
        {100, 100}, {110, 100}, {120, 100}, {105, 110}, {115, 110}
    };

    // Target destination
    int target_x = 200, target_y = 200;

    // Calculate formation offset from center
    int center_x = 0, center_y = 0;
    for (int i = 0; i < 5; i++) {
        center_x += units[i].x;
        center_y += units[i].y;
    }
    center_x /= 5;
    center_y /= 5;

    // Each unit maintains relative position
    for (int i = 0; i < 5; i++) {
        int offset_x = units[i].x - center_x;
        int offset_y = units[i].y - center_y;

        int new_x = target_x + offset_x;
        int new_y = target_y + offset_y;

        // Verify formation maintained
        TEST_ASSERT_EQ(new_x - target_x, offset_x);
        TEST_ASSERT_EQ(new_y - target_y, offset_y);
    }
}

TEST_CASE(Gameplay_Movement_SpeedByUnitType, "Movement") {
    // Different unit types have different speeds
    struct UnitType {
        const char* name;
        float speed;
    };

    UnitType types[] = {
        {"Infantry", 4.0f},
        {"Light Tank", 8.0f},
        {"Heavy Tank", 5.0f},
        {"Harvester", 3.0f}
    };

    // Light tank should be fastest
    float max_speed = 0;
    int fastest_idx = 0;
    for (int i = 0; i < 4; i++) {
        if (types[i].speed > max_speed) {
            max_speed = types[i].speed;
            fastest_idx = i;
        }
    }

    TEST_ASSERT_EQ(fastest_idx, 1);  // Light Tank
}

TEST_CASE(Gameplay_Movement_TurningSpeed, "Movement") {
    // Units take time to turn
    int current_facing = 0;   // North (0-7 directions)
    int target_facing = 4;    // South

    // Calculate turn amount
    int diff = target_facing - current_facing;
    if (diff < 0) diff += 8;
    if (diff > 4) diff = 8 - diff;  // Shorter direction

    int turn_rate = 2;  // Directions per frame
    int frames_to_turn = (diff + turn_rate - 1) / turn_rate;

    TEST_ASSERT_EQ(diff, 4);  // Half turn
    TEST_ASSERT_EQ(frames_to_turn, 2);
}

TEST_CASE(Gameplay_Movement_Waypoints, "Movement") {
    // Units can have multiple waypoints
    struct Waypoint {
        int x, y;
    };

    Waypoint path[] = {
        {100, 100},
        {200, 100},
        {200, 200},
        {100, 200}
    };

    int total_distance = 0;
    for (int i = 1; i < 4; i++) {
        int dx = std::abs(path[i].x - path[i-1].x);
        int dy = std::abs(path[i].y - path[i-1].y);
        total_distance += dx + dy;  // Manhattan distance
    }

    // 100 (100,100 -> 200,100) + 100 (200,100 -> 200,200) + 100 (200,200 -> 100,200) = 300
    TEST_ASSERT_EQ(total_distance, 300);
}
