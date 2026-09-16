#ifndef C3D__COMPONENTS__PHYSICS__CAPSULECOLLIDER_H
#define C3D__COMPONENTS__PHYSICS__CAPSULECOLLIDER_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

namespace c3d::components::physics
{
	class CapsuleCollider final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::physics::CapsuleCollider, c3d::Component);

		struct Internal;
		Internal* m_ccImpl;

	public:
		ga::core::Property<float> Height;
		ga::core::Property<float> Radius;
		ga::core::Property<float> Friction;
		ga::core::Property<float> RollingResistance;
		ga::core::Property<float> Bounciness;
		ga::core::Property<bool>  IsTrigger;
		ga::core::Property<bool>  IsDynamic;

		explicit CapsuleCollider(c3d::GameObject* owner);
		explicit CapsuleCollider(ga::core::CttiDeserializer& deserializer);
		virtual ~CapsuleCollider() noexcept;

		void FixedUpdate(float t, float dt, c3d::GameSubsystems&) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__PHYSICS__CAPSULECOLLIDER_H */
