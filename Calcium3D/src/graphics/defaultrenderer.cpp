#include <c3d/graphics/defaultrenderer.h>

#include <gallium/gpu/buffer.h>
#include <gallium/gpu/commandbuffer.h>
#include <gallium/gpu/shadercache.h>
#include <gallium/render/rendergraph.h>
#include <gallium/render/mesh.h>
#include <gallium/assets/assetmanager.h>

using namespace c3d::graphics;

#pragma pack(push, 1)
struct FrameUniforms
{
	glm::mat4x4 viewProj;
	glm::mat4x4 viewProjInverse;
	glm::vec3   eye;
	glm::uint   frameIndex;
	uintptr_t   materials;
	uint32_t    environmentDiffuseId;
	uint32_t    environmentSpecularId;
	float       environmentIntensity;
};

struct Light
{
	glm::mat4              viewProj;
	ga::render::ELightType type;
	glm::vec3  position;
	float _0;
	glm::vec3  direction;
	float _1;
	glm::vec3  color;
	float _2;
	float      intensity;
	float      spotInnerAngle;
	float      spotOuterAngle;
	float      radius;
	uint32_t   shadowmapId;
};

struct MaterialValue
{
	glm::vec4  value;
	glm::uvec4 textureId;
};

struct Material
{
	MaterialValue baseColor;
	MaterialValue orm;
	MaterialValue normal;
	MaterialValue emissive;
};
#pragma pack(pop)

#define kMINIMUM_MATERIALS_CAPACITY (8)
#define kMAX_SHADOWMAPS             (4)
#define kSHADOWMAP_RESOLUTION       (512u)

struct DefaultRenderer::Internal
{
	struct CameraData
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	std::optional<CameraData> camera;
	uint32_t                  frameIndex = 0;

	std::unique_ptr<ga::gpu::Buffer> frameUniformsBuffer;

	std::vector<Light>               lights;
	std::unique_ptr<ga::gpu::Buffer> lightsBuffer;
	std::vector<uint32_t>            shadowmapLights;

	std::vector<Material>            materials;
	std::unique_ptr<ga::gpu::Buffer> materialsBuffer;
	uint32_t                         maxMaterials = kMINIMUM_MATERIALS_CAPACITY;

	std::vector<std::tuple<glm::mat4, const ga::render::MeshInstance*, uint32_t>> geometries;

	uint32_t environmentDiffuseId;
	uint32_t environmentSpecularId;
	float    environmentIntensity;

	ga::render::RenderGraph renderGraph;

	void RenderGeometries(const ga::gpu::RenderEncoder& encoder);
	void RenderShadowGeometries(const ga::gpu::RenderEncoder& encoder, const glm::mat4& lightViewProj);

	Internal(ga::gpu::Device& gpu)
		: renderGraph(gpu)
	{
		frameUniformsBuffer = std::make_unique<ga::gpu::Buffer>(gpu, ga::gpu::BufferInfo {
			.usage                   = ga::gpu::EBufferUsage::UniformBuffer | ga::gpu::EBufferUsage::ShaderDeviceAddress,
			.size                    = sizeof(FrameUniforms),
			.createPersistentStaging = true
		});
		frameUniformsBuffer->SetDebugName("DefaultRenderer: Frame uniforms");

		lightsBuffer = std::make_unique<ga::gpu::Buffer>(gpu, ga::gpu::BufferInfo {
			.usage                   = ga::gpu::EBufferUsage::UniformBuffer | ga::gpu::EBufferUsage::ShaderDeviceAddress,
			.size                    = 64 * sizeof(Light),
			.createPersistentStaging = true
		});
		lightsBuffer->SetDebugName("DefaultRenderer: Lights");
	}
};

