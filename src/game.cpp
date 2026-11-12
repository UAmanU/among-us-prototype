#include "game.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr Color BACKGROUND{13, 20, 34, 255};
constexpr Color FLOOR{39, 55, 70, 255};
constexpr Color ROOM{52, 74, 89, 255};
constexpr Color WALL{104, 132, 146, 255};
constexpr Color ACCENT{74, 219, 186, 255};
constexpr Color TEXT{234, 241, 245, 255};
constexpr Color MUTED{157, 178, 191, 255};
constexpr std::array<Color, 5> AGENT_COLORS{
    Color{75, 205, 225, 255}, Color{239, 176, 82, 255},
    Color{170, 120, 231, 255}, Color{106, 202, 126, 255},
    Color{231, 107, 116, 255}
};

void draw_panel(Rectangle rectangle, Color color = Color{10, 17, 28, 225}) {
    DrawRectangleRounded(rectangle, 0.12F, 8, color);
    DrawRectangleRoundedLines(rectangle, 0.12F, 8, Color{82, 111, 128, 255});
}
}

Game::Game() : random_engine_(std::random_device{}()) {
    start_round();
}

void Game::start_round() {
    const std::array<Vector2, AGENT_COUNT> starts{{
        {560.0F, 355.0F}, {470.0F, 315.0F}, {815.0F, 300.0F},
        {865.0F, 455.0F}, {420.0F, 480.0F}
    }};
    const std::array<const char*, TASK_COUNT> names{
        "Power relay", "Signal array", "Air filter", "Data terminal"
    };
    const std::array<Vector2, TASK_COUNT> task_positions{{
        {280.0F, 205.0F}, {965.0F, 205.0F}, {965.0F, 515.0F}, {285.0F, 520.0F}
    }};

    std::uniform_int_distribution<int> saboteur_pick(0, AGENT_COUNT - 1);
    const int saboteur_index = saboteur_pick(random_engine_);
    for (int index = 0; index < AGENT_COUNT; ++index) {
        agents_[index] = Agent{};
        agents_[index].position = starts[index];
        agents_[index].destination = random_destination();
        agents_[index].role = index == saboteur_index ? Role::saboteur : Role::crew;
        agents_[index].color = AGENT_COLORS[index];
        agents_[index].decision_timer = static_cast<float>(index) * 0.4F;
    }

    for (int index = 0; index < TASK_COUNT; ++index) {
        tasks_[index] = TaskStation{task_positions[index], names[index], false};
    }

    votes_.fill(-1);
    phase_ = Phase::playing;
    selected_vote_ = -1;
    reported_agent_ = -1;
    winner_ = -1;
    completed_tasks_ = 0;
    emergency_used_ = false;
    sabotage_active_ = false;
    bot_sabotage_triggered_ = false;
    sabotage_timer_ = 0.0F;
    round_timer_ = 0.0F;
    player_kill_timer_ = 0.0F;
}

void Game::update(float delta_time) {
    if (phase_ == Phase::finished) {
        if (IsKeyPressed(KEY_ENTER)) {
            start_round();
        }
        return;
    }

    if (phase_ == Phase::meeting) {
        update_meeting();
        return;
    }

    update_playing(std::min(delta_time, 0.05F));
}

