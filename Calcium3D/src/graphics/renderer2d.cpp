#include <c3d/graphics/renderer2d.h>

#include <gallium/platform/platform.h>
#include <gallium/platform/vfs.h>

#include <gallium/gpu/device.h>
#include <gallium/gpu/buffer.h>
#include <gallium/gpu/shadercache.h>
#include <gallium/gpu/shadermodule.h>
#include <gallium/gpu/computepipeline.h>
#include <gallium/gpu/commandbuffer.h>
#include <gallium/gpu/descriptorregistry.h>
#include <gallium/gpu/image.h>

#include <gallium/render/font.h>

#include <format>
#include <ranges>

using namespace c3d::graphics;

struct ShapeInternal
{
    EShape2DType type;
    uint32_t     image;
    uint32_t     isMsdf;
    float        msdfScale;
    glm::vec4    transform[3];
    glm::vec4    color;
    glm::vec2    uvOffset;
    glm::vec2    uvScale;
};

static ga::gpu::ShaderStructDesc s_ShaderStruct(ga::gpu::Device& gpu, const std::string& file, const std::string& name)
{
    auto& shader = gpu.GetShaderCache().Get(std::format("/shaders/2d/{}.slang.spv", file));
	return *shader.GetStruct(name);
}

static std::unique_ptr<ga::gpu::ComputePipeline> s_Shape2DPipeline(ga::gpu::Device& gpu)
{
    auto& shader = gpu.GetShaderCache().Get("/shaders/2d/shape2d.slang.spv");

	return std::make_unique<ga::gpu::ComputePipeline>(gpu, ga::gpu::ComputePipelineInfo {
		.stage = { shader, ga::gpu::EShaderStage::Compute, "cs_main" }
	});
}

Renderer2D::Renderer2D(ga::gpu::Device& gpu)
	: m_gpu(gpu)
	, m_shapeRendererPC(s_ShaderStruct(gpu, "shape2d", "PushConstants").Instantiate())
{
	m_shapeRendererCS = s_Shape2DPipeline(gpu);
}

Renderer2D::~Renderer2D()
{
}

void Renderer2D::AddShape(const Shape2D& shape)
{
	m_shapes.push_back(shape);
}

void Renderer2D::AddText(const Text2D& text)
{
    float penX = 0.0f;
    float penY = text.font.ascent;

    uint32_t prevCp = 0;

    const char* s = text.text.c_str();
    while (*s)
    {
        uint32_t cp = ga::render::Font::DecodeUtf8(s);

        if (cp == '\n')
        {
            penX = 0.0f;
            penY += text.font.ascent - text.font.descent + text.font.lineGap;
            prevCp = 0;
            continue;
        }

        auto it = text.font.glyphs.find(cp);
        if (it == text.font.glyphs.end())
        {
            prevCp = cp;
            continue;
        }

        const ga::render::Glyph& glyph = it->second;

        penX += glyph.leftBearing;

        // apply kerning
        if (prevCp != 0)
        {
            uint64_t key = ((uint64_t)prevCp << 32) | cp;
            auto kit = text.font.kerning.find(key);
            if (kit != text.font.kerning.end())
                penX += kit->second;
        }

        // glyph quad size in virtual pixels
        glm::vec2 size = glyph.xy1 - glyph.xy0;

        // center of the glyph quad, offset from pen position
        // xy0/xy1 are in font space (y-up), flip y for screen space (y-down)
        float quadX = penX + glyph.xy0.x + size.x * 0.5f;
        float quadY = penY - glyph.xy1.y + size.y * 0.5f; // flip y

        glm::mat3 localTransform = glm::mat3(1.0f);
        localTransform[2][0] = quadX;
        localTransform[2][1] = quadY;
        localTransform[0][0] = size.x;
        localTransform[1][1] = size.y;

        if (cp != ' ')
        {
            Shape2D shape;
            shape.type      = EShape2DType::Box;
            shape.transform = text.transform * localTransform;
            shape.color     = text.color;
            shape.image     = text.font.atlas->GetDescriptorIndex().slot;
            shape.uvOffset  = glyph.uv0;
            shape.uvScale   = glyph.uv1 - glyph.uv0;
            shape.isMsdf    = true;
            shape.msdfScale = glyph.msdfScale;

            AddShape(shape);
        }

        penX += glyph.advance;
        prevCp = cp;
    }
}

void Renderer2D::Render(const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& output)
{
	if (m_shapes.empty())
		return;

	if ((m_shapeBufferMaxCount < m_shapes.size()) || !m_shapeBuffer)
	{
		if (m_shapeBuffer)
			m_gpu.Defer([b = std::move(m_shapeBuffer)] {});

		m_shapeBuffer = std::make_unique<ga::gpu::Buffer>(m_gpu, ga::gpu::BufferInfo {
			.usage                   = ga::gpu::EBufferUsage::StorageBuffer,
			.size                    = m_shapes.size() * sizeof(ShapeInternal),
			.createPersistentStaging = true
		});

		m_shapeBufferMaxCount = m_shapes.size();
	}

	encoder.Transfer([&](const ga::gpu::TransferEncoder& transfer) {
		auto internalShapes = m_shapes | std::views::transform([](const Shape2D& s) {
			return ShapeInternal {
				.type      = s.type,
				.image     = s.image,
                .isMsdf    = s.isMsdf ? 1u : 0u,
                .msdfScale = s.msdfScale,
				.transform = {
					glm::vec4(s.transform[0].x, s.transform[1].x, s.transform[2].x, 0.0f),
					glm::vec4(s.transform[0].y, s.transform[1].y, s.transform[2].y, 0.0f),
					glm::vec4(s.transform[0].z, s.transform[1].z, s.transform[2].z, 0.0f)
				},
				.color     = s.color,
				.uvOffset  = s.uvOffset,
				.uvScale   = s.uvScale
			};
		}) | std::ranges::to<std::vector>();

		m_shapeBuffer->UploadInline(transfer, internalShapes);
	});

	encoder.Transition(output, ga::gpu::EImageLayout::General);

	encoder.Compute({ *m_shapeRendererCS }, [&](const ga::gpu::ComputeEncoder& compute) {
		glm::uvec3 size = (output.Size() + glm::uvec3(15)) / glm::uvec3(16);

		m_shapeRendererPC["shapes"]     = m_shapeBuffer->GetGpuAddress();
		m_shapeRendererPC["shapeCount"] = uint32_t(m_shapes.size());
		m_shapeRendererPC["output"]     = output.GetDescriptorIndex().slot;
		compute.PushConstants(m_shapeRendererPC);
		compute.Dispatch(size);
	});

	Reset();
}

void Renderer2D::Reset()
{
	m_shapes.clear();
}
