# Engine model

Calcium3D is organized around a small number of cooperating objects.

```text
Game
 │
 ├── GameScene
 │    │
 │    ├── GameObject
 │    │    ├── TransformData
 │    │    ├── Mesh
 │    │    ├── Light
 │    │    └── ...
 │    │
 │    └── physics world
 │
 └── renderer
```

## `Game`

`c3d::Game` is the application-level lifecycle.

Override:

- `Title()`
- `Version()`
- `Initialize(...)`
- `Update(...)`
- `Render(...)`
- optionally `RenderUi()`
- `Deinitialize()`

`Game::Run<T>()` creates an instance and runs it.

## `GameScene`

`c3d::GameScene` owns game objects and the physics world.

Useful operations:

- `CreateObject`
- `RemoveObject`
- `GetObject`
- `GetRootObjects`
- `Update`
- `Render`
- `Clear`

A scene also owns the engine's ReactPhysics3D world.

## `GameObject`

A `c3d::GameObject` has:

- `Name`
- `Transform`
- `RenderOrder`
- zero or more components
- an enabled/disabled state
- optional parent/child transform relationships

The usual gameplay pattern is:

```cpp
auto object = scene.CreateObject("Player");

object->Transform->Position = { 0, 1, 0 };

auto body = object->AddComponent<c3d::components::physics::Rigidbody>();
auto collider = object->AddComponent<c3d::components::physics::CapsuleCollider>();
```

## Components

`c3d::Component` is the common base for engine behaviors.

The supplied engine registers these built-in components:

### Audio
- `AudioSource`
- `AudioListener`

### Physics
- `Rigidbody`
- `BoxCollider`
- `CapsuleCollider`
- `SphereCollider`

### Rendering
- `Camera`
- `Light`
- `Mesh`

### Other
- `LookAt`

Components participate in the scene lifecycle and can serialize their state.

## Properties

Many public component fields are `ga::core::Property<T>` rather than raw C++ fields:

```cpp
camera->AddComponent<c3d::components::rendering::Camera>()->Fov = 75.0f;
```

The property wrapper is important because the engine uses it for change tracking and UI/serialization integration.

When you need the current value in ordinary code, the property is callable:

```cpp
float fov = cameraComponent->Fov();
```

!!! tip "Property vs value"
    Think of a `Property<T>` as an engine-aware value. Assign to it normally; call it when you want the current value.
