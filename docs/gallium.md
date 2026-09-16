# The Gallium boundary

Gallium is a **separate repository** and should stay conceptually separate from Calcium3D.

Calcium3D depends on Gallium for lower-level services, including:

- platform/window management
- input
- GPU device and command buffers
- assets
- audio
- rendering infrastructure
- messaging
- properties and reflection/serialization utilities

## What Calcium3D adds

Calcium3D turns those lower-level services into a game-engine model:

```text
Gallium
  ↓
services/resources/runtime primitives
  ↓
Calcium3D
  ↓
Game / Scene / GameObject / Component
```

## When should I use Gallium directly?

Use Gallium directly when the Calcium3D API intentionally exposes the lower-level concept or when you need a capability Calcium3D does not wrap.

Examples from the supplied example app:

```cpp
subsystems.platform.Input().IsHeld(...);
```

and:

```cpp
ga::assets::AssetManager::Global()
    .Load<ga::render::Mesh>(path);
```

The first uses Gallium input; the second uses Gallium's asset manager.

## What this documentation does not cover

This site does not duplicate:

- the complete Gallium GPU API,
- the complete asset manager API,
- Gallium's renderer API,
- Gallium audio internals,
- Gallium platform internals.

Those belong in Gallium's own documentation.

!!! note "Repository boundary"
    The supplied snapshot contains a `gallium-core` directory because the Visual Studio solution builds against it. For documentation purposes, treat that code as a dependency snapshot rather than as part of the Calcium3D engine itself.
