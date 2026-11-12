# Social Deduction Prototype

- **Creator:** AmanU
- **Project started:** November 9, 2026
- **Language:** C++20
- **Graphics library:** Raylib 6.0

## Overview

A local single-player social-deduction prototype. Choose one of three maps, set a match for 4–12 participants, and play with computer-controlled crewmates and impostors. The round includes interactive tasks, sabotage, reporting, meetings, and voting.

## Controls and Rules

- **WASD / arrow keys:** move.
- **E:** use a nearby task or repair a critical system.
- **R:** report a nearby body.
- **Q:** call one emergency meeting per round.
- **F / G:** eliminate a nearby crewmate / sabotage as an impostor.
- **Meetings:** select a player and press Enter to vote; ties and skips eject nobody.
- **Escape:** open settings. Enter at the round results returns to the lobby.

Crewmates win by completing all tasks or eliminating every impostor. Impostors win by reaching parity with the crew or allowing critical sabotage to expire.

## Architecture

`main.cpp` creates `ProgrammLauncher` through `std::unique_ptr`. `ILauncher` defines the common prepare/start lifecycle for application and game launchers. `ProgrammLauncher` initializes Raylib from `Config/config.json` and owns the application loop, `Menu`, and `GameLauncher`. `GameLauncher` owns the game instance and delegates update, input, draw, continuation, and result queries.

`ConfigManager` loads configuration once and provides typed getters. `GameMap` contains room connectivity, walkability, and station data. `TaskGame` implements task interactions.

**Current migration limitation:** the existing `Game` still combines game rules, menu flow, and rendering, and `GameMap` / `TaskGame` still issue Raylib drawing calls. The new `Menu` and `MenuRenderer` are a small standalone foundation and are not yet connected to the legacy in-game menu. These are the next extraction steps needed to complete the requested ownership boundaries; the launch/configuration layer and folder layout are in place.

## Structure

```text
Config/config.json
include/
├── ConfigManager/
├── Core/             # ILauncher, ProgrammLauncher, program state
├── GUI/              # MenuRenderer foundation
├── Game/             # GameLauncher, game/result states, map/task enums
├── Menu/             # Menu controller and menu state
├── Object/           # Object enums and future game objects
├── game.hpp          # Legacy game pending extraction
├── map.hpp
└── task_game.hpp
src/
├── ConfigManager/
├── Core/
├── GUI/
├── Game/
├── Menu/
├── main.cpp
├── game.cpp          # Legacy game pending extraction
├── map.cpp
└── task_game.cpp
images/               # Project image assets
lib/raylib/           # Local Raylib files (ignored by Git)
build/                # CMake output (ignored by Git)
```

## Dependencies and Build

- C++20 compiler (current setup: MinGW-w64)
- CMake 3.20 or newer
- Raylib 6.0 files at `lib/raylib/raylib.h` and `lib/raylib/libraylib.a`

Configure and build from the project directory:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=mingw32-make
cmake --build build --config Release
```

Run from the project directory:

```powershell
./build/among_us_prototype.exe
```
