#include "GUI/MenuRenderer.hpp"
#include "raylib.h"

void MenuRenderer::draw(MenuState state) const {
    if (state == MenuState::main_menu) {
        DrawText("SOCIAL DEDUCTION", 100, 100, 46, RAYWHITE);
        DrawText("Press Enter to begin", 104, 170, 22, LIGHTGRAY);
    } else {
        DrawText("LOBBY", 100, 100, 46, RAYWHITE);
        DrawText("Press Escape to return", 104, 170, 22, LIGHTGRAY);
    }
}
