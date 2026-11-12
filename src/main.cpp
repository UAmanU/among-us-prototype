#include "game.hpp"

int main() {
    InitWindow(1280, 720, "Social Deduction Prototype");
    SetTargetFPS(60);

    Game game;
    while (!WindowShouldClose()) {
        game.update(GetFrameTime());

        BeginDrawing();
        ClearBackground(Color{13, 20, 34, 255});
        game.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
