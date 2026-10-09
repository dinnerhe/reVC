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
resolutions. Select display mode and MSAA through the normal graphics menu.
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
MTL_DEBUG_LAYER=1 ../reVC/build-metal/src/reVC
```

The overlay supplies reVC's fonts, frontend assets, text, and other gamefiles.
Settings and any generated caches are written in the test directory. On a
filesystem without cloning support, use `cp -R` for the initial copy.

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
The sandboxed desktop launch and both Metal GPU suites are blocked by macOS
desktop-service access: `com.apple.hiservices-xpcservice` reports a connection
error before renderer initialization. The GPU checks and menu/gameplay visual
checks still need to run outside that sandbox.
