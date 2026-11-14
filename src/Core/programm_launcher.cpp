#include "Core/ProgrammLauncher.hpp"
#include "Menu/Menu.hpp"

#include "raylib.h"

ProgrammLauncher::~ProgrammLauncher() {
    menu_.reset();
    game_launcher_.reset();
    if (IsWindowReady()) CloseWindow();
}

bool ProgrammLauncher::prepare() {
    if (!config_manager_.load("Config/config.json") && !config_manager_.load("../Config/config.json")) return false;
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(config_manager_.get_int("window_width", 1440), config_manager_.get_int("window_height", 900),
               config_manager_.get_string("window_title", "Social Deduction Prototype").c_str());
    SetExitKey(KEY_NULL);
    SetTargetFPS(config_manager_.get_int("target_fps", 60));
    game_launcher_ = std::make_unique<GameLauncher>();
    menu_ = std::make_unique<Menu>();
    menu_->prepare();
    return game_launcher_->prepare();
}

void ProgrammLauncher::start() {
    while (!WindowShouldClose() && state_ != ProgramState::exit && game_launcher_->game_should_continue()) {
        game_launcher_->check_input();
        game_launcher_->update(GetFrameTime());
        BeginDrawing();
        ClearBackground(Color{13, 20, 34, 255});
        game_launcher_->draw();
        EndDrawing();
    }
}
