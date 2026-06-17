<p align="center">
  <img src="docs/VEngine2D%20logo.png" alt="VEngine2D logo" width="360">
</p>

# VEngine2D

**A minimal C++ 2D game engine for building scriptable, data-driven games across desktop and web.**

VEngine2D is a compact engine architecture project focused on the systems that make 2D games flexible: a Lua scripting layer, JSON-defined scenes, reusable actor templates, SDL-powered rendering/audio/input, Box2D physics, and a custom data-oriented particle system. It is small enough to understand end-to-end, but complete enough to ship playable prototypes.

## Why VEngine2D?

- **Scriptable runtime:** Gameplay is authored in Lua components while core systems remain in C++.
- **Data-driven content:** Scenes, actor templates, configs, and components are loaded from resource files.
- **Native engine components:** Physics and particles are exposed as first-class Lua-usable components.
- **Cross-platform build strategy:** Windows, macOS, Linux, and browser/WebAssembly builds are supported.
- **Small but complete:** Rendering, input, audio, text, scene loading, actors, components, events, physics, and particles live in one readable codebase.
- **Playable examples included:** Demo games show action gameplay, puzzle/stealth design, physics interaction, and narrative systems.

## Feature Overview

### Core Architecture

VEngine2D uses an actor/component model:

- **Scenes** describe collections of actors.
- **Actors** are runtime game objects with stable IDs, names, and component maps.
- **Actor templates** work like prefabs, allowing reusable actor definitions with per-instance overrides.
- **Components** can be Lua-authored gameplay scripts or native C++ systems exposed to Lua.
- **Lifecycle methods** such as `OnStart`, `OnUpdate`, `OnLateUpdate`, and `OnDestroy` are called by the engine.
- **Runtime mutation** supports spawning/destroying actors and adding/removing components during play.

The engine deliberately keeps game logic outside the C++ loop. Designers and programmers can iterate through Lua scripts and JSON data while the C++ layer owns performance-sensitive systems and platform integration.

### Lua Scripting

Lua is embedded through LuaBridge and receives a practical gameplay API:

- `Actor.Find`, `Actor.FindAll`, `Actor.Instantiate`, `Actor.Destroy`
- Actor component lookup, component addition, and component removal
- `Input` keyboard, mouse, and scroll access
- `Image`, `Text`, and `Audio` drawing/playback APIs
- `Camera` position and zoom controls
- `Scene.Load`, `Scene.GetCurrent`, and persistent actors through `Scene.DontDestroy`
- `Event.Publish`, `Event.Subscribe`, and `Event.Unsubscribe`
- `Physics.Raycast` and `Physics.RaycastAll`
- `Application.Quit`, frame access, sleeping, and URL opening

Lua exceptions are isolated so one script failure can be reported without collapsing the whole engine loop.

### Rendering, UI, and Audio

Rendering is powered by SDL2 and SDL_image with queued draw calls for predictable ordering:

- World-space image rendering
- Screen-space UI rendering
- Text rendering through SDL_ttf
- Pixel drawing support
- Tinting, alpha, rotation, scale, pivot, sorting order, and camera zoom
- Configurable resolution, clear color, camera easing, and movement-based actor flipping

Audio playback uses SDL_mixer:

- Channel-based playback
- Looping support
- Halt and volume control
- WAV, OGG, and MP3 support in web builds

### Physics

VEngine2D integrates Box2D through a native `Rigidbody` component:

- Dynamic, kinematic, and static bodies
- Box and circle colliders
- Trigger/sensor fixtures
- Gravity scale, density, friction, bounciness, and angular friction
- Force, velocity, position, rotation, and direction APIs
- Collision and trigger lifecycle callbacks
- Raycast and raycast-all queries exposed to Lua

### Data-Oriented Particle System

The engine includes a custom C++ particle component designed around contiguous per-property storage:

- Burst-based emission
- Start/end scale and color interpolation
- Randomized spawn angle, radius, speed, rotation, and angular velocity
- Gravity, drag, and angular drag controls
- Image-backed particles with sorting order
- Runtime `Play`, `Stop`, and `Burst` controls

