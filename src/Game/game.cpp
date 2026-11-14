#include "Game/game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {
constexpr Color INK{9, 14, 24, 255};
constexpr Color PANEL{20, 29, 43, 248};
constexpr Color PANEL_LIGHT{34, 47, 63, 255};
constexpr Color WHITE_TEXT{237, 242, 246, 255};
constexpr Color MUTED{158, 176, 190, 255};
constexpr Color CYAN{84, 220, 230, 255};
constexpr Color UI_LIME{115, 220, 139, 255};
constexpr Color UI_RED{245, 94, 103, 255};
constexpr std::array<Color, 12> PLAYER_COLORS{
    Color{220, 57, 62, 255}, Color{48, 106, 220, 255}, Color{40, 177, 83, 255},
    Color{242, 210, 71, 255}, Color{151, 79, 188, 255}, Color{235, 128, 49, 255},
    Color{72, 206, 207, 255}, Color{242, 156, 185, 255}, Color{135, 137, 143, 255},
    Color{113, 70, 38, 255}, Color{223, 225, 226, 255}, Color{99, 56, 165, 255}
};

bool button_pressed(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void draw_button(Rectangle bounds, const char* label, bool emphasized = false) {
    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, bounds);
    const Color base = emphasized ? Color{39, 162, 150, 255} : PANEL_LIGHT;
    const Color fill = hovered ? (emphasized ? Color{57, 197, 177, 255} : Color{54, 74, 94, 255}) : base;
    DrawRectangleRounded(bounds, 0.16F, 10, fill);
    DrawRectangleRoundedLines(bounds, 0.16F, 10, hovered ? WHITE_TEXT : Color{91, 116, 135, 255});
    const int text_width = MeasureText(label, 21);
    DrawText(label, static_cast<int>(bounds.x + (bounds.width - text_width) / 2.0F),
             static_cast<int>(bounds.y + (bounds.height - 22.0F) / 2.0F), 21, WHITE_TEXT);
}

float length(Vector2 vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

const char* map_description(MapId id) {
    switch (id) {
    case MapId::polus: return "Snowy research base with outdoor paths.";
    case MapId::mira: return "Compact high-altitude station.";
    case MapId::skeld:
    default: return "Deep-space vessel with central and side routes.";
    }
}

const char* role_name(bool impostor) {
    return impostor ? "IMPOSTOR" : "CREWMATE";
}

int impostor_limit(int players) {
    return std::clamp((players - 1) / 3, 1, 3);
}
}

Game::Game() : random_engine_(std::random_device{}()) {
    load_sprites();
}

Game::~Game() {
    for (Texture2D& sprite : agent_sprites_) {
        if (sprite.id != 0) {
            UnloadTexture(sprite);
        }
    }
}

void Game::load_sprites() {
    Image source = LoadImage("images/image-8.png");
    if (source.data == nullptr) {
        source = LoadImage("../images/image-8.png");
    }
    if (source.data == nullptr) {
        return;
    }

    ImageResize(&source, 128, 138);
    ImageFormat(&source, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* base_pixels = LoadImageColors(source);
    if (base_pixels == nullptr) {
        UnloadImage(source);
        return;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(source.width) * static_cast<std::size_t>(source.height);
    for (int player = 0; player < MAX_PLAYERS; ++player) {
        auto* pixels = static_cast<Color*>(std::malloc(pixel_count * sizeof(Color)));
        if (pixels == nullptr) {
            break;
        }
        std::memcpy(pixels, base_pixels, pixel_count * sizeof(Color));
        const Color tint = PLAYER_COLORS[player];
        for (std::size_t index = 0; index < pixel_count; ++index) {
            Color& pixel = pixels[index];
            if (pixel.a == 0 || pixel.r < 38 || pixel.r <= pixel.g * 1.2F || pixel.r <= pixel.b * 1.2F) {
                continue;
            }
            const float brightness = std::clamp((static_cast<float>(pixel.r) + pixel.g + pixel.b) / 390.0F, 0.25F, 1.0F);
            pixel.r = static_cast<unsigned char>(static_cast<float>(tint.r) * brightness);
            pixel.g = static_cast<unsigned char>(static_cast<float>(tint.g) * brightness);
            pixel.b = static_cast<unsigned char>(static_cast<float>(tint.b) * brightness);
        }
        Image tinted{pixels, source.width, source.height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        agent_sprites_[player] = LoadTextureFromImage(tinted);
        std::free(pixels);
    }

    UnloadImageColors(base_pixels);
    UnloadImage(source);
}

void Game::update(float delta_time) {
    const float safe_delta = std::clamp(delta_time, 0.0F, 0.05F);
    switch (screen_) {
    case Screen::menu: update_menu(); break;
    case Screen::lobby: update_lobby(); break;
    case Screen::playing: update_playing(safe_delta); break;
    case Screen::task: update_task(safe_delta); break;
    case Screen::meeting: update_meeting(); break;
    case Screen::how_to_play:
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || button_pressed({270, 713, 260, 56})) {
            screen_ = screen_before_settings_;
        }
        break;
    case Screen::settings: update_settings(); break;
    case Screen::results:
        if (IsKeyPressed(KEY_ENTER)) {
            screen_ = Screen::lobby;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            screen_ = Screen::menu;
        }
        break;
    }
}

void Game::update_menu() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        return;
    }
    if (button_pressed({118, 443, 360, 66})) {
        screen_ = Screen::lobby;
    }
    if (button_pressed({118, 527, 360, 58})) {
        screen_before_settings_ = Screen::menu;
        screen_ = Screen::how_to_play;
    }
    if (button_pressed({118, 601, 360, 58})) {
        screen_before_settings_ = Screen::menu;
        screen_ = Screen::settings;
    }
    if (button_pressed({118, 675, 360, 58})) {
        quit_requested_ = true;
    }
}