void Game::update_playing(float delta_time) {
    round_timer_ += delta_time;
    player_kill_timer_ = std::max(0.0F, player_kill_timer_ - delta_time);
    if (sabotage_active_) {
        sabotage_timer_ -= delta_time;
        if (sabotage_timer_ <= 0.0F) {
            winner_ = static_cast<int>(Role::saboteur);
            phase_ = Phase::finished;
            return;
        }
    }

    Agent& player = agents_[0];
    if (player.alive) {
        Vector2 movement{};
        movement.x = static_cast<float>(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
                     static_cast<float>(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
        movement.y = static_cast<float>(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) -
                     static_cast<float>(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
        const float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        if (length > 0.0F) {
            movement.x /= length;
            movement.y /= length;
            player.position.x = std::clamp(player.position.x + movement.x * 220.0F * delta_time, 62.0F, WORLD_WIDTH - 62.0F);
            player.position.y = std::clamp(player.position.y + movement.y * 220.0F * delta_time, 145.0F, WORLD_HEIGHT - 58.0F);
        }

        if (IsKeyPressed(KEY_E) && player.role == Role::crew) {
            const int task_index = nearest_task(player.position, 76.0F);
            if (task_index >= 0) {
                tasks_[task_index].complete = true;
                completed_tasks_ = static_cast<int>(std::count_if(tasks_.begin(), tasks_.end(), [](const TaskStation& task) {
                    return task.complete;
                }));
            } else if (sabotage_active_ && distance(player.position, Vector2{640.0F, 355.0F}) < 105.0F) {
                sabotage_active_ = false;
                sabotage_timer_ = 0.0F;
            }
        }

        if (player.role == Role::saboteur && IsKeyPressed(KEY_F) && player_kill_timer_ <= 0.0F) {
            const int target = nearest_agent(0, 72.0F);
            if (target >= 0 && agents_[target].role == Role::crew) {
                agents_[target].alive = false;
                player_kill_timer_ = 7.0F;
            }
        }

        if (player.role == Role::saboteur && IsKeyPressed(KEY_G) && !sabotage_active_) {
            start_sabotage();
        }

        if (IsKeyPressed(KEY_R)) {
            const int body = nearest_body(player.position, 82.0F);
            if (body >= 0 && !agents_[body].alive) {
                begin_meeting(body);
                return;
            }
        }

        if (IsKeyPressed(KEY_Q) && !emergency_used_) {
            emergency_used_ = true;
            begin_meeting(-1);
            return;
        }
    }

    update_bots(delta_time);
    if (!sabotage_active_ && !bot_sabotage_triggered_ && round_timer_ > 32.0F) {
        const int saboteur = static_cast<int>(std::find_if(agents_.begin(), agents_.end(), [](const Agent& agent) {
            return agent.role == Role::saboteur;
        }) - agents_.begin());
        if (saboteur != 0) {
            start_sabotage();
            bot_sabotage_triggered_ = true;
        }
    }
    update_winner();
}

void Game::update_bots(float delta_time) {
    for (int index = 1; index < AGENT_COUNT; ++index) {
        Agent& bot = agents_[index];
        if (!bot.alive) {
            continue;
        }

        bot.action_timer = std::max(0.0F, bot.action_timer - delta_time);
        bot.decision_timer -= delta_time;
        if (bot.role == Role::saboteur) {
            const int target = nearest_agent(index, 900.0F);
            if (target >= 0 && agents_[target].role == Role::crew) {
                bot.destination = agents_[target].position;
                if (distance(bot.position, bot.destination) < 58.0F && bot.action_timer <= 0.0F) {
                    agents_[target].alive = false;
                    bot.action_timer = 9.0F;
                    bot.destination = random_destination();
                }
            }
        } else if (bot.decision_timer <= 0.0F || distance(bot.position, bot.destination) < 18.0F) {
            bot.destination = random_destination();
            bot.decision_timer = 2.0F + static_cast<float>(index) * 0.3F;
        }

        move_toward(bot, bot.destination, bot.role == Role::saboteur ? 86.0F : 58.0F, delta_time);
    }
}

void Game::update_meeting() {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_LEFT)) {
        selected_vote_ = (selected_vote_ + AGENT_COUNT) % (AGENT_COUNT + 1);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_RIGHT)) {
        selected_vote_ = (selected_vote_ + 1) % (AGENT_COUNT + 1);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        resolve_meeting();
    }
}

void Game::begin_meeting(int reported_agent) {
    reported_agent_ = reported_agent;
    selected_vote_ = 0;
    votes_.fill(-1);
    phase_ = Phase::meeting;
}

