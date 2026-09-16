# Project structure

The supplied repository snapshot contains three relevant pieces.

```text
Calcium3D/
    include/c3d/       # public engine headers
    src/               # engine implementation
    shaders/           # Calcium3D shaders

gallium-core/
    include/gallium/   # lower-level Gallium API
    src/                # Gallium implementation

Calcium3DExampleApp/
    include/           # example game header
    src/                # example application
```

## What belongs in a game project?

A game/application normally includes Calcium3D public headers and links against Calcium3D and Gallium.

The supplied Visual Studio project shows the intended dependency direction:

```text
Your application
      │
      ├── Calcium3D
      │      │
      │      └── Gallium
      │
      └── Gallium
```

The engine's public headers intentionally expose some Gallium types. For example, `c3d::GameSubsystems` contains references to Gallium's platform, GPU, asset, and audio systems.

That means Gallium is a real dependency, but it remains a separate layer/repository.

## Public vs internal

The main API surface to document and use is:

```text
Calcium3D/include/c3d/
```

The `src/` directory contains implementation details such as physics registration, renderer internals, serialization, and ImGui UI code. Those details can explain behavior, but they should not be treated automatically as stable API.

!!! note "Documentation convention"
    This site describes the public `c3d` API first. When implementation behavior is useful to know, it is called out explicitly rather than presenting an internal detail as a guaranteed contract.
