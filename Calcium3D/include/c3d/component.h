#ifndef C3D__COMPONENT_H
#define C3D__COMPONENT_H
#pragma once

#include <memory>

#include <gallium/core/ctti.h>
#include <gallium/core/uiobject.h>

#include <c3d/physics/collision.h>

namespace ga::render { class RendererBase; }

namespace c3d
{
	struct GameSubsystems;
	class GameObject;

	namespace graphics { class Renderer2D; }

	class Component
		: public ga::core::CttiObject
		, public ga::core::UiObject
	{
		GA_CTTI_OBJECT(c3d::Component, ga::core::CttiObject);

		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

	public:
		explicit Component(GameObject* owner);
		explicit Component(ga::core::CttiDeserializer& deserializer);
		virtual ~Component() noexcept;

		GameObject* GetGameObject();

		virtual void Start()                                                         {}
		virtual void Reset()                                                         {}
		virtual void FixedUpdate(float t, float dt, c3d::GameSubsystems& subsystems) {}
		virtual void PreUpdate(float t, float dt, c3d::GameSubsystems& subsystems)   {}
		virtual void Update(float t, float dt, c3d::GameSubsystems& subsystems)      {}
		virtual void LateUpdate(float t, float dt, c3d::GameSubsystems& subsystems)  {}
		virtual void Render(ga::render::RendererBase&)                               {}
		virtual void Render2D(c3d::graphics::Renderer2D&)                            {}
		virtual void OnEnable()                                                      {}
		virtual void OnDisable()                                                     {}
		virtual void OnCollisionEnter(const physics::Collision& collision)           {}
		virtual void OnCollisionStay(const physics::Collision& collision)            {}
		virtual void OnCollisionLeave(const physics::Collision& collision)           {}
		virtual void OnTriggerEnter(GameObject* collision)                           {}
		virtual void OnTriggerStay(GameObject* collision)                            {}
		virtual void OnTriggerLeave(GameObject* collision)                           {}

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		virtual void RenderUi() override;
		virtual void RenderGizmo() {}
	};

	class ComponentConstructorBase
	{
	public:
		virtual Component* Construct(GameObject*) = 0;
	};

	template<typename T>
	class ComponentConstructor final
		: public ComponentConstructorBase
	{
	public:
		Component* Construct(GameObject* owner) override
		{
			return new T(owner);
		}
	};
}

#endif /* C3D__COMPONENT_H */
