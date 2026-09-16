#ifndef C3D__COMPONENTS__PHYSICS__SPHERECOLLIDER_H
#define C3D__COMPONENTS__PHYSICS__SPHERECOLLIDER_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

namespace c3d::components::physics
{
	class SphereCollider final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::physics::SphereCollider, c3d::Component);

		struct Internal;
		Internal* m_scImpl;

	public:
		ga::core::Property<float> Radius;
		ga::core::Property<float> Friction;
		ga::core::Property<float> RollingResistance;
		ga::core::Property<float> Bounciness;
		ga::core::Property<bool>  IsTrigger;
		ga::core::Property<bool>  IsDynamic;

		explicit SphereCollider(c3d::GameObject* owner);
		explicit SphereCollider(ga::core::CttiDeserializer& deserializer);
		virtual ~SphereCollider() noexcept;

		void FixedUpdate(float t, float dt, c3d::GameSubsystems&) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__PHYSICS__SPHERECOLLIDER_H */