void Game::resolve_meeting() {
    votes_.fill(-1);
    if (agents_[0].alive) {
        votes_[0] = selected_vote_ == AGENT_COUNT ? -1 : selected_vote_;
        if (votes_[0] >= 0 && !agents_[votes_[0]].alive) {
            votes_[0] = -1;
        }
    }

    std::uniform_int_distribution<int> ballot(0, AGENT_COUNT);
    for (int index = 1; index < AGENT_COUNT; ++index) {
        if (!agents_[index].alive) {
            continue;
        }
        int choice = ballot(random_engine_);
        if (choice < AGENT_COUNT && !agents_[choice].alive) {
            choice = AGENT_COUNT;
        }
        votes_[index] = choice == AGENT_COUNT ? -1 : choice;
    }

    std::array<int, AGENT_COUNT> totals{};
    for (const int vote : votes_) {
        if (vote >= 0 && vote < AGENT_COUNT) {
            ++totals[vote];
        }
    }

    const int highest = *std::max_element(totals.begin(), totals.end());
    int ejected = -1;
    if (highest > 0) {
        int leaders = 0;
        for (int index = 0; index < AGENT_COUNT; ++index) {
            if (totals[index] == highest) {
                ejected = index;
                ++leaders;
            }
        }
        if (leaders != 1) {
            ejected = -1;
        }
    }
    if (ejected >= 0) {
        agents_[ejected].alive = false;
    }

    phase_ = Phase::playing;
    update_winner();
}

void Game::update_winner() {
    const bool all_tasks_done = completed_tasks_ == TASK_COUNT;
    const int crew_alive = living_count(Role::crew);
    const int saboteurs_alive = living_count(Role::saboteur);
    if (all_tasks_done || saboteurs_alive == 0) {
        winner_ = static_cast<int>(Role::crew);
        phase_ = Phase::finished;
    } else if (saboteurs_alive >= crew_alive || crew_alive == 0) {
        winner_ = static_cast<int>(Role::saboteur);
        phase_ = Phase::finished;
    }
}

void Game::start_sabotage() {
    sabotage_active_ = true;
    sabotage_timer_ = 22.0F;
    bot_sabotage_triggered_ = true;
}

void Game::move_toward(Agent& agent, Vector2 target, float speed, float delta_time) {
    const float distance_to_target = distance(agent.position, target);
    if (distance_to_target <= 1.0F) {
        return;
    }
    const float step = std::min(speed * delta_time, distance_to_target);
    agent.position.x += (target.x - agent.position.x) / distance_to_target * step;
    agent.position.y += (target.y - agent.position.y) / distance_to_target * step;
    agent.position.x = std::clamp(agent.position.x, 62.0F, WORLD_WIDTH - 62.0F);
    agent.position.y = std::clamp(agent.position.y, 145.0F, WORLD_HEIGHT - 58.0F);
}

int Game::nearest_agent(int from_index, float max_distance) const {
    int nearest = -1;
    float best_distance = max_distance;
    for (int index = 0; index < AGENT_COUNT; ++index) {
        if (index == from_index || !agents_[index].alive) {
            continue;
        }
        const float candidate = distance(agents_[from_index].position, agents_[index].position);
        if (candidate < best_distance) {
            nearest = index;
            best_distance = candidate;
        }
    }
    return nearest;
}

int Game::nearest_body(Vector2 position, float max_distance) const {
    int nearest = -1;
    float best_distance = max_distance;
    for (int index = 0; index < AGENT_COUNT; ++index) {
        if (agents_[index].alive) {
            continue;
        }
        const float candidate = distance(position, agents_[index].position);
        if (candidate < best_distance) {
            nearest = index;
            best_distance = candidate;
        }
    }
    return nearest;
}

int Game::nearest_task(Vector2 position, float max_distance) const {
    int nearest = -1;
    float best_distance = max_distance;
    for (int index = 0; index < TASK_COUNT; ++index) {
        if (tasks_[index].complete) {
            continue;
        }
        const float candidate = distance(position, tasks_[index].position);
        if (candidate < best_distance) {
            nearest = index;
            best_distance = candidate;
        }
    }
    return nearest;
}

int Game::living_count(Role role) const {
    return static_cast<int>(std::count_if(agents_.begin(), agents_.end(), [role](const Agent& agent) {
        return agent.alive && agent.role == role;
    }));
}

float Game::distance(Vector2 first, Vector2 second) const {
    return std::sqrt(std::pow(first.x - second.x, 2.0F) + std::pow(first.y - second.y, 2.0F));
}

Vector2 Game::random_destination() {
    std::uniform_real_distribution<float> x(110.0F, WORLD_WIDTH - 110.0F);
    std::uniform_real_distribution<float> y(170.0F, WORLD_HEIGHT - 95.0F);
    return {x(random_engine_), y(random_engine_)};
}

