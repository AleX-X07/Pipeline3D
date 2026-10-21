#include "raquettecomponent.h"

#include <gallium/platform/platform.h>
#include <gallium/platform/input.h>
#include <c3d/game.h>
#include <c3d/gamescene.h>
#include <c3d/component.h>
#include <c3d/gameobject.h>

#include "cubecomponent.h"

RaquetteComponent::RaquetteComponent(c3d::GameObject* owner, float _r, glm::vec3 _center, float _speed) :
	c3d::Component(owner), r(_r), center(_center), speed(_speed)
{

}

RaquetteComponent::RaquetteComponent(ga::core::CttiDeserializer& deserializer) :
	c3d::Component(deserializer), r(1.0f), center({0,0,0}), speed(1.0f)
{

}

void RaquetteComponent::Update(float f, float dt, c3d::GameSubsystems& subsystems)
{
	
	float finalSpeed;

	bool left = subsystems.platform.Input().IsHeld(ga::platform::EKey::A);
	bool right = subsystems.platform.Input().IsHeld(ga::platform::EKey::D);

	if (left) 
	{
		finalSpeed = -speed;
	}
	else if (right)
	{
		finalSpeed = speed;
	}
	else
	{
		finalSpeed = 0.0f;
	}

	angle += finalSpeed * dt;

	glm::vec3 offset =
	{
		center.x + glm::cos(angle) * r,
		0.0f,
		center.z + glm::sin(angle) * r
	};

	GetGameObject()->Transform->Position = offset;
	GetGameObject()->Transform->Orientation = glm::quat({ 0.0f, -angle, 0.0f });
}
