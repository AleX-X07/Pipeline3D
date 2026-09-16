# Audio

Audio components are thin Calcium3D wrappers around Gallium's audio system.

## Audio source

```cpp
auto audio =
    object->AddComponent<
        c3d::components::audio::AudioSource>();

audio->Sound =
    ga::assets::AssetManager::Global()
        .Load<ga::audio::SoundHandle>("/assets/music.mp3");
```

Properties:

| Property | Meaning | Current default |
|---|---|---|
| `Sound` | sound asset handle | invalid/unset |
| `Volume` | playback volume | `1.0` |
| `IsSpatialized` | positional audio | `false` |
| `IsLooping` | restart when playback ends | `false` |

The source is created lazily when `Update` sees a valid `Sound`.

## Spatial audio

Enable it with:

```cpp
audio->IsSpatialized = true;
```

The component follows its `GameObject` transform.

A listener is normally attached to the camera:

```cpp
camera->AddComponent<
    c3d::components::audio::AudioListener>();
```

The example application does exactly this.

## Looping

```cpp
audio->IsLooping = true;
```

The component checks the underlying Gallium source and restarts playback if a looping source is no longer playing.

## Audio system

`c3d::Game::Initialize` creates the Gallium `AudioSystem` and places it into `GameSubsystems`.

Normal gameplay code should therefore use the `AudioSource` component for scene sounds rather than constructing low-level audio sources for every object.

!!! note
    `AudioSource` uses the Gallium master bus when it creates its underlying source in the supplied implementation.
