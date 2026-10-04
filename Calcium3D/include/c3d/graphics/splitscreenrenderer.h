#ifndef CALCIUM3D__GRAPHICS__SPLITSCREENRENDERER_H
#define CALCIUM3D__GRAPHICS__SPLITSCREENRENDERER_H
#pragma once

#include <gallium/render/rendererbase.h>
#include <memory>

namespace c3d::graphics
{
	enum class EScreenSplitDirection
	{
		Horizontal,
		Vertical
	};

	class SplitScreenRenderer
		: public ga::render::RendererBase
	{
		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

		float     m_splitLineWidth = 2.0f;
		glm::vec3 m_splitLineColor = glm::vec3(1.0f);

	public:
		SplitScreenRenderer(ga::gpu::Device& gpu, EScreenSplitDirection direction = EScreenSplitDirection::Vertical);
		~SplitScreenRenderer();

		void AddGeometry(const ga::render::MeshInstance& mesh, const glm::mat4& transform, uint32_t arrayIndex = 0) override;
		void AddLight(const ga::render::LightData& light, const glm::mat4& transform) override;
		void SetEnvironment(uint32_t diffuse, uint32_t specular, float intensity) override;
		void SetCamera(const glm::mat4& view, const glm::mat4& projection, uint32_t index = 0) override;
		bool HasCamera() const override;

		void SetSplitLine(float width, const glm::vec3& color);

		std::vector<ga::gpu::SemaphoreId> Render(ga::gpu::Device& gpu, const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& output) override;
		void Reset() override;
	};
}

#endif /* CALCIUM3D__GRAPHICS__SPLITSCREENRENDERER_H */