void Game::update_lobby() {
    const std::array<MapId, 3> maps{MapId::skeld, MapId::polus, MapId::mira};
    int map_index = static_cast<int>(map_.id);
    if (button_pressed({74, 226, 64, 50})) {
        map_index = (map_index + static_cast<int>(maps.size()) - 1) % static_cast<int>(maps.size());
        map_ = create_map(maps[map_index]);
    }
    if (button_pressed({278, 226, 64, 50})) {
        map_index = (map_index + 1) % static_cast<int>(maps.size());
        map_ = create_map(maps[map_index]);
    }

    if (button_pressed({74, 354, 54, 52})) {
        player_count_ = std::max(4, player_count_ - 1);
        impostor_count_ = std::min(impostor_count_, impostor_limit(player_count_));
    }
    if (button_pressed({278, 354, 54, 52})) {
        player_count_ = std::min(MAX_PLAYERS, player_count_ + 1);
        impostor_count_ = std::min(impostor_count_, impostor_limit(player_count_));
    }
    if (button_pressed({74, 470, 54, 52})) {
        impostor_count_ = std::max(1, impostor_count_ - 1);
    }
    if (button_pressed({278, 470, 54, 52})) {
        impostor_count_ = std::min(impostor_limit(player_count_), impostor_count_ + 1);
    }
    if (button_pressed({60, 600, 280, 65})) {
        start_round();
    }
    if (button_pressed({60, 682, 280, 54})) {
        screen_ = Screen::menu;
    }
}

void Game::update_settings() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        screen_ = screen_before_settings_;
        return;
    }
    if (button_pressed({468, 365, 440, 58})) {
        ToggleFullscreen();
        fullscreen_enabled_ = !fullscreen_enabled_;
    }
    if (button_pressed({468, 608, 230, 56})) {
        screen_ = screen_before_settings_;
    }
}

void Game::start_round() {
    map_ = create_map(map_.id);
    agents_.clear();
    bodies_.clear();
    tasks_done_.assign(map_.tasks.size(), false);
    votes_.assign(player_count_, -1);
    suspicion_.assign(player_count_, 0.0F);

    std::vector<int> roles(player_count_, static_cast<int>(Role::crew));
    std::fill_n(roles.begin(), impostor_count_, static_cast<int>(Role::impostor));
    std::shuffle(roles.begin(), roles.end(), random_engine_);
    for (int index = 0; index < player_count_; ++index) {
        Agent agent;
        agent.position = random_spawn(index);
        agent.role = roles[index] == static_cast<int>(Role::impostor) ? Role::impostor : Role::crew;
        agent.color_index = index;
        agent.decision_timer = 0.5F + static_cast<float>(index % 4) * 0.55F;
        agents_.push_back(agent);
    }

    selected_vote_ = -1;
    reported_agent_ = -1;
    winner_ = -1;
    active_task_ = -1;
    emergency_used_ = false;
    sabotage_active_ = false;
    sabotage_started_by_ai_ = false;
    sabotage_timer_ = 0.0F;
    player_kill_cooldown_ = 0.0F;
    round_time_ = 0.0F;
    screen_ = Screen::playing;
}

Vector2 Game::random_spawn(int agent_index) const {
    const int room = (agent_index * 5 + 2) % static_cast<int>(map_.rooms.size());
    return map_.rooms[room].center;
}