void Game::draw() const {
    draw_world();
    if (phase_ == Phase::meeting) {
        draw_meeting();
    } else if (phase_ == Phase::finished) {
        draw_finished();
    } else {
        draw_hud();
    }
}

void Game::draw_world() const {
    DrawRectangle(0, 0, static_cast<int>(WORLD_WIDTH), static_cast<int>(WORLD_HEIGHT), BACKGROUND);
    DrawRectangle(32, 126, 1216, 564, FLOOR);

    const std::array<Rectangle, 4> rooms{{
        {72, 154, 340, 215}, {868, 154, 340, 215},
        {72, 420, 340, 220}, {868, 420, 340, 220}
    }};
    const std::array<const char*, 4> labels{"RELAY", "SIGNALS", "FILTER", "ARCHIVE"};
    for (int index = 0; index < static_cast<int>(rooms.size()); ++index) {
        DrawRectangleRec(rooms[index], ROOM);
        DrawRectangleLinesEx(rooms[index], 5.0F, WALL);
        DrawText(labels[index], static_cast<int>(rooms[index].x + 18), static_cast<int>(rooms[index].y + 16), 18, MUTED);
    }

    DrawRectangle(0, 0, 1280, 112, Color{20, 32, 47, 255});
    DrawText("ORBITAL STATION  /  SOCIAL DEDUCTION", 32, 28, 26, TEXT);
    DrawText("Prototype build", 34, 65, 16, MUTED);

    for (int index = 0; index < TASK_COUNT; ++index) {
        const TaskStation& task = tasks_[index];
        const Color station_color = task.complete ? ACCENT : Color{231, 182, 91, 255};
        DrawRectangleRounded({task.position.x - 23, task.position.y - 20, 46, 40}, 0.18F, 6, station_color);
        DrawRectangleRoundedLines({task.position.x - 23, task.position.y - 20, 46, 40}, 0.18F, 6, Color{12, 21, 31, 255});
        DrawText(task.complete ? "OK" : "E", static_cast<int>(task.position.x - 10), static_cast<int>(task.position.y - 8), 18, Color{16, 28, 39, 255});
        DrawText(task.name, static_cast<int>(task.position.x - 54), static_cast<int>(task.position.y + 27), 13, TEXT);
    }

    if (sabotage_active_) {
        DrawCircleV({640.0F, 355.0F}, 42.0F, Color{201, 74, 77, 255});
        DrawText("!", 628, 326, 54, WHITE);
        DrawText("REPAIR", 603, 397, 15, TEXT);
    } else {
        DrawCircleV({640.0F, 355.0F}, 38.0F, Color{82, 105, 118, 255});
        DrawCircleV({640.0F, 355.0F}, 15.0F, ACCENT);
        DrawText("CORE", 610, 400, 15, MUTED);
    }

    for (int index = 0; index < AGENT_COUNT; ++index) {
        const Agent& agent = agents_[index];
        if (agent.alive) {
            draw_agent(agent, index);
        } else {
            DrawEllipse(static_cast<int>(agent.position.x), static_cast<int>(agent.position.y), 25.0F, 9.0F, Color{116, 60, 67, 255});
            DrawCircle(static_cast<int>(agent.position.x), static_cast<int>(agent.position.y), 5.0F, Color{232, 109, 111, 255});
        }
    }
}

void Game::draw_agent(const Agent& agent, int index) const {
    const int x = static_cast<int>(agent.position.x);
    const int y = static_cast<int>(agent.position.y);
    DrawEllipse(x, y + 17, 22.0F, 7.0F, Color{8, 13, 21, 130});
    DrawRectangleRounded({static_cast<float>(x - 15), static_cast<float>(y - 2), 30, 27}, 0.3F, 8, agent.color);
    DrawCircle(x, y - 8, 16.0F, agent.color);
    DrawEllipse(x + 6, y - 9, 9.0F, 6.0F, Color{189, 236, 242, 255});
    DrawEllipse(x + 8, y - 10, 3.0F, 2.0F, WHITE);
    if (index == 0) {
        DrawCircleLines(x, y - 3, 25.0F, TEXT);
        DrawText("YOU", x - 16, y - 48, 14, TEXT);
    }
}

