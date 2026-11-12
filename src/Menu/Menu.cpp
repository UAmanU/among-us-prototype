#include "Menu/Menu.hpp"
#include "GUI/MenuRenderer.hpp"
#include "raylib.h"

Menu::Menu() : renderer_(std::make_unique<MenuRenderer>()) {}
Menu::~Menu() = default;
void Menu::prepare() {}
void Menu::update() {
    if (IsKeyPressed(KEY_ESCAPE)) state_ = MenuState::main_menu;
    if (IsKeyPressed(KEY_ENTER) && state_ == MenuState::main_menu) state_ = MenuState::lobby;
}
void Menu::draw() const { renderer_->draw(state_); }
