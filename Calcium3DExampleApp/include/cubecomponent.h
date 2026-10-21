#ifndef CUBECOMPONENT_H
#define CUBECOMPONENT_H
#pragma once

#include <gallium/render/mesh.h>
#include <c3d/component.h>

class CubeComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(CubeComponent, c3d::Component);

	ga::render::Mesh myMesh;

	glm::vec3 center = { 0,0,0 };
	float speed = 0.1f;
	glm::vec3 size = { 0,0,0 };

public:

	explicit CubeComponent(c3d::GameObject* owner);
	explicit CubeComponent(ga::core::CttiDeserializer& deserializer);

	void CreateFace(std::vector<glm::vec3> fPositions, std::vector<uint32_t> fIndices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateRectangle(float kWIDTH, float kHEIGHT, float kDEPTH, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT, float kDEPTH);

	ga::render::Mesh& GetMyMesh();
	glm::vec3& GetSize();
};
#endif