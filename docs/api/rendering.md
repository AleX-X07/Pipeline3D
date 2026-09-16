# Rendering API

## `c3d::graphics::DefaultRenderer`

Header:

```cpp
#include <c3d/graphics/defaultrenderer.h>
```

`DefaultRenderer` implements Gallium's `RendererBase`.

### Input methods

```cpp
void AddGeometry(const ga::render::MeshInstance& mesh,
                 const glm::mat4& transform,
                 uint32_t arrayIndex = 0);

void AddLight(const ga::render::LightData& light,
              const glm::mat4& transform);

void SetEnvironment(uint32_t diffuse,
                    uint32_t specular,
                    float intensity);

void SetCamera(const glm::mat4& view,
               const glm::mat4& projection);

bool HasCamera() const;
```

### Frame methods

```text
Render(gpu, encoder, output)
Reset()
```

Normally you do not call `AddGeometry` yourself when using the scene/component model. `GameScene::Render` feeds the renderer through the appropriate components.

## Rendering components

The scene-facing API is:

```text
Mesh       → geometry
Light      → lighting
Environment → image-based environment lighting
Camera     → view/projection
```

This is the preferred abstraction for ordinary Calcium3D game code.
