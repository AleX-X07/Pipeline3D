#include "raquettecomponent.h"

#include <gallium/platform/platform.h>
#include <gallium/platform/input.h>

#include <c3d/game.h>
#include <c3d/gameobject.h>

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
	float finalSpeed;

	bool left = subsystems.platform.Input().IsHeld(ga::platform::EKey::A);
	bool right = subsystems.platform.Input().IsHeld(ga::platform::EKey::D);

	if (left) 
	{
		finalSpeed = -speed;
	}
	else if (right)
	{
		finalSpeed = speed;
	}
	else
	{
		finalSpeed = 0.0f;
	}

	glm::vec3 offset =
	{
		center.x + glm::cos(dt * speed * glm::pi<float>()) * r,
		0.0f,
		center.z + glm::sin(dt * speed * glm::pi<float>()) * r
	};

	GetGameObject()->Transform->Position += offset;
}

void RaquetteComponent::SetVar(float _r, glm::vec3 _center, float _speed)
{
	r = _r;
	center = _center;
	speed = _speed;
}

void RaquetteComponent::CreateFace(std::vector<glm::vec3> fPositions, std::vector<uint32_t> fIndices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
{
	float test = fPositions.size();

	if (fPositions.size() <= 4.0f) 
	{
		glm::vec3 u;
		glm::vec3 v;

		for (int X = 0; X < 4.0f; X++) 
		{
			positions.push_back(fPositions[X]);
			if (X == 3) 
			{
				u =
				{
					fPositions[X-1].x - fPositions[X].x,
					fPositions[X-1].y - fPositions[X].y,
					fPositions[X-1].z - fPositions[X].z,
				};
				v =
				{
					fPositions[X - 1].x - fPositions[X - 2].x,
					fPositions[X - 1].y - fPositions[X - 2].y,
					fPositions[X - 1].z - fPositions[X - 2].z,
				};
			}
		}

		glm::vec3 n = glm::cross(u,v);
		glm::vec3 nNorm = glm::normalize(n);

		for (int X = 0; X < 4; X++) {
			normals.push_back(nNorm);
		}
	}

	if (fIndices.size() <= 6.0f) 
	{
		for (int X = 0; X < 6.0f; X++) 
		{
			indices.push_back(fIndices[X]);
		}
	}
}

void RaquetteComponent::CreateRectangle(float kWIDTH, float kHEIGHT, float kDEPTH, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
{

	//Première face
	std::vector<glm::vec3> posF1 = 
	{
		{ 0, 0, 0 },
		{ 0 + kWIDTH, 0, 0 },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 },
		{0 , 0 + kHEIGHT, 0 }
	};

	std::vector<uint32_t> indF1 =
	{
		0,1,2,
		0,2,3
	};

	CreateFace(posF1, indF1, positions, normals, indices);

	//Deuxième face
	std::vector<glm::vec3> posF2 =
	{
		{ 0 + kWIDTH, 0, 0 },
		{ 0 + kWIDTH, 0, 0 + kDEPTH },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 + kDEPTH },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 }
	};

	std::vector<uint32_t> indF2 =
	{
		4,5,6,
		4,6,7
	};

	CreateFace(posF2, indF2, positions, normals, indices);

	//Troisème face
	std::vector<glm::vec3> posF3 =
	{
		{ 0, 0, 0 + kDEPTH },
		{ 0 + kWIDTH, 0, 0 + kDEPTH },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 + kDEPTH},
		{0 , 0 + kHEIGHT, 0 + kDEPTH}
	};

	std::vector<uint32_t> indF3 =
	{
		8,9,10,
		8,10,11
	};

	CreateFace(posF3, indF3, positions, normals, indices);

	//Quatrième face
	std::vector<glm::vec3> posF4 =
	{
		{ 0 , 0, 0 },
		{ 0 , 0, 0 + kDEPTH },
		{ 0 , 0 + kHEIGHT, 0 + kDEPTH },
		{ 0 , 0 + kHEIGHT, 0 }
	};

	std::vector<uint32_t> indF4 =
	{
		12,13,14,
		12,14,15
	};

	CreateFace(posF4, indF4, positions, normals, indices);

	// Cinquième et dernière face, pas besoin de faire celle de dessous.
	std::vector<glm::vec3> posF5 =
	{
		{ 0 , 0 + kHEIGHT, 0 },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 },
		{ 0 + kWIDTH, 0 + kHEIGHT, 0 + kDEPTH },
		{ 0 , 0 + kHEIGHT, 0 + kDEPTH }
	};

	std::vector<uint32_t> indF5 =
	{
		16,17,18,
		16,18,19
	};

	CreateFace(posF5, indF5, positions, normals, indices);

}

void RaquetteComponent::CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT, float kDEPTH)
{
	std::vector<glm::vec3> positions, normals;
	std::vector<uint32_t> indices;
	CreateRectangle(kWIDTH, kHEIGHT, kDEPTH, positions, normals, indices);

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