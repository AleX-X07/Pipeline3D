#ifndef CALCIUM3D__COMPONENTS__RENDERING__ENVIRONMENT_H
#define CALCIUM3D__COMPONENTS__RENDERING__ENVIRONMENT_H
#pragma once

#include <c3d/component.h>

#include <gallium/core/property.h>
#include <gallium/gpu/image.h>

namespace c3d::components::rendering
{
	class Environment final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::rendering::Environment, c3d::Component);

	public:
		std::optional<std::reference_wrapper<ga::gpu::Image>> Diffuse;
		std::optional<std::reference_wrapper<ga::gpu::Image>> Specular;
		ga::core::Property<float>                             Intensity;

		explicit Environment(GameObject* owner);
		explicit Environment(ga::core::CttiDeserializer& deserializer);
		~Environment();

		void Render(ga::render::RendererBase& renderer) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* CALCIUM3D__COMPONENTS__RENDERING__ENVIRONMENT_H */