static std::unique_ptr<ga::gpu::GraphicsPipeline> s_GBufferPipeline(ga::gpu::Device& gpu)
{
	auto& shader = gpu.GetShaderCache().Get("/shaders/c3d/gbuffer.slang.spv");

	return std::make_unique<ga::gpu::GraphicsPipeline>(gpu, ga::gpu::GraphicsPipelineInfo {
		.stages = {
			{ shader, ga::gpu::EShaderStage::Vertex,   "vs_main" },
			{ shader, ga::gpu::EShaderStage::Fragment, "fs_main" },
		},
		.vertexInput = {
			.bindings = {
				{ 0u, sizeof(glm::vec3),  ga::gpu::EVertexBindingRate::Vertex },
				{ 1u, sizeof(glm::vec3),  ga::gpu::EVertexBindingRate::Vertex },
				{ 2u, sizeof(glm::vec2),  ga::gpu::EVertexBindingRate::Vertex },
				{ 3u, sizeof(glm::vec3),  ga::gpu::EVertexBindingRate::Vertex },
				{ 4u, sizeof(glm::vec3),  ga::gpu::EVertexBindingRate::Vertex },
			},
			.attributes = {
				{ 0, 0, ga::gpu::EFormat::R32G32B32_SFloat, 0 },
				{ 1, 1, ga::gpu::EFormat::R32G32B32_SFloat, 0 },
				{ 2, 2, ga::gpu::EFormat::R32G32_SFloat,    0 },
				{ 3, 3, ga::gpu::EFormat::R32G32B32_SFloat, 0 },
				{ 4, 4, ga::gpu::EFormat::R32G32B32_SFloat, 0 },
			}
		},
		.attachmentFormats = {
			.colorFormats = {
				/* rt0 */ ga::gpu::EFormat::R8G8B8A8_UNorm,
				/* rt1 */ ga::gpu::EFormat::R16G16B16A16_SFloat,
				/* rt2 */ ga::gpu::EFormat::R16G16B16A16_SFloat
			},
			.depthFormat = ga::gpu::EFormat::D32_SFloat
		}
	});
}

static std::unique_ptr<ga::gpu::GraphicsPipeline> s_ShadowPipeline(ga::gpu::Device& gpu)
{
	auto& shader = gpu.GetShaderCache().Get("/shaders/c3d/shadow.slang.spv");
	return std::make_unique<ga::gpu::GraphicsPipeline>(gpu, ga::gpu::GraphicsPipelineInfo {
		.stages = {
			{ shader, ga::gpu::EShaderStage::Vertex, "vs_main" },
		},
		.vertexInput = {
			.bindings   = { { 0u, sizeof(glm::vec3), ga::gpu::EVertexBindingRate::Vertex } },
			.attributes = { { 0, 0, ga::gpu::EFormat::R32G32B32_SFloat, 0 } }
		},
		.attachmentFormats = {
			.depthFormat = ga::gpu::EFormat::D32_SFloat
		}
	});
}

static std::unique_ptr<ga::gpu::ComputePipeline> s_ComputePipeline(ga::gpu::Device& gpu, std::string_view path, std::string_view entrypoint = "cs_main")
{
	auto& shader = gpu.GetShaderCache().Get(path);

	return std::make_unique<ga::gpu::ComputePipeline>(gpu, ga::gpu::ComputePipelineInfo {
		.stage = { shader, ga::gpu::EShaderStage::Compute, entrypoint }
	});
}

