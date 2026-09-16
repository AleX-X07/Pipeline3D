# Core API

This page summarizes the public core headers under `c3d/`.

## `c3d::Game`

Header:

```cpp
#include <c3d/game.h>
```

### Methods

| Member | Purpose |
|---|---|
| `Title()` | Application title |
| `Version()` | Application version as `glm::uvec3` |
| `Initialize(platform, gpu)` | Base engine initialization |
| `Deinitialize()` | Base engine shutdown |
| `Update(subsystems, t, dt)` | Per-frame game update |
| `Render(subsystems, frameImage, encoder)` | Render the current frame |
| `RenderUi()` | Optional ImGui UI |
| `Run<T>(args...)` | Static application entry helper |

### `GameSubsystems`

```cpp
struct GameSubsystems
{
    ga::platform::Platform& platform;
    ga::gpu::Device& gpu;
    ga::assets::AssetManager& assets;
    ga::audio::AudioSystem& audio;
};
```

This is the main service bundle passed into update/render code.

---

## `c3d::GameScene`

Header:

```cpp
#include <c3d/gamescene.h>
```

### Scene management

```text
CreateObject(name, parent)
RemoveObject(object)
GetObject(name)
GetRootObjects()
Clear()
```

### Runtime

```text
Start()
Update(subsystems, passiveOnly)
Render(renderer)
PostRender(renderer)
```

### State

```text
Gravity
IsDirty()
```

The scene also owns the physics world and exposes internal registration methods used by the built-in collider components.

---

## `c3d::GameObject`

Header:

```cpp
#include <c3d/gameobject.h>
```

### Properties

```text
Name
Transform
RenderOrder
```

### Components

```cpp
HasComponent<T>()
GetComponent<T>()
AddComponent<T>()
RemoveComponent<T>()
```

There are also non-templated versions using `string_hash_t`.

### Activation

```text
Enable()
Disable()
IsEnabled()
```

### Lifecycle

```text
Start()
Reset()
FixedUpdate()
PreUpdate()
Update()
LateUpdate()
Render()
```

### Events

```text
OnCollisionEnter()
OnCollisionStay()
OnCollisionLeave()
OnTriggerEnter()
OnTriggerStay()
OnTriggerLeave()
```

---

## `c3d::Component`

Header:

```cpp
#include <c3d/component.h>
```

The base component defines overridable lifecycle, rendering, collision, trigger, enable/disable, serialization, and UI methods.

The most commonly overridden gameplay methods are:

```cpp
void Start() override;
void Update(float t, float dt, c3d::GameSubsystems& subsystems) override;
void FixedUpdate(float t, float dt, c3d::GameSubsystems& subsystems) override;
void Render(ga::render::RendererBase& renderer) override;
```

---

## `c3d::ComponentFactory`

Header:

```cpp
#include <c3d/componentfactory.h>
```

The factory supports runtime component registration and construction.

```cpp
c3d::ComponentFactory::Global()
    .RegisterComponent<MyComponent>("Gameplay");
```

Use this when a custom component should participate in the inspector/runtime factory system.
