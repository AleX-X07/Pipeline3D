# GameObjects & Components

## Creating objects

```cpp
auto enemy = scene.CreateObject("Enemy");
```

An optional parent can be supplied:

```cpp
auto weapon = scene.CreateObject("Weapon", enemy);
```

The parent must belong to the same scene.

## Accessing objects

By name:

```cpp
auto camera = scene.GetObject("Camera");
```

Root objects:

```cpp
for (auto* object : scene.GetRootObjects())
{
    // ...
}
```

!!! warning "Names are not IDs"
    `GetObject(name)` performs a recursive name lookup. Treat names as convenient scene labels, not as a substitute for a stable entity ID.

## Components

The templated helpers are the easiest API:

```cpp
auto mesh = object->AddComponent<c3d::components::rendering::Mesh>();

if (object->HasComponent<c3d::components::rendering::Mesh>())
{
    auto* mesh = object->GetComponent<c3d::components::rendering::Mesh>();
}
```

Remove with:

```cpp
object->RemoveComponent<c3d::components::rendering::Mesh>();
```

The non-templated API uses the engine's `string_hash_t` component type IDs.

## Component lifecycle

A component can participate in:

```cpp
Start()
Reset()

FixedUpdate(t, dt, subsystems)
PreUpdate(t, dt, subsystems)
Update(t, dt, subsystems)
LateUpdate(t, dt, subsystems)

Render(renderer)

OnEnable()
OnDisable()

OnCollisionEnter(...)
OnCollisionStay(...)
OnCollisionLeave(...)

OnTriggerEnter(...)
OnTriggerStay(...)
OnTriggerLeave(...)
```

Not every component needs every callback.

## Enabling/disabling objects

```cpp
object->Disable();

if (!object->IsEnabled())
{
    object->Enable();
}
```

The scene's collision callbacks ignore disabled objects.

## Component factory

`ComponentFactory` is initialized by `Game::Initialize` and registers the built-in components into categories such as `Audio`, `Physics`, `Rendering`, and `Other`.

This is also what powers the inspector's **Add component** menu.

For custom components, the intended registration mechanism is:

```cpp
c3d::ComponentFactory::Global().RegisterComponent<MyComponent>("Gameplay");
```

!!! note
    Custom component registration should happen after the base `Game::Initialize` has initialized the factory.