void DefaultRenderer::Internal::RenderGeometries(const ga::gpu::RenderEncoder& encoder)
{
#pragma pack(push, 1)
	struct GBufferPushConstants
	{
		uintptr_t frameUniforms;
		uintptr_t submeshData;
		glm::mat4 model;
		uint32_t  material;
	};
#pragma pack(pop)

	for (auto& [transform, meshInstance, arrayIndex] : geometries)
	{
		std::vector<std::pair<ga::gpu::Buffer*, size_t>> buffers;
		for (size_t i = 0; i < 4; ++i)
		{
			if (meshInstance->mesh->processedVertexBuffers[i])
				buffers.push_back(std::make_pair(meshInstance->mesh->processedVertexBuffers[i].get(), 0));
			else
				buffers.push_back(std::make_pair(meshInstance->mesh->vertexBuffers[i].get(), 0));
		}

		if (meshInstance->mesh->previousFramePositions)
			buffers.push_back(std::make_pair(meshInstance->mesh->previousFramePositions.get(), 0));
		else if (meshInstance->mesh->processedVertexBuffers[0])
			buffers.push_back(std::make_pair(meshInstance->mesh->processedVertexBuffers[0].get(), 0));
		else
			buffers.push_back(std::make_pair(meshInstance->mesh->vertexBuffers[0].get(), 0));

		std::vector<std::tuple<uint32_t, uint32_t, uintptr_t, uint32_t>> submeshes;

		for (size_t i = 0; i < meshInstance->primitiveEnablement.size(); ++i)
		{
			if (!meshInstance->primitiveEnablement[i])
				continue;

			auto& prim = meshInstance->mesh->primitives[i];

			uint32_t materialIndex = uint32_t(-1);

			if (prim.materialIndex != uint32_t(-1))
			{
				auto& material = ga::assets::AssetManager::Global().MaterialLibrary().ValueOf(prim.materialIndex);
				materialIndex = uint32_t(materials.size());

				materials.push_back(Material {
					.baseColor = MaterialValue {
						.value = glm::vec4(material.baseColor.value, 1.0f),
						.textureId = glm::uvec4(material.baseColor.textureId, 0, 0, 0)
					},
					.orm = MaterialValue {
						.value = glm::vec4(material.orm.value, 1.0f),
						.textureId = glm::uvec4(material.orm.textureId, 0, 0, 0)
					},
					.normal = MaterialValue {
						.value = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
						.textureId = glm::uvec4(material.normal.textureId, 0, 0, 0)
					},
					.emissive = MaterialValue {
						.value = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
						.textureId = glm::uvec4(material.emissive.textureId, 0, 0, 0)
					}
				});
			}
			submeshes.push_back({ prim.startIndex, prim.indexCount, prim.localTransformBuffer->GetGpuAddress(), materialIndex });
		}

		encoder.BindVertexBuffers(0, buffers);
		encoder.BindIndexBuffer(meshInstance->mesh->indexBuffer.get(), 0);

		auto pc = GBufferPushConstants {
			.frameUniforms = frameUniformsBuffer->GetGpuAddress(),
			.model         = transform
		};

		for (const auto& [firstIndex, indexCount, localMatrices, materialIndex] : submeshes)
		{
			pc.submeshData = localMatrices;
			pc.material    = materialIndex;
			encoder.PushConstants(pc);
			encoder.DrawIndexed(indexCount, 1, firstIndex, 0, 0);
		}
	}
}

static glm::mat4 s_ComputeSpotLightViewProj(const Light& light)
{
	constexpr float kNear = 5e-2f;

	float outerHalfAngle = glm::acos(glm::clamp(light.spotOuterAngle, -1.0f, 1.0f));
	float fov = 2.0f * outerHalfAngle;

	glm::vec3 up = (glm::abs(-light.direction.y) > 0.999f)
		? glm::vec3(0.0f, 0.0f, 1.0f)
		: glm::vec3(0.0f, 1.0f, 0.0f);

	float far = glm::max(100.0f, kNear + 1e-2f);

	glm::mat4 view = glm::lookAt(light.position, light.position - light.direction, up);
	glm::mat4 proj = glm::perspective(fov, 1.0f, kNear, far);
	return proj * view;
}

