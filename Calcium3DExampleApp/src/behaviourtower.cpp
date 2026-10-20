#include "behaviourtower.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>
#include <c3d/components/rendering/mesh.h>

#include <cubecomponent.h>

BehaviourTower::BehaviourTower(c3d::GameObject* owner) :
	Component(owner)
{

}

BehaviourTower::BehaviourTower(ga::core::CttiDeserializer& deserializer) :
	Component(deserializer)
{

}

void BehaviourTower::Update(float t, float dt, c3d::GameSubsystems& subsystem)
{
	if(listLine.size() > 1) 
	{
		bool needToMoveLine = false;
		int X = 0;
		int toErase = 0;
		for (auto& vec : listLine)
		{
			X++;
			if (vec.empty())
			{
				toErase = X;
				needToMoveLine = true;
			}
		}
		if (needToMoveLine)
		{
			listLine.erase(listLine.begin() + toErase);
			for (auto& vec : listLine)
			{
				for (auto& cube : vec)
				{
					auto pos = cube->Transform->Position;
					pos->y -= hCube;
				}
			}
			needToMoveLine = false;
		}
	}
}

std::vector<c3d::GameObject*> BehaviourTower::CreateLineOfCubeAroundTower(ga::gpu::Device& gpu, c3d::GameScene& m_gamescene, glm::vec3 center, unsigned int nbrCube,  float r,  float hCube, float offsetY)
{
	std::vector<c3d::GameObject*> listCubeByLine;

	constexpr float kBrickW = 1.0f;
	float kBrickH = hCube;
	constexpr float kBrickD = 0.5f;

	float c      = 2.0f * r * glm::tan(glm::pi<float>() / float(nbrCube));
	float scale  = c / kBrickW;
	float rPrime = r + (kBrickD / 2.0f);

	for (unsigned int X = 0; X < nbrCube; X++)
	{
		float angle = (2.0f * glm::pi<float>()) * (float(X) / float(nbrCube));

		float dx = rPrime * glm::cos(angle);
		float dz = rPrime * glm::sin(angle);

		glm::vec3 cubePos = center + glm::vec3(dx, offsetY, dz);

		// Creating a cube
		auto cube = m_gamescene.CreateObject("Cube" + std::to_string(X));
		{
			auto mesh = cube->AddComponent<c3d::components::rendering::Mesh>();
			auto createMesh = cube->AddComponent<CubeComponent>();

			createMesh->CreateProceduralMesh(gpu, kBrickW*scale, kBrickH, kBrickD);
			mesh->Asset = createMesh->GetMyMesh();
		}
		cube->Transform->Rotate({0,1,0}, -angle + 1.57f);
		cube->Transform->Position = cubePos;
		listCubeByLine.push_back(cube);
	};
	return listCubeByLine;
}

void BehaviourTower::CreateListOfLineAroundTower(float hCylinder, ga::gpu::Device& gpu, c3d::GameScene& m_gamescene, glm::vec3 center, unsigned int nbrCubeStart, float r, float _hCube)
{
	
	hCube = _hCube;
	float nbrLine = hCylinder / hCube;

	for (int X = 0; X < nbrLine; X++)
	{
		float hFactor = float(X) / float(nbrLine);

		float yMin = -hCylinder / 2 + hCube/2;
		float y = yMin + hFactor * hCylinder;

		listLine.push_back(CreateLineOfCubeAroundTower(gpu, m_gamescene, center, nbrCubeStart, r, hCube, y));
		nbrCubeStart += 2;
	}
}

std::vector<std::vector<c3d::GameObject*>>& BehaviourTower::GetListCube()
{
	return listLine;
}
