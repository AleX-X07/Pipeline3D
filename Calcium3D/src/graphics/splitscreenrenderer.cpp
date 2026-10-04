#include <c3d/graphics/splitscreenrenderer.h>
#include <c3d/graphics/defaultrenderer.h>

#include <gallium/gpu/descriptorregistry.h>
#include <gallium/gpu/image.h>
#include <gallium/gpu/commandbuffer.h>
#include <gallium/gpu/computepipeline.h>
#include <gallium/gpu/shadercache.h>

#include <array>

using namespace c3d::graphics;

struct SplitScreenRenderer::Internal
{
	std::array<std::unique_ptr<DefaultRenderer>, 2> renderers;
	std::unique_ptr<ga::gpu::ComputePipeline>       mergePipeline;

	EScreenSplitDirection                           splitDirection = EScreenSplitDirection::Vertical;
	std::array<std::unique_ptr<ga::gpu::Image>, 2>  outputs;
};

static std::unique_ptr<ga::gpu::ComputePipeline> s_MergePipeline(ga::gpu::Device& gpu)
{
	auto& shader = gpu.GetShaderCache().Get("/shaders/c3d/splitscreen.slang.spv");

	return std::make_unique<ga::gpu::ComputePipeline>(gpu, ga::gpu::ComputePipelineInfo {
		.stage = { shader, ga::gpu::EShaderStage::Compute, "cs_main" }
	});
}

SplitScreenRenderer::SplitScreenRenderer(ga::gpu::Device& gpu, EScreenSplitDirection direction /* = EScreenSplitDirection::Vertical */)
	: m_pImpl(new Internal)
{
	m_pImpl->renderers[0]   = std::make_unique<DefaultRenderer>(gpu);
	m_pImpl->renderers[1]   = std::make_unique<DefaultRenderer>(gpu);
	m_pImpl->mergePipeline  = s_MergePipeline(gpu);
	m_pImpl->splitDirection = direction;
}

SplitScreenRenderer::~SplitScreenRenderer()
{
	m_pImpl->renderers[0].reset();
	m_pImpl->renderers[1].reset();
}

void SplitScreenRenderer::AddGeometry(const ga::render::MeshInstance& mesh, const glm::mat4& transform, uint32_t arrayIndex /* = 0 */)
{
	m_pImpl->renderers[0]->AddGeometry(mesh, transform, arrayIndex);
	m_pImpl->renderers[1]->AddGeometry(mesh, transform, arrayIndex);
}

void SplitScreenRenderer::AddLight(const ga::render::LightData& light, const glm::mat4& transform)
{
	m_pImpl->renderers[0]->AddLight(light, transform);
	m_pImpl->renderers[1]->AddLight(light, transform);
}

void SplitScreenRenderer::SetEnvironment(uint32_t diffuse, uint32_t specular, float intensity)
{
	m_pImpl->renderers[0]->SetEnvironment(diffuse, specular, intensity);
	m_pImpl->renderers[1]->SetEnvironment(diffuse, specular, intensity);
}

void SplitScreenRenderer::SetCamera(const glm::mat4& view, const glm::mat4& projection, uint32_t index /* = 0 */)
{
	if (index < 2)
		m_pImpl->renderers[index]->SetCamera(view, projection);
}

bool SplitScreenRenderer::HasCamera() const
{
	return m_pImpl->renderers[0]->HasCamera() && m_pImpl->renderers[1]->HasCamera();
}

void SplitScreenRenderer::SetSplitLine(float width, const glm::vec3& color)
{
	m_splitLineWidth = width;
	m_splitLineColor = color;
}

