# Components API

## Rendering

### `c3d::components::rendering::Mesh`

```text
Asset : optional reference to ga::render::Mesh
```

### `c3d::components::rendering::Camera`

```text
AspectRatio : float
Fov         : float
ZBounds     : glm::vec2
```

Methods include:

```text
UpdateMatrix()
ProjectionMatrix()
OnWindowResize(...)
RenderGizmo()
```

### `c3d::components::rendering::Light`

```text
Type       : ELightType
Color      : glm::vec3
Intensity  : float
SpotAngles : glm::vec2
```

`ELightType` values:

```text
Point
Spot
Directional
```

### `c3d::components::rendering::Environment`

```text
Diffuse   : optional reference to ga::gpu::Image
Specular  : optional reference to ga::gpu::Image
Intensity : float
```

---

## Physics

### `Rigidbody`

```text
Velocity        : glm::vec3
AngularVelocity : glm::vec3
Mass            : float
IsDynamic       : bool
```

Methods:

```text
EnforceTransform()
```

plus normal component lifecycle/serialization/UI methods.

### `SphereCollider`

```text
Radius
Friction
RollingResistance
Bounciness
IsTrigger
IsDynamic
```

### `BoxCollider`

```text
Size
Friction
RollingResistance
Bounciness
IsTrigger
IsDynamic
```

### `CapsuleCollider`

```text
Height
Radius
Friction
RollingResistance
Bounciness
IsTrigger
IsDynamic
```

---

## Audio

### `AudioSource`

```text
Sound
Volume
IsSpatialized
IsLooping
```

### `AudioListener`

No public configurable properties in the supplied API. It follows the owning object's transform when updated.

---

## Other

### `LookAt`

```text
Target : c3d::GameObject*
```

During update it points the owning object's transform toward the target.

---

## Common component pattern

All built-in components derive from:

```cpp
c3d::Component
```

and expose their configurable data as `ga::core::Property<T>` where appropriate.
