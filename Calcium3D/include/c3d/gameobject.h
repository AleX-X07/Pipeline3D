#ifndef CALCIUM3D__GAMEOBJECT_H
#define CALCIUM3D__GAMEOBJECT_H
#pragma once

#include <memory>
#include <string>

#include <gallium/badge.h>
#include <gallium/string_hash.h>
#include <gallium/core/ctti.h>
#include <gallium/core/property.h>
#include <gallium/core/uiobject.h>

#include <c3d/transformdata.h>
#include <c3d/physics/collision.h>

namespace ga::render { class RendererBase; }

namespace c3d
{
	struct GameSubsystems;
	class GameScene;
	class Component;

	class GameObject
		: public ga::core::CttiObject
		, public ga::core::UiObject
	{
		GA_CTTI_OBJECT(c3d::GameObject, ga::core::CttiObject);

		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

		GameObject();

	public:
		ga::core::Property<std::string>   Name;
		ga::core::Property<TransformData> Transform;
		ga::core::Property<int32_t>       RenderOrder;

		GameObject(const ga::Badge<GameScene>&, GameScene* scene, const std::string& name = "");
		explicit GameObject(ga::core::CttiDeserializer& deserializer);
		~GameObject();

		GameScene* Owner() const;

		bool       HasComponent(string_hash_t componentId);
		Component* GetComponent(string_hash_t componentId);
		Component* AddComponent(string_hash_t componentId, Component* component);
		void       RemoveComponent(string_hash_t componentId);

		template<typename T> bool HasComponent()
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive directly or indirectly from Component");
			return HasComponent(T::GetTypeStatic());
		}

		template<typename T> T* GetComponent()
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive directly or indirectly from Component");
			Component* result = GetComponent(T::GetTypeStatic());
			if (result)
				return result->As<T>();
			return nullptr;
		}

		template<typename T, typename... ARGS> T* AddComponent(ARGS... args)
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive directly or indirectly from Component");
			return AddComponent(T::GetTypeStatic(), new T(this, args...))->As<T>();
		}

		template<typename T> void RemoveComponent()
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive directly or indirectly from Component");
			RemoveComponent(T::GetTypeStatic());
		}

		void Serialize(ga::core::CttiSerializer&) const override;
		void Deserialize(ga::core::CttiDeserializer&) override;
		void ResolvePointers(ga::core::CttiDeserializer&) override;

		void Enable();
		void Disable();
		bool IsEnabled() const;

		void Start(const ga::Badge<GameScene>&);
		void Reset(const ga::Badge<GameScene>&);
		void FixedUpdate(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems);
		void PreUpdate(const ga::Badge<GameScene>& , float t, float dt, c3d::GameSubsystems& subsystems);
		void Update(const ga::Badge<GameScene>& , float t, float dt, c3d::GameSubsystems& subsystems);
		void LateUpdate(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems);
		void Render(const ga::Badge<GameScene>&, ga::render::RendererBase& rp);

		void OnCollisionEnter(const physics::Collision& collision);
		void OnCollisionStay(const physics::Collision& collision);
		void OnCollisionLeave(const physics::Collision& collision);
		void OnTriggerEnter(GameObject* other);
		void OnTriggerStay(GameObject* other);
		void OnTriggerLeave(GameObject* other);

		void RenderUi() override;
		void RenderGizmos();
	};
}

#endif /* CALCIUM3D__GAMEOBJECT_H */
