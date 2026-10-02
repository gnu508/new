# Watermelon Impact Lab

A small interactive OpenGL 3.3 demo: a watermelon assembled from individually
rendered wedges breaks apart under an impact, then tumbles and bounces across a
lit test floor.

## Controls

- Click the scene or press `Space` to trigger an impact.
- Drag with the left mouse button to orbit the camera; use the scroll wheel to zoom.
- Press `R` to reset, `Up` / `Down` to change impact strength, and `Esc` to quit.

## Build and run

Requires CMake 3.20+, a C++17 compiler, OpenGL 3.3 drivers, and the platform
window-system development libraries. GLFW and GLAD are fetched automatically by
CMake. On Ubuntu, install the system prerequisites with:

```sh
sudo apt update
sudo apt install build-essential cmake libgl1-mesa-dev xorg-dev
```

Then build and launch:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/watermelon_impact
```