void Game::draw_hud() const {
    draw_panel({18, 594, 772, 108});
    DrawText("TASKS", 38, 610, 18, ACCENT);
    DrawText(TextFormat("Progress: %d / %d", completed_tasks_, TASK_COUNT), 38, 637, 20, TEXT);
    DrawRectangle(244, 642, 500, 12, Color{38, 52, 65, 255});
    DrawRectangle(244, 642, static_cast<int>(500.0F * completed_tasks_ / TASK_COUNT), 12, ACCENT);

    const Agent& player = agents_[0];
    const char* role_text = player.role == Role::saboteur ? "Role: Saboteur" : "Role: Expedition crew";
    DrawText(role_text, 38, 670, 16, player.role == Role::saboteur ? Color{245, 114, 116, 255} : MUTED);
    DrawText("WASD / arrows: move   E: task or repair   Q: meeting   R: report", 806, 615, 17, TEXT);
    if (player.role == Role::saboteur) {
        DrawText("F: eliminate nearby crew   G: sabotage", 806, 645, 17, Color{245, 114, 116, 255});
        DrawText(TextFormat("Ability cooldown: %.1f s", player_kill_timer_), 806, 675, 16, MUTED);
    } else {
        DrawText("Complete tasks and watch the other crew members.", 806, 645, 17, MUTED);
        DrawText("A critical system can be repaired at the station core.", 806, 675, 16, MUTED);
    }

    if (sabotage_active_) {
        DrawRectangle(448, 126, 384, 42, Color{116, 40, 49, 235});
        DrawText(TextFormat("CRITICAL FAILURE  %.1f s", sabotage_timer_), 474, 137, 20, WHITE);
    }
}

void Game::draw_meeting() const {
    DrawRectangle(0, 112, 1280, 608, Color{5, 10, 18, 220});
    draw_panel({340, 145, 600, 535}, Color{19, 30, 44, 250});
    DrawText(reported_agent_ >= 0 ? "INCIDENT REPORTED" : "EMERGENCY MEETING", 394, 174, 28, TEXT);
    DrawText("Select a suspect, or choose skip. Press Enter to cast votes.", 394, 211, 17, MUTED);

    for (int row = 0; row <= AGENT_COUNT; ++row) {
        const int y = 260 + row * 55;
        const bool is_selected = row == selected_vote_;
        if (is_selected) {
            DrawRectangleRounded({382, static_cast<float>(y - 7), 516, 45}, 0.15F, 6, Color{46, 77, 91, 255});
        }
        if (row == AGENT_COUNT) {
            DrawText("Skip vote", 408, y, 21, is_selected ? ACCENT : TEXT);
        } else {
            const Agent& agent = agents_[row];
            const char* label = row == 0 ? "You" : TextFormat("Crew member %d", row);
            DrawCircle(407, y + 10, 10, agent.color);
            DrawText(label, 433, y, 21, agent.alive ? (is_selected ? ACCENT : TEXT) : MUTED);
            if (!agent.alive) {
                DrawText("OUT", 814, y + 2, 16, Color{235, 119, 119, 255});
            }
        }
    }

    DrawText("Use arrow keys to change selection. Ties eject nobody.", 394, 620, 17, MUTED);
}

void Game::draw_finished() const {
    DrawRectangle(0, 0, 1280, 720, Color{5, 10, 18, 210});
    draw_panel({300, 236, 680, 246}, Color{18, 31, 45, 250});
    const bool crew_won = winner_ == static_cast<int>(Role::crew);
    const char* heading = crew_won ? "EXPEDITION SURVIVED" : "SABOTEUR VICTORY";
    DrawText(heading, 376, 284, 34, crew_won ? ACCENT : Color{246, 111, 116, 255});
    DrawText("Press Enter to start a new round.", 432, 362, 22, TEXT);
    DrawText("Roles are revealed at the end of each round.", 412, 405, 18, MUTED);

    for (int index = 0; index < AGENT_COUNT; ++index) {
        const char* role = agents_[index].role == Role::saboteur ? "Saboteur" : "Crew";
        const char* name = index == 0 ? "You" : TextFormat("Agent %d", index);
        DrawText(TextFormat("%s: %s%s", name, role, agents_[index].alive ? "" : " (eliminated)"),
                 330 + (index % 3) * 210, 520 + (index / 3) * 30, 16, agents_[index].color);
    }
}