void DefaultRenderer::Internal::RenderShadowGeometries(const ga::gpu::RenderEncoder& encoder, const glm::mat4& lightViewProj)
{
#pragma pack(push, 1)
	struct ShadowPushConstants
	{
		glm::mat4 lightViewProj;
		glm::mat4 model;
		uintptr_t submeshData;
	};
#pragma pack(pop)

	for (auto& [transform, meshInstance, arrayIndex] : geometries)
	{
		std::vector<std::pair<ga::gpu::Buffer*, size_t>> buffers;
		for (size_t i = 0; i < 4; ++i)
		{
			if (meshInstance->mesh->processedVertexBuffers[i])
				buffers.push_back(std::make_pair(meshInstance->mesh->processedVertexBuffers[i].get(), 0));
			else
				buffers.push_back(std::make_pair(meshInstance->mesh->vertexBuffers[i].get(), 0));
		}

		if (meshInstance->mesh->previousFramePositions)
			buffers.push_back(std::make_pair(meshInstance->mesh->previousFramePositions.get(), 0));
		else if (meshInstance->mesh->processedVertexBuffers[0])
			buffers.push_back(std::make_pair(meshInstance->mesh->processedVertexBuffers[0].get(), 0));
		else
			buffers.push_back(std::make_pair(meshInstance->mesh->vertexBuffers[0].get(), 0));

		std::vector<std::tuple<uint32_t, uint32_t, uintptr_t>> submeshes;	

		encoder.BindVertexBuffers(0, buffers);
		encoder.BindIndexBuffer(meshInstance->mesh->indexBuffer.get(), 0);

		auto pc = ShadowPushConstants {
			.lightViewProj = lightViewProj,
			.model = transform
		};

		for (size_t i = 0; i < meshInstance->primitiveEnablement.size(); ++i)
		{
			if (!meshInstance->primitiveEnablement[i])
				continue;

			auto& prim = meshInstance->mesh->primitives[i];

			pc.submeshData = prim.localTransformBuffer->GetGpuAddress();
			encoder.PushConstants(pc);
			encoder.DrawIndexed(prim.indexCount, 1, prim.startIndex, 0, 0);
		}
	}
}

DefaultRenderer::DefaultRenderer(ga::gpu::Device& gpu)
	: m_pImpl(new Internal(gpu))
{
	m_pImpl->renderGraph.AddPass("GBuffer", s_GBufferPipeline(gpu))
		.WritesTo("rt0"_h, ga::gpu::EImageUsage::ColorAttachment, ga::render::RGImageInfo { .format = ga::gpu::EFormat::R8G8B8A8_UNorm })
			.LoadOp(ga::gpu::ELoadOp::Clear, { 0.0f, 0.0f, 0.0f, 0.0f })
		.WritesTo("rt1"_h, ga::gpu::EImageUsage::ColorAttachment, ga::render::RGImageInfo { .format = ga::gpu::EFormat::R16G16B16A16_SFloat })
			.LoadOp(ga::gpu::ELoadOp::Clear, { 0.0f, 0.0f, 0.0f, 0.0f })
		.WritesTo("rt2"_h, ga::gpu::EImageUsage::ColorAttachment, ga::render::RGImageInfo { .format = ga::gpu::EFormat::R16G16B16A16_SFloat })
			.LoadOp(ga::gpu::ELoadOp::Clear, { 0.0f, 0.0f, 0.0f, 0.0f })
		.WritesTo("zbuffer"_h, ga::gpu::EImageUsage::DepthStencil, ga::render::RGImageInfo { .format = ga::gpu::EFormat::D32_SFloat })
			.LoadOp(ga::gpu::ELoadOp::Clear, 1.0f)
		.Render([&](const ga::gpu::RenderEncoder& encoder) { m_pImpl->RenderGeometries(encoder); });

	for (size_t i = 0; i < kMAX_SHADOWMAPS; ++i)
	{
		string_hash_t shadowmapId = ga::hash_string(std::format("shadowmap{}", i));

		m_pImpl->renderGraph.AddPass(std::format("Shadowmap {}", i), s_ShadowPipeline(gpu))
			.If([&, i] { return i < m_pImpl->shadowmapLights.size(); })
			.WritesTo(shadowmapId, ga::gpu::EImageUsage::DepthStencil, ga::render::RGImageInfo{ .format = ga::gpu::EFormat::D32_SFloat, .size = { kSHADOWMAP_RESOLUTION, kSHADOWMAP_RESOLUTION } })
				.LoadOp(ga::gpu::ELoadOp::Clear, 1.0f)
			.Render([&, shadowmapId, i](const ga::gpu::RenderEncoder& encoder) {
				uint32_t lightId = m_pImpl->shadowmapLights[i];
				glm::mat4 lightViewProj = s_ComputeSpotLightViewProj(m_pImpl->lights[lightId]);
				m_pImpl->RenderShadowGeometries(encoder, lightViewProj); 

				m_pImpl->lights[lightId].viewProj = lightViewProj;
				m_pImpl->lights[lightId].shadowmapId = m_pImpl->renderGraph.IndexOf(shadowmapId);
			});
	}

	m_pImpl->renderGraph.AddPass("Light accumulation", s_ComputePipeline(gpu, "/shaders/c3d/light_accumulation.slang.spv"))
		.ReadsFrom("rt0"_h, ga::gpu::EImageUsage::Sampled)
		.ReadsFrom("rt1"_h, ga::gpu::EImageUsage::Sampled)
		.ReadsFrom("rt2"_h, ga::gpu::EImageUsage::Sampled)
		.ReadsFrom("zbuffer"_h, ga::gpu::EImageUsage::Sampled)
		.WritesTo("output"_h, ga::gpu::EImageUsage::ColorAttachment, ga::render::RGImageInfo { .external = true })
		.Compute([&](const ga::gpu::ComputeEncoder& encoder) {
			glm::uvec2 outputSize = m_pImpl->renderGraph.OutputSize();
			glm::uvec2 workgroups = outputSize / glm::uvec2(8) + glm::uvec2(1);

#pragma pack(push, 1)
			struct PushConstants
			{
				uintptr_t frameUniforms;
				uintptr_t lights;
				uint32_t  numLights;
				uint32_t  rt0;
				uint32_t  rt1;
				uint32_t  rt2;
				uint32_t  zbuffer;
				uint32_t  output;
			};
#pragma pack(pop)

			const auto pc = PushConstants {
				.frameUniforms = m_pImpl->frameUniformsBuffer->GetGpuAddress(),
				.lights        = m_pImpl->lightsBuffer->GetGpuAddress(),
				.numLights	   = uint32_t(m_pImpl->lights.size()),
				.rt0		   = m_pImpl->renderGraph.IndexOf("rt0"_h),
				.rt1		   = m_pImpl->renderGraph.IndexOf("rt1"_h),
				.rt2		   = m_pImpl->renderGraph.IndexOf("rt2"_h),
				.zbuffer	   = m_pImpl->renderGraph.IndexOf("zbuffer"_h),
				.output		   = m_pImpl->renderGraph.IndexOf("output"_h),
			};
			encoder.PushConstants(pc);
			encoder.Dispatch({ workgroups, 1u });
		});

	m_pImpl->renderGraph.Compile();
}

