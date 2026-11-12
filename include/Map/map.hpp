#pragma once

#include "raylib.h"
#include "Map/map_id.hpp"
#include "Task/task_type.hpp"

#include <string>
#include <vector>

struct MapRoom
{
    std::string name;
    Rectangle bounds{};
    Vector2 center{};
    std::vector<int> connections;
};

struct TaskStation
{
    std::string name;
    std::string detail;
    TaskType type{};
    int room_index{};
    Vector2 position{};
};

struct GameMap
{
    MapId id{};
    std::string name;
    std::string subtitle;
    std::vector<MapRoom> rooms;
    std::vector<TaskStation> tasks;

    [[nodiscard]] bool is_walkable(Vector2 position, float radius = 12.0F) const;
    [[nodiscard]] int room_at(Vector2 position) const;
    [[nodiscard]] std::vector<int> route(int from_room, int to_room) const;
    void draw(Vector2 offset = {}) const;
};

GameMap create_map(MapId id);
