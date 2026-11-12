#include "Map/map.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>

namespace {
constexpr float ROOM_WIDTH = 248.0F;
constexpr float ROOM_HEIGHT = 136.0F;
constexpr float COLUMN_GAP = 54.0F;
constexpr float ROW_GAP = 56.0F;
constexpr float MAP_LEFT = 46.0F;
constexpr float MAP_TOP = 104.0F;
constexpr Color FLOOR_COLOR{52, 66, 79, 255};
constexpr Color ROOM_COLOR{76, 91, 103, 255};
constexpr Color WALL_COLOR{30, 41, 53, 255};
constexpr Color LINE_COLOR{128, 149, 160, 255};
constexpr Color TASK_COLOR{238, 188, 84, 255};

using RoomNames = std::array<const char*, 12>;

RoomNames names_for(MapId id) {
    switch (id) {
    case MapId::polus:
        return {"Dropship", "Office", "Laboratory", "Specimen", "Electrical", "Storage",
                "Communications", "Security", "O2", "Boiler Room", "Weapons", "Admin"};
    case MapId::mira:
        return {"Launchpad", "Greenhouse", "MedBay", "Cafeteria", "Laboratory", "Office",
                "Communications", "Decontamination", "Admin", "Locker Room", "Reactor", "Balcony"};
    case MapId::skeld:
    default:
        return {"Upper Engine", "MedBay", "Cafeteria", "Weapons", "Reactor", "Security",
                "Admin", "O2", "Lower Engine", "Electrical", "Storage", "Navigation"};
    }
}

std::string map_title(MapId id) {
    switch (id) {
    case MapId::polus: return "Polus";
    case MapId::mira: return "MIRA HQ";
    case MapId::skeld:
    default: return "The Skeld";
    }
}

std::string map_subtitle(MapId id) {
    switch (id) {
    case MapId::polus: return "Remote planetary research base";
    case MapId::mira: return "High-altitude operations center";
    case MapId::skeld:
    default: return "Deep-space research vessel";
    }
}

Rectangle corridor_between(Vector2 from, Vector2 to, float width) {
    if (from.x == to.x) {
        return {from.x - width / 2.0F, std::min(from.y, to.y), width, std::abs(to.y - from.y)};
    }
    return {std::min(from.x, to.x), from.y - width / 2.0F, std::abs(to.x - from.x), width};
}

void add_task(GameMap& map, int room, const char* name, const char* detail, TaskType type, Vector2 local_offset) {
    const MapRoom& target = map.rooms[room];
    map.tasks.push_back({name, detail, type, room,
                         {target.center.x + local_offset.x, target.center.y + local_offset.y}});
}
}

