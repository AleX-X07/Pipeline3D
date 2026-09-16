# Physics API

## `c3d::physics::Collision`

```cpp
struct Collision
{
    float                  penetrationDepth;
    glm::vec3              normal;
    std::vector<glm::vec3> contactPoints;
    GameObject*            other;
};
```

The object receiving a collision callback can use `other` to identify the other game object.

## `GameScene`

The scene owns the ReactPhysics3D world and provides collider/rigidbody registration internally to the built-in components.

Public gameplay code normally does not need to call the registration functions directly.

## Physics components

| Component | Shape/data |
|---|---|
| `Rigidbody` | simulated body |
| `SphereCollider` | sphere |
| `BoxCollider` | box |
| `CapsuleCollider` | capsule |

A mesh collider is visible as a commented-out placeholder in the supplied source; it is not currently part of the public built-in component set.
