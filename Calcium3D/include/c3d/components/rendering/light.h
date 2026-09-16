#ifndef CALCIUM3D__COMPONENTS__RENDERING__LIGHT_H
#define CALCIUM3D__COMPONENTS__RENDERING__LIGHT_H
#pragma once

#include <gallium/core/property.h>
#include <c3d/component.h>

#include <glm/glm.hpp>

#include <memory>

namespace c3d::components::rendering
{
	enum class ELightType
		: uint32_t
	{
		Point,
		Spot,
		Directional
	};

	class Light final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::rendering::Light, c3d::Component);

		struct Internal;
		std::unique_ptr<Internal> m_lightImpl;

	public:
		ga::core::Property<ELightType> Type;
		ga::core::Property<glm::vec3>  Color;
		ga::core::Property<float>      Intensity;
		ga::core::Property<glm::vec2>  SpotAngles;

		explicit Light(GameObject* owner);
		virtual ~Light() noexcept;

		void Render(ga::render::RendererBase& renderer);

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* CALCIUM3D__COMPONENTS__RENDERING__LIGHT_H */
