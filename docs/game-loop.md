# Game loop

The engine's main loop is implemented by `c3d::Game::Run`.

At a high level:

```text
poll platform events
        ↓
Game::Update(...)
        ↓
acquire frame image / command buffer
        ↓
Game::Render(...)
        ↓
copy rendered image to presentation image
        ↓
optional ImGui rendering
        ↓
present
        ↓
repeat
```

## Gameplay update

Your `Update` method receives:

```cpp
c3d::GameSubsystems& subsystems,
float t,
float dt
```

`GameSubsystems` provides references to:

- `platform`
- `gpu`
- `assets`
- `audio`

This is the bridge between game code and Gallium services.

## Scene update

`GameScene::Update` drives the object/component lifecycle and the physics simulation.

The object API exposes these lifecycle phases:

```text
Start
FixedUpdate
PreUpdate
Update
LateUpdate
Render
```

Components can override the corresponding callbacks.

### Fixed update

Physics uses a fixed simulation step in the current implementation:

```cpp
1.0f / 100.0f
```

That is a 100 Hz physics step.

!!! note "Implementation note"
    The fixed step is currently an implementation detail of `GameScene`, not a configurable public setting in the supplied API.

## Collision callbacks

`GameObject` exposes:

```cpp
OnCollisionEnter
OnCollisionStay
OnCollisionLeave

OnTriggerEnter
OnTriggerStay
OnTriggerLeave
```

Components can also override these callbacks through `c3d::Component`.

## Rendering

The application controls the renderer explicitly. A typical frame is:

```cpp
scene.Render(*renderer);
renderer->Render(subsystems.gpu, encoder, frameImage);
```

This separation is useful: the scene knows what should be rendered, while the renderer knows how to turn that information into GPU work.

## UI

When `C3D_ENABLE_UI` is enabled, `Game::Run` initializes Dear ImGui through Gallium.

Override:

```cpp
void RenderUi() override;
```

The example simply delegates to:

```cpp
scene.RenderUi();
```

The built-in scene/object/component inspector UI is therefore available without a separate editor application.
