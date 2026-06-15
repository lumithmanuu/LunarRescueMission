# Lunar Rescue Mission

A 2D OpenGL / GLUT Computer Graphics game designed for a 5-member group project.

## Game idea

You control a lunar lander. Collect every energy crystal, avoid meteors, and then return to the green landing pad to complete the mission. The landing pad repairs hull damage and refuels the ship. A successful final landing requires:

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

