#pragma once

#include "Menu/menu_states.hpp"
#include <memory>

class MenuRenderer;

class Menu {
public:
    Menu();
    ~Menu();
    void prepare();
    void update();
    void draw() const;
    [[nodiscard]] MenuState state() const { return state_; }
private:
    std::unique_ptr<MenuRenderer> renderer_;
    MenuState state_{MenuState::main_menu};
};
