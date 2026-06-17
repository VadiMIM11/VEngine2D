# VEngine2D Documentation

This document explains how VEngine2D projects are structured, how game content is loaded, and how Lua scripts interact with the C++ engine runtime.

VEngine2D is a minimal data-driven 2D game engine written in C++17. It uses SDL2 for platform, rendering, input, text, and audio support; Lua for gameplay scripting; rapidjson for configuration and scene data; Box2D for physics; and a custom C++ particle component for efficient visual effects.

## Table of Contents

- [Engine Model](#engine-model)
- [Project Structure](#project-structure)
- [Resource Files](#resource-files)
- [Scenes](#scenes)
- [Actors](#actors)
- [Actor Templates](#actor-templates)
- [Components](#components)
- [Lua Scripting](#lua-scripting)
- [Lua API Reference](#lua-api-reference)
- [Native Components](#native-components)
- [Rendering](#rendering)
- [Physics](#physics)
- [Particle System](#particle-system)
- [Audio](#audio)
- [Scene Management](#scene-management)
- [Events](#events)
- [Build Targets](#build-targets)
- [Demo Projects](#demo-projects)

## Engine Model

VEngine2D uses an actor/component runtime.

- A **scene** is a JSON file that lists actors.
- An **actor** is a named runtime object with a generated engine ID.
- A **component** is a reusable behavior attached to an actor.
- A **Lua component** is written as a Lua table in `resources/component_types/`.
- A **native component** is implemented in C++ and exposed to Lua through LuaBridge.
- An **actor template** is a reusable JSON actor definition, similar to a prefab.

The C++ engine owns the main loop, platform integration, resource loading, native systems, and lifecycle dispatch. Lua scripts own most game-specific behavior.

## Project Structure

Typical project resources live under `resources/`:

```text
resources/
+-- game.config
+-- rendering.config
+-- scenes/
+-- actor_templates/
+-- component_types/
+-- images/
+-- audio/
+-- fonts/
```

Engine and support files live at the repository root:

```text
Actor.*              Actor identity, component ownership, runtime mutation
ActorDB.*            Actor storage and name/ID lookup
AudioDB.*            SDL_mixer chunk cache and playback wrapper
ComponentDB.*        Lua/native component registration and instancing
Engine.*             Main loop, scene loading, lifecycle orchestration
ImageDB.*            SDL texture loading and caching
InputManager.*       Keyboard, mouse, and scroll state
ParticleSystem.*     Native data-oriented particle component
Renderer.*           SDL window, renderer, camera zoom, render config
Rigidbody.*          Native Box2D-backed physics component
ScriptBindings.*     Lua API bindings
TextDB.*             SDL_ttf font cache and text drawing
```

## Resource Files

### `resources/game.config`

`game.config` defines application-level settings.

```json
{
  "game_title": "My VEngine2D Game",
  "initial_scene": "basic"
}
```

Fields:

| Field | Type | Description |
| --- | --- | --- |
| `game_title` | string | Window title used by the SDL window. |
| `initial_scene` | string | Name of the first scene file, without the `.scene` extension. |

### `resources/rendering.config`

`rendering.config` controls renderer defaults.

```json
{
  "x_resolution": 960,
  "y_resolution": 540,
  "clear_color_r": 255,
  "clear_color_g": 255,
  "clear_color_b": 255,
  "zoom_factor": 1.0,
  "cam_ease_factor": 1.0,
  "x_scale_actor_flipping_on_movement": false
}
```

Fields:

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `x_resolution` | int | `640` | Window width. |
| `y_resolution` | int | `360` | Window height. |
| `clear_color_r` | int | `255` | Background red channel. |
| `clear_color_g` | int | `255` | Background green channel. |
| `clear_color_b` | int | `255` | Background blue channel. |
| `zoom_factor` | number | `1.0` | Initial world camera zoom. |
| `cam_ease_factor` | number | `1.0` | Renderer-side camera easing value exposed for engine use. |
| `x_scale_actor_flipping_on_movement` | bool | `false` | Config flag for movement-based x-scale flipping. |

## Scenes

Scene files live in `resources/scenes/` and use the `.scene` extension. The file name is the scene name used by `Scene.Load`.

Example: `resources/scenes/basic.scene`

```json
{
  "actors": [
    {
      "name": "camera",
      "components": {
        "1": {
          "type": "CameraManager"
        }
      }
    },
    {
      "template": "Player",
      "components": {
        "2": {
          "speed": 4.0
        }
      }
    }
  ]
}
```

Rules:

- The top-level object must contain an `actors` array.
- Actors are loaded in the order they appear in the file.
- Actors receive engine IDs when inserted into the scene.
- Components execute in actor ID order, then component key order.
- Scene files may instantiate actor templates and override component properties.

## Actors

Actors are runtime objects with:

- `name`
- generated numeric `id`
- component map
- component property overrides
- Lua-accessible methods

Lua components automatically receive:

```lua
self.actor       -- Actor reference
self.actor_name  -- Actor name, unless already provided
self.id          -- Actor ID, unless already provided
self.key         -- Component key
self.type        -- Component type name
self.enabled     -- Component enabled flag
```

If `self.enabled` is `false`, lifecycle callbacks such as `OnStart`, `OnUpdate`, and `OnLateUpdate` are skipped.

## Actor Templates

Actor templates live in `resources/actor_templates/` and use the `.template` extension.

Example: `resources/actor_templates/Player.template`

```json
{
  "name": "player",
  "components": {
    "1": {
      "type": "Rigidbody",
      "collider_type": "circle",
      "radius": 0.45,
      "density": 0.5,
      "trigger_radius": 0.55
    },
    "2": {
      "type": "KeyboardControls"
    },
    "3": {
      "type": "SpriteRenderer",
      "sprite": "circle"
    }
  }
}
```

Template use in a scene:

```json
{
  "template": "Player",
  "components": {
    "2": {
      "speed": 6.0
    }
  }
}
```

Rules:

- The template name maps to `resources/actor_templates/<name>.template`.
- Scene actors can override template component properties by reusing the same component key.
- A component inherited from a template can be overridden without repeating its `type`.
- New components must provide a `type`.

## Components

Components can be Lua-backed or native C++ backed.

### Lua Components

Lua component files live in `resources/component_types/`.

The file name and global table name must match.

Example: `resources/component_types/Spinner.lua`

```lua
Spinner = {
    speed = 90
}

function Spinner:OnStart()
    self.angle = 0
end

function Spinner:OnUpdate()
    self.angle = self.angle + self.speed
end
```

If the file is named `Spinner.lua`, it must define a global table named `Spinner`.

### Native Components

The engine currently registers these native components:

- `Rigidbody`
- `ParticleSystem`

They are used from JSON like Lua components:

```json
{
  "components": {
    "1": {
      "type": "Rigidbody",
      "body_type": "dynamic"
    },
    "2": {
      "type": "ParticleSystem",
      "burst_quantity": 8
    }
  }
}
```

## Lua Scripting

### Lifecycle Callbacks

Components may define any of the following callbacks:

```lua
function MyComponent:OnStart()
end

function MyComponent:OnUpdate()
end

function MyComponent:OnLateUpdate()
end

function MyComponent:OnDestroy()
end
```

Physics callbacks may also be defined:

```lua
function MyComponent:OnCollisionEnter(collision)
end

function MyComponent:OnCollisionExit(collision)
end

function MyComponent:OnTriggerEnter(collision)
end

function MyComponent:OnTriggerExit(collision)
end
```

Lifecycle order:

- `OnStart` callbacks are drained before normal updates.
- `OnUpdate` runs for each actor by ID, then each component by key.
- `OnLateUpdate` runs after all `OnUpdate` calls.
- Components added at runtime begin lifecycle execution on a later frame.
- Actors spawned during a frame are skipped by update iteration until the next frame.
- Removed components are disabled immediately and erased after deferred processing.

Lua errors are caught and reported with the actor name so other components can continue running.

### Component Properties

JSON fields other than `type` become variables on the component instance.

```json
{
  "type": "Mover",
  "speed": 5.0,
  "axis": "x",
  "enabled": true
}
```

Lua:

```lua
function Mover:OnUpdate()
    Debug.Log("speed = " .. self.speed)
end
```

Supported JSON value types for component properties:

- string
- bool
- int
- number/float

## Lua API Reference

### Debug

| Function | Description |
| --- | --- |
| `Debug.Log(message)` | Prints a message to stdout. |

### Application

| Function | Description |
| --- | --- |
| `Application.Quit()` | Ends the process. |
| `Application.Sleep(milliseconds)` | Sleeps the current thread. |
| `Application.GetFrame()` | Returns the engine frame number. |
| `Application.OpenURL(url)` | Opens a URL through the host operating system. |

### Input

| Function | Returns | Description |
| --- | --- | --- |
| `Input.GetKey(keycode)` | bool | True while the key is held. |
| `Input.GetKeyDown(keycode)` | bool | True on the frame the key is pressed. |
| `Input.GetKeyUp(keycode)` | bool | True on the frame the key is released. |
| `Input.GetMousePosition()` | `vec2` | Mouse position in screen coordinates. |
| `Input.GetMouseButton(button)` | bool | True while mouse button is held. |
| `Input.GetMouseButtonDown(button)` | bool | True on button press frame. |
| `Input.GetMouseButtonUp(button)` | bool | True on button release frame. |
| `Input.GetMouseScrollDelta()` | number | Mouse wheel delta for the frame. |
| `Input.HideCursor()` | nil | Hides the OS cursor. |
| `Input.ShowCursor()` | nil | Shows the OS cursor. |

Mouse buttons:

| Number | Button |
| --- | --- |
| `1` | Left |
| `2` | Middle |
| `3` | Right |

Keycodes are resolved case-insensitively through the engine's SDL scancode map.

### Actor Instance Methods

These methods are called on an actor reference.

| Method | Returns | Description |
| --- | --- | --- |
| `actor:GetName()` | string | Actor name. |
| `actor:GetID()` | int | Actor ID. |
| `actor:GetComponentByKey(key)` | component or nil | Component matching the exact key. |
| `actor:GetComponent(type_name)` | component or nil | First component of the requested type, ordered by key. |
| `actor:GetComponents(type_name)` | table | All components of the requested type, ordered by key. |
| `actor:AddComponent(type_name)` | component or nil | Queues a runtime component addition. |
| `actor:RemoveComponent(component_ref)` | nil | Disables and queues a runtime component removal. |

Example:

```lua
function Powerup:OnStart()
    local body = self.actor:GetComponent("Rigidbody")
    if body ~= nil then
        body:SetGravityScale(0)
    end
end
```

### Actor Namespace

| Function | Returns | Description |
| --- | --- | --- |
| `Actor.Find(name)` | actor or nil | First actor with the matching name. |
| `Actor.FindAll(name)` | table | All actors with the matching name. |
| `Actor.Instantiate(template_name)` | actor or nil | Spawns an actor from a template. |
| `Actor.Destroy(actor)` | nil | Destroys an actor and unregisters it from lookup. |

Example:

```lua
function Spawner:OnUpdate()
    if Input.GetKeyDown("space") then
        Actor.Instantiate("Projectile")
    end
end
```

### Scene

| Function | Returns | Description |
| --- | --- | --- |
| `Scene.Load(scene_name)` | nil | Queues a scene load by name. |
| `Scene.GetCurrent()` | string | Current scene name. |
| `Scene.DontDestroy(actor)` | nil | Marks an actor to transfer into the next scene. |

`Scene.Load("level2")` loads `resources/scenes/level2.scene`.

### Camera

| Function | Returns | Description |
| --- | --- | --- |
| `Camera.SetPosition(x, y)` | nil | Sets world camera position. |
| `Camera.GetPositionX()` | number | Camera x position. |
| `Camera.GetPositionY()` | number | Camera y position. |
| `Camera.SetZoom(zoom)` | nil | Sets camera zoom if `zoom > 0`. |
| `Camera.GetZoom()` | number | Current camera zoom. |

World image coordinates are transformed by camera position and zoom. UI, text, and pixels are not camera-transformed.

### Image

| Function | Description |
| --- | --- |
| `Image.Draw(image_name, x, y)` | Draws an image in world space. |
| `Image.DrawEx(image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order)` | Draws a transformed/tinted world-space image. |
| `Image.DrawUI(image_name, x, y)` | Draws an image in screen/UI space. |
| `Image.DrawUIEx(image_name, x, y, r, g, b, a, sorting_order)` | Draws a tinted UI image. |
| `Image.DrawPixel(x, y, r, g, b, a)` | Draws one screen-space pixel. |

Image names are loaded from `resources/images/`. Draw calls are frame-based: call them every frame that the image should appear.

Render order:

1. World images, sorted by `sorting_order`
2. UI images, sorted by `sorting_order`
3. Text
4. Pixels

Sorting ties preserve call order.

### Text

| Function | Description |
| --- | --- |
| `Text.Draw(content, x, y, font_name, font_size, r, g, b, a)` | Draws screen-space text. |

Fonts are loaded from `resources/fonts/<font_name>.ttf`.

Example:

```lua
function Hud:OnUpdate()
    Text.Draw("Score: " .. self.score, 16, 16, "NotoSans-Regular", 24, 255, 255, 255, 255)
end
```

### Audio

| Function | Description |
| --- | --- |
| `Audio.Play(channel, clip_name, does_loop)` | Plays a clip on a channel. |
| `Audio.Halt(channel)` | Stops playback on a channel. |
| `Audio.SetVolume(channel, volume)` | Sets channel volume, clamped to `0..128`. |

Clip lookup:

- If `clip_name` includes an extension, the engine loads `resources/audio/<clip_name>`.
- If no extension is provided, it checks for `.wav`, then `.ogg`.

Example:

```lua
Audio.Play(0, "jump", false)
Audio.SetVolume(0, 96)
```

### Event

| Function | Description |
| --- | --- |
| `Event.Publish(event_type, event_object)` | Publishes an event object to subscribers. |
| `Event.Subscribe(event_type, component, fn)` | Queues a subscription. |
| `Event.Unsubscribe(event_type, component, fn)` | Queues an unsubscribe. |

Subscriptions are applied after update processing, so event subscription changes are deferred.

Example:

```lua
Door = {}

function Door:OnStart()
    Event.Subscribe("key_collected", self, self.OnKeyCollected)
end

function Door:OnKeyCollected(event)
    self.open = true
end

function Door:OnDestroy()
    Event.Unsubscribe("key_collected", self, self.OnKeyCollected)
end
```

### Vector2

`Vector2` wraps Box2D's `b2Vec2`.

| API | Description |
| --- | --- |
| `Vector2(x, y)` | Constructs a vector. |
| `v.x`, `v.y` | Public coordinates. |
| `v:Normalize()` | Normalizes in place. |
| `v:Length()` | Returns vector length. |
| `a + b`, `a - b`, `a * scalar` | Basic vector operators. |
| `Vector2.Distance(a, b)` | Distance between vectors. |
| `Vector2.Dot(a, b)` | Dot product. |

### Physics

| Function | Returns | Description |
| --- | --- | --- |
| `Physics.Raycast(pos, dir, dist)` | hit or nil | Closest raycast hit. |
| `Physics.RaycastAll(pos, dir, dist)` | table | All raycast hits ordered by distance. |

Hit result fields:

| Field | Type | Description |
| --- | --- | --- |
| `actor` | actor | Actor hit by the ray. |
| `point` | `Vector2` | World-space hit point. |
| `normal` | `Vector2` | Hit normal. |
| `is_trigger` | bool | True if the fixture was a trigger/sensor. |

Example:

```lua
local hit = Physics.Raycast(Vector2(0, 0), Vector2(1, 0), 10)
if hit ~= nil then
    Debug.Log("Hit " .. hit.actor:GetName())
end
```

## Native Components

Native components are implemented in C++ but participate in the same actor/component model as Lua components.

### Rigidbody

`Rigidbody` is a native C++ component backed by Box2D.

### Properties

| Property | Type | Default | Description |
| --- | --- | --- | --- |
| `x` | number | `0.0` | Initial body x position. |
| `y` | number | `0.0` | Initial body y position. |
| `width` | number | `1.0` | Box collider width. |
| `height` | number | `1.0` | Box collider height. |
| `radius` | number | `0.5` | Circle collider radius. |
| `body_type` | string | `"dynamic"` | `"dynamic"`, `"kinematic"`, or `"static"`. |
| `precise` | bool | `true` | Enables Box2D bullet behavior. |
| `gravity_scale` | number | `1.0` | Per-body gravity scale. |
| `density` | number | `1.0` | Fixture density. |
| `angular_friction` | number | `0.3` | Angular damping. |
| `rotation` | number | `0.0` | Initial rotation in degrees. |
| `has_collider` | bool | `true` | Creates a solid collider fixture. |
| `collider_type` | string | `"box"` | `"box"` or `"circle"`. |
| `friction` | number | `0.3` | Solid fixture friction. |
| `bounciness` | number | `0.3` | Solid fixture restitution. |
| `has_trigger` | bool | `true` | Creates a sensor trigger fixture. |
| `trigger_type` | string | `"box"` | `"box"` or `"circle"`. |
| `trigger_width` | number | `1.0` | Trigger box width. |
| `trigger_height` | number | `1.0` | Trigger box height. |
| `trigger_radius` | number | `0.5` | Trigger circle radius. |

### Methods

| Method | Description |
| --- | --- |
| `AddForce(force)` | Applies force to the body center. |
| `SetVelocity(velocity)` | Sets linear velocity. |
| `SetPosition(position)` | Sets body position. |
| `SetRotation(degrees)` | Sets rotation. |
| `SetAngularVelocity(degrees)` | Sets angular velocity. |
| `SetGravityScale(value)` | Sets gravity scale. |
| `SetUpDirection(direction)` | Rotates the body to face up direction. |
| `SetRightDirection(direction)` | Rotates the body to face right direction. |
| `GetPosition()` | Returns `Vector2`. |
| `GetRotation()` | Returns rotation in degrees. |
| `GetVelocity()` | Returns `Vector2`. |
| `GetAngularVelocity()` | Returns angular velocity. |
| `GetGravityScale()` | Returns gravity scale. |
| `GetUpDirection()` | Returns body up direction. |
| `GetRightDirection()` | Returns body right direction. |

### Collision Callbacks

Any component on an actor can respond to physics callbacks if the actor has a body involved in the contact.

Collision callbacks:

```lua
function MyComponent:OnCollisionEnter(collision)
end

function MyComponent:OnCollisionExit(collision)
end
```

Trigger callbacks:

```lua
function MyComponent:OnTriggerEnter(collision)
end

function MyComponent:OnTriggerExit(collision)
end
```

Collision object fields:

| Field | Description |
| --- | --- |
| `other` | Other actor in the contact. |
| `point` | Contact point as `Vector2`. |
| `relative_velocity` | Relative velocity as `Vector2`. |
| `normal` | Contact normal as `Vector2`. |

Trigger callbacks use sentinel values for `point` and `normal`.

### Particle System

`ParticleSystem` is a native C++ component using separate contiguous arrays for particle properties.

### Properties

| Property | Type | Default | Description |
| --- | --- | --- | --- |
| `x`, `y` | number | `0.0` | Emitter position. |
| `frames_between_bursts` | int | `1` | Frames between automatic bursts. |
| `burst_quantity` | int | `1` | Particles emitted per burst. |
| `duration_frames` | int | `300` | Particle lifetime in frames. |
| `start_scale_min` | number | `1.0` | Minimum start scale. |
| `start_scale_max` | number | `1.0` | Maximum start scale. |
| `rotation_min` | number | `0.0` | Minimum start rotation. |
| `rotation_max` | number | `0.0` | Maximum start rotation. |
| `end_scale` | number | NaN | Optional final scale. |
| `start_color_r/g/b/a` | int | `255` | Starting color channels. |
| `end_color_r/g/b/a` | int | `-1` | Optional final color channels. |
| `emit_angle_min` | number | `0.0` | Minimum emission angle. |
| `emit_angle_max` | number | `360.0` | Maximum emission angle. |
| `emit_radius_min` | number | `0.0` | Minimum spawn radius. |
| `emit_radius_max` | number | `0.5` | Maximum spawn radius. |
| `start_speed_min` | number | `0.0` | Minimum initial speed. |
| `start_speed_max` | number | `0.0` | Maximum initial speed. |
| `rotation_speed_min` | number | `0.0` | Minimum angular velocity. |
| `rotation_speed_max` | number | `0.0` | Maximum angular velocity. |
| `gravity_scale_x` | number | `0.0` | Per-frame x acceleration. |
| `gravity_scale_y` | number | `0.0` | Per-frame y acceleration. |
| `drag_factor` | number | `1.0` | Velocity multiplier each frame. |
| `angular_drag_factor` | number | `1.0` | Angular velocity multiplier each frame. |
| `image` | string | `""` | Particle image name, or default generated texture if empty. |
| `sorting_order` | int | `9999` | Render sorting order. |

### Methods

| Method | Description |
| --- | --- |
| `Stop()` | Stops automatic emission. |
| `Play()` | Resumes automatic emission. |
| `Burst()` | Emits `burst_quantity` particles immediately. |

Example:

```json
{
  "type": "ParticleSystem",
  "x": 0,
  "y": 0,
  "burst_quantity": 20,
  "frames_between_bursts": 6,
  "duration_frames": 45,
  "start_speed_min": 0.04,
  "start_speed_max": 0.12,
  "start_color_r": 255,
  "start_color_g": 180,
  "start_color_b": 64,
  "end_color_a": 0,
  "sorting_order": 100
}
```

## Rendering

VEngine2D separates world-space and screen-space rendering.

World rendering:

- Uses `Image.Draw` and `Image.DrawEx`.
- Coordinates are world units.
- One world unit maps to 100 pixels.
- Camera position and zoom affect placement.

UI rendering:

- Uses `Image.DrawUI`, `Image.DrawUIEx`, `Text.Draw`, and `Image.DrawPixel`.
- Coordinates are screen pixels.
- Camera position and zoom do not affect UI.

Rendering requests are queued during update and flushed during render. This makes draw order deterministic even when different components issue render calls at different times.

## Physics

The engine steps the Box2D world once per engine tick after Lua updates and before rendering.

Important behavior:

- Physics bodies are created during the `Rigidbody` `OnStart`.
- Solid colliders only collide with solid colliders.
- Trigger fixtures only report trigger overlap with other triggers.
- Bodies with no collider and no trigger create a phantom fixture so the body can still exist.
- Raycasts ignore phantom fixtures.

Use `Rigidbody` for dynamic simulation and `Physics.Raycast` for physics queries.

## Audio

Audio uses SDL_mixer and a cache of loaded chunks.

Common workflow:

```lua
function PlayAudio:OnStart()
    Audio.Play(self.channel, self.clip, self.loop)
end

function PlayAudio:OnDestroy()
    Audio.Halt(self.channel)
end
```

JSON:

```json
{
  "type": "PlayAudio",
  "channel": 0,
  "clip": "theme",
  "loop": true
}
```

## Scene Management

`Scene.Load(scene_name)` queues a scene change. At the beginning of the next update, the engine loads:

```text
resources/scenes/<scene_name>.scene
```

Actors marked with `Scene.DontDestroy(actor)` are transferred to the next scene. Other actors receive `OnDestroy` callbacks on their components during scene transition.

Example:

```lua
function GameManager:OnStart()
    Scene.DontDestroy(self.actor)
end

function GameManager:LoadNextLevel()
    Scene.Load("level2")
end
```

## Events

The event system provides simple decoupled messaging between Lua components.

Publisher:

```lua
Event.Publish("damage", {
    amount = 10,
    source = self.actor
})
```

Subscriber:

```lua
Health = {
    value = 100
}

function Health:OnStart()
    Event.Subscribe("damage", self, self.OnDamage)
end

function Health:OnDamage(event)
    self.value = self.value - event.amount
end

function Health:OnDestroy()
    Event.Unsubscribe("damage", self, self.OnDamage)
end
```

Subscription and unsubscription are deferred and applied after update processing.

## Build Targets

### Windows

```bat
BUILD_WIN.bat
```

Equivalent CMake commands:

```bat
cmake --preset win-release
cmake --build --preset win-release
```

### Web / Emscripten

HTML shell:

```bat
BUILD_WEB_HTML.bat
```

JavaScript module:

```bat
BUILD_WEB_JS.bat
```

The web build:

- Uses Emscripten SDL ports.
- Preloads `resources/` into the virtual filesystem.
- Enables memory growth.
- Supports HTML shell and ES module output modes.
- Treats uppercase resource file extensions as errors by default in web presets.

### Linux

```sh
make release
```

Debug:

```sh
make debug
```

### macOS

Open the included Xcode project:

```text
game_engine.xcodeproj
```

## Demo Projects

Demo projects are stored in `demo games/`:

| Demo | Focus |
| --- | --- |
| `Donna The Pilot` | Scrolling action gameplay, projectiles, enemies, HUD, audio. |
| `Physics Obstacle Course` | Rigidbody motion, collision, triggers, physics gameplay. |
| `Rift Bound Signal` | Lua-driven project structure and resource workflow. |
| `The Whispering Crown` | Multi-scene flow, dialogue, tile data, stealth/puzzle logic. |

To test a demo, copy or adapt its `resources/` folder into the active root `resources/` folder before building/running the engine.

## Practical Workflow

1. Create or edit Lua components in `resources/component_types/`.
2. Define reusable actors in `resources/actor_templates/`.
3. Place actors in a `.scene` file under `resources/scenes/`.
4. Set `initial_scene` in `resources/game.config`.
5. Run a native or web build.
6. Iterate on JSON/Lua content without changing the C++ engine unless adding new native systems.

## Notes and Limitations

- The engine currently focuses on runtime systems, not editor tooling.
- Resource paths are expected under `resources/`.
- Draw calls are immediate-per-frame requests; persistent visuals should be requested each frame.
- Lua component scripts must define a global table matching the file name.
- Web builds are case-sensitive, so lowercase resource extensions are recommended.
