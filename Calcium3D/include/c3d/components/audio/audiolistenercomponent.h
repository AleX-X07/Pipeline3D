#ifndef C3D__COMPONENTS__AUDIO__AUDIOLISTENERCOMPONENT_H
#define C3D__COMPONENTS__AUDIO__AUDIOLISTENERCOMPONENT_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

#include <gallium/audio/audiosource.h>

namespace c3d::components::audio
{
	class AudioListener
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::audio::AudioListener, c3d::Component);

	public:
		explicit AudioListener(c3d::GameObject* owner);
		explicit AudioListener(ga::core::CttiDeserializer& deserializer);
		virtual ~AudioListener() noexcept;

		void Update(float t, float dt, c3d::GameSubsystems& subsystems) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__AUDIO__AUDIOLISTENERCOMPONENT_H */
