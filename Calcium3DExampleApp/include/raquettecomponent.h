#ifndef RAQUETTECOMPONENT_H
#define RAQUETTECOMPONENT_H
#pragma once

#include <gallium/render/mesh.h>
#include <c3d/component.h>

class RaquetteComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(RaquetteComponent, c3d::Component);

	ga::render::Mesh myMesh;

	float r = 1.0f;
	glm::vec3 center = { 0,0,0 };
	float speed = 0.1f;

public:

	explicit RaquetteComponent(c3d::GameObject* owner);
	explicit RaquetteComponent(ga::core::CttiDeserializer& deserializer);

	virtual void Update(float f, float dt, c3d::GameSubsystems& subsystems) override;

	void SetVar(float _r, glm::vec3 _center, float _speed);
	void CreateFace(std::vector<glm::vec3> fPositions, std::vector<uint32_t> fIndices, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateRectangle(float kWIDTH, float kHEIGHT, float kDEPTH, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT, float kDEPTH);

	ga::render::Mesh& GetMyMesh();
};

#endif