GameMap create_map(MapId id) {
    GameMap map;
    map.id = id;
    map.name = map_title(id);
    map.subtitle = map_subtitle(id);

    const RoomNames names = names_for(id);
    const std::array<int, 12> polus_slots{0, 1, 3, 11, 4, 5, 8, 9, 2, 6, 10, 7};
    const std::array<int, 12> mira_slots{4, 1, 5, 9, 2, 6, 10, 3, 7, 11, 0, 8};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int index = row * 4 + column;
            const int slot = id == MapId::polus ? polus_slots[index] : id == MapId::mira ? mira_slots[index] : index;
            const int slot_column = slot % 4;
            const int slot_row = slot / 4;
            const float x = MAP_LEFT + static_cast<float>(slot_column) * (ROOM_WIDTH + COLUMN_GAP);
            const float y = MAP_TOP + static_cast<float>(slot_row) * (ROOM_HEIGHT + ROW_GAP);
            map.rooms.push_back({names[index], {x, y, ROOM_WIDTH, ROOM_HEIGHT},
                                 {x + ROOM_WIDTH / 2.0F, y + ROOM_HEIGHT / 2.0F}, {}});
        }
    }

    const auto connect = [&map](int first, int second) {
        map.rooms[first].connections.push_back(second);
        map.rooms[second].connections.push_back(first);
    };
    const auto connect_edges = [&connect](const auto& edges) {
        for (const auto& edge : edges) {
            connect(edge[0], edge[1]);
        }
    };
    constexpr std::array<std::array<int, 2>, 17> skeld_edges{{
        {0, 1}, {1, 2}, {2, 3}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
        {4, 5}, {5, 6}, {6, 7}, {4, 8}, {5, 9}, {6, 10}, {7, 11},
        {8, 9}, {9, 10}, {10, 11}
    }};
    constexpr std::array<std::array<int, 2>, 16> polus_edges{{
        {0, 1}, {1, 8}, {8, 2}, {2, 11}, {11, 3}, {0, 4}, {4, 5}, {5, 9},
        {9, 8}, {9, 10}, {10, 7}, {7, 6}, {6, 4}, {5, 7}, {7, 10}, {5, 11}
    }};
    constexpr std::array<std::array<int, 2>, 16> mira_edges{{
        {0, 10}, {10, 1}, {1, 4}, {4, 7}, {1, 2}, {2, 3}, {2, 5}, {5, 6},
        {5, 8}, {8, 9}, {3, 11}, {11, 0}, {0, 2}, {4, 5}, {6, 9}, {7, 8}
    }};
    if (id == MapId::polus) {
        connect_edges(polus_edges);
    } else if (id == MapId::mira) {
        connect_edges(mira_edges);
    } else {
        connect_edges(skeld_edges);
    }
    if (id == MapId::polus) {
        add_task(map, 0, "Chart course", "Set the destination", TaskType::calibrate, {-58, 18});
        add_task(map, 2, "Align telescope", "Match the signal", TaskType::calibrate, {58, 20});
        add_task(map, 4, "Fix wiring", "Restore three circuits", TaskType::wires, {-60, 20});
        add_task(map, 5, "Upload data", "Transmit the sample", TaskType::download, {58, 20});
        add_task(map, 7, "Enter code", "Use the access card", TaskType::card_swipe, {-60, 20});
        add_task(map, 9, "Restore boiler", "Synchronize the valves", TaskType::calibrate, {58, 20});
        add_task(map, 11, "Scan samples", "Upload research data", TaskType::download, {-58, 20});
    } else if (id == MapId::mira) {
        add_task(map, 0, "Enter launch code", "Swipe the access card", TaskType::card_swipe, {-58, 20});
        add_task(map, 1, "Route power", "Connect the circuit", TaskType::wires, {58, 20});
        add_task(map, 2, "Submit scan", "Transmit the scan", TaskType::download, {-60, 20});
        add_task(map, 5, "Enter ID code", "Swipe the access card", TaskType::card_swipe, {58, 20});
        add_task(map, 6, "Clear communications", "Reconnect the lines", TaskType::wires, {-58, 20});
        add_task(map, 10, "Start reactor", "Calibrate the core", TaskType::calibrate, {58, 20});
        add_task(map, 11, "Check weather node", "Align the signal", TaskType::calibrate, {-58, 20});
    } else {
        add_task(map, 0, "Start engine", "Calibrate the engine", TaskType::calibrate, {-58, 20});
        add_task(map, 1, "Inspect samples", "Upload the scan", TaskType::download, {58, 20});
        add_task(map, 2, "Swipe access card", "Card reader", TaskType::card_swipe, {-58, 20});
        add_task(map, 3, "Clear asteroids", "Align the targeting array", TaskType::calibrate, {58, 20});
        add_task(map, 5, "Repair wiring", "Connect three circuits", TaskType::wires, {-58, 20});
        add_task(map, 6, "Swipe access card", "Admin terminal", TaskType::card_swipe, {58, 20});
        add_task(map, 9, "Fix wiring", "Reconnect the power", TaskType::wires, {-58, 20});
        add_task(map, 11, "Upload navigation data", "Transmit the chart", TaskType::download, {58, 20});
    }

    return map;
}

bool GameMap::is_walkable(Vector2 position, float radius) const {
    for (const MapRoom& room : rooms) {
        const Rectangle safe{room.bounds.x + radius, room.bounds.y + radius,
                             room.bounds.width - radius * 2.0F, room.bounds.height - radius * 2.0F};
        if (CheckCollisionPointRec(position, safe)) {
            return true;
        }
    }
    for (int room_index = 0; room_index < static_cast<int>(rooms.size()); ++room_index) {
        for (const int neighbor : rooms[room_index].connections) {
            if (neighbor <= room_index) {
                continue;
            }
            const Rectangle hall = corridor_between(rooms[room_index].center, rooms[neighbor].center, 56.0F);
            const Rectangle safe{hall.x + radius, hall.y + radius, hall.width - radius * 2.0F, hall.height - radius * 2.0F};
            if (safe.width > 0.0F && safe.height > 0.0F && CheckCollisionPointRec(position, safe)) {
                return true;
            }
        }
    }
    return false;
}

int GameMap::room_at(Vector2 position) const {
    for (int index = 0; index < static_cast<int>(rooms.size()); ++index) {
        if (CheckCollisionPointRec(position, rooms[index].bounds)) {
            return index;
        }
    }
    int closest = 0;
    float closest_distance = 1.0e9F;
    for (int index = 0; index < static_cast<int>(rooms.size()); ++index) {
        const float dx = rooms[index].center.x - position.x;
        const float dy = rooms[index].center.y - position.y;
        const float candidate = dx * dx + dy * dy;
        if (candidate < closest_distance) {
            closest = index;
            closest_distance = candidate;
        }
    }
    return closest;
}

