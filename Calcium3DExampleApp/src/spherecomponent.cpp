#include <spherecomponent.h>


SphereComponent::SphereComponent(c3d::GameObject* owner) : 
	c3d::Component(owner) 
{

}

SphereComponent::SphereComponent(ga::core::CttiDeserializer& deserializer) : 
	c3d::Component(deserializer)
{

}

void SphereComponent::CreateSphere(float h, float r, uint32_t numSegments, uint32_t numSlices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
{
	size_t numVertices = numSegments * (numSlices + 1);
	size_t numIndicesPerSlice = 6 * numSegments;
	size_t numIndices = numIndicesPerSlice * numSlices;

	positions.resize(numVertices);
	normals.resize(numVertices);
	indices.resize(numIndices);

	for (uint32_t circle = 0; circle <= numSlices; ++circle)
	{
		float hFactor = float(circle) / float(numSlices);

		float yMin = -h / 2;
		//float y = yMin + hFactor * h;

		for (uint32_t i = 0; i < numSegments; ++i)
		{
			uint32_t vertexId = circle * numSegments + i;

			float theta = (float(i) / float(numSegments)) * 2.0f * glm::pi<float>();
			float phi = -glm::pi<float>() * 0.5f + glm::pi<float>() * hFactor;

			float x = r * glm::cos(phi) * glm::cos(theta);
			float z = r * glm::cos(phi) * glm::sin(theta);
			float y = r * glm::sin(phi);

			positions[vertexId] = glm::vec3(x, y, z);
			normals[vertexId] = glm::normalize(positions[vertexId]);
		}
	}

	for (uint32_t slice = 0; slice < numSlices; ++slice)
	{
		uint32_t c0baseVertex = slice * numSegments;
		uint32_t c1baseVertex = (slice + 1) * numSegments;

		for (uint32_t i = 0; i < numSegments; ++i)
		{
			uint32_t p0 = c0baseVertex + i;
			uint32_t p1 = c0baseVertex + (i + 1) % numSegments;
			uint32_t p2 = c1baseVertex + (i + 1) % numSegments;
			uint32_t p3 = c1baseVertex + i;

			indices[slice * numIndicesPerSlice + i * 6 + 0] = p0;
			indices[slice * numIndicesPerSlice + i * 6 + 1] = p1;
			indices[slice * numIndicesPerSlice + i * 6 + 2] = p2;
			indices[slice * numIndicesPerSlice + i * 6 + 3] = p0;
			indices[slice * numIndicesPerSlice + i * 6 + 4] = p2;
			indices[slice * numIndicesPerSlice + i * 6 + 5] = p3;
		}
	}
}

void SphereComponent::CreateProceduralMesh(ga::gpu::Device& gpu, float h, float r, uint32_t numSegments, uint32_t numSlices)
{
	constexpr float kWIDTH = 2.0f;
	constexpr float kHEIGHT = 2.0f;

	std::vector<glm::vec3> positions, normals;
	std::vector<uint32_t> indices;
	CreateSphere(h, r, numSegments, numSlices, positions, normals, indices);

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

void SphereComponent::Update(float t, float dt, c3d::GameSubsystems& subsystems)
{
	auto ball = GetGameObject();
	bool pressBar = subsystems.platform.Input().IsPressed(ga::platform::EKey::Space);

	if (pressBar) {
		isMoving = true;
	}
	
	if (isMoving)
	{
		float speed = 5.0f; 
		float pos = dt * speed;
		ball->Transform->Position = ball->Transform->Position() + glm::vec3(0.0f, 0.0f, 1.0f) * pos;
	}

}

ga::render::Mesh& SphereComponent::GetMyMesh()
{
	return myMesh;
}
