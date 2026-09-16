# Example application

`Calcium3DExampleApp` is intentionally simple. It demonstrates the intended way a student can assemble an application without needing an editor.

It creates:

```text
Damaged Helmet
Box
Camera
Envmap
Light1
Light2
Audio
```

## Scene recipe

The example's main ideas are:

1. create a `GameScene`;
2. create a `GameObject`;
3. add one or more components;
4. load assets through Gallium's global asset manager;
5. set properties;
6. update the scene;
7. render the scene with `DefaultRenderer`.

For example:

```cpp
auto mesh1 = m_gameScene.CreateObject("Damaged Helmet");

auto meshComponent =
    mesh1->AddComponent<
        c3d::components::rendering::Mesh>();

meshComponent->Asset =
    ga::assets::AssetManager::Global()
        .Load<ga::render::Mesh>("/assets/DamagedHelmet.glb");
```

## Camera recipe

The example combines three components on the camera:

```text
Camera
LookAt
AudioListener
```

The `LookAt` target is the helmet object, so the camera orientation is driven by the target rather than manually calculated each frame.

## Lighting recipe

Two spot lights are created, each with a `LookAt` component targeting the helmet.

This is a useful teaching pattern:

```text
Light
 +
LookAt
```

rather than writing custom orientation math.

## Input

The example moves the camera with the arrow keys through Gallium's input API:

```cpp
subsystems.platform.Input().IsHeld(...)
```

This is one of the places where a game crosses from the Calcium3D layer into Gallium.

## What students should change first

Good exercises are:

- change the camera controls;
- add a second mesh;
- make an object move;
- add a rigidbody and collider;
- create a trigger;
- add a third light;
- change the environment;
- add looping spatial audio;
- write a custom component.

!!! note
    The example is a base project for learning. It should not be treated as a canonical engine architecture or as part of the public Calcium3D API.