This system demonstrates a data-oriented approach inside a compact gameplay engine.

## Technology Stack

| Area | Technology |
| --- | --- |
| Core language | C++17 |
| Scripting | Lua + LuaBridge |
| Rendering/input/windowing | SDL2 |
| Images | SDL_image |
| Audio | SDL_mixer |
| Text | SDL_ttf |
| Physics | Box2D 2.4.1 |
| JSON/data loading | rapidjson |
| Math utilities | glm |
| Web builds | Emscripten / WebAssembly |
| Build systems | CMake, Makefile, Visual Studio solution, Xcode project |

## Repository Layout

```text
.
+-- Actor.*              # Actor identity, components, runtime component mutation
+-- ComponentDB.*        # Lua component loading, inheritance, native component registration
+-- Engine.*             # Main loop, scene loading, lifecycle orchestration
+-- Renderer.*           # SDL window/renderer setup, camera and render config
+-- ScriptBindings.*     # Lua API surface exposed by the C++ engine
+-- Rigidbody.*          # Box2D-backed native physics component
+-- ParticleSystem.*     # Data-oriented native particle component
+-- InputManager.*       # Keyboard, mouse, scroll state tracking
+-- ImageDB.*            # Texture loading and caching
+-- TextDB.*             # Font loading and text drawing
+-- AudioDB.*            # Audio chunk cache and channel playback
+-- resources/           # Active game resources
+-- demo games/          # Example projects built with the engine
+-- third_party/         # SDL2, Box2D, Lua, LuaBridge, rapidjson, glm
```

## Demo Games

The repository includes several sample projects that exercise different engine systems:

- **Donna The Pilot** - arcade-style character movement, projectiles, enemies, HUD, audio, and scrolling backgrounds.
- **The Whispering Crown** - scene transitions, dialogue, stealth/puzzle logic, tile maps, NPC behavior, and narrative flow.
- **Physics Obstacle Course** - rigidbody-driven movement, triggers, collisions, and physics gameplay.
- **Rift Bound Signal** - an additional Lua-driven project demonstrating the engine's resource workflow.

## Building

### Windows

```bat
BUILD_WIN.bat
```

Or directly:

```bat
cmake --preset win-release
cmake --build --preset win-release
```

The CMake Windows build copies SDL runtime DLLs and the `resources` folder next to the executable.

### Web / Emscripten

HTML shell build:

```bat
BUILD_WEB_HTML.bat
```

ES module JavaScript build:

```bat
BUILD_WEB_JS.bat
```

The web build packages `resources/` into Emscripten's virtual filesystem and enables SDL2, SDL_image, SDL_ttf, and SDL_mixer ports.

### Linux

```sh
make release
```

Debug build:

```sh
make debug
```

### macOS

An Xcode project is included:

```text
game_engine.xcodeproj
```

## Resource Model

VEngine2D expects game content under `resources/`:

```text
resources/
+-- game.config
+-- rendering.config
+-- scenes/
+-- actor_templates/
+-- component_types/
+-- images/
+-- fonts/
```

This keeps code, data, and assets cleanly separated. Designers can change scenes, actor composition, rendering settings, and Lua behavior without recompiling the engine.

## Engineering Highlights

- C++ engine core with embedded Lua gameplay scripting
- Runtime-safe actor/component lifecycle management
- Deferred component and actor mutation to avoid invalidating update iteration
- Native C++ components registered into the Lua component model
- Box2D contact listener integration for collision and trigger callbacks
- Queued rendering API for stable draw order across world, UI, text, and pixels
- Resource caches for images, fonts, and audio clips
- WebAssembly build path with resource preloading and case-sensitivity checks
- Compact codebase that demonstrates practical engine tradeoffs without excessive abstraction

## Project Status

VEngine2D is a minimal engine intended for learning, experimentation, and fast 2D game prototyping. The current focus is a clear, scriptable runtime architecture rather than editor tooling.

Engine documentation is available in [docs/VEngine2D Documentation.md](docs/VEngine2D%20Documentation.md).
