#ifndef CALCIUM3D__GAMESCENE_H
#define CALCIUM3D__GAMESCENE_H
#pragma once

#include <memory>
#include <optional>
#include <string>

#include <gallium/badge.h>
#include <gallium/core/ctti.h>
#include <gallium/core/uiobject.h>
#include <gallium/core/messagebus.h>

#include <c3d/physics/collision.h>
#include <c3d/components/physics/rigidbody.h>
#include <c3d/components/physics/spherecollider.h>
#include <c3d/components/physics/capsulecollider.h>
#include <c3d/components/physics/boxcollider.h>

namespace ga::render { class RendererBase; }
namespace reactphysics3d { class RigidBody; class CollisionShape; class Collider; class PhysicsCommon; }

namespace c3d
{
	struct GameSubsystems;
	class  GameObject;

	namespace graphics { class Renderer2D; }

	class GameScene
		: public ga::core::CttiObject
		, public ga::core::UiObject
	{
		GA_CTTI_OBJECT(c3d::GameScene, ga::core::CttiObject);

		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

	public:
		glm::vec3 Gravity;

		GameScene();
		explicit GameScene(ga::core::CttiDeserializer& deserializer);
		~GameScene();

		static reactphysics3d::PhysicsCommon& PhysicsCommon();

		bool                           IsDirty() const;
		GameObject*                    CreateObject(const std::string& name, GameObject* parent = nullptr);
		void                           RemoveObject(GameObject* so);
		GameObject*                    GetObject(const std::string& name);
		const std::vector<GameObject*> GetRootObjects() const;

		std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterSphereCollider(const ga::Badge<c3d::components::physics::SphereCollider>&, c3d::components::physics::SphereCollider* collider);
		std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterCapsuleCollider(const ga::Badge<c3d::components::physics::CapsuleCollider>&, c3d::components::physics::CapsuleCollider* collider);
		std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterBoxCollider(const ga::Badge<c3d::components::physics::BoxCollider>&, c3d::components::physics::BoxCollider* collider);
		//std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterMeshCollider(const ga::Badge<c3d::components::physics::MeshCollider>&, c3d::components::physics::MeshCollider* collider);

		void UnregisterSphereCollider(const ga::Badge<c3d::components::physics::SphereCollider>&, c3d::components::physics::SphereCollider* collider);
		void UnregisterCapsuleCollider(const ga::Badge<c3d::components::physics::CapsuleCollider>&, c3d::components::physics::CapsuleCollider* collider);
		void UnregisterBoxCollider(const ga::Badge<c3d::components::physics::BoxCollider>&, c3d::components::physics::BoxCollider* collider);
		//void UnregisterMeshCollider(const ga::Badge<c3d::components::physics::MeshCollider>&, c3d::components::physics::MeshCollider* collider);

		reactphysics3d::RigidBody* GetRigidbody(const ga::Badge<c3d::components::physics::Rigidbody>&, GameObject* object);
		std::optional<float> RayCast(const glm::vec3& ro, const glm::vec3& rd, float tMax = 1e3f);

		void Start();
		void Clear();
		void Update(c3d::GameSubsystems& subsystems, bool passiveOnly = false);
		void Render(ga::render::RendererBase& renderPipeline, c3d::graphics::Renderer2D* renderer2D = nullptr);
		void PostRender(ga::render::RendererBase& renderPipeline);

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void OnWindowResize(ga::core::MessageDataBase* msg);
		void RenderUi() override;
	};
}

#endif /* CALCIUM3D__GAMESCENE_H */