void Game::update_playing(float delta_time) {
    round_time_ += delta_time;
    player_kill_cooldown_ = std::max(0.0F, player_kill_cooldown_ - delta_time);
    if (sabotage_active_) {
        sabotage_timer_ -= delta_time;
        if (sabotage_timer_ <= 0.0F) {
            winner_ = static_cast<int>(Role::impostor);
            finish_round();
            return;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        screen_before_settings_ = Screen::playing;
        screen_ = Screen::settings;
        return;
    }

    Agent& player = agents_[0];
    if (player.alive) {
        Vector2 movement{
            static_cast<float>(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - static_cast<float>(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)),
            static_cast<float>(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - static_cast<float>(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        };
        move_agent(player, movement, 205.0F, delta_time);

        if (IsKeyPressed(KEY_E)) {
            const int task = nearest_task(player.position, 68.0F);
            if (task >= 0 && player.role == Role::crew && !tasks_done_[task]) {
                open_task(task);
                return;
            }
            if (sabotage_active_ && player.role == Role::crew) {
                int repair_room = 4;
                for (int room = 0; room < static_cast<int>(map_.rooms.size()); ++room) {
                    const std::string& name = map_.rooms[room].name;
                    if (name == "Reactor" || name == "Boiler Room" || name == "Laboratory") {
                        repair_room = room;
                        break;
                    }
                }
                if (distance(player.position, map_.rooms[repair_room].center) < 86.0F) {
                    open_task(-1, true);
                    return;
                }
            }
        }

        if (player.role == Role::impostor && IsKeyPressed(KEY_F) && player_kill_cooldown_ <= 0.0F) {
            const int target = nearest_living_agent(0, Role::crew, 68.0F);
            if (target >= 0) {
                bodies_.push_back({agents_[target].position, target, false, 0.0F});
                agents_[target].alive = false;
                player_kill_cooldown_ = 12.0F;
                for (int observer = 1; observer < static_cast<int>(agents_.size()); ++observer) {
                    if (agents_[observer].alive && distance(agents_[observer].position, agents_[target].position) < 220.0F) {
                        suspicion_[0] += 2.0F;
                    }
                }
            }
        }

        if (player.role == Role::impostor && IsKeyPressed(KEY_G) && !sabotage_active_) {
            start_sabotage();
        }

        if (IsKeyPressed(KEY_R)) {
            const int body = nearest_body(player.position, 76.0F);
            if (body >= 0) {
                bodies_[body].reported = true;
                begin_meeting(bodies_[body].agent_index);
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
    for (Body& body : bodies_) {
        if (body.reported) {
            continue;
        }
        body.age += delta_time;
        if (body.age > 12.0F) {
            body.reported = true;
            begin_meeting(body.agent_index);
            return;
        }
    }

    if (!sabotage_active_ && !sabotage_started_by_ai_ && round_time_ > 40.0F && count_living(Role::impostor) > 0) {
        const bool player_is_impostor = agents_[0].role == Role::impostor;
        if (!player_is_impostor) {
            start_sabotage();
            sabotage_started_by_ai_ = true;
        }
    }
    update_winner();
}

void Game::update_bots(float delta_time) {
    std::uniform_int_distribution<int> random_room(0, static_cast<int>(map_.rooms.size()) - 1);
    for (int index = 1; index < static_cast<int>(agents_.size()); ++index) {
        Agent& bot = agents_[index];
        if (!bot.alive) {
            continue;
        }
        bot.action_cooldown = std::max(0.0F, bot.action_cooldown - delta_time);
        bot.decision_timer -= delta_time;

        if (bot.role == Role::impostor) {
            const int target = nearest_living_agent(index, Role::crew, 390.0F);
            if (target >= 0) {
                move_agent_toward(bot, agents_[target].position, 88.0F, delta_time);
                if (distance(bot.position, agents_[target].position) < 46.0F && bot.action_cooldown <= 0.0F) {
                    bodies_.push_back({agents_[target].position, target, false, 0.0F});
                    agents_[target].alive = false;
                    bot.action_cooldown = 12.0F;
                    for (int observer = 0; observer < static_cast<int>(agents_.size()); ++observer) {
                        if (observer != index && agents_[observer].alive && distance(agents_[observer].position, agents_[target].position) < 250.0F) {
                            suspicion_[index] += 2.5F;
                        }
                    }
                }
            } else if (bot.decision_timer <= 0.0F) {
                const int target_room = random_room(random_engine_);
                move_agent_toward(bot, map_.rooms[target_room].center, 66.0F, delta_time);
                bot.decision_timer = 2.2F;
            }
            continue;
        }

        if (bot.task_index < 0 || tasks_done_[bot.task_index]) {
            bot.task_index = -1;
            float nearest = 1.0e9F;
            for (int task = 0; task < static_cast<int>(map_.tasks.size()); ++task) {
                if (tasks_done_[task]) {
                    continue;
                }
                const float candidate = distance(bot.position, map_.tasks[task].position);
                if (candidate < nearest) {
                    nearest = candidate;
                    bot.task_index = task;
                }
            }
        }

        if (bot.task_index >= 0) {
            const TaskStation& task = map_.tasks[bot.task_index];
            move_agent_toward(bot, task.position, 72.0F, delta_time);
            if (distance(bot.position, task.position) < 31.0F) {
                bot.task_timer += delta_time;
                if (bot.task_timer > 4.5F) {
                    tasks_done_[bot.task_index] = true;
                    bot.task_index = -1;
                    bot.task_timer = 0.0F;
                }
            }
        } else if (bot.decision_timer <= 0.0F) {
            move_agent_toward(bot, map_.rooms[random_room(random_engine_)].center, 60.0F, delta_time);
            bot.decision_timer = 2.0F;
        }
    }
}

void Game::update_task(float delta_time) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        screen_ = Screen::playing;
        return;
    }
    if (sabotage_active_) {
        sabotage_timer_ -= delta_time;
        if (sabotage_timer_ <= 0.0F) {
            winner_ = static_cast<int>(Role::impostor);
            finish_round();
            return;
        }
    }
    task_game_.update(delta_time);
    if (!task_game_.is_finished()) {
        return;
    }
    if (task_game_.succeeded()) {
        if (active_task_ >= 0) {
            tasks_done_[active_task_] = true;
        } else {
            sabotage_active_ = false;
            sabotage_timer_ = 0.0F;
        }
    }
    active_task_ = -1;
    screen_ = Screen::playing;
    update_winner();
}

void Game::open_task(int task_index, bool repair) {
    active_task_ = repair ? -1 : task_index;
    if (repair) {
        TaskStation repair_task{"Restore critical system", "Reconnect the four emergency circuits", TaskType::wires, 0, {}};
        task_game_.begin(repair_task);
    } else {
        task_game_.begin(map_.tasks[task_index]);
    }
    screen_ = Screen::task;
}

void Game::begin_meeting(int reported_agent) {
    if (screen_ == Screen::results) {
        return;
    }
    reported_agent_ = reported_agent;
    selected_vote_ = -1;
    std::fill(votes_.begin(), votes_.end(), -2);
    screen_ = Screen::meeting;
}

void Game::update_meeting() {
    const int skip_index = static_cast<int>(agents_.size());
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_LEFT)) {
        selected_vote_ = selected_vote_ < 0 ? skip_index : (selected_vote_ + skip_index) % (skip_index + 1);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_RIGHT)) {
        selected_vote_ = selected_vote_ < 0 ? 0 : (selected_vote_ + 1) % (skip_index + 1);
    }
    const Vector2 mouse = GetMousePosition();
    for (int index = 0; index < skip_index; ++index) {
        const Rectangle row{430.0F, 262.0F + static_cast<float>(index) * 43.0F, 580.0F, 38.0F};
        if (agents_[index].alive && CheckCollisionPointRec(mouse, row) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected_vote_ = index;
        }
    }
    const Rectangle skip_row{430.0F, 262.0F + static_cast<float>(skip_index) * 43.0F, 580.0F, 38.0F};
    if (CheckCollisionPointRec(mouse, skip_row) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        selected_vote_ = skip_index;
    }
    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) && selected_vote_ >= 0) {
        resolve_meeting();
    }
}

