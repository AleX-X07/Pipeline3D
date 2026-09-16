#include <c3d/components/audio/audiolistenercomponent.h>

#include <gallium/audio/audiosystem.h>
#include <c3d/game.h>
#include <c3d/gameobject.h>
#include <c3d/transformdata.h>

using namespace c3d::components::audio;

AudioListener::AudioListener(c3d::GameObject* owner)
	: c3d::Component(owner)
{
}

AudioListener::AudioListener(ga::core::CttiDeserializer& deserializer)
	: c3d::Component(deserializer)
{
}

AudioListener::~AudioListener() noexcept
{
}

void AudioListener::Update(float t, float dt, c3d::GameSubsystems& subsystems)
{
	const auto& transform = GetGameObject()->Transform.ConstRef();

	glm::vec3 position = transform.Position;
	glm::vec3 up       = transform.TransformVector({ 0.0f, 1.0f, 0.0f });
	glm::vec3 forward  = transform.TransformVector({ 0.0f, 0.0f, -10.0f });
	subsystems.audio.SetListenerTransform(position, forward, up);
}

void AudioListener::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);
}

void AudioListener::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);
}

void AudioListener::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	Component::ResolvePointers(deserializer);
}

void AudioListener::RenderUi()
{
	Component::RenderUi();
}
