#include "behaviourtower.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>
#include <c3d/components/rendering/mesh.h>

#include <cubecomponent.h>
#include <spherecomponent.h>
#include <iostream>

BehaviourTower::BehaviourTower(c3d::GameObject* owner) :
	Component(owner)
{

}

BehaviourTower::BehaviourTower(ga::core::CttiDeserializer& deserializer) :
	Component(deserializer)
{

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

void BehaviourTower::MoveDownLine()
{
	for (auto& vec : listLine)
	{
		for (auto& obj : vec)
		{
			obj->Transform->Position->y -= hCube;
		}
	}
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

void BehaviourTower::DestroyBloc(c3d::GameScene& m_gameScene, c3d::GameObject* sphere)
{
	auto tower = this->GetGameObject();
	auto behaviour = tower->GetComponent<BehaviourTower>();

	auto sphereComp = sphere->GetComponent<SphereComponent>();

	std::vector<c3d::GameObject*>& vec = behaviour->listLine[currentLine];
	auto toErase = vec.begin();
	bool needToErase = false;

	for (auto it = vec.begin(); it != vec.end(); it++)
	{
		auto b = vec[it - vec.begin()];
		auto SphereInB = b->Transform->DetransformPoint(sphere->Transform->Position());
		auto bComp = b->GetComponent<CubeComponent>();

		if (sphereComp->OnCollisionCube(SphereInB, bComp->GetSize(), sphereComp->GetRadius()) && collide == false)
		{
			sphereComp->GetScore() += 10;
			collide = true;
			m_gameScene.RemoveObject(b);
			b->RemoveComponent<c3d::components::rendering::Mesh>();
			toErase = it;
			needToErase = true;
		}
		else 
		{
			collide = false;
		}
	}
	if (needToErase)
	{
		vec.erase(toErase);
		needToErase = false;
	}

	auto raquette = m_gameScene.GetObject("Raquette");

	auto SphereInRaquette = raquette->Transform->DetransformPoint(sphere->Transform->Position());

	auto cubeComp = raquette->GetComponent<CubeComponent>();

 	if (vec.empty() && sphereComp->OnCollisionCube(SphereInRaquette, cubeComp->GetSize(), sphereComp->GetRadius()))
	{
		currentLine++;
		MoveDownLine();
	}
}

bool BehaviourTower::VecBlockEmpty()
{
	int size = listLine.size();
	int size2 = 0;
	for (auto vec : listLine)
	{
		if (vec.empty())
		{
			size2++;
		}
	}

	if (size2 == size)
	{
		return true;
	}
	return false;
}