DefaultRenderer::~DefaultRenderer()
{
}

void DefaultRenderer::AddGeometry(const ga::render::MeshInstance& mesh, const glm::mat4& transform, uint32_t arrayIndex /* = 0 */)
{
	m_pImpl->geometries.push_back({ transform, &mesh, arrayIndex });
}

void DefaultRenderer::AddLight(const ga::render::LightData& light, const glm::mat4& transform)
{
	glm::vec3 position = glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	glm::vec3 direction = normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

	auto cosAngles = glm::cos(light.spotAngles);

	m_pImpl->lights.push_back(Light {
		.viewProj       = glm::identity<glm::mat4>(),
		.type           = light.type,
		.position       = position,
		.direction      = direction,
		.color          = light.color,
		.intensity      = light.intensity,
		.spotInnerAngle = glm::max(cosAngles.x, cosAngles.y),
		.spotOuterAngle = glm::min(cosAngles.x, cosAngles.y),
		.radius = light.type == ga::render::ELightType::Directional
			? glm::sin(glm::radians(light.radius))
			: light.radius,
		.shadowmapId    = uint32_t(-1)
	});

	if (light.type == ga::render::ELightType::Spot)
		m_pImpl->shadowmapLights.push_back(uint32_t(m_pImpl->lights.size()) - 1);
}

void DefaultRenderer::SetEnvironment(uint32_t diffuse, uint32_t specular, float intensity)
{
	m_pImpl->environmentDiffuseId  = diffuse;
	m_pImpl->environmentSpecularId = specular;
	m_pImpl->environmentIntensity  = intensity;
}

