#include "Core/ProgrammLauncher.hpp"

#include <memory>

int main() {
    auto launcher = std::make_unique<ProgrammLauncher>();
    if (!launcher->prepare()) return 1;
    launcher->start();
    return 0;
}
