# Calcium3D

A small, component-based 3D game engine built on top of the **Gallium** platform/rendering layer.

Calcium3D is the part of the stack that gives an application a game-oriented model:

- `Game` owns the application lifecycle.
- `GameScene` owns objects and physics.
- `GameObject` combines a transform with components.
- Components provide rendering, physics, audio, and gameplay behaviors.
- The default renderer turns scene rendering data into the final image.

> **Mental model:** write your game against `c3d`. Reach into `ga` when you need a lower-level platform, asset, GPU, rendering, or audio service.

## Where to start

| If you want to… | Start here |
|---|---|
| Make a game application | [First game](getting-started.md) |
| Understand the architecture | [Engine model](concepts.md) |
| Add objects and behavior | [GameObjects & Components](gameobjects-components.md) |
| Move/rotate objects | [Transforms](transforms.md) |
| Draw meshes and lights | [Rendering](rendering.md) |
| Add collisions | [Physics](physics.md) |
| Play sounds | [Audio](audio.md) |
| Look up a class/member | [API reference](api/core.md) |
| Avoid common gotchas | [Tips & notes](tips.md) |

## The three projects

### Calcium3D

The engine itself. Its public API lives under `Calcium3D/include/c3d/`.

### Gallium

A **separate repository** providing the lower-level runtime services Calcium3D uses: platform/windowing, GPU, assets, audio, rendering infrastructure, input, messaging, and utilities.

This documentation does **not** attempt to turn the bundled `gallium-core` source snapshot into a second API reference.

### Calcium3DExampleApp

A small application showing how to put the engine together. It is best read as a starting point for students rather than as an engine subsystem.

## A useful first principle

Most game code should live at the `Game`, `GameScene`, `GameObject`, and `Component` level.

If you find yourself manually driving GPU command buffers just to display a mesh, you are probably working below the level Calcium3D intends for ordinary gameplay code.
