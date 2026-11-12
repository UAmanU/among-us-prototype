# Social Deduction Prototype

- **Creator:** AmanU
- **Project started:** November 9, 2026
- **Language:** C++20
- **Graphics library:** Raylib 6.0

## Overview

A small, local, single-player prototype for a social-deduction game. One player and four computer-controlled agents receive hidden roles; exactly one agent is the saboteur. The crew can complete station tasks, while the saboteur can eliminate crew members and trigger a critical system failure. Reports and emergency meetings lead to a vote.

The current build uses original, code-drawn placeholder graphics and does not load files from `images/`. Local files in `images/` and `lib/` are excluded by `.gitignore`; the Raylib header and static library must be supplied locally before building a fresh checkout.

## How to Play

- Move with **WASD** or the **arrow keys**.
- As crew, press **E** beside an unfinished station to complete its task. During a critical failure, press **E** beside the station core to repair it.
- Press **Q** once per round to call an emergency meeting.
- Press **R** beside an eliminated crew member to report the incident.
- During a meeting, use the arrow keys to select a player or skip, then press **Enter** to vote. A tie ejects nobody.
- As the saboteur, press **F** near a crew member to eliminate them and **G** to trigger a critical failure.
- Press **Enter** after a round ends to start another round.

The crew wins by completing every station task or removing the saboteur. The saboteur wins when the remaining crew are outnumbered or the critical failure timer expires.

## Project Structure

```text
among-us-prototype/
├── CMakeLists.txt
├── GAME-LOGIC.md
├── README.md
├── include/
│   └── game.hpp
├── src/
│   ├── game.cpp
│   └── main.cpp
├── images/                 # Local-only assets; ignored by Git
├── lib/                    # Local Raylib files; ignored by Git
└── build/                  # CMake build output; ignored by Git
```

## Ownership and Architecture

`main.cpp` owns application startup, the window loop, and frame timing. The `Game` class in `game.hpp` and `game.cpp` owns round state, agents, tasks, meetings, voting, and drawing. Raylib is used for input, drawing, and the window; gameplay state stays inside `Game`.

The first prototype keeps the simulation in one class to make the round loop easy to change. Separate game systems can be introduced when the rules grow beyond this MVP.

## Dependencies

- C++20 compiler. The current local setup uses MinGW-w64.
- CMake 3.20 or newer.
- Raylib 6.0 static library and `raylib.h` in the ignored `lib/` directory.

The Raylib files are not tracked by Git. A fresh checkout needs compatible local copies at `lib/libraylib.a` and `lib/raylib.h`.

## Build on Windows with MinGW

From the project directory, configure and build with:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=mingw32-make
cmake --build build --config Release
```

Run the executable:

```powershell
./build/among_us_prototype.exe
```

## Design Notes

`GAME-LOGIC.md` records the genre references and the rules selected for this original prototype. The image files currently present in the local, ignored `images/` folder are not loaded or distributed by the program.
