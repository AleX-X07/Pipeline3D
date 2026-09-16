# Audio API

## `AudioSource`

Header:

```cpp
#include <c3d/components/audio/audiosourcecomponent.h>
```

Properties:

```text
Sound
Volume
IsSpatialized
IsLooping
```

The source creates its Gallium audio source lazily during `Update` when `Sound` is valid.

## `AudioListener`

Header:

```cpp
#include <c3d/components/audio/audiolistenercomponent.h>
```

Attach it to the object that should represent the listener, typically the camera:

```cpp
camera->AddComponent<c3d::components::audio::AudioListener>();
```

The implementation updates the underlying Gallium listener from the owning object's transform.