std::vector<int> GameMap::route(int from_room, int to_room) const {
    if (from_room == to_room || from_room < 0 || to_room < 0 ||
        from_room >= static_cast<int>(rooms.size()) || to_room >= static_cast<int>(rooms.size())) {
        return {};
    }

    std::vector<int> previous(rooms.size(), -1);
    std::queue<int> frontier;
    frontier.push(from_room);
    previous[from_room] = from_room;
    while (!frontier.empty()) {
        const int current = frontier.front();
        frontier.pop();
        if (current == to_room) {
            break;
        }
        for (const int neighbor : rooms[current].connections) {
            if (previous[neighbor] == -1) {
                previous[neighbor] = current;
                frontier.push(neighbor);
            }
        }
    }

    if (previous[to_room] == -1) {
        return {};
    }
    std::vector<int> path;
    for (int current = to_room; current != from_room; current = previous[current]) {
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

void GameMap::draw(Vector2 offset) const {
    Color board_color = Color{27, 39, 52, 255};
    Color room_color = ROOM_COLOR;
    Color corridor_color = FLOOR_COLOR;
    if (id == MapId::polus) {
        board_color = Color{35, 57, 74, 255};
        room_color = Color{75, 99, 111, 255};
        corridor_color = Color{104, 128, 138, 255};
    } else if (id == MapId::mira) {
        board_color = Color{35, 31, 54, 255};
        room_color = Color{76, 69, 99, 255};
        corridor_color = Color{94, 82, 114, 255};
    }
    DrawRectangleRounded({28.0F + offset.x, 92.0F + offset.y, 1192.0F, 568.0F}, 0.025F, 10, board_color);
    DrawRectangleRoundedLines({28.0F + offset.x, 92.0F + offset.y, 1192.0F, 568.0F}, 0.025F, 10, WALL_COLOR);
    if (id == MapId::polus) {
        for (int index = 0; index < 42; ++index) {
            const int x = 42 + (index * 173) % 1160;
            const int y = 108 + (index * 97) % 530;
            DrawCircle(x + static_cast<int>(offset.x), y + static_cast<int>(offset.y),
                       index % 5 == 0 ? 2.0F : 1.0F, Color{188, 217, 224, 85});
        }
    } else if (id == MapId::mira) {
        for (int index = 0; index < 9; ++index) {
            const int y = 116 + index * 62;
            DrawLine(44 + static_cast<int>(offset.x), y + static_cast<int>(offset.y),
                     1202 + static_cast<int>(offset.x), y + static_cast<int>(offset.y), Color{126, 111, 159, 35});
        }
    }

    for (int room_index = 0; room_index < static_cast<int>(rooms.size()); ++room_index) {
        for (const int neighbor : rooms[room_index].connections) {
            if (neighbor <= room_index) {
                continue;
            }
            const Rectangle hall = corridor_between(rooms[room_index].center, rooms[neighbor].center, 56.0F);
            DrawRectangleRounded({hall.x + offset.x, hall.y + offset.y, hall.width, hall.height}, 0.16F, 8, corridor_color);
            DrawRectangleRoundedLines({hall.x + offset.x, hall.y + offset.y, hall.width, hall.height}, 0.16F, 8, WALL_COLOR);
        }
    }

    for (const MapRoom& room : rooms) {
        const Rectangle bounds{room.bounds.x + offset.x, room.bounds.y + offset.y,
                               room.bounds.width, room.bounds.height};
        DrawRectangleRounded(bounds, 0.12F, 10, room_color);
        DrawRectangleRoundedLines(bounds, 0.12F, 10, WALL_COLOR);
        DrawRectangle(static_cast<int>(bounds.x + 12), static_cast<int>(bounds.y + 12), 8, 8, LINE_COLOR);
        DrawText(room.name.c_str(), static_cast<int>(bounds.x + 27), static_cast<int>(bounds.y + 7), 16, Color{216, 229, 234, 255});
        DrawLine(static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 31),
                 static_cast<int>(bounds.x + bounds.width - 14), static_cast<int>(bounds.y + 31), WALL_COLOR);
    }

    for (const TaskStation& task : tasks) {
        const Vector2 position{task.position.x + offset.x, task.position.y + offset.y};
        DrawCircleV(position, 17.0F, Color{16, 25, 35, 255});
        DrawCircleV(position, 12.0F, TASK_COLOR);
        DrawCircleV({position.x - 3.0F, position.y - 3.0F}, 3.0F, WHITE);
    }
}
