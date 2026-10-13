#ifndef CYLINDRECOMPONENT_H
#define CYLINDRECOMPONENT_H
#pragma once

#include <c3d/game.h>
#include <c3d/gamescene.h>
#include <c3d/component.h>

class CylinderComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(CylinderComponent, c3d::Component);

	ga::render::Mesh myMesh;

	float r = 0.0f;

public:
	explicit CylinderComponent(c3d::GameObject* owner);
	explicit CylinderComponent(ga::core::CttiDeserializer& deserializer);

	void CreateCylinder(float h, float r, uint32_t numSegments, uint32_t numSlices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateProceduralMesh(ga::gpu::Device& gpu, float h, float r, uint32_t numSegments, uint32_t numSlices);

	ga::render::Mesh& GetMyMesh();
	float& GetRadius();
};

#endif