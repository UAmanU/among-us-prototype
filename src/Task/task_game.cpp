#include "Task/task_game.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr Color PANEL{19, 29, 42, 250};
constexpr Color TEXT{236, 242, 245, 255};
constexpr Color MUTED{161, 181, 192, 255};
constexpr Color TASK_GREEN{71, 220, 169, 255};
constexpr std::array<Color, 4> WIRE_COLORS{
    Color{237, 83, 84, 255}, Color{63, 194, 230, 255},
    Color{242, 198, 73, 255}, Color{169, 113, 235, 255}
};

bool hit(Vector2 point, Rectangle rectangle) {
    return CheckCollisionPointRec(point, rectangle);
}
}

void TaskGame::begin(const TaskStation& task) {
    task_ = task;
    connected_.fill(false);
    selected_wire_ = -1;
    calibration_step_ = 0;
    progress_ = 0.0F;
    elapsed_ = 0.0F;
    calibration_phase_ = 0.0F;
    card_x_ = 470.0F;
    drag_duration_ = 0.0F;
    dragging_card_ = false;
    finished_ = false;
    succeeded_ = false;
    failed_attempt_ = false;
}

void TaskGame::update(float delta_time) {
    if (finished_) {
        return;
    }
    elapsed_ += delta_time;
    switch (task_.type) {
    case TaskType::wires: update_wires(); break;
    case TaskType::card_swipe: update_card_swipe(delta_time); break;
    case TaskType::download: update_download(delta_time); break;
    case TaskType::calibrate: update_calibration(delta_time); break;
    }
}

void TaskGame::update_wires() {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const Vector2 mouse = GetMousePosition();
    for (int index = 0; index < 4; ++index) {
        const float y = 320.0F + static_cast<float>(index) * 75.0F;
        if (hit(mouse, {414.0F, y - 17.0F, 85.0F, 34.0F})) {
            selected_wire_ = index;
            return;
        }
        if (hit(mouse, {927.0F, y - 17.0F, 100.0F, 34.0F}) && selected_wire_ >= 0) {
            const int target_wire = (index * 3 + 1) % 4;
            if (target_wire == selected_wire_) {
                connected_[target_wire] = true;
                if (std::all_of(connected_.begin(), connected_.end(), [](bool connected) { return connected; })) {
                    finished_ = true;
                    succeeded_ = true;
                }
            } else {
                failed_attempt_ = true;
            }
            selected_wire_ = -1;
            return;
        }
    }
}

void TaskGame::update_card_swipe(float delta_time) {
    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hit(mouse, {card_x_, 411.0F, 110.0F, 70.0F})) {
        dragging_card_ = true;
        drag_duration_ = 0.0F;
        failed_attempt_ = false;
    }
    if (dragging_card_) {
        drag_duration_ += delta_time;
        card_x_ = std::clamp(mouse.x - 55.0F, 430.0F, 1010.0F);
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            const bool correct_speed = drag_duration_ >= 0.45F && drag_duration_ <= 1.55F;
            if (card_x_ > 950.0F && correct_speed) {
                finished_ = true;
                succeeded_ = true;
            } else {
                failed_attempt_ = true;
                card_x_ = 470.0F;
                dragging_card_ = false;
            }
        }
    }
}

void TaskGame::update_download(float delta_time) {
    if (IsKeyDown(KEY_SPACE)) {
        progress_ = std::min(1.0F, progress_ + delta_time / 3.8F);
    }
    if (progress_ >= 1.0F) {
        finished_ = true;
        succeeded_ = true;
    }
}

void TaskGame::update_calibration(float delta_time) {
    calibration_phase_ += delta_time * 1.65F;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    if (!hit(GetMousePosition(), {430.0F, 416.0F, 580.0F, 74.0F})) {
        return;
    }
    const float indicator = std::abs(std::sin(calibration_phase_));
    if (indicator > 0.42F && indicator < 0.59F) {
        ++calibration_step_;
        failed_attempt_ = false;
        if (calibration_step_ >= 3) {
            finished_ = true;
            succeeded_ = true;
        }
    } else {
        failed_attempt_ = true;
    }
}

void TaskGame::draw() const {
    DrawRectangle(0, 0, 1440, 900, Color{3, 7, 13, 205});
    DrawRectangleRounded({310, 180, 820, 550}, 0.045F, 12, PANEL);
    DrawRectangleRoundedLines({310, 180, 820, 550}, 0.045F, 12, Color{106, 130, 145, 255});
    DrawText(task_.name.c_str(), 365, 218, 30, TEXT);
    DrawText(task_.detail.c_str(), 367, 259, 18, MUTED);

    switch (task_.type) {
    case TaskType::wires: draw_wires(); break;
    case TaskType::card_swipe: draw_card_swipe(); break;
    case TaskType::download: draw_download(); break;
    case TaskType::calibrate: draw_calibration(); break;
    }
    if (failed_attempt_) {
        DrawText("Not quite. Try again.", 570, 676, 18, Color{247, 137, 127, 255});
    }
    DrawText("ESC  Close task", 368, 690, 16, MUTED);
}