void Game::resolve_meeting() {
    const int count = static_cast<int>(agents_.size());
    const int skip_index = count;
    std::fill(votes_.begin(), votes_.end(), -1);
    if (agents_[0].alive && selected_vote_ >= 0) {
        votes_[0] = selected_vote_ < count && agents_[selected_vote_].alive ? selected_vote_ : skip_index;
    }

    for (int voter = 1; voter < count; ++voter) {
        if (!agents_[voter].alive) {
            continue;
        }
        int suspect = -1;
        float highest_suspicion = 0.0F;
        for (int target = 0; target < count; ++target) {
            if (target != voter && agents_[target].alive && suspicion_[target] > highest_suspicion) {
                highest_suspicion = suspicion_[target];
                suspect = target;
            }
        }
        std::uniform_int_distribution<int> choose(0, count);
        if (suspect >= 0 && highest_suspicion >= 2.0F) {
            votes_[voter] = suspect;
        } else {
            const int choice = choose(random_engine_);
            votes_[voter] = choice == skip_index || !agents_[choice].alive ? skip_index : choice;
        }
    }

    std::vector<int> totals(count + 1, 0);
    for (const int vote : votes_) {
        if (vote >= 0 && vote <= count) {
            ++totals[vote];
        }
    }
    const int highest = *std::max_element(totals.begin(), totals.end());
    int ejected = -1;
    int leaders = 0;
    for (int candidate = 0; candidate < count; ++candidate) {
        if (totals[candidate] == highest && highest > totals[skip_index]) {
            ejected = candidate;
            ++leaders;
        }
    }
    if (leaders != 1) {
        ejected = -1;
    }
    if (ejected >= 0) {
        agents_[ejected].alive = false;
    }
    for (Body& body : bodies_) {
        body.reported = true;
    }
    update_winner();
    if (screen_ != Screen::results) {
        screen_ = Screen::playing;
    }
}

void Game::update_winner() {
    if (count_done_tasks() == static_cast<int>(tasks_done_.size()) || count_living(Role::impostor) == 0) {
        winner_ = static_cast<int>(Role::crew);
        finish_round();
    } else if (count_living(Role::impostor) >= count_living(Role::crew) || count_living(Role::crew) == 0) {
        winner_ = static_cast<int>(Role::impostor);
        finish_round();
    }
}

void Game::finish_round() {
    screen_ = Screen::results;
}

