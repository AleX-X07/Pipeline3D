#ifndef SPHEREBEHAVIOURCOMPONENT_H 
#define SPHEREBEHAVIOURCOMPONENT_H
#pragma once

#include <c3d/component.h>

class SphereBehaviourComponent :
	public c3d::Component
{
	GA_CTTI_OBJECT(SphereBehaviourComponent, c3d::Component);

	float speed = 0.0f;
	bool start = false;

public:

	explicit SphereBehaviourComponent(c3d::GameObject* owner, float _speed);
	explicit SphereBehaviourComponent(ga::core::CttiDeserializer& deserializer);

	virtual void Update(float f, float dt, c3d::GameSubsystems& subsystems) override;
};

#endif