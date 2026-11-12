#pragma once

#include "ConfigManager/ConfigManager.hpp"
#include "Core/ILauncher.hpp"
#include "Core/programm_states.hpp"
#include "Game/GameLauncher.hpp"
#include "Menu/Menu.hpp"

#include <memory>

class ProgrammLauncher final : public ILauncher {
public:
    ~ProgrammLauncher() override;
    bool prepare() override;
    void start() override;

private:
    ConfigManager config_manager_;
    std::unique_ptr<GameLauncher> game_launcher_;
    std::unique_ptr<Menu> menu_;
    ProgramState state_{ProgramState::menu};
};