std::vector<ga::gpu::SemaphoreId> SplitScreenRenderer::Render(ga::gpu::Device& gpu, const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& output)
{
	
	glm::uvec2 targetSize = output.Size();
	if (m_pImpl->splitDirection == EScreenSplitDirection::Vertical)
		targetSize.x >>= 1;
	else
		targetSize.y >>= 1;

	bool shouldRebuildSplitOutputs = false;
	if (!m_pImpl->outputs[0] || !m_pImpl->outputs[1])
		shouldRebuildSplitOutputs = true;
	else if (glm::uvec2(m_pImpl->outputs[0]->Size()) != targetSize)
			shouldRebuildSplitOutputs = true;

	if (shouldRebuildSplitOutputs)
	{
		gpu.Defer([o = std::move(m_pImpl->outputs[0])] {});
		gpu.Defer([o = std::move(m_pImpl->outputs[1])] {});

		glm::uvec2 size0 = targetSize;
		glm::uvec2 size1;
		if (m_pImpl->splitDirection == EScreenSplitDirection::Vertical)
			size1 = glm::uvec2(output.Size().x - targetSize.x, output.Size().y);
		else
			size1 = glm::uvec2(output.Size().x, output.Size().y - targetSize.y);

		m_pImpl->outputs[0] = std::make_unique<ga::gpu::Image>(gpu, ga::gpu::ImageInfo {
			.type         = ga::gpu::EImageType::Image2D,
			.format       = ga::gpu::EFormat::R16G16B16A16_SFloat,
			.width        = size0.x,
			.height       = size0.y,
			.usage        = ga::gpu::EImageUsage::ColorAttachment | ga::gpu::EImageUsage::Storage | ga::gpu::EImageUsage::Sampled,
			.samplingMode = ga::gpu::EImageSamplingMode::ClampToBorder,
			.debugName    = "SplitScreenRenderer: split 0"
		});

		m_pImpl->outputs[1] = std::make_unique<ga::gpu::Image>(gpu, ga::gpu::ImageInfo {
			.type         = ga::gpu::EImageType::Image2D,
			.format       = ga::gpu::EFormat::R16G16B16A16_SFloat,
			.width        = size1.x,
			.height       = size1.y,
			.usage        = ga::gpu::EImageUsage::ColorAttachment | ga::gpu::EImageUsage::Storage | ga::gpu::EImageUsage::Sampled,
			.samplingMode = ga::gpu::EImageSamplingMode::ClampToBorder,
			.debugName    = "SplitScreenRenderer: split 1"
		});
	}

	std::vector<ga::gpu::SemaphoreId> waitOps;

	auto splitRenderCb = gpu.AcquireCommandBuffer();
	splitRenderCb->Record([&](const ga::gpu::CommandEncoder& e) {
		e.Transition(*m_pImpl->outputs[0], ga::gpu::EImageLayout::ColorAttachmentOptimal);
		auto sem0 = m_pImpl->renderers[0]->Render(gpu, e, *m_pImpl->outputs[0]);
		for (auto& s : sem0) waitOps.push_back(s);

		e.Transition(*m_pImpl->outputs[1], ga::gpu::EImageLayout::ColorAttachmentOptimal);
		auto sem1 = m_pImpl->renderers[1]->Render(gpu, e, *m_pImpl->outputs[1]);
		for (auto& s : sem1) waitOps.push_back(s);
	}, ga::gpu::ECommandBufferUsage::OneTimeSubmit);
	auto splitRenderDone = *gpu.Submit(splitRenderCb, waitOps);
	gpu.ReleaseCommandBuffer(splitRenderCb);

	encoder.Transition(*m_pImpl->outputs[0], ga::gpu::EImageLayout::ShaderReadOnlyOptimal);
	encoder.Transition(*m_pImpl->outputs[1], ga::gpu::EImageLayout::ShaderReadOnlyOptimal);
	encoder.Transition(output, ga::gpu::EImageLayout::General);

	encoder.Compute({ *m_pImpl->mergePipeline }, [&](const ga::gpu::ComputeEncoder& compute) {
		glm::uvec2 workgroups = glm::uvec2(output.Size()) / glm::uvec2(8) + glm::uvec2(1);

#pragma pack(push, 1)
		struct PushConstants
		{
			glm::vec4 lineProperties;
			uint32_t  split0;
			uint32_t  split1;
			uint32_t  output;
			uint32_t  splitDirection;
		};
#pragma pack(pop)

		const auto pc = PushConstants {
			.lineProperties = glm::vec4(m_splitLineColor, m_splitLineWidth),
			.split0         = m_pImpl->outputs[0]->GetDescriptorIndex().slot,
			.split1         = m_pImpl->outputs[1]->GetDescriptorIndex().slot,
			.output         = output.GetDescriptorIndex().slot,
			.splitDirection = uint32_t(m_pImpl->splitDirection)
		};
		compute.PushConstants(pc);
		compute.Dispatch({ workgroups, 1u });
	});

	Reset();
	return { splitRenderDone };
}

void SplitScreenRenderer::Reset()
{
	m_pImpl->renderers[0]->Reset();
	m_pImpl->renderers[1]->Reset();
}

