# First game

The smallest Calcium3D application is a subclass of `c3d::Game`.

```cpp
#include <c3d/game.h>

class MyGame : public c3d::Game
{
public:
    const char* Title() const override { return "My Game"; }
    glm::uvec3 Version() const override { return { 1, 0, 0 }; }

    void Initialize(ga::platform::Platform& platform,
                    ga::gpu::Device& gpu) override;

    void Update(c3d::GameSubsystems& subsystems,
                float t, float dt) override;

    std::vector<ga::gpu::SemaphoreId>
    Render(c3d::GameSubsystems& subsystems,
           ga::gpu::Image& frameImage,
           const ga::gpu::CommandEncoder& encoder) override;
};
```

Then the application entry point can simply be:

```cpp
int main()
{
    return c3d::Game::Run<MyGame>();
}
```

## What `Game::Run` gives you

The engine's `Run` implementation creates and owns:

1. the Gallium platform,
2. the Gallium GPU device,
3. the optional Dear ImGui integration,
4. the game's audio system,
5. the global asset manager,
6. the component factory,
7. the main loop.

Your `Game` subclass plugs into that lifecycle.

## Initialize

Use `Initialize` for things that need the platform or GPU to exist:

```cpp
void MyGame::Initialize(ga::platform::Platform& platform,
                        ga::gpu::Device& gpu)
{
    Game::Initialize(platform, gpu);

    // Create your scene, renderer, assets, etc.
}
```

Calling `Game::Initialize` first is important: the base implementation initializes the message bus, logging, component registrations, audio system, and asset manager.

## Build a scene

Create a scene and populate it with objects:

```cpp
c3d::GameScene scene;

auto player = scene.CreateObject("Player");
player->Transform->Position = { 0.0f, 1.0f, 0.0f };

auto camera = scene.CreateObject("Camera");
camera->Transform->Position = { 0.0f, 2.0f, 5.0f };
camera->AddComponent<c3d::components::rendering::Camera>();
```

Components are normally created through the templated `AddComponent<T>()` helper.

## Update

The example application does its gameplay work in `Game::Update`, then lets the scene process its objects:

```cpp
void MyGame::Update(c3d::GameSubsystems& subsystems,
                    float t, float dt)
{
    // Your gameplay/input code.

    scene.Update(subsystems);
}
```

`dt` is the elapsed frame time supplied by Gallium's timer.

## Render

A common setup is the built-in `DefaultRenderer`:

```cpp
std::unique_ptr<ga::render::RendererBase> renderer;

renderer = std::make_unique<c3d::graphics::DefaultRenderer>(gpu);
```

Then:

```cpp
std::vector<ga::gpu::SemaphoreId>
MyGame::Render(c3d::GameSubsystems& subsystems,
               ga::gpu::Image& frameImage,
               const ga::gpu::CommandEncoder& encoder)
{
    scene.Render(*renderer);
    return renderer->Render(subsystems.gpu, encoder, frameImage);
}
```

The scene collects geometry, lights, camera information, and environment information through components. The renderer consumes that information.

## Deinitialize

Release your scene/renderer resources before calling the base implementation:

```cpp
void MyGame::Deinitialize()
{
    renderer.reset();
    scene.Clear();

    Game::Deinitialize();
}
```

The example application follows this pattern.

!!! tip "A good student workflow"
    Start by copying the shape of `Calcium3DExampleApp`, then replace its scene contents one system at a time. Keep the engine itself untouched while learning the API.
