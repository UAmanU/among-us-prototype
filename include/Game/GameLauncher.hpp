#pragma once

#include "Core/ILauncher.hpp"
#include "Game/game_results.hpp"
#include "Game/game.hpp"

#include <memory>

class GameLauncher final : public ILauncher {
public:
    bool prepare() override;
    void start() override;
    void update(float delta_time);
    void check_input();
    void draw() const;
    [[nodiscard]] bool game_should_continue() const;
    [[nodiscard]] GameResult get_game_result() const;

private:
    std::unique_ptr<Game> game_;
};
