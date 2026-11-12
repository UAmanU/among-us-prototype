#pragma once

#include "Map/map.hpp"
#include "Task/task_game.hpp"

#include <array>
#include <random>
#include <vector>

class Game
{
public:
    Game();
    ~Game();

    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    void update(float delta_time);
    void draw() const;

private:
    enum class Screen
    {
        menu,
        lobby,
        how_to_play,
        settings,
        playing,
        task,
        meeting,
        results
    };

    enum class Role
    {
        crew,
        impostor
    };

    struct Agent
    {
        Vector2 position{};
        Role role{Role::crew};
        int color_index{};
        int task_index{-1};
        bool alive{true};
        float action_cooldown{};
        float task_timer{};
        float decision_timer{};
    };

    struct Body
    {
        Vector2 position{};
        int agent_index{};
        bool reported{};
        float age{};
    };

    static constexpr int MAX_PLAYERS = 12;
    static constexpr float WINDOW_WIDTH = 1440.0F;
    static constexpr float WINDOW_HEIGHT = 900.0F;

    void load_sprites();
    void start_round();
    void update_menu();
    void update_lobby();
    void update_settings();
    void update_playing(float delta_time);
    void update_task(float delta_time);
    void update_meeting();
    void update_bots(float delta_time);
    void update_winner();
    void move_agent(Agent &agent, Vector2 movement, float speed, float delta_time);
    void move_agent_toward(Agent &agent, Vector2 target, float speed, float delta_time);
    void begin_meeting(int reported_agent);
    void resolve_meeting();
    void open_task(int task_index, bool repair = false);
    void finish_round();
    void start_sabotage();
    [[nodiscard]] int nearest_task(Vector2 position, float radius) const;
    [[nodiscard]] int nearest_body(Vector2 position, float radius) const;
    [[nodiscard]] int nearest_living_agent(int from_index, Role role, float radius) const;
    [[nodiscard]] int count_living(Role role) const;
    [[nodiscard]] int count_done_tasks() const;
    [[nodiscard]] float distance(Vector2 first, Vector2 second) const;
    [[nodiscard]] Vector2 random_spawn(int agent_index) const;

    void draw_menu() const;
    void draw_lobby() const;
    void draw_how_to_play() const;
    void draw_settings() const;
    void draw_game() const;
    void draw_tasks_panel() const;
    void draw_agent(const Agent &agent, int index) const;
    void draw_meeting() const;
    void draw_results() const;
    void draw_background() const;

    Screen screen_{Screen::menu};
    Screen screen_before_settings_{Screen::menu};
    GameMap map_{create_map(MapId::skeld)};
    TaskGame task_game_{};
    std::vector<Agent> agents_;
    std::vector<Body> bodies_;
    std::vector<bool> tasks_done_;
    std::vector<int> votes_;
    std::vector<float> suspicion_;
    std::array<Texture2D, MAX_PLAYERS> agent_sprites_{};
    std::mt19937 random_engine_;
    int player_count_{8};
    int impostor_count_{1};
    int selected_vote_{-1};
    int reported_agent_{-1};
    int winner_{-1};
    int active_task_{-1};
    bool emergency_used_{};
    bool sabotage_active_{};
    bool sabotage_started_by_ai_{};
    bool fullscreen_enabled_{};
    bool quit_requested_{};
    float sabotage_timer_{};
    float player_kill_cooldown_{};
    float round_time_{};

public:
    [[nodiscard]] bool should_close() const { return quit_requested_; }
    [[nodiscard]] int result_code() const { return winner_; }
};
