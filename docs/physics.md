# Physics

Calcium3D integrates ReactPhysics3D through `GameScene`.

A physical object is normally composed from:

```text
GameObject
 ├── Rigidbody
 └── Collider
```

## Rigidbody

```cpp
auto body = object->AddComponent<
    c3d::components::physics::Rigidbody>();
```

Public properties:

| Property | Meaning | Current default |
|---|---|---|
| `Velocity` | linear velocity | `(0, 0, 0)` |
| `AngularVelocity` | angular velocity | `(0, 0, 0)` |
| `Mass` | body mass | `1.0` |
| `IsDynamic` | dynamic vs static body | `true` |

## Colliders

Three collider shapes are currently exposed:

- `SphereCollider`
- `BoxCollider`
- `CapsuleCollider`

### Sphere

```cpp
auto collider =
    object->AddComponent<
        c3d::components::physics::SphereCollider>();

collider->Radius = 0.5f;
```

### Box

```cpp
auto collider =
    object->AddComponent<
        c3d::components::physics::BoxCollider>();

collider->Size = { 1.0f, 2.0f, 1.0f };
```

### Capsule

```cpp
auto collider =
    object->AddComponent<
        c3d::components::physics::CapsuleCollider>();

collider->Radius = 0.5f;
collider->Height = 2.0f;
```

All three expose:

```text
Friction
RollingResistance
Bounciness
IsTrigger
IsDynamic
```

Current collider defaults in the supplied implementation are:

```text
Friction          = 1.5
RollingResistance = 7.5
Bounciness        = 0.5
IsTrigger          = false
IsDynamic          = true
```

## Gravity

`GameScene` initializes:

```cpp
Gravity = { 0.0f, -9.81f, 0.0f };
```

The property is a public `glm::vec3`, so a game can change it:

```cpp
scene.Gravity = { 0.0f, -20.0f, 0.0f };
```

## Collision callbacks

At the object level:

```cpp
void OnCollisionEnter(const c3d::physics::Collision& collision);
void OnCollisionStay(const c3d::physics::Collision& collision);
void OnCollisionLeave(const c3d::physics::Collision& collision);
```

Trigger callbacks receive the other object:

```cpp
void OnTriggerEnter(GameObject* other);
void OnTriggerStay(GameObject* other);
void OnTriggerLeave(GameObject* other);
```

A `Collision` contains:

```text
penetrationDepth
normal
contactPoints
other
```

## Static vs dynamic

`Rigidbody::IsDynamic` determines whether the physics body is created as dynamic or static.

Collider `IsDynamic` is also exposed because collider changes are synchronized with the physics representation.

!!! note "Physics step"
    The supplied implementation advances the physics world at 100 Hz. The public API does not currently expose the fixed-step size.

!!! warning "Transform ownership"
    A `Rigidbody` participates in synchronizing physics and the object's transform. Avoid repeatedly fighting physics by assigning the transform every frame to a body that is supposed to be simulated.
