# Transforms

Every `GameObject` owns a `c3d::TransformData` exposed as:

```cpp
object->Transform
```

The transform contains:

- `Position`
- `Orientation`
- `Scale`
- `Parent`

Defaults are:

```text
Position    = (0, 0, 0)
Orientation = identity quaternion
Scale       = (1, 1, 1)
Parent      = null
```

## Position

```cpp
object->Transform->Position = { 0.0f, 1.0f, 0.0f };
```

Read the current value:

```cpp
glm::vec3 position = object->Transform->Position();
```

## Rotation

The transform stores orientation as a `glm::quat`.

You can rotate with:

```cpp
object->Transform->Rotate(axis, angle);
```

or:

```cpp
object->Transform->Rotate(rotationQuaternion);
```

Angles are passed to the underlying GLM quaternion helpers, so use radians when supplying an angle.

## Looking at a target

```cpp
object->Transform->LookAt(targetPosition, upVector);
```

There is also a `LookAt` component, which is convenient when the target is another `GameObject`:

```cpp
auto lookAt = object->AddComponent<c3d::components::other::LookAt>();
lookAt->Target = target;
```

## Parent/child transforms

A child can be created directly:

```cpp
auto child = scene.CreateObject("Child", parent);
```

or assigned through the property:

```cpp
child->Transform->Parent = &parent->Transform();
```

The transform maintains local and global matrices:

```cpp
const glm::mat4& local = object->Transform->LocalMatrix();
const glm::mat4& world = object->Transform->GlobalMatrix();
```

## Transforming points and vectors

Use:

```cpp
glm::vec3 worldPoint =
    object->Transform->TransformPoint(localPoint);

glm::vec3 worldVector =
    object->Transform->TransformVector(localVector);
```

and the inverse operations:

```cpp
glm::vec3 localPoint =
    object->Transform->DetransformPoint(worldPoint);
```

The distinction matters:

- **Point**: translation, rotation, and scale apply.
- **Vector**: translation does not apply.

!!! tip "Keep gameplay in local space when possible"
    Parent objects are useful for weapons, cameras, attachments, and other hierarchies. Let the transform system calculate the global matrix rather than manually composing parent transforms.