void DefaultRenderer::SetCamera(const glm::mat4& view, const glm::mat4& projection)
{
	m_pImpl->camera = {
		.view       = view,
		.projection = projection
	};
}

bool DefaultRenderer::HasCamera() const
{
	return m_pImpl->camera.has_value();
}

std::vector<ga::gpu::SemaphoreId> DefaultRenderer::Render(ga::gpu::Device& gpu, const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& output)
{
	std::vector<ga::gpu::SemaphoreId> waitOps;

	if (HasCamera())
	{
		m_pImpl->renderGraph.Execute(encoder, { { "output"_h, &output, ga::render::ERGBindingFlags::MatchSize } });

		auto      viewProj           = m_pImpl->camera->projection * m_pImpl->camera->view;
		glm::mat3 rotationTransposed = glm::transpose(glm::mat3(m_pImpl->camera->view));
		glm::vec3 viewTranslation    = m_pImpl->camera->view[3];
		glm::vec3 eyePosition        = -(rotationTransposed * glm::vec3(m_pImpl->camera->view[3]));

		bool recreateMaterialsBuffer = false;
		if (!m_pImpl->materialsBuffer)
			recreateMaterialsBuffer = true;
		else if (m_pImpl->maxMaterials < m_pImpl->materials.size())
				recreateMaterialsBuffer = true;
		
		if (recreateMaterialsBuffer)
		{
			gpu.Defer([mats = std::move(m_pImpl->materialsBuffer)] {});

			const uint32_t size     = uint32_t(m_pImpl->materials.size());
			const uint32_t capacity = (size / kMINIMUM_MATERIALS_CAPACITY + 1) * kMINIMUM_MATERIALS_CAPACITY;
			m_pImpl->maxMaterials = std::max(m_pImpl->maxMaterials, capacity);

			m_pImpl->materialsBuffer = std::make_unique<ga::gpu::Buffer>(gpu, ga::gpu::BufferInfo {
				.usage                   = ga::gpu::EBufferUsage::StorageBuffer | ga::gpu::EBufferUsage::ShaderDeviceAddress,
				.size                    = m_pImpl->maxMaterials * sizeof(Material),
				.createPersistentStaging = true
			});
		}

		auto frameUniforms = FrameUniforms {
			.viewProj              = viewProj,
			.viewProjInverse       = glm::inverse(viewProj),
			.eye                   = eyePosition,
			.frameIndex            = ++m_pImpl->frameIndex,
			.materials             = m_pImpl->materialsBuffer->GetGpuAddress(),
			.environmentDiffuseId  = m_pImpl->environmentDiffuseId,
			.environmentSpecularId = m_pImpl->environmentSpecularId,
			.environmentIntensity  = m_pImpl->environmentIntensity
		};

		auto transferCb = gpu.AcquireCommandBuffer();
		transferCb->Record([&](const ga::gpu::CommandEncoder& e) {
			e.Transfer([&](const ga::gpu::TransferEncoder& transfer) {
				m_pImpl->frameUniformsBuffer->UploadInline(transfer, &frameUniforms, sizeof(FrameUniforms));

				if (!m_pImpl->lights.empty())
					m_pImpl->lightsBuffer->UploadInline(transfer, m_pImpl->lights.data(), sizeof(Light) * m_pImpl->lights.size());

				if (!m_pImpl->materials.empty())
					m_pImpl->materialsBuffer->UploadInline(transfer, m_pImpl->materials.data(), sizeof(Material) * m_pImpl->materials.size());
			});
		}, ga::gpu::ECommandBufferUsage::OneTimeSubmit);
		waitOps.push_back(*gpu.Submit(transferCb));
		gpu.ReleaseCommandBuffer(transferCb);
	}

	Reset();
	return waitOps;
}

void DefaultRenderer::Reset()
{
	m_pImpl->camera.reset();
	m_pImpl->lights.clear();
	m_pImpl->geometries.clear();
	m_pImpl->materials.clear();
	m_pImpl->environmentDiffuseId = uint32_t(-1);
	m_pImpl->environmentSpecularId = uint32_t(-1);
}