void TaskGame::draw_wires() const {
    DrawText("Connect each colored wire to the matching socket.", 414, 300, 18, TEXT);
    for (int index = 0; index < 4; ++index) {
        const float y = 337.0F + static_cast<float>(index) * 75.0F;
        const int right_index = (index * 3 + 1) % 4;
        DrawCircle(427, static_cast<int>(y), 12, WIRE_COLORS[index]);
        DrawCircle(1014, static_cast<int>(y), 12, WIRE_COLORS[right_index]);
        DrawRectangleRounded({446, y - 16.0F, 50, 32}, 0.2F, 5, Color{33, 47, 61, 255});
        DrawRectangleRounded({946, y - 16.0F, 50, 32}, 0.2F, 5, Color{33, 47, 61, 255});
        if (connected_[index]) {
            DrawLineEx({497, y}, {945, 337.0F + static_cast<float>(right_index) * 75.0F}, 5.0F, WIRE_COLORS[index]);
            DrawCircle(427, static_cast<int>(y), 16, WIRE_COLORS[index]);
            DrawCircle(1014, static_cast<int>(337.0F + static_cast<float>(right_index) * 75.0F), 16, WIRE_COLORS[index]);
        }
    }
    DrawText("LEFT SIDE: choose a wire, then choose its color match on the right.", 413, 652, 16, MUTED);
}

void TaskGame::draw_card_swipe() const {
    DrawText("Swipe the card through the reader at a steady speed.", 414, 305, 18, TEXT);
    DrawRectangleRounded({420, 390, 600, 112}, 0.1F, 10, Color{31, 47, 61, 255});
    DrawRectangleRoundedLines({420, 390, 600, 112}, 0.1F, 10, Color{104, 132, 147, 255});
    DrawRectangle(902, 398, 14, 96, TASK_GREEN);
    DrawText("CARD", 888, 512, 15, MUTED);
    DrawRectangleRounded({card_x_, 411, 110, 70}, 0.12F, 8, Color{226, 232, 227, 255});
    DrawRectangleRounded({card_x_ + 10, 423, 26, 26}, 0.1F, 5, Color{89, 165, 177, 255});
    DrawText("CREW ID", static_cast<int>(card_x_ + 41), 423, 13, Color{28, 39, 47, 255});
    DrawText("Drag from left to right. Too fast or too slow? Try again.", 414, 570, 17, MUTED);
    if (dragging_card_) {
        DrawText(TextFormat("Swipe time: %.2f s", drag_duration_), 414, 603, 16, TASK_GREEN);
    }
}

void TaskGame::draw_download() const {
    DrawText("Hold SPACE to transmit the data. Releasing pauses the upload.", 414, 316, 18, TEXT);
    DrawRectangleRounded({430, 405, 580, 138}, 0.08F, 10, Color{31, 48, 61, 255});
    DrawRectangle(466, 449, 504, 22, Color{12, 20, 30, 255});
    DrawRectangle(470, 453, static_cast<int>(496.0F * progress_), 14, TASK_GREEN);
    DrawText(TextFormat("%d%%", static_cast<int>(progress_ * 100.0F)), 652, 491, 24, TEXT);
    DrawText("TRANSMITTING DATA", 595, 579, 16, MUTED);
}

void TaskGame::draw_calibration() const {
    DrawText("Click the marker when it crosses the highlighted zone.", 414, 307, 18, TEXT);
    DrawText(TextFormat("Calibration %d / 3", calibration_step_ + 1), 414, 359, 18, TASK_GREEN);
    DrawRectangleRounded({430, 426, 580, 54}, 0.2F, 8, Color{34, 50, 63, 255});
    DrawRectangleRounded({430 + 580.0F * 0.42F, 426, 580.0F * 0.17F, 54}, 0.15F, 8, Color{44, 125, 102, 255});
    const float marker_x = 430.0F + std::abs(std::sin(calibration_phase_)) * 580.0F;
    DrawRectangleRounded({marker_x - 9.0F, 416, 18, 74}, 0.25F, 8, Color{244, 218, 125, 255});
    DrawText("The marker reverses direction at each end.", 430, 520, 17, MUTED);
}

bool TaskGame::is_finished() const {
    return finished_;
}

bool TaskGame::succeeded() const {
    return succeeded_;
}

