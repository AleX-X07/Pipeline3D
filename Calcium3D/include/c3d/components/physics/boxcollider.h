#ifndef C3D__COMPONENTS__PHYSICS__BOXCOLLIDER_H
#define C3D__COMPONENTS__PHYSICS__BOXCOLLIDER_H
#pragma once

#include <c3d/component.h>
#include <gallium/core/property.h>

namespace c3d::components::physics
{
	class BoxCollider final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::physics::BoxCollider, c3d::Component);

		struct Internal;
		Internal* m_bcImpl;

	public:
		ga::core::Property<glm::vec3> Size;
		ga::core::Property<float>     Friction;
		ga::core::Property<float>     RollingResistance;
		ga::core::Property<float>     Bounciness;
		ga::core::Property<bool>      IsTrigger;
		ga::core::Property<bool>      IsDynamic;

		explicit BoxCollider(c3d::GameObject* owner);
		explicit BoxCollider(ga::core::CttiDeserializer& deserializer);
		virtual ~BoxCollider() noexcept;

		void FixedUpdate(float t, float dt, c3d::GameSubsystems&) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__PHYSICS__BOXCOLLIDER_H */