void Game::start_sabotage() {
    sabotage_active_ = true;
    sabotage_timer_ = 45.0F;
}

void Game::move_agent(Agent& agent, Vector2 movement, float speed, float delta_time) {
    const float movement_length = length(movement);
    if (movement_length < 0.01F) {
        return;
    }
    movement.x /= movement_length;
    movement.y /= movement_length;
    const Vector2 next{agent.position.x + movement.x * speed * delta_time,
                       agent.position.y + movement.y * speed * delta_time};
    if (map_.is_walkable(next)) {
        agent.position = next;
        return;
    }
    const Vector2 next_x{next.x, agent.position.y};
    const Vector2 next_y{agent.position.x, next.y};
    if (map_.is_walkable(next_x)) {
        agent.position.x = next_x.x;
    }
    if (map_.is_walkable(next_y)) {
        agent.position.y = next_y.y;
    }
}

void Game::move_agent_toward(Agent& agent, Vector2 target, float speed, float delta_time) {
    const int current_room = map_.room_at(agent.position);
    const int target_room = map_.room_at(target);
    Vector2 waypoint = target;
    if (current_room != target_room) {
        const std::vector<int> path = map_.route(current_room, target_room);
        if (!path.empty()) {
            waypoint = map_.rooms[path.front()].center;
        }
    }
    const Vector2 direction{waypoint.x - agent.position.x, waypoint.y - agent.position.y};
    move_agent(agent, direction, speed, delta_time);
}

int Game::nearest_task(Vector2 position, float radius) const {
    int nearest = -1;
    float best = radius;
    for (int index = 0; index < static_cast<int>(map_.tasks.size()); ++index) {
        if (tasks_done_[index]) {
            continue;
        }
        const float candidate = distance(position, map_.tasks[index].position);
        if (candidate < best) {
            nearest = index;
            best = candidate;
        }
    }
    return nearest;
}

int Game::nearest_body(Vector2 position, float radius) const {
    int nearest = -1;
    float best = radius;
    for (int index = 0; index < static_cast<int>(bodies_.size()); ++index) {
        if (bodies_[index].reported) {
            continue;
        }
        const float candidate = distance(position, bodies_[index].position);
        if (candidate < best) {
            nearest = index;
            best = candidate;
        }
    }
    return nearest;
}

int Game::nearest_living_agent(int from_index, Role role, float radius) const {
    int nearest = -1;
    float best = radius;
    for (int index = 0; index < static_cast<int>(agents_.size()); ++index) {
        if (index == from_index || !agents_[index].alive || agents_[index].role != role) {
            continue;
        }
        const float candidate = distance(agents_[from_index].position, agents_[index].position);
        if (candidate < best) {
            nearest = index;
            best = candidate;
        }
    }
    return nearest;
}

int Game::count_living(Role role) const {
    return static_cast<int>(std::count_if(agents_.begin(), agents_.end(), [role](const Agent& agent) {
        return agent.alive && agent.role == role;
    }));
}

int Game::count_done_tasks() const {
    return static_cast<int>(std::count(tasks_done_.begin(), tasks_done_.end(), true));
}

float Game::distance(Vector2 first, Vector2 second) const {
    const float dx = first.x - second.x;
    const float dy = first.y - second.y;
    return std::sqrt(dx * dx + dy * dy);
}

void Game::draw() const {
    switch (screen_) {
    case Screen::menu: draw_menu(); break;
    case Screen::lobby: draw_lobby(); break;
    case Screen::how_to_play: draw_how_to_play(); break;
    case Screen::settings: draw_settings(); break;
    case Screen::playing: draw_game(); break;
    case Screen::task:
        draw_game();
        task_game_.draw();
        break;
    case Screen::meeting:
        draw_game();
        draw_meeting();
        break;
    case Screen::results:
        draw_game();
        draw_results();
        break;
    }
}

void Game::draw_background() const {
    DrawRectangleGradientV(0, 0, static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT),
                           Color{12, 24, 42, 255}, INK);
    for (int index = 0; index < 95; ++index) {
        const int x = (index * 197 + 43) % static_cast<int>(WINDOW_WIDTH);
        const int y = (index * 113 + 17) % static_cast<int>(WINDOW_HEIGHT);
        const int radius = index % 11 == 0 ? 2 : 1;
        const unsigned char alpha = static_cast<unsigned char>(75 + (index * 29) % 130);
        DrawCircle(x, y, static_cast<float>(radius), Color{160, 208, 221, alpha});
    }
    DrawCircleGradient({1135.0F, 383.0F}, 285.0F, Color{41, 105, 132, 75}, Color{10, 16, 27, 0});
}

