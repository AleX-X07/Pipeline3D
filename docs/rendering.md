# Rendering

Calcium3D's built-in rendering model is component based.

The supplied `DefaultRenderer` understands:

- geometry from `Mesh`
- lights from `Light`
- environment lighting from `Environment`
- a camera from `Camera`

## Mesh

```cpp
auto mesh = object->AddComponent<c3d::components::rendering::Mesh>();

mesh->Asset =
    ga::assets::AssetManager::Global()
        .Load<ga::render::Mesh>("/assets/ship.glb");
```

The path is resolved through Gallium's asset manager.

The example uses:

```text
/assets/DamagedHelmet.glb
/assets/plane.glb
```

## Camera

```cpp
auto camera = scene.CreateObject("Camera");

auto cameraComponent =
    camera->AddComponent<c3d::components::rendering::Camera>();

camera->Transform->Position = { 0, 2, 5 };
```

Current defaults in the supplied implementation:

```text
Fov         = 60 degrees
AspectRatio = 1.7
ZBounds     = (0.1, 100)
```

The camera listens for window resize messages and updates its aspect ratio.

## Light

```cpp
auto light = object->AddComponent<c3d::components::rendering::Light>();

light->Type = c3d::components::rendering::ELightType::Point;
light->Color = glm::vec3(1.0f);
light->Intensity = 10.0f;
```

Supported types:

- `Point`
- `Spot`
- `Directional`

For spot lights:

```cpp
light->SpotAngles = { 12.0f, 15.0f };
```

The exact interpretation of the two angles belongs to the renderer implementation; the inspector exposes them as the spot angles.

## Environment

`Environment` supplies diffuse/specular environment images and an intensity:

```cpp
auto env = object->AddComponent<c3d::components::rendering::Environment>();

env->Diffuse =
    ga::assets::AssetManager::Global()
        .Load<ga::gpu::Image>("/assets/envmap/diffuse.hdr");

env->Specular =
    ga::assets::AssetManager::Global()
        .Load<ga::gpu::Image>("/assets/envmap/specular.hdr");

env->Intensity = 1.0f;
```

The environment images are Gallium GPU image assets.

## Default renderer

```cpp
std::unique_ptr<ga::render::RendererBase> renderer =
    std::make_unique<c3d::graphics::DefaultRenderer>(gpu);
```

The renderer interface receives:

- geometry
- lights
- environment
- camera

and eventually produces GPU work for the output image.

!!! tip "Scene first, renderer second"
    Prefer adding `Mesh`, `Light`, `Camera`, and `Environment` components to your scene rather than calling renderer methods directly. The scene-to-renderer boundary is the engine's intended high-level path.
