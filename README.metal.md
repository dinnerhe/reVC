# Native Metal on macOS

This build uses the native Metal backend in the sibling librw checkout. It keeps
GLFW for the window, keyboard, mouse, and controllers. The standard RenderWare
geometry, skin, and MatFX pipelines are used; Neo pipelines, the extended color
filter, screen droplets, and the optional Leeds-style renderer are disabled for
Metal until their backend-specific implementations are ported. The original
motion-blur renderer remains available.

## Build

Run these commands from the directory containing both `reVC/` and `librw/`:

```sh
cmake -S reVC -B reVC/build-metal \
  -DREVC_LIBRW_SOURCE_DIR="$PWD/librw" \
  -DLIBRW_PLATFORM=METAL \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOPENAL_INCLUDE_DIR="$(brew --prefix openal-soft)/include" \
  -DOPENAL_LIBRARY="$(brew --prefix openal-soft)/lib/libopenal.dylib"
cmake --build reVC/build-metal --parallel
```

Dependencies are CMake, GLFW, OpenAL Soft, and mpg123. The OpenAL arguments select
OpenAL Soft instead of Apple's OpenAL framework, which does not supply the EFX
headers this audio backend uses. `REVC_LIBRW_SOURCE_DIR` takes precedence over
the vendored-library option. Leave it empty to use the existing vendored or
installed-library paths.

Metal exposes a windowed mode followed by the primary monitor's fullscreen
resolutions, including the current scaled desktop mode even when GLFW omits it
from its fullscreen enumeration. Select display mode and MSAA through the normal graphics menu.
Changes restart the RenderWare engine. Camera dimensions and mouse coordinates
use logical window pixels; Metal scales rendering to the Retina framebuffer.

## Prepare and launch

Use a separate game-data directory for testing. On an APFS volume, `cp -cR`
creates a copy using filesystem clones. From the same parent directory, with
`runtime-metal` not already present:

```sh
cp -cR "Grand Theft Auto Vice City" runtime-metal
cp -R reVC/gamefiles/. runtime-metal/
cd runtime-metal
env -u MTL_DEBUG_LAYER MTL_HUD_ENABLED=1 ../reVC/build-metal/src/reVC
```

The overlay supplies reVC's fonts, frontend assets, text, and other gamefiles.
Settings and any generated caches are written in the test directory. On a
filesystem without cloning support, use `cp -R` for the initial copy.

`MTL_HUD_ENABLED=1` enables [Apple's Metal performance HUD](https://developer.apple.com/documentation/xcode/monitoring-your-metal-apps-graphics-performance).
API validation is separate: use `MTL_DEBUG_LAYER=1` while checking rendering
correctness, and unset it for gameplay performance measurements. It has a large
CPU cost in this renderer; the diagnostic launch command is unsuitable for FPS
comparisons with an OpenGL release build.

First confirm that menu text and textures render, mouse selection matches the
cursor, and keyboard/controller navigation works. Start a new game, then check
the HUD, pedestrian animation, vehicle paint and MatFX reflections, glass,
particles, water, and shadows. Also check window resizing, fullscreen/windowed
switching, and MSAA with Metal API validation enabled.

## Backend regression tests

From the parent directory:

```sh
cmake -S librw -B librw/build-metal-integration \
  -DLIBRW_PLATFORM=METAL -DLIBRW_METAL_TESTS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build librw/build-metal-integration --parallel
ctest --test-dir librw/build-metal-integration --output-on-failure
```

Both the one-sample and 4× MSAA suites execute real GPU pixel checks and validate
video-mode enumeration, selection, and invalid indices. Run them from a normal
macOS desktop terminal.

## Verification status

The native Metal and OpenGL GLFW builds both compile and link successfully.
Both Metal GPU suites pass with desktop access and Metal API validation enabled.
A 20-second native game launch reaches `GS_FRONTEND`, loads both frontend texture
dictionaries, and shuts down cleanly on termination. Matched gameplay captures
against OpenGL verify fog colors and the circular radar mask. GPU regressions
cover fog byte order, depth writes independent of testing, queued texture writes,
and transient buffer lifetimes. Full interactive coverage of every game effect
remains future work.

Sandboxed desktop launches can fail with a `com.apple.hiservices-xpcservice`
connection error before renderer initialization. Use a normal macOS desktop
terminal for runtime validation.

## Repeatable gameplay capture

Configure with `-DREVC_RENDER_TESTS=ON` to enable the optional GLFW test harness.
It has no effect unless `REVC_RENDER_TEST` is set. Run from a separate test game
directory with a supported video mode:

```sh
env -u MTL_DEBUG_LAYER REVC_RENDER_TEST=game \
  REVC_RENDER_TEST_FRAMES=360 \
  REVC_RENDER_TEST_CAPTURE=/tmp/revc-metal-game.png \
  /absolute/path/to/reVC/build-metal/src/reVC
```

The harness starts a new game through the frontend, fixes the clock at 22:20 and
weather to sunny, disables optional custom effects for backend comparison, warms
up for 120 gameplay frames, measures the requested number of frames, captures
the framebuffer, and exits. It prints framebuffer resolution, average FPS,
median/p95/max frame intervals. Use the included `freeroam_miami.scm` as
`data/main.scm` in the test copy to avoid opening story cutscenes. Set
`REVC_RENDER_TEST_TRAVEL=600` to traverse 600 game units along Y during the run
and exercise streaming. `REVC_RENDER_TEST=menu` captures the menu after 60 frames.
Framebuffer PNGs include the game HUD but omit Apple's overlay, which is added
to the presented drawable.

Measured smoke runs on Apple M2 (120 warmup frames, 360 measured frames):

| Renderer / options | Framebuffer | Average FPS | p95 frame interval |
| --- | --- | ---: | ---: |
| Previous Metal, no validation | 1920×1200 | 120.0 | 8.77 ms |
| Previous Metal, API validation only | 2560×1600 | 34.5 | 31.92 ms |
| Previous Metal, API validation and HUD | 2560×1600 | 34.3 | 32.49 ms |
| Updated Metal, API validation and HUD | 2560×1600 | 51.8 | 20.92 ms |
| Updated Metal, Apple HUD, no validation | 2560×1600 | 120.0 | 8.78 ms |
| Previous Metal, 600-unit streaming traversal | 1920×1200 | 119.6 | 9.28 ms |
| Updated Metal, 600-unit streaming traversal | 1920×1200 | 119.9 | 9.00 ms |
| OpenGL, no validation | 1920×1200 | 269.8 | 4.70 ms |

These are short runs in one fixed-night gameplay scenario, not guarantees for
every location or workload. These numbers measure application frame intervals;
they do not establish the display refresh rate or GPU execution time.
