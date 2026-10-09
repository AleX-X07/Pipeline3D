#ifndef RAQUETTECOMPONENT_H
#define RAQUETTECOMPONENT_H
#pragma once

#include <c3d/game.h>
#include <c3d/component.h>

class RaquetteComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(RaquetteComponent, c3d::Component);

	ga::render::Mesh myMesh;

public:

	explicit RaquetteComponent(c3d::GameObject* owner);
	explicit RaquetteComponent(ga::core::CttiDeserializer& deserializer);

	virtual void Update(float f, float dt, c3d::GameSubsystems& subsystems) override ;

	void CreateRectangle(float kWIDTH, float kHEIGHT, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<uint32_t>& indices);
	void CreateProceduralMesh(ga::gpu::Device& gpu, float kWIDTH, float kHEIGHT);

	ga::render::Mesh& GetMyMesh();
};

#endif