void Game::draw_menu() const {
    draw_background();
    DrawText("AMONG US", 112, 151, 76, WHITE_TEXT);
    DrawText("LOCAL PROTOTYPE", 120, 239, 22, CYAN);
    DrawText("A crew. A hidden impostor. One way home.", 120, 300, 22, MUTED);
    DrawText("PLAY WITH BOTS   /   THREE MAPS   /   TASK MINIGAMES", 120, 351, 16, Color{117, 143, 160, 255});

    DrawRectangleRounded({92, 414, 412, 361}, 0.045F, 12, Color{12, 20, 31, 170});
    draw_button({118, 443, 360, 66}, "LOCAL GAME", true);
    draw_button({118, 527, 360, 58}, "HOW TO PLAY");
    draw_button({118, 601, 360, 58}, "SETTINGS");
    draw_button({118, 675, 360, 58}, "QUIT");

    if (agent_sprites_[0].id != 0) {
        const Texture2D sprite = agent_sprites_[0];
        DrawTexturePro(sprite, {0, 0, static_cast<float>(sprite.width), static_cast<float>(sprite.height)},
                       {895, 248, 355, 381}, {0, 0}, 0.0F, WHITE);
        DrawText("CREWMATE", 967, 648, 20, WHITE_TEXT);
        DrawText("...or impostor?", 960, 679, 18, MUTED);
    }
    DrawText("VERSION 2.0   |   WASD / MOUSE / KEYBOARD", 120, 842, 16, Color{105, 129, 147, 255});
}

void Game::draw_lobby() const {
    draw_background();
    map_.draw();
    DrawRectangleRounded({32, 92, 350, 772}, 0.04F, 12, PANEL);
    DrawText("LOCAL LOBBY", 62, 118, 17, CYAN);
    DrawText(map_.name.c_str(), 62, 149, 31, WHITE_TEXT);
    DrawText(map_description(map_.id), 62, 192, 15, MUTED);
    draw_button({74, 226, 64, 50}, "<");
    draw_button({278, 226, 64, 50}, ">");
    DrawText(map_.subtitle.c_str(), 62, 293, 15, MUTED);

    DrawLine(62, 326, 350, 326, Color{67, 86, 103, 255});
    DrawText("PLAYERS", 62, 340, 16, CYAN);
    DrawText(TextFormat("%d", player_count_), 177, 356, 35, WHITE_TEXT);
    DrawText("4 - 12", 177, 392, 14, MUTED);
    draw_button({74, 354, 54, 52}, "-");
    draw_button({278, 354, 54, 52}, "+");

    DrawLine(62, 438, 350, 438, Color{67, 86, 103, 255});
    DrawText("IMPOSTORS", 62, 456, 16, UI_RED);
    DrawText(TextFormat("%d", impostor_count_), 177, 472, 35, WHITE_TEXT);
    DrawText(TextFormat("Up to %d for this crew", impostor_limit(player_count_)), 140, 510, 14, MUTED);
    draw_button({74, 470, 54, 52}, "-");
    draw_button({278, 470, 54, 52}, "+");

    DrawText(TextFormat("%d players will enter this round", player_count_), 62, 556, 15, WHITE_TEXT);
    DrawText(TextFormat("%d interactive stations", static_cast<int>(map_.tasks.size())), 62, 579, 15, MUTED);
    draw_button({60, 600, 280, 65}, "START GAME", true);
    draw_button({60, 682, 280, 54}, "BACK TO MENU");

    DrawRectangleRounded({421, 694, 970, 174}, 0.04F, 10, PANEL);
    DrawText("MAP PREVIEW", 456, 716, 15, CYAN);
    DrawText(map_.name.c_str(), 456, 742, 24, WHITE_TEXT);
    DrawText(map_.subtitle.c_str(), 456, 778, 16, MUTED);
    DrawText("Choose from The Skeld, Polus, or MIRA HQ.", 456, 815, 16, MUTED);
    DrawText("Bots fill every open player slot.", 456, 840, 16, MUTED);
}

void Game::draw_how_to_play() const {
    draw_background();
    DrawRectangleRounded({214, 130, 1012, 644}, 0.04F, 12, PANEL);
    DrawText("HOW TO PLAY", 270, 173, 34, WHITE_TEXT);
    DrawText("Crewmates", 270, 246, 22, CYAN);
    DrawText("Complete station tasks. Watch who leaves an area, report bodies, and vote.", 270, 282, 19, WHITE_TEXT);
    DrawText("Impostors", 270, 345, 22, UI_RED);
    DrawText("Blend in, eliminate crewmates, and use sabotage to split the group.", 270, 381, 19, WHITE_TEXT);
    DrawText("Controls", 270, 444, 22, CYAN);
    DrawText("WASD / arrows   Move", 270, 485, 19, WHITE_TEXT);
    DrawText("E   Use a station and start its task", 270, 520, 19, WHITE_TEXT);
    DrawText("R   Report a nearby body       Q   Emergency meeting", 270, 555, 19, WHITE_TEXT);
    DrawText("F   Eliminate nearby crew (impostor)       G   Sabotage", 270, 590, 19, WHITE_TEXT);
    DrawText("Task panels: mouse, Space, and Enter depending on the station.", 270, 640, 18, MUTED);
    DrawText("Arrow keys or mouse choose a vote. Ties and skip votes eject nobody.", 270, 673, 18, MUTED);
    draw_button({270, 713, 260, 56}, "BACK (ESC)");
}

