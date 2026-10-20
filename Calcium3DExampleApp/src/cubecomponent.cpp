#include "cubecomponent.h"

CubeComponent::CubeComponent(c3d::GameObject* owner) :
	c3d::Component(owner)
{

}

CubeComponent::CubeComponent(ga::core::CttiDeserializer& deserializer) :
	c3d::Component(deserializer)
{

}

void CubeComponent::CreateFace(std::vector<glm::vec3> fPositions, std::vector<uint32_t> fIndices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
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
					fPositions[X - 1].x - fPositions[X].x,
					fPositions[X - 1].y - fPositions[X].y,
					fPositions[X - 1].z - fPositions[X].z,
				};
				v =
				{
					fPositions[X - 1].x - fPositions[X - 2].x,
					fPositions[X - 1].y - fPositions[X - 2].y,
					fPositions[X - 1].z - fPositions[X - 2].z,
				};
			}
		}

		glm::vec3 n = glm::cross(u, v);
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

void CubeComponent::CreateRectangle(float kWIDTH, float kHEIGHT, float kDEPTH, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices)
{

	//Première face
	std::vector<glm::vec3> posF1 =
	{
		{ -kWIDTH/2, -kHEIGHT/2, kDEPTH/2  },
		{ kWIDTH/2, -kHEIGHT/2, kDEPTH/2  },
		{ kWIDTH/2, kHEIGHT/2, kDEPTH/2  },
		{ -kWIDTH/2, kHEIGHT/2, kDEPTH/2  },
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
		{ kWIDTH / 2, -kHEIGHT / 2, kDEPTH / 2  },
		{ kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
		{ kWIDTH / 2, kHEIGHT / 2, kDEPTH / 2  },
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
		{ kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ -kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ -kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
		{ kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
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
		{ -kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ -kWIDTH / 2, -kHEIGHT / 2, kDEPTH / 2  },
		{ -kWIDTH / 2, kHEIGHT / 2, kDEPTH / 2  },
		{ -kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
	};

	std::vector<uint32_t> indF4 =
	{
		12,13,14,
		12,14,15
	};

	CreateFace(posF4, indF4, positions, normals, indices);

	// Cinquième face
	std::vector<glm::vec3> posF5 =
	{
		{ -kWIDTH / 2, kHEIGHT / 2, kDEPTH / 2  },
		{ kWIDTH / 2, kHEIGHT / 2, kDEPTH / 2  },
		{ kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
		{ -kWIDTH / 2, kHEIGHT / 2, -kDEPTH / 2  },
	};

	std::vector<uint32_t> indF5 =

	{
		16,17,18,
		16,18,19
	};

	CreateFace(posF5, indF5, positions, normals, indices);

	// Sixième face
	std::vector<glm::vec3> posF6 =
	{
		{ -kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ kWIDTH / 2, -kHEIGHT / 2, -kDEPTH / 2  },
		{ kWIDTH / 2, -kHEIGHT / 2, kDEPTH / 2  },
		{ -kWIDTH / 2, -kHEIGHT / 2, kDEPTH / 2  },
	};

	std::vector<uint32_t> indF6 =

	{
		20,21,22,
		20,22,23
	};

	CreateFace(posF6, indF6, positions, normals, indices);
}

void CubeComponent::CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT, float kDEPTH)
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

ga::render::Mesh& CubeComponent::GetMyMesh()
{
	return myMesh;
}
