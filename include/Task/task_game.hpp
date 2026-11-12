#pragma once

#include "Map/map.hpp"

#include <array>

class TaskGame {
public:
    void begin(const TaskStation& task);
    void update(float delta_time);
    void draw() const;
    [[nodiscard]] bool is_finished() const;
    [[nodiscard]] bool succeeded() const;

private:
    void update_wires();
    void update_card_swipe(float delta_time);
    void update_download(float delta_time);
    void update_calibration(float delta_time);
    void draw_wires() const;
    void draw_card_swipe() const;
    void draw_download() const;
    void draw_calibration() const;

    TaskStation task_{};
    std::array<bool, 4> connected_{};
    int selected_wire_{-1};
    int calibration_step_{};
    float progress_{};
    float elapsed_{};
    float calibration_phase_{};
    float card_x_{};
    float drag_duration_{};
    bool dragging_card_{};
    bool finished_{};
    bool succeeded_{};
    bool failed_attempt_{};
};
