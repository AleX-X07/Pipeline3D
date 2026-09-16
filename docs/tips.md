# Tips & notes

This page collects the things that are easy to miss when first using the engine.

!!! tip "Call the base `Game::Initialize`"
    The base implementation initializes the message bus, logging, component factory, audio system, and asset manager. Call it before relying on those services.

!!! tip "Use `Property<T>` like an engine value"
    Assign with `object->Transform->Position = ...` and read with `object->Transform->Position()`. The property wrapper participates in change tracking and UI/serialization.

!!! tip "Keep assets in Gallium"
    Meshes, images, and sounds are loaded through Gallium's asset manager. Calcium3D components hold references to those assets rather than implementing their own asset loading layer.

!!! tip "Use `DefaultRenderer` for normal scenes"
    `GameScene::Render` plus `DefaultRenderer` is the high-level rendering path. Only work directly with the Gallium renderer/GPU APIs when you intentionally need lower-level control.

!!! tip "Put listeners on cameras"
    A common setup is a `Camera` and `AudioListener` on the same object, so the visual and audio viewpoints follow one transform.

!!! tip "Prefer components over large Game subclasses"
    If a behavior belongs naturally to an object, a component is usually a better fit than adding more unrelated logic to `Game::Update`.

!!! warning "Don't treat object names as unique IDs"
    `GameScene::GetObject` is a recursive name lookup. If a game needs stable identity, it should introduce its own identity scheme.

!!! warning "Be careful with physics transforms"
    A dynamic rigidbody is controlled by the physics simulation. Writing its transform every frame can fight the simulation.

!!! note "Physics is fixed at 100 Hz"
    The supplied implementation uses a `1 / 100` second physics step. This is not currently exposed as a public configuration setting.

!!! note "The engine is Windows/Visual Studio oriented in this snapshot"
    The supplied projects are `.vcxproj` files, target Windows, use the Visual Studio v145 toolset, and compile Slang shaders through custom MSBuild targets.

!!! note "The default camera aspect ratio is 1.7"
    The camera implementation starts at `1.7` and listens for window resize messages. In normal use, the resize path updates the ratio.

!!! note "Mesh collider is not currently registered"
    The scene source contains commented-out mesh-collider registration code. The public built-in physics component set currently contains sphere, box, and capsule colliders.

## A small style rule for gameplay code

Prefer:

```cpp
auto player = scene.CreateObject("Player");
player->Transform->Position = spawnPoint;
player->AddComponent<PlayerController>();
player->AddComponent<c3d::components::physics::Rigidbody>();
```

over building a large custom object abstraction around every engine feature.

The engine's core strength is the small composition model: **object + transform + components**.
