#ifndef CALCIUM3D__COMPONENTS__RENDERING__MESH_H
#define CALCIUM3D__COMPONENTS__RENDERING__MESH_H
#pragma once

#include <c3d/component.h>
#include <gallium/render/mesh.h>

namespace c3d::components::rendering
{
	class Mesh final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::rendering::Mesh, c3d::Component);
		std::unique_ptr<ga::render::MeshInstance> m_meshInstance;

	public:
		std::optional<std::reference_wrapper<ga::render::Mesh>> Asset;

		explicit Mesh(GameObject* owner);
		explicit Mesh(ga::core::CttiDeserializer& deserializer);
		~Mesh();

		void Render(ga::render::RendererBase& renderer) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* CALCIUM3D__COMPONENTS__RENDERING__MESH_H */
