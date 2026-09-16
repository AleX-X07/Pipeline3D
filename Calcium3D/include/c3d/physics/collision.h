#ifndef CALCIUM3D__PHYSICS__COLLISION_H
#define CALCIUM3D__PHYSICS__COLLISION_H
#pragma once

#include <vector>
#include <glm/vec3.hpp>

namespace c3d { class GameObject; }

namespace c3d::physics
{
	struct Collision
	{
		float                  penetrationDepth;
		glm::vec3              normal;
		std::vector<glm::vec3> contactPoints;

		GameObject* other;
	};
}

#endif /* CALCIUM3D__PHYSICS__COLLISION_H */
