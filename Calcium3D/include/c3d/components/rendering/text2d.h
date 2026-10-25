#ifndef CALCIUM3D__COMPONENTS__RENDERING__TEXT2D_H
#define CALCIUM3D__COMPONENTS__RENDERING__TEXT2D_H
#pragma once

#include <c3d/component.h>

#include <gallium/core/property.h>
#include <gallium/render/font.h>

namespace c3d::components::rendering
{
	class Text2D final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::rendering::Text2D, c3d::Component);

	public:
		std::optional<std::reference_wrapper<ga::render::Font>> Font;
		ga::core::Property<std::string>                         Text;
		ga::core::Property<glm::vec4>                           Color;

		explicit Text2D(GameObject* owner);
		explicit Text2D(ga::core::CttiDeserializer& deserializer);
		~Text2D();

		void Render2D(c3d::graphics::Renderer2D& renderer) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* CALCIUM3D__COMPONENTS__RENDERING__TEXT2D_H */
