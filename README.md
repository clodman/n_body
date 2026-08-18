# n-body

A real-time 2D gravitational n-body simulation written in C, rendered with
[raylib](https://www.raylib.com/).



![1](https://github.com/user-attachments/assets/1f24b0b6-9a08-45e5-a60f-ead7cdc9e18e)
![2](https://github.com/user-attachments/assets/d366a718-609f-44e5-86a4-c3293e33d5fa)


Particles attract each other under Newtonian gravity, collide elastically, and
leave fading motion trails. The simulation uses leapfrog (velocity Verlet)
integration with a fixed physics timestep and gravitational softening for
numerical stability.

## Features

- Leapfrog integration with a fixed physics timestep and time accumulator
- Newtonian gravity with gravitational softening
- Elastic collisions with per-particle restitution and overlap separation
- Fading trajectory trails via a render texture
- Pan and zoom camera, resizable window, pause

## Controls




| Key           | Action              |
| ------------- | ------------------- |
| `Space`       | Pause / resume      |
| `W` / `S`     | Zoom in / out       |
| Arrow keys    | Pan the camera      |

## Building

Requires a C compiler with C23 support, [CMake](https://cmake.org/) 3.15+, and
raylib installed on the system.

```sh
cmake -B build
cmake --build build
./build/raylib_app
```

On macOS, raylib can be installed with `brew install raylib`.

## Layout

- `src/main.c` — simulation loop, physics, collisions, and rendering
- `src/base.c` — vector math and small utility types
