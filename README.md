# Lunar Rescue Mission

A 2D OpenGL / GLUT Computer Graphics game designed for a 5-member group project.

## Game idea

You control a lunar lander. Collect energy crystals, avoid meteors, and safely land on the green landing pad. A successful landing requires:

- landing on the pad,
- low horizontal and vertical speed,
- ship angle close to upright.

## CG concepts used

- Translation: lander, meteors, particles, stars, floating text
- Rotation: lander rotation and rotating crystals/meteors
- Scaling / UI bars: fuel bar and object size differences
- Animation: moving stars, meteors, thrust flame, explosions
- Collision detection: lander with terrain, meteors, crystals
- 2D primitives: polygons, triangles, circles, lines, quads
- Keyboard interaction: movement, thrust, pause, restart
- Game states: menu, playing, pause, game over, victory

## Controls

| Key | Action |
|---|---|
| ENTER | Start / play again |
| A / D | Rotate ship |
| Left / Right arrows | Rotate ship |
| W | Thrust |
| Up arrow | Thrust |
| P | Pause / resume |
| R | Restart |
| ESC | Exit |

## How to run on macOS

Install Xcode command line tools if you have not already:

```bash
xcode-select --install
```

Go to this folder in Terminal and run:

```bash
make
./lunar_rescue
```

Or compile manually:

```bash
clang++ main.cpp -o lunar_rescue -std=c++11 -framework OpenGL -framework GLUT
./lunar_rescue
```

## Suggested 5-member contribution split

1. Member 1: Lander drawing and rotation
2. Member 2: Physics, gravity, thrust, velocity, fuel
3. Member 3: Terrain and landing pad + safe landing logic
4. Member 4: Meteors, crystals, particles, collisions
5. Member 5: UI, game states, score, README/presentation explanation
