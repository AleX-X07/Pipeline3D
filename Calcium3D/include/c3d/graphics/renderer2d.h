#ifndef C3D__GRAPHICS__RENDERER2D_H
#define C3D__GRAPHICS__RENDERER2D_H
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <gallium/gpu/shadermodule_reflection.h>

#include <glm/glm.hpp>

namespace ga::gpu
{
	class Device;
	class Buffer;
	class ComputePipeline;
	class CommandEncoder;
	class Image;
}
namespace ga::render { struct Font; }

namespace c3d::graphics
{
	enum class EShape2DType
		: uint32_t
	{
		Box,
		Circle
	};

	struct Shape2D
	{
		EShape2DType type;
		glm::mat3    transform;
		glm::vec4    color;
		
		bool         isMsdf    = false;
		float        msdfScale = 0.0f;

		uint32_t     image     = uint32_t(-1);
		glm::vec2    uvOffset  = glm::vec2(0.0f);
		glm::vec2    uvScale   = glm::vec2(1.0f);
	};

	struct Text2D
	{
		ga::render::Font& font;
		std::string       text;
		glm::mat3         transform;
		glm::vec4         color = glm::vec4(1.0f);
	};

	class Renderer2D
	{
		ga::gpu::Device& m_gpu;

		std::vector<Shape2D> m_shapes;

		std::unique_ptr<ga::gpu::Buffer>          m_shapeBuffer;
		size_t                                    m_shapeBufferMaxCount = 0;
		std::unique_ptr<ga::gpu::ComputePipeline> m_shapeRendererCS;
		ga::gpu::ShaderStructInstance             m_shapeRendererPC;

	public:
		Renderer2D(ga::gpu::Device& gpu);
		~Renderer2D();

		void AddShape(const Shape2D& shape);
		void AddText(const Text2D& text);

		void Render(const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& output);
		void Reset();
	};
}

#endif /* C3D__GRAPHICS__RENDERER2D_H */
