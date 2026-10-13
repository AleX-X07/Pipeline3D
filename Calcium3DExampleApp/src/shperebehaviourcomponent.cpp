#include "spherebehaviourcomponent.h"

#include <gallium/platform/platform.h>
#include <gallium/platform/input.h>

#include <c3d/game.h>
#include <c3d/gameobject.h>

SphereBehaviourComponent::SphereBehaviourComponent(c3d::GameObject* owner, float _speed) :
	Component(owner)
{
	speed = _speed;
}

SphereBehaviourComponent::SphereBehaviourComponent(ga::core::CttiDeserializer& deserializer) :
	Component(deserializer)
{

}

void SphereBehaviourComponent::Update(float f, float dt, c3d::GameSubsystems& subsystems)
{
	bool spaceClick = subsystems.platform.Input().IsPressed(ga::platform::EKey::Space);

	if (spaceClick)
	{
		start = true;
	}
	if (start) 
	{
		float newPos = dt * speed;
		GetGameObject()->Transform->Position->x += newPos;
	}
}
