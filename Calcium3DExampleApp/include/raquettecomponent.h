#ifndef RAQUETTECOMPONENT_H
#define RAQUETTECOMPONENT_H
#pragma once

#include <gallium/render/mesh.h>
#include <c3d/component.h>

class RaquetteComponent
	: public c3d::Component
{
	GA_CTTI_OBJECT(RaquetteComponent, c3d::Component);

	float r;
	glm::vec3 center;
	float speed;

	float angle = 0.0f;
	float width, height, depth;

public:

	explicit RaquetteComponent(c3d::GameObject* owner, float _r, glm::vec3 _center, float _speed, float _width, float _height, float _depth);
	explicit RaquetteComponent(ga::core::CttiDeserializer& deserializer);

	virtual void Update(float f, float dt, c3d::GameSubsystems& subsystems) override;

	glm::vec3 GetSize();
};

#endif
