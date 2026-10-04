#ifndef C3D__COMPONENTS__PHYSICS__RIGIDBODY_H
#define C3D__COMPONENTS__PHYSICS__RIGIDBODY_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

namespace c3d::components::physics
{
	class Rigidbody final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::physics::Rigidbody, c3d::Component);

		struct Internal;
		Internal* m_rbImpl;

	public:
		ga::core::Property<glm::vec3> Velocity;
		ga::core::Property<glm::vec3> AngularVelocity;
		ga::core::Property<float>     Mass;
		ga::core::Property<bool>      IsDynamic;

		explicit Rigidbody(c3d::GameObject* owner);
		explicit Rigidbody(ga::core::CttiDeserializer& deserializer);
		virtual ~Rigidbody() noexcept;

		bool RayCast(const glm::vec3& ro, const glm::vec3& rd, float tMax = 1e3f);

		void EnforceTransform();

		void FixedUpdate(float t, float dt, c3d::GameSubsystems&) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__PHYSICS__RIGIDBODY_H */

