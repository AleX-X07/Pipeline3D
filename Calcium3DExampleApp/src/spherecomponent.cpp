#include <spherecomponent.h>
#include <iostream>

#include <cylindercomponent.h>
#include <cubecomponent.h>


SphereComponent::SphereComponent(c3d::GameObject* owner, float _speed) :
	c3d::Component(owner), speed(_speed)
{
	auto ball = GetGameObject();
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

	radius = r;

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

void SphereComponent::CreateProceduralMesh(ga::gpu::Device& gpu, float h, float r, uint32_t numSegments, uint32_t numSlices, c3d::GameObject* raquetteGet)
{
	raquette = raquetteGet;
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

bool SphereComponent::OnCollisionCube(glm::vec3 sphereInRaquette, glm::vec3 sizeRaquette, float r)
{
	return sphereInRaquette.x < (sizeRaquette.x / 2 + r) &&
		sphereInRaquette.x > (-sizeRaquette.x / 2 - r) &&
		sphereInRaquette.z < (sizeRaquette.z / 2 + r) &&
		sphereInRaquette.z > (-sizeRaquette.z / 2 - r);
}

bool SphereComponent::OnCollisionCylinder(glm::vec3 posSphere, glm::vec3 posCylinder, float radiusSphere, float radiusCylinder)
{
	float rad = radiusSphere + radiusCylinder;
	auto res = glm::distance(
		glm::vec3 { posSphere.x, 0.0f, posSphere.z },
		glm::vec3 { posCylinder.x, 0.0f, posCylinder.z }
	);
	if (res < rad)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void SphereComponent::HandleRaquetteCollision(bool isColliding)
{
	if (isColliding)
	{
		auto raquetteCenter = raquette->Transform->Position();
		auto sphereCenter = GetGameObject()->Transform->Position();

		direction = glm::normalize(glm::vec3{
			sphereCenter.x - raquetteCenter.x,
			0.0f,
			sphereCenter.z - raquetteCenter.z
		});
	}
}

void SphereComponent::HandleCylinderCollision(bool isColliding, c3d::GameObject* tower)
{
	if (isColliding)
	{
		auto towerCenter = tower->Transform->Position();
		auto sphereCenter = GetGameObject()->Transform->Position();

		direction = glm::normalize(glm::vec3{
			sphereCenter.x - towerCenter.x,
			0.0f,
			sphereCenter.z - towerCenter.z
		});

		auto test = 0.0f;
	}
}

void SphereComponent::CheckWin(c3d::GameObject* area, c3d::GameObject* raquette)
{
	auto ownerCenter = GetGameObject()->Transform->Position();
	auto areaCenter = area->Transform->Position();
	auto areaComp = area->GetComponent<CylinderComponent>();
	auto raquetteComp = raquette->GetComponent<CubeComponent>();

	if (!OnCollisionCylinder(ownerCenter, areaCenter, this->GetRadius() + raquetteComp->GetSize().x, areaComp->GetRadius()))
	{
		isMoving = false;
		life--;
		if (life == 0)
		{
			loose = true;
		}
		GetGameObject()->Transform->Position = startPos;
	}
}

void SphereComponent::Update(float t, float dt, c3d::GameSubsystems& subsystems)
{
	auto ball = GetGameObject();
	bool pressBar = subsystems.platform.Input().IsPressed(ga::platform::EKey::Space);
	glm::vec3 vecRebond = { 0.0f,0.0f,0.0f };
	if (pressBar && !loose) {
		direction = glm::normalize(glm::vec3{
			raquette->Transform->Position->x - ball->Transform->Position->x,
			0.0f,
			raquette->Transform->Position->z - ball->Transform->Position->z,
		});
		isMoving = true;
	}

	if (isMoving)
	{
		ball->Transform->Position += direction * speed * dt;
	}
}

ga::render::Mesh& SphereComponent::GetMyMesh()
{
	return myMesh;
}


float& SphereComponent::GetRadius()
{
	return radius;
}

int& SphereComponent::GetLife()
{
	return life;
}
