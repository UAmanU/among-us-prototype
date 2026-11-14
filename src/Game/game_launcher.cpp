#include "Game/GameLauncher.hpp"

bool GameLauncher::prepare() {
    game_ = std::make_unique<Game>();
    return true;
}

void GameLauncher::start() {
    if (!game_) prepare();
}

void GameLauncher::update(float delta_time) { if (game_) game_->update(delta_time); }
void GameLauncher::check_input() { update(0.0F); }
void GameLauncher::draw() const { if (game_) game_->draw(); }
bool GameLauncher::game_should_continue() const { return game_ && !game_->should_close(); }
GameResult GameLauncher::get_game_result() const {
    if (!game_ || game_->result_code() < 0) return GameResult::none;
    return game_->result_code() == 0 ? GameResult::crewmates_win : GameResult::impostors_win;
}
