#pragma once

#include "raylib.h"

#include <array>
#include <random>

class Game {
public:
    Game();

    void update(float delta_time);
    void draw() const;

private:
    enum class Role {
        crew,
        saboteur
    };

    enum class Phase {
        playing,
        meeting,
        finished
    };

    struct Agent {
        Vector2 position{};
        Vector2 destination{};
        Role role{Role::crew};
        Color color{WHITE};
        bool alive{true};
        float action_timer{};
        float decision_timer{};
    };

    struct TaskStation {
        Vector2 position{};
        const char* name{};
        bool complete{};
    };

    static constexpr int AGENT_COUNT = 5;
    static constexpr int TASK_COUNT = 4;
    static constexpr float WORLD_WIDTH = 1280.0F;
    static constexpr float WORLD_HEIGHT = 720.0F;

    void start_round();
    void update_playing(float delta_time);
    void update_bots(float delta_time);
    void update_meeting();
    void draw_world() const;
    void draw_agent(const Agent& agent, int index) const;
    void draw_hud() const;
    void draw_meeting() const;
    void draw_finished() const;
    void begin_meeting(int reported_agent);
    void resolve_meeting();
    void update_winner();
    void start_sabotage();
    void move_toward(Agent& agent, Vector2 target, float speed, float delta_time);
    [[nodiscard]] int nearest_agent(int from_index, float max_distance) const;
    [[nodiscard]] int nearest_body(Vector2 position, float max_distance) const;
    [[nodiscard]] int nearest_task(Vector2 position, float max_distance) const;
    [[nodiscard]] int living_count(Role role) const;
    [[nodiscard]] float distance(Vector2 first, Vector2 second) const;
    [[nodiscard]] Vector2 random_destination();

    std::array<Agent, AGENT_COUNT> agents_{};
    std::array<TaskStation, TASK_COUNT> tasks_{};
    std::array<int, AGENT_COUNT> votes_{};
    std::mt19937 random_engine_;
    Phase phase_{Phase::playing};
    int selected_vote_{-1};
    int reported_agent_{-1};
    int winner_{-1};
    int completed_tasks_{};
    bool emergency_used_{};
    bool sabotage_active_{};
    bool bot_sabotage_triggered_{};
    float sabotage_timer_{};
    float round_timer_{};
    float player_kill_timer_{};
};
