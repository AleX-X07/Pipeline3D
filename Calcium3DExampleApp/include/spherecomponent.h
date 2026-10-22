#ifndef SPHERECOMPONENT_H
#define SPHERECOMPONENT_H
#pragma once

#include <c3d/game.h>
#include <c3d/gamescene.h>
#include <c3d/component.h>

#include <gallium/platform/input.h>
#include <gallium/platform/platform.h>
#include <c3d/gameobject.h>

class SphereComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(SphereComponent, c3d::Component);

	float speed = 1.0f;

	ga::render::Mesh myMesh;
	bool isMoving = false;

	c3d::GameObject* raquette;

	float radius;
	glm::vec3 direction = { 1.0f, 0.0f, 0.0f };  

public:
	explicit SphereComponent(c3d::GameObject* owner, float _speed);
	explicit SphereComponent(ga::core::CttiDeserializer& deserializer);

	// Mesh
	void CreateSphere(float h, float r, uint32_t numSegments, uint32_t numSlices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateProceduralMesh(ga::gpu::Device& gpu, float h, float r, uint32_t numSegments, uint32_t numSlices, c3d::GameObject* raquetteGet);
	
	// Behaviour
	bool OnCollisionCube(glm::vec3 sphereInRaquette, glm::vec3 sizeRaquette, float r);
	bool OnCollisionCylinder(glm::vec3 posSphere, glm::vec3 posCylinder, float radiusSphere, float radiusCylinder);

	void HandleRaquetteCollision(bool isColliding);
	void HandleCylinderCollision(bool isColliding, c3d::GameObject* tower);
	
	virtual void Update(float t, float dt, c3d::GameSubsystems& subsystems) override;

	ga::render::Mesh& GetMyMesh();
	float& GetRadius();
};

#endif