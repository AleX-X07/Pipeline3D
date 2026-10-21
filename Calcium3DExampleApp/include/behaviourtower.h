#ifndef BEHAVIOURTOWER_H
#define BEHAVIOURTOWER_H
#pragma once

#include <gallium/gpu/device.h>
#include <gallium/gpu/commandbuffer.h>

#include <c3d/component.h>
#include <c3d/gamescene.h>

class BehaviourTower :
	public c3d::Component
{
	GA_CTTI_OBJECT(BehaviourTower, c3d::Component);

	std::vector<std::vector<c3d::GameObject*>> listLine;
	int currentLine = 0.0f;
	float hCube = 0.0f;
	bool collide = false;

public:
	explicit BehaviourTower(c3d::GameObject* owner);
	explicit BehaviourTower(ga::core::CttiDeserializer& deserializer);

private:
	std::vector<c3d::GameObject*> CreateLineOfCubeAroundTower(ga::gpu::Device& gpu, c3d::GameScene& m_gamescene, glm::vec3 centerunsigned, unsigned int nbrCube, float r,  float hCube, float offsetY);
	
	void MoveDownLine();

public:
	void CreateListOfLineAroundTower(float hCylinder, ga::gpu::Device& gpu, c3d::GameScene& m_gamescene, glm::vec3 center, unsigned int nbrCubeStart, float r, float hCube);
	
	void DestroyBloc(c3d::GameScene& m_gameScene, c3d::GameObject* sphere);
};

#endif