void Game::draw_settings() const {
    draw_background();
    DrawRectangleRounded({400, 200, 640, 500}, 0.05F, 12, PANEL);
    DrawText("SETTINGS", 468, 244, 34, WHITE_TEXT);
    DrawText("Window mode", 468, 332, 20, MUTED);
    DrawText(fullscreen_enabled_ ? "Fullscreen" : "Windowed", 700, 332, 20, WHITE_TEXT);
    draw_button({468, 365, 440, 58}, fullscreen_enabled_ ? "SWITCH TO WINDOWED" : "SWITCH TO FULLSCREEN");
    DrawText("Input", 468, 470, 20, MUTED);
    DrawText("Keyboard movement and mouse-driven task panels", 468, 505, 18, WHITE_TEXT);
    draw_button({468, 608, 230, 56}, "BACK (ESC)");
}

void Game::draw_game() const {
    DrawRectangleGradientV(0, 0, 1440, 900, Color{14, 23, 35, 255}, INK);
    DrawRectangle(0, 0, 1440, 83, Color{10, 16, 27, 245});
    DrawText(map_.name.c_str(), 34, 17, 24, WHITE_TEXT);
    DrawText(TextFormat("CREWMATES  %d     IMPOSTORS  %d", count_living(Role::crew), count_living(Role::impostor)), 264, 24, 16, MUTED);
    if (sabotage_active_) {
        DrawText(TextFormat("CRITICAL SABOTAGE  %.1f", sabotage_timer_), 1076, 24, 16, UI_RED);
    }

    map_.draw();
    for (const Body& body : bodies_) {
        if (body.reported) {
            continue;
        }
        DrawEllipse(static_cast<int>(body.position.x), static_cast<int>(body.position.y), 23.0F, 8.0F, Color{84, 25, 37, 255});
        DrawCircle(static_cast<int>(body.position.x), static_cast<int>(body.position.y), 5.0F, UI_RED);
    }
    for (int index = 0; index < static_cast<int>(agents_.size()); ++index) {
        if (agents_[index].alive) {
            draw_agent(agents_[index], index);
        }
    }

    DrawRectangleRounded({1230, 96, 190, 642}, 0.04F, 8, PANEL);
    draw_tasks_panel();
    DrawRectangle(0, 754, 1440, 146, Color{9, 14, 23, 246});
    DrawText("WASD  Move", 32, 780, 17, WHITE_TEXT);
    DrawText("E  Use", 32, 812, 17, CYAN);
    DrawText("Q  Meeting", 32, 844, 17, WHITE_TEXT);
    DrawText("R  Report", 32, 876, 17, WHITE_TEXT);
    DrawText(TextFormat("TASK PROGRESS   %d / %d", count_done_tasks(), static_cast<int>(tasks_done_.size())), 240, 785, 18, CYAN);
    DrawRectangleRounded({240, 818, 705, 17}, 0.3F, 6, Color{39, 54, 68, 255});
    const float ratio = tasks_done_.empty() ? 0.0F : static_cast<float>(count_done_tasks()) / static_cast<float>(tasks_done_.size());
    DrawRectangleRounded({240, 818, 705.0F * ratio, 17}, 0.3F, 6, UI_LIME);
    const Agent& player = agents_[0];
    DrawText(TextFormat("YOUR ROLE  %s", role_name(player.role == Role::impostor)), 1001, 784, 18,
             player.role == Role::impostor ? UI_RED : WHITE_TEXT);
    DrawText(player.alive ? "Stay alert. Meetings pause the round." : "You are out. Watch the rest of the round.", 1001, 818, 16, MUTED);
    if (player.role == Role::impostor) {
        DrawText("F  Eliminate     G  Sabotage", 1001, 852, 16, UI_RED);
    }
}

void Game::draw_tasks_panel() const {
    DrawText("TASK LIST", 1250, 112, 16, CYAN);
    for (int index = 0; index < static_cast<int>(map_.tasks.size()); ++index) {
        const int y = 150 + index * 76;
        const TaskStation& task = map_.tasks[index];
        DrawCircle(1259, y + 10, 6, tasks_done_[index] ? UI_LIME : Color{239, 187, 87, 255});
        DrawText(task.name.c_str(), 1275, y, 14, WHITE_TEXT);
        DrawText(task.detail.c_str(), 1250, y + 24, 12, MUTED);
        if (index < static_cast<int>(map_.tasks.size()) - 1) {
            DrawLine(1248, y + 51, 1402, y + 51, Color{50, 66, 81, 255});
        }
    }
}

