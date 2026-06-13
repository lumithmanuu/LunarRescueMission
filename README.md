# Lunar Rescue Mission

Lunar Rescue Mission is a 2D C++ OpenGL/GLUT game created for a university Computer Graphics group assignment. The player controls a lunar lander, collects energy crystals, avoids moving meteors, manages fuel and health, and attempts a safe landing on the marked landing pad.

The project is intentionally kept beginner-friendly. It uses only basic OpenGL shapes and GLUT, so every group member can understand the code and explain the graphics techniques used during the presentation.

## Game Overview

The mission starts from a main menu with:

- Start Game
- Instructions
- Credits / Team Contributions
- Exit

During gameplay, the player must:

- rotate the lander and use thrust carefully,
- collect all energy crystals,
- avoid meteor collisions,
- monitor fuel, health, score, and lives,
- land slowly and upright on the landing pad to win.

If the player crashes too hard or loses all lives, the game ends.

## Controls

| Key | Action |
|---|---|
| `W` or `Up Arrow` | Thrust |
| `A` or `Left Arrow` | Rotate left |
| `D` or `Right Arrow` | Rotate right |
| `Enter` | Select menu item / play again |
| `P` | Pause / resume |
| `R` | Restart mission |
| `M` | Return to main menu |
| `ESC` | Back / exit |

## How to Compile and Run on macOS

Make sure Xcode Command Line Tools are installed:

```bash
xcode-select --install
```

Build with the existing Makefile:

```bash
make
./lunar_rescue
```

Or compile manually with the required macOS-compatible command:

```bash
g++ main.cpp -o lunar_rescue -framework OpenGL -framework GLUT
./lunar_rescue
```

If `g++` on your Mac maps to Apple Clang, that is fine for this project.

## Gameplay Features

- Main menu, instructions screen, credits screen, pause screen, game over screen, and win screen
- Fuel system for the thruster
- Score system with collection and landing bonuses
- Health and lives system
- Collectible energy crystals
- Moving meteors as obstacles
- Safe landing rules based on:
  - landing pad contact
  - low horizontal speed
  - low vertical speed
  - near-upright lander angle

## Graphics Improvements

- layered moon-space background
- parallax-style animated stars
- glowing moon and crater details
- animated thrust flame
- crash and collection particle effects
- clean 2D OpenGL primitives only, with no external textures or assets

## Computer Graphics Concepts Used

- Translation
  - moving the lander, stars, meteors, particles, and floating text
- Rotation
  - rotating the lander, crystals, and meteors
- Scaling
  - pulsing crystal animation and UI bar sizing
- Animation
  - timer-based updates for stars, meteors, particles, flames, and menu/game transitions
- Collision Detection
  - lander with terrain, landing pad, meteors, and crystals
- 2D Shape Construction
  - triangles, quads, polygons, circles, and lines
- State-Based Rendering
  - different screens for menu, instructions, gameplay, pause, game over, win, and credits
- User Interaction
  - keyboard controls for gameplay and menu navigation

## 5-Member Contribution Breakdown

This is a suggested contribution split for the group:

1. Member 1: Lander model, controls, rotation, and thrust behavior
2. Member 2: Physics system, gravity, fuel usage, health, and lives
3. Member 3: Terrain drawing, landing pad, and safe landing logic
4. Member 4: Meteors, collectibles, collisions, particles, and score events
5. Member 5: Menu system, game states, HUD, README, and presentation preparation

## Presentation Explanation Points

Use these points during the demo or viva:

1. Explain how GLUT uses a timer loop to update movement and animation every frame.
2. Show how translation changes the position of the lander, meteors, stars, and particles.
3. Explain rotation using the lander angle and rotating meteor/crystal objects.
4. Describe the collision checks used for collectibles, meteors, and terrain contact.
5. Show how the safe landing condition is calculated using speed, angle, and landing pad position.
6. Highlight how game states are used to switch between menu, instructions, gameplay, pause, win, and lose screens.
7. Mention that all visuals are built from simple OpenGL primitives without textures or external assets.

## Project Files

- `main.cpp` - full game logic, rendering, physics, UI, states, and effects
- `Makefile` - build instructions for macOS and other GLUT-capable systems
- `CG_Presentation_Notes.md` - extra notes that can support the presentation

## Notes for the Team

- The code includes comments in major sections so each member can understand their area quickly.
- The project is kept simple enough for a group assignment, while still showing important Computer Graphics features.
- After making future changes, rebuild with `make` to check that the project still compiles on macOS.
