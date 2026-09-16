#ifndef C3D__COMPONENTS__OTHER__LOOKAT_H
#define C3D__COMPONENTS__OTHER__LOOKAT_H
#pragma once

#include <c3d/component.h>
#include <c3d/gameobject.h>

#include <gallium/core/property.h>


namespace c3d { class Scene; }

namespace c3d::components::other
{
	class LookAt final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::other::LookAt, c3d::Component);

	public:
		ga::core::Property<c3d::GameObject*> Target;

		explicit LookAt(c3d::GameObject* owner);
		explicit LookAt(ga::core::CttiDeserializer& deserializer);
		virtual ~LookAt() noexcept;

		void Update(float t, float dt, c3d::GameSubsystems&) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__COMPONENTS__OTHER__LOOKAT_H */