void Game::draw_agent(const Agent& agent, int index) const {
    if (index < MAX_PLAYERS && agent_sprites_[index].id != 0) {
        const Texture2D sprite = agent_sprites_[index];
        DrawTexturePro(sprite, {0, 0, static_cast<float>(sprite.width), static_cast<float>(sprite.height)},
                       {agent.position.x - 22.0F, agent.position.y - 28.0F, 44.0F, 50.0F},
                       {0, 0}, 0.0F, WHITE);
    } else {
        const Color color = PLAYER_COLORS[agent.color_index];
        DrawEllipse(static_cast<int>(agent.position.x), static_cast<int>(agent.position.y + 16), 22.0F, 7.0F, Color{0, 0, 0, 120});
        DrawRectangleRounded({agent.position.x - 15, agent.position.y - 3, 30, 28}, 0.35F, 8, color);
        DrawCircleV({agent.position.x, agent.position.y - 7}, 16.0F, color);
        DrawEllipse(static_cast<int>(agent.position.x + 7), static_cast<int>(agent.position.y - 8), 9.0F, 6.0F,
                    Color{178, 232, 241, 255});
    }
    if (index == 0) {
        DrawCircleLines(static_cast<int>(agent.position.x), static_cast<int>(agent.position.y), 31.0F, CYAN);
        DrawText("YOU", static_cast<int>(agent.position.x - 17), static_cast<int>(agent.position.y - 56), 14, WHITE_TEXT);
    }
}

void Game::draw_meeting() const {
    DrawRectangle(0, 0, 1440, 900, Color{5, 9, 16, 218});
    DrawRectangleRounded({300, 97, 840, 701}, 0.04F, 12, PANEL);
    DrawRectangleRoundedLines({300, 97, 840, 701}, 0.04F, 12, Color{118, 150, 167, 255});
    DrawText(reported_agent_ >= 0 ? "BODY REPORTED" : "EMERGENCY MEETING", 354, 127, 30, WHITE_TEXT);
    DrawText("Review the crew list and cast your vote.", 354, 166, 18, MUTED);
    const int count = static_cast<int>(agents_.size());
    const int rows = std::min(count, 12);
    for (int index = 0; index < rows; ++index) {
        const int column = index / 6;
        const int row = index % 6;
        const float x = 350.0F + static_cast<float>(column) * 398.0F;
        const float y = 219.0F + static_cast<float>(row) * 61.0F;
        const Rectangle bounds{x, y, 370.0F, 52.0F};
        const Color fill = selected_vote_ == index ? Color{51, 84, 99, 255} : Color{30, 42, 56, 255};
        DrawRectangleRounded(bounds, 0.14F, 8, fill);
        DrawRectangleRoundedLines(bounds, 0.14F, 8, Color{74, 96, 112, 255});
        DrawCircle(static_cast<int>(x + 27), static_cast<int>(y + 26), 13, PLAYER_COLORS[index]);
        const char* name = index == 0 ? "You" : TextFormat("Crewmate %d", index + 1);
        DrawText(name, static_cast<int>(x + 53), static_cast<int>(y + 9), 19, WHITE_TEXT);
        if (!agents_[index].alive) {
            DrawText("ELIMINATED", static_cast<int>(x + 236), static_cast<int>(y + 17), 12, UI_RED);
        }
    }
    const Rectangle skip{350, 622, 768, 54};
    DrawRectangleRounded(skip, 0.14F, 8, selected_vote_ == count ? Color{51, 84, 99, 255} : Color{30, 42, 56, 255});
    DrawText("SKIP VOTE", 669, 639, 19, WHITE_TEXT);
    DrawText("Click a name or use arrows. Press Enter to confirm.", 440, 711, 17, MUTED);
}

void Game::draw_results() const {
    DrawRectangle(0, 0, 1440, 900, Color{3, 7, 13, 225});
    DrawRectangleRounded({330, 155, 780, 590}, 0.04F, 12, PANEL);
    const bool crew_won = winner_ == static_cast<int>(Role::crew);
    DrawText(crew_won ? "CREWMATES WIN" : "IMPOSTORS WIN", 505, 199, 38, crew_won ? CYAN : UI_RED);
    DrawText(crew_won ? "The station is safe again." : "The impostors took control of the station.", 460, 255, 20, MUTED);
    DrawText("CREW MANIFEST", 430, 320, 16, CYAN);
    for (int index = 0; index < static_cast<int>(agents_.size()); ++index) {
        const int column = index / 6;
        const int row = index % 6;
        const int x = 430 + column * 310;
        const int y = 359 + row * 42;
        DrawCircle(x + 8, y + 9, 8, PLAYER_COLORS[index]);
        const char* name = index == 0 ? "You" : TextFormat("Crewmate %d", index + 1);
        DrawText(TextFormat("%s  —  %s%s", name, role_name(agents_[index].role == Role::impostor),
                            agents_[index].alive ? "" : "  (out)"), x + 27, y, 17, WHITE_TEXT);
    }
    DrawText(TextFormat("Tasks completed: %d / %d", count_done_tasks(), static_cast<int>(tasks_done_.size())), 430, 650, 17, MUTED);
    DrawText("Enter  Lobby      Esc  Main menu", 528, 696, 18, WHITE_TEXT);
}
