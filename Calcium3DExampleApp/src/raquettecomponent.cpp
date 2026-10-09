#include "raquettecomponent.h"

RaquetteComponent::RaquetteComponent(c3d::GameObject* owner) :
	c3d::Component(owner)
{

}

RaquetteComponent::RaquetteComponent(ga::core::CttiDeserializer& deserializer) :
	c3d::Component(deserializer)
{

}

void RaquetteComponent::Update(float f, float dt, c3d::GameSubsystems& subsystems)
{
	
}

void RaquetteComponent::CreateRectangle(float kWIDTH, float kHEIGHT, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
{
	// Première face
	positions.push_back({ 0 ,0, 0 });
	positions.push_back({ 0 + kWIDTH, 0, 0 });
	positions.push_back({0 + kWIDTH, 0 + kHEIGHT, 0 });
	positions.push_back({0 , 0 + kHEIGHT, 0 });

	normals.push_back({-1,0,0});
	normals.push_back({-1,0,0});
	normals.push_back({-1,0,0});
	normals.push_back({-1,0,0});

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	indices.push_back(0);
	indices.push_back(2);
	indices.push_back(3);

	// Deuxième face
	positions.push_back({ 0 + kWIDTH, 0, 0 });
	positions.push_back({ 0 + kWIDTH, 0, 0 + kWIDTH });
	positions.push_back({ 0 + kWIDTH, 0 + kHEIGHT, 0 + kWIDTH });
	positions.push_back({ 0 + kWIDTH, 0 + kHEIGHT, 0 });

}

void RaquetteComponent::CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT)
{
	std::vector<glm::vec3> positions, normals;
	std::vector<uint32_t> indices;
	CreateRectangle(kWIDTH, kHEIGHT, positions, normals, indices);

	myMesh.UploadStream<ga::render::EVertexStream::Position>(gpu, positions);
	myMesh.UploadStream<ga::render::EVertexStream::Normal>(gpu, normals);
	myMesh.UploadIndices(gpu, indices);

	std::vector<glm::mat4> localMatrices = {
		glm::identity<glm::mat4>(),
		glm::identity<glm::mat4>()
	};

	myMesh.primitives.push_back(ga::render::Mesh::Primitive{
		.name = "Procedural mesh",
		.materialIndex = uint32_t(-1),
		.startIndex = 0,
		.indexCount = uint32_t(indices.size()),
		.localTransformBuffer = std::make_unique<ga::gpu::Buffer>(gpu, ga::gpu::BufferInfo {
			.usage = ga::gpu::EBufferUsage::UniformBuffer,
			.size = 2 * sizeof(glm::mat4),
			.initialData = localMatrices.data()
		}),
		.localTransform = glm::identity<glm::mat4>()
	});
}

ga::render::Mesh& RaquetteComponent::GetMyMesh() 
{
	return myMesh;
}