#ifndef C3D__COMPONENTS__AUDIO__AUDIOSOURCECOMPONENT_H
#define C3D__COMPONENTS__AUDIO__AUDIOSOURCECOMPONENT_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

#include <gallium/audio/audiosource.h>

namespace c3d::components::audio
{
	class AudioSource
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::audio::AudioSource, c3d::Component);

		ga::audio::AudioSourceHandle m_source;
		glm::vec3                    m_lastKnownPosition;

	public:
		ga::core::Property<ga::audio::SoundHandle> Sound;
		ga::core::Property<float>                  Volume;
		ga::core::Property<bool>                   IsSpatialized;
		ga::core::Property<bool>                   IsLooping;

		explicit AudioSource(c3d::GameObject* owner);
		explicit AudioSource(ga::core::CttiDeserializer& deserializer);
		virtual ~AudioSource() noexcept;

		void Update(float t, float dt, c3d::GameSubsystems& subsystems) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__AUDIO__AUDIOSOURCE_H */
