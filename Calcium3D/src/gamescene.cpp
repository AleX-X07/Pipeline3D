#include <c3d/gamescene.h>
#include <c3d/gameobject.h>
#include <c3d/component.h>
#include <c3d/physics/collision.h>
#include <c3d/components/rendering/camera.h>
#include <c3d/components/rendering/light.h>

#include <gallium/macros.h>
#include <gallium/core/messagebus.h>
#include <gallium/platform/timer.h>
#include <gallium/platform/platform.h>
#include <gallium/render/rendererbase.h>

#include <map>

#include <reactphysics3d/reactphysics3d.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#ifndef NDEBUG
#pragma comment(lib, "reactphysics3d-s-d.lib")
#else /* !NDEBUG */
#pragma comment(lib, "reactphysics3d-s.lib")
#endif /* !NDEBUG */

#define kFIXED_UPDATE_STEP (1.0f / 100.0f)

using namespace c3d;

class c3d_PhysicsEventListener final
	: public reactphysics3d::EventListener
{
	void onContact(const reactphysics3d::CollisionCallback::CallbackData& callbackData) override
	{
		for (uint32_t p = 0; p < callbackData.getNbContactPairs(); ++p)
		{
			CollisionCallback::ContactPair contactPair = callbackData.getContactPair(p);

			physics::Collision collision;

			for (uint32_t c = 0; c < contactPair.getNbContactPoints(); ++c)
			{
				auto contactPoint = contactPair.getContactPoint(c);

				if (!c)
				{
					collision.normal = *reinterpret_cast<const glm::vec3*>(&contactPoint.getWorldNormal());
					collision.penetrationDepth = contactPoint.getPenetrationDepth();
				}

				reactphysics3d::Vector3 worldPoint = contactPair.getCollider1()->getLocalToWorldTransform() * contactPoint.getLocalPointOnCollider1();
				collision.contactPoints.push_back(*reinterpret_cast<glm::vec3*>(&worldPoint));
			}

			GameObject* so1 = ((Component*)contactPair.getCollider1()->getUserData())->GetGameObject();
			GameObject* so2 = ((Component*)contactPair.getCollider2()->getUserData())->GetGameObject();

			if ((!so1->IsEnabled()) || (!so2->IsEnabled()))
				return;

			switch (contactPair.getEventType())
			{
			case CollisionCallback::ContactPair::EventType::ContactStart:
				collision.other = so2;
				so1->OnCollisionEnter(collision);

				collision.other = so1;
				so2->OnCollisionEnter(collision);
				break;

			case CollisionCallback::ContactPair::EventType::ContactStay:
				collision.other = so2;
				so1->OnCollisionStay(collision);

				collision.other = so1;
				so2->OnCollisionStay(collision);
				break;

			case CollisionCallback::ContactPair::EventType::ContactExit:
				collision.other = so2;
				so1->OnCollisionLeave(collision);

				collision.other = so1;
				so2->OnCollisionLeave(collision);
				break;
			}
		}
	}

	void onTrigger(const reactphysics3d::OverlapCallback::CallbackData& callbackData) override
	{
		for (uint32_t p = 0; p < callbackData.getNbOverlappingPairs(); ++p)
		{
			reactphysics3d::OverlapCallback::OverlapPair overlapPair = callbackData.getOverlappingPair(p);

			GameObject* so1 = ((Component*)overlapPair.getCollider1()->getUserData())->GetGameObject();
			GameObject* so2 = ((Component*)overlapPair.getCollider2()->getUserData())->GetGameObject();

			if ((!so1->IsEnabled()) || (!so2->IsEnabled()))
				return;

			physics::Collision dummyCollision1 { 0.0f, glm::vec3(0.0f), {}, so2 };
			physics::Collision dummyCollision2 { 0.0f, glm::vec3(0.0f), {}, so1 };

			switch (overlapPair.getEventType())
			{
			case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStart:
				if (overlapPair.getCollider2()->getIsTrigger())
					so1->OnTriggerEnter(so2);
				else
					so1->OnCollisionEnter(dummyCollision1);

				if (overlapPair.getCollider1()->getIsTrigger())
					so2->OnTriggerEnter(so1);
				else
					so2->OnCollisionEnter(dummyCollision2);
				break;

			case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStay:
				if (overlapPair.getCollider2()->getIsTrigger())
					so1->OnTriggerStay(so2);
				else
					so1->OnCollisionStay(dummyCollision1);

				if (overlapPair.getCollider1()->getIsTrigger())
					so2->OnTriggerStay(so1);
				else
					so2->OnCollisionStay(dummyCollision2);
				break;

			case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapExit:
				if (overlapPair.getCollider2()->getIsTrigger())
					so1->OnTriggerLeave(so2);
				else
					so1->OnCollisionStay(dummyCollision1);

				if (overlapPair.getCollider1()->getIsTrigger())
					so2->OnTriggerLeave(so1);
				else
					so2->OnCollisionLeave(dummyCollision2);
				break;
			}
		}
	}
};

// ----------------------------------------------------------------------------

struct GameScene::Internal
{
	bool                                                                                       isDirty           = true;
	bool                                                                                       isClearInProgress = false;
	bool                                                                                       processing        = false;

	std::unordered_map<GameObject*, std::vector<GameObject*>>                                  allObjects;
	GameObject*                                                                                activeObject = nullptr;

	std::unordered_map<GameObject*, std::vector<GameObject*>>                                  pendingObjectCreations;
	std::vector<GameObject*>                                                                   pendingObjectDeletions;

	std::mutex                                                                                 cameraMutex;
	std::vector<GameObject*>                                                                   cameras;

	static reactphysics3d::PhysicsCommon                                                       physicsCommon;
	reactphysics3d::PhysicsWorld*                                                              physicsWorld;
	double                                                                                     physicsTime;
	std::unordered_map<GameObject*, reactphysics3d::RigidBody*>                                physicsBodies;
	std::unordered_map<GameObject*, std::unordered_map<Component*, reactphysics3d::Collider*>> physicsColliders;
	std::vector<std::pair<GameObject*, GameObject*>>                                           physicsActiveCollisions;
	c3d_PhysicsEventListener                                                                   physicsEventListener;

	ga::platform::Timer                                                                        wallTime;
	float                                                                                      lastPreUpdate  = 0.0f;
	float                                                                                      lastUpdate     = 0.0f;
	float                                                                                      lastLateUpdate = 0.0f;
	
	ga::core::MessageHandler*                                                                  resizeHandler       = nullptr;
	glm::uvec2                                                                                 lastKnownWindowSize = glm::uvec2(1280, 720);

	reactphysics3d::RigidBody* m_RegisterRigidbody(GameObject* object, bool isDynamic);
	void m_UnregisterRigidbody(GameObject* object);

	std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterSphereCollider(c3d::components::physics::SphereCollider* collider);
	std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterCapsuleCollider(c3d::components::physics::CapsuleCollider* collider);
	std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> RegisterBoxCollider(c3d::components::physics::BoxCollider* collider);
	//std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> m_RegisterMeshCollider(c3d::components::physics::MeshCollider* collider);

	void UnregisterSphereCollider(c3d::components::physics::SphereCollider* collider);
	void UnregisterCapsuleCollider(c3d::components::physics::CapsuleCollider* collider);
	void UnregisterBoxCollider(c3d::components::physics::BoxCollider* collider);
	//void m_UnregisterMeshCollider(c3d::components::physics::MeshCollider* collider);
};

reactphysics3d::PhysicsCommon GameScene::Internal::physicsCommon;

// ----------------------------------------------------------------------------

GameScene::GameScene()
	: m_pImpl(new Internal)
{
	m_pImpl->physicsWorld = Internal::physicsCommon.createPhysicsWorld();
	m_pImpl->physicsWorld->setEventListener(&m_pImpl->physicsEventListener);

	reactphysics3d::DefaultLogger* logger = Internal::physicsCommon.createDefaultLogger();
	Internal::physicsCommon.setLogger(logger);

	Gravity = glm::vec3(0.0f, -9.81f, 0.0f);

	m_pImpl->physicsTime = 0.0;

	m_pImpl->resizeHandler = new ga::core::ObjectMemberDelegate(this, &GameScene::OnWindowResize);
	ga::core::MessageBus::Global().AddRecipient("window:resize"_h, m_pImpl->resizeHandler);
}

GameScene::GameScene(ga::core::CttiDeserializer& deserializer)
	: GameScene()
{
	Deserialize(deserializer);
}

GameScene::~GameScene()
{
	Clear();
	Internal::physicsCommon.destroyPhysicsWorld(m_pImpl->physicsWorld);

	ga::core::MessageBus::Global().RemoveRecipient("window:resize"_h, m_pImpl->resizeHandler);
	GA_SAFE_DELETE(m_pImpl->resizeHandler);
}

reactphysics3d::PhysicsCommon& GameScene::PhysicsCommon()
{
	return Internal::physicsCommon;
}

bool GameScene::IsDirty() const
{
	return m_pImpl->isDirty;
}

GameObject* GameScene::CreateObject(const std::string& name, GameObject* parent /* = nullptr */)
{
	if (parent && parent->Owner() != this)
		throw std::runtime_error("An object and its parent must be from the same scene");

	GameObject* newObject = new GameObject({}, this, name);
	newObject->Transform->Parent = parent ? &parent->Transform() : nullptr;

	if (m_pImpl->processing)
		m_pImpl->pendingObjectCreations[parent].push_back(newObject);
	else
		m_pImpl->allObjects[parent].push_back(newObject);

	return newObject;
}

void GameScene::RemoveObject(GameObject* so)
{
	if (m_pImpl->isClearInProgress)
		return;

	auto it = std::find(m_pImpl->pendingObjectDeletions.begin(), m_pImpl->pendingObjectDeletions.end(), so);

	if (it == m_pImpl->pendingObjectDeletions.end())
		m_pImpl->pendingObjectDeletions.push_back(so);
}

GameObject* GameScene::GetObject(const std::string& name)
{
	std::function<GameObject*(const std::string&, GameObject*, std::unordered_map<GameObject*, std::vector<GameObject*>>&)> recursiveSearch;
	recursiveSearch = [&recursiveSearch](const std::string& name, GameObject* parent, std::unordered_map<GameObject*, std::vector<GameObject*>>& objects) {
		if (parent)
			if (parent->Name() == name)
				return parent;

		if (objects.count(parent))
		{
			for (auto& o : objects[parent])
			{
				if (GameObject* result = recursiveSearch(name, o, objects))
					return result;
			}
		}

		return (GameObject*)nullptr;
	};

	return recursiveSearch(name, nullptr, m_pImpl->allObjects);
}

const std::vector<GameObject*> GameScene::GetRootObjects() const
{
	return m_pImpl->allObjects[nullptr];
}

reactphysics3d::RigidBody* GameScene::Internal::m_RegisterRigidbody(GameObject* object, bool isDynamic)
{
	GA_ASSERT_MSG(!physicsBodies.count(object), "This object already has a rigidbody");

	reactphysics3d::Transform bodyXform;
	bodyXform.setPosition(*reinterpret_cast<reactphysics3d::Vector3*>(&object->Transform.NonConstRef_Unguarded().Position.NonConstRef_Unguarded()));
	bodyXform.setOrientation(*reinterpret_cast<reactphysics3d::Quaternion*>(&object->Transform.NonConstRef_Unguarded().Orientation.NonConstRef_Unguarded()));
	reactphysics3d::RigidBody* body = physicsWorld->createRigidBody(bodyXform);

	body->setType(isDynamic ? rp3d::BodyType::DYNAMIC : rp3d::BodyType::STATIC);
	physicsBodies[object] = body;

	return body;
}

void GameScene::Internal::m_UnregisterRigidbody(GameObject* object)
{
	GA_ASSERT_MSG(physicsBodies.count(object), "This object has no rigidbody");
	GA_ASSERT_MSG(!physicsColliders.count(object), "This object still has active colliders");

	reactphysics3d::RigidBody* body = physicsBodies[object];
	physicsBodies.erase(object);
	physicsWorld->destroyRigidBody(body);
}

std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::Internal::RegisterSphereCollider(c3d::components::physics::SphereCollider* collider)
{
	GameObject* object = collider->GetGameObject();
	reactphysics3d::RigidBody* rbody = nullptr;

	if (physicsBodies.count(object))
		rbody = physicsBodies[object];
	else
		rbody = m_RegisterRigidbody(object, collider->IsDynamic());

	reactphysics3d::SphereShape* sphere = Internal::physicsCommon.createSphereShape(collider->Radius);
	reactphysics3d::Collider*    shape  = rbody->addCollider(sphere, reactphysics3d::Transform::identity());
	physicsColliders[object][collider] = shape;

	shape->getMaterial().setRollingResistance(1.5f);
	shape->getMaterial().setFrictionCoefficient(7.5f);
	shape->getMaterial().setBounciness(0.5f);

	shape->setUserData(collider);
	return std::make_pair(sphere, shape);
}

void GameScene::Internal::UnregisterSphereCollider(c3d::components::physics::SphereCollider* collider)
{
	auto shape = physicsColliders[collider->GetGameObject()][collider];
	physicsColliders[collider->GetGameObject()].erase(collider);
	shape->getBody()->removeCollider(shape);

	if (physicsColliders[collider->GetGameObject()].empty())
	{
		physicsColliders.erase(collider->GetGameObject());
		m_UnregisterRigidbody(collider->GetGameObject());
	}

}

std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*>GameScene::Internal::RegisterCapsuleCollider(c3d::components::physics::CapsuleCollider* collider)
{
	GameObject* object = collider->GetGameObject();
	reactphysics3d::RigidBody* rbody = nullptr;

	if (physicsBodies.count(object))
		rbody = physicsBodies[object];
	else
		rbody = m_RegisterRigidbody(object, collider->IsDynamic());

	reactphysics3d::CapsuleShape* capsule = Internal::physicsCommon.createCapsuleShape(collider->Radius, collider->Height);
	reactphysics3d::Collider* shape = rbody->addCollider(capsule, reactphysics3d::Transform::identity());
	physicsColliders[collider->GetGameObject()][collider] = shape;

	shape->getMaterial().setRollingResistance(1.5f);
	shape->getMaterial().setFrictionCoefficient(7.5f);

	shape->setUserData(collider);
	return std::make_pair(capsule, shape);
}

void GameScene::Internal::UnregisterCapsuleCollider(c3d::components::physics::CapsuleCollider* collider)
{
	auto shape = physicsColliders[collider->GetGameObject()][collider];
	physicsColliders[collider->GetGameObject()].erase(collider);
	shape->getBody()->removeCollider(shape);

	if (physicsColliders[collider->GetGameObject()].empty())
	{
		physicsColliders.erase(collider->GetGameObject());
		m_UnregisterRigidbody(collider->GetGameObject());
	}
}

std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::Internal::RegisterBoxCollider(c3d::components::physics::BoxCollider* collider)
{
	GameObject* object = collider->GetGameObject();
	reactphysics3d::RigidBody* rbody = nullptr;

	if (physicsBodies.count(object))
		rbody = physicsBodies[object];
	else
		rbody = m_RegisterRigidbody(object, collider->IsDynamic());

	reactphysics3d::BoxShape* box = Internal::physicsCommon.createBoxShape(*reinterpret_cast<reactphysics3d::Vector3*>(&collider->Size.NonConstRef_Unguarded()));
	reactphysics3d::Collider* shape = rbody->addCollider(box, reactphysics3d::Transform::identity());
	physicsColliders[collider->GetGameObject()][collider] = shape;

	shape->getMaterial().setRollingResistance(1.5f);
	shape->getMaterial().setFrictionCoefficient(7.5f);

	shape->setUserData(collider);
	return std::make_pair(box, shape);
}

void GameScene::Internal::UnregisterBoxCollider(c3d::components::physics::BoxCollider* collider)
{
	auto shape = physicsColliders[collider->GetGameObject()][collider];
	physicsColliders[collider->GetGameObject()].erase(collider);
	shape->getBody()->removeCollider(shape);

	if (physicsColliders[collider->GetGameObject()].empty())
	{
		physicsColliders.erase(collider->GetGameObject());
		m_UnregisterRigidbody(collider->GetGameObject());
	}
}

//std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::Internal::m_RegisterMeshCollider(c3d::components::physics::MeshCollider* collider)
//{
//	GameObject* object = collider->GetGameObject();
//	reactphysics3d::RigidBody* rbody = nullptr;
//
//	if (m_pImpl->physicsBodies.count(object))
//		rbody = m_pImpl->physicsBodies[object];
//	else
//		rbody = m_RegisterRigidbody(object, collider->IsDynamic);
//
//	auto triangleMesh = collider->GetGameObject()->GetComponent<c3d::components::rendering::StaticMesh>()->GetMeshGeometry().GetTriangleMesh();
//
//	reactphysics3d::ConcaveMeshShape* mesh = Internal::physicsCommon.createConcaveMeshShape(const_cast<rp3d::TriangleMesh*>(triangleMesh));
//	reactphysics3d::Collider* shape = rbody->addCollider(mesh, reactphysics3d::Transform::identity());
//	m_pImpl->physicsColliders[collider->GetGameObject()][collider] = shape;
//
//	shape->getMaterial().setRollingResistance(1.5f);
//	shape->getMaterial().setFrictionCoefficient(7.5f);
//
//	shape->setUserData(collider);
//	return std::make_pair(mesh, shape);
//}
//
//void GameScene::Internal::m_UnregisterMeshCollider(c3d::components::physics::MeshCollider* collider)
//{
//	auto shape = m_pImpl->physicsColliders[collider->GetGameObject()][collider];
//	m_pImpl->physicsColliders[collider->GetGameObject()].erase(collider);
//	shape->getBody()->removeCollider(shape);
//
//	if (m_pImpl->physicsColliders[collider->GetGameObject()].empty())
//	{
//		m_pImpl->physicsColliders.erase(collider->GetGameObject());
//		m_UnregisterRigidbody(collider->GetGameObject());
//	}
//}


std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::RegisterSphereCollider(const ga::Badge<c3d::components::physics::SphereCollider>&, c3d::components::physics::SphereCollider* collider)
{
	return m_pImpl->RegisterSphereCollider(collider);
}

std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::RegisterCapsuleCollider(const ga::Badge<c3d::components::physics::CapsuleCollider>&, c3d::components::physics::CapsuleCollider* collider)
{
	return m_pImpl->RegisterCapsuleCollider(collider);
}

std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::RegisterBoxCollider(const ga::Badge<c3d::components::physics::BoxCollider>&, c3d::components::physics::BoxCollider* collider)
{
	return m_pImpl->RegisterBoxCollider(collider);
}

//std::pair<reactphysics3d::CollisionShape*, reactphysics3d::Collider*> GameScene::RegisterMeshCollider(const ga::Badge<c3d::components::physics::MeshCollider>&, c3d::components::physics::MeshCollider* collider)
//{
//	return m_pImpl->RegisterMeshCollider(collider);
//}

void GameScene::UnregisterSphereCollider(const ga::Badge<c3d::components::physics::SphereCollider>&, c3d::components::physics::SphereCollider* collider)
{
	m_pImpl->UnregisterSphereCollider(collider);
}

void GameScene::UnregisterCapsuleCollider(const ga::Badge<c3d::components::physics::CapsuleCollider>&, c3d::components::physics::CapsuleCollider* collider)
{
	m_pImpl->UnregisterCapsuleCollider(collider);
}

void GameScene::UnregisterBoxCollider(const ga::Badge<c3d::components::physics::BoxCollider>&, c3d::components::physics::BoxCollider* collider)
{
	m_pImpl->UnregisterBoxCollider(collider);
}

//void GameScene::UnregisterMeshCollider(const ga::Badge<c3d::components::physics::MeshCollider>&, c3d::components::physics::MeshCollider* collider)
//{
//	m_UnregisterMeshCollider(collider);
//}


reactphysics3d::RigidBody* GameScene::GetRigidbody(const ga::Badge<c3d::components::physics::Rigidbody>&, GameObject* object)
{
	if (m_pImpl->physicsBodies.count(object))
		return m_pImpl->physicsBodies[object];

	return nullptr;
}

void GameScene::Start()
{
	m_pImpl->processing = true;

	m_pImpl->wallTime.Reset();

	for (auto& parent : m_pImpl->allObjects)
		for (auto& o : parent.second)
			o->Start({});

	m_pImpl->processing = false;
}

void GameScene::Clear()
{
	std::function<void(GameObject*)> recursiveDelete;

	auto allObjects = m_pImpl->allObjects;

	recursiveDelete = [&](GameObject* object) {
		if (allObjects.count(object))
		{
			for (auto& o : allObjects[object])
				recursiveDelete(o);
		}

		if (object)
			delete object;
	};

	m_pImpl->isClearInProgress = true;
	{
		recursiveDelete(nullptr);
		m_pImpl->allObjects.clear();

		m_pImpl->activeObject = nullptr;
	}
	m_pImpl->isClearInProgress = false;
}

void GameScene::Update(c3d::GameSubsystems& subsystems, bool passiveOnly /* = false */)
{
	m_pImpl->cameras.clear();
	m_pImpl->wallTime.Tick();

	std::function<void(GameObject*)> recursiveComponentClassifier;
	recursiveComponentClassifier = [&](GameObject* o) {
		if (o)
		{
			std::unique_lock<std::mutex> lock(m_pImpl->cameraMutex);

			if (o->IsEnabled() && o->HasComponent<components::rendering::Camera>())
				m_pImpl->cameras.push_back(o);
		}

		if (m_pImpl->allObjects.count(o))
			for (auto& c : m_pImpl->allObjects[o])
				recursiveComponentClassifier(c);
	};

	// find cameras
	recursiveComponentClassifier(nullptr);
	
	std::function<void(float, float, GameObject*, c3d::GameSubsystems&, void(GameObject::*)(const ga::Badge<GameScene>&, float, float, c3d::GameSubsystems&))> recursiveSingleThreadedGenericUpdateCall;
	recursiveSingleThreadedGenericUpdateCall = [&](float t, float dt, GameObject* o, c3d::GameSubsystems& subsystems, void (GameObject::* Call)(const ga::Badge<GameScene>&, float, float, c3d::GameSubsystems&)) {
		if (o)
			((*o).*Call)({}, t, dt, subsystems);

		if (m_pImpl->allObjects.count(o))
			for (auto& c : m_pImpl->allObjects[o])
				recursiveSingleThreadedGenericUpdateCall(t, dt, c, subsystems, Call);
	};

	m_pImpl->processing = true;

	std::function<void(float, float, GameObject*, c3d::GameSubsystems&, void(GameObject::*)(const ga::Badge<GameScene>&, float, float, c3d::GameSubsystems&))> recursiveGenericUpdateCall;
	recursiveGenericUpdateCall = recursiveSingleThreadedGenericUpdateCall;

	std::function<void(GameObject*)> recursiveTransformUpdate;
	recursiveTransformUpdate = [&](GameObject* o) {
		if (o)
		{
			if (m_pImpl->physicsBodies.count(o))
			{
				o->Transform->Position    = *reinterpret_cast<const glm::vec3*>(&m_pImpl->physicsBodies[o]->getTransform().getPosition());
				o->Transform->Orientation = *reinterpret_cast<const glm::quat*>(&m_pImpl->physicsBodies[o]->getTransform().getOrientation());
			}
		}

		if (m_pImpl->allObjects.count(o))
			for (auto& c : m_pImpl->allObjects[o])
				recursiveTransformUpdate(c);
	};

	if (!passiveOnly)
	{
		// Fixed-step physics update
		double physicsT = m_pImpl->wallTime.ElapsedTime();
		double physicsDt = physicsT - m_pImpl->physicsTime;
		int physicsNSteps = (int)ceilf(float(physicsDt) / float(kFIXED_UPDATE_STEP));

		while (m_pImpl->physicsTime < physicsT)
		{
			m_pImpl->physicsWorld->update(kFIXED_UPDATE_STEP);
			m_pImpl->physicsTime += physicsDt;

			for (auto& o : m_pImpl->allObjects[nullptr])
				recursiveSingleThreadedGenericUpdateCall(float(m_pImpl->physicsTime), kFIXED_UPDATE_STEP, o, subsystems, &GameObject::FixedUpdate);
		}

		// Transform updates
		recursiveTransformUpdate(nullptr);

		// Pre-update
		float preUpdateT = float(m_pImpl->wallTime.ElapsedTime());
		float preUpdateDt = preUpdateT - m_pImpl->lastPreUpdate;
		m_pImpl->lastPreUpdate = preUpdateT;
		for (auto& o : m_pImpl->allObjects[nullptr])
			recursiveGenericUpdateCall(preUpdateT, preUpdateDt, o, subsystems, &GameObject::PreUpdate);

		// Update
		float updateT = float(m_pImpl->wallTime.ElapsedTime());
		float updateDt = updateT - m_pImpl->lastUpdate;
		m_pImpl->lastUpdate = updateT;
		for (auto& o : m_pImpl->allObjects[nullptr])
			recursiveGenericUpdateCall(updateT, updateDt, o, subsystems, &GameObject::Update);

		// Late update
		float lateUpdateT = float(m_pImpl->wallTime.ElapsedTime());
		float lateUpdateDt = lateUpdateT - m_pImpl->lastLateUpdate;
		m_pImpl->lastLateUpdate = updateT;
		for (auto& o : m_pImpl->allObjects[nullptr])
			recursiveGenericUpdateCall(lateUpdateT, lateUpdateDt, o, subsystems, &GameObject::LateUpdate);
	}

	m_pImpl->processing = false;
}

void GameScene::Render(ga::render::RendererBase& renderPipeline)
{
	if (m_pImpl->cameras.empty())
		throw std::runtime_error("Scene.Render: Cannot render a scene without an active camera");
	
	std::map<int32_t, std::vector<GameObject*>> orderedObjects;

	std::function<void(GameObject*)> orderObjectsRecursive;
	orderObjectsRecursive = [&](GameObject* object) {
		if (object)
			orderedObjects[object->RenderOrder].push_back(object);

		if (m_pImpl->allObjects.count(object))
			for (auto& c : m_pImpl->allObjects[object])
				orderObjectsRecursive(c);
	};

	orderObjectsRecursive(nullptr);

	for (auto& [order, objects] : orderedObjects)
	{
		if (order > 9000)
			continue;

		for (auto& object : objects)
			object->Render({}, renderPipeline);
	}

	auto camera = m_pImpl->cameras[0]->GetComponent<components::rendering::Camera>();

	renderPipeline.SetCamera(glm::inverse(m_pImpl->cameras[0]->Transform->GlobalMatrix()), camera->ProjectionMatrix());
}

void GameScene::PostRender(ga::render::RendererBase& renderPipeline)
{
	std::map<int32_t, std::vector<GameObject*>> orderedObjects;

	std::function<void(GameObject*)> orderObjectsRecursive;
	orderObjectsRecursive = [&](GameObject* object) {
		if (object)
			if (object->RenderOrder >= 9000)
				orderedObjects[object->RenderOrder].push_back(object);

		if (m_pImpl->allObjects.count(object))
			for (auto& c : m_pImpl->allObjects[object])
				orderObjectsRecursive(c);
	};

	orderObjectsRecursive(nullptr);

	for (auto& [_, objects] : orderedObjects)
		for (auto& object : objects)
			object->Render({}, renderPipeline);

	// Process pending creations
	for (auto& newParent : m_pImpl->pendingObjectCreations)
		for (auto& o : newParent.second)
			m_pImpl->allObjects[newParent.first].push_back(o);

	// Process pending deletions
	for (auto& so : m_pImpl->pendingObjectDeletions)
	{
		TransformData* parentXform = so->Transform->Parent;
		GameObject* parent = nullptr;

		if (parentXform)
			parent = parentXform->GetGameObject();

		if (m_pImpl->allObjects.count(parent))
		{
			auto it = std::find(m_pImpl->allObjects[parent].begin(), m_pImpl->allObjects[parent].end(), so);
			if (it != m_pImpl->allObjects[parent].end())
			{
				m_pImpl->allObjects[parent].erase(it);

				if (m_pImpl->allObjects[parent].empty())
					m_pImpl->allObjects.erase(parent);
			}
		}

		if (m_pImpl->allObjects.count(so))
		{
			for (auto& a : m_pImpl->allObjects[so])
			{
				m_pImpl->allObjects[parent].push_back(a);
				a->Transform->Parent = parentXform;
			}

			m_pImpl->allObjects.erase(so);
		}

		// FIXME: Is this really safe ?
		delete so;
	}

	m_pImpl->pendingObjectCreations.clear();
	m_pImpl->pendingObjectDeletions.clear();
}


void GameScene::Serialize(ga::core::CttiSerializer& serializer) const
{
	serializer << m_pImpl->allObjects;
}

void GameScene::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	deserializer >> m_pImpl->allObjects;
	m_pImpl->isDirty = true;
}

void GameScene::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	std::unordered_map<GameObject*, std::vector<GameObject*>> realAllObjects;

	for (auto& parent : m_pImpl->allObjects)
	{
		GameObject* resolvedParent = deserializer.ResolvePointer(parent.first);
		std::vector<GameObject*> children;

		for (auto& child : parent.second)
			children.push_back(deserializer.ResolvePointer(child));

		realAllObjects[resolvedParent] = children;
	}

	m_pImpl->allObjects = realAllObjects;

	std::vector<GameObject*> processedObjects;
	for (auto& parent : m_pImpl->allObjects)
	{
		for (auto& child : parent.second)
		{
			if (std::find(processedObjects.begin(), processedObjects.end(), child) == processedObjects.end())
			{
				child->ResolvePointers(deserializer);
				processedObjects.push_back(child);
			}
		}
	}
}

void GameScene::OnWindowResize(ga::core::MessageDataBase* msg)
{
	ga::platform::SurfaceResizeData* resize = reinterpret_cast<ga::platform::SurfaceResizeData*>(msg);
	m_pImpl->lastKnownWindowSize = resize->data.surfaceSize;
}

void GameScene::RenderUi()
{
	GameObject* objectToDelete   = nullptr;
	GameObject* objectToReparent = nullptr;

	std::function<void(GameObject*)> RenderContextMenu = [&](GameObject* so) {
		if (so)
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.25f), so->Name.ConstRef().c_str());
			ImGui::Separator();
			
			if (ImGui::MenuItem("Create parent"))
				objectToReparent = so;
		}

		if (ImGui::MenuItem(so ? "Create child" : "Create object"))
		{
			CreateObject("New object", so);
		}

		if (so) if (ImGui::MenuItem("Delete object"))
			objectToDelete = so;
	};

	std::function<void(GameObject*)> RenderSubtree;
	RenderSubtree = [&](GameObject* so) {
		std::string objName = so->Name.ConstRef();
		if (objName.empty())
			objName = "(Unnamed)";

		bool isItemOpen = ImGui::TreeNode(objName.c_str());

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("c3d__GameObject", &so, sizeof(c3d::GameObject*));
			ImGui::TextUnformatted(so->Name.ConstRef().c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
		{
			m_pImpl->activeObject = so;
		}

		if (ImGui::BeginPopupContextItem(objName.c_str()))
		{
			RenderContextMenu(so);
			ImGui::EndPopup();
		}

		if (isItemOpen)
		{
			if (m_pImpl->allObjects.count(so))
				for (auto& child : m_pImpl->allObjects[so])
					RenderSubtree(child);
		
			ImGui::TreePop();
		}
	};

	if (ImGui::Begin("Scene outline"))
	{
		auto bbMin = ImGui::GetWindowContentRegionMin();
		auto bbMax = ImGui::GetWindowContentRegionMax();
		auto curId = ImGui::GetID("Scene outline");

		if (ImGui::BeginDragDropTargetCustom(ImRect(bbMin, bbMax), curId))
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ga__path__gascn");
			if (payload)
			{
				// TODO: deserialize scene from file
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextWindow())
		{
			RenderContextMenu(nullptr);
			ImGui::EndPopup();
		}

		for (auto& object : m_pImpl->allObjects[nullptr])
			RenderSubtree(object);
	}
	ImGui::End();

	if (objectToDelete == m_pImpl->activeObject)
		m_pImpl->activeObject = nullptr;

	if (objectToDelete)
		RemoveObject(objectToDelete);

	if (objectToReparent)
	{
		TransformData* oldParentXform = objectToReparent->Transform.ConstRef().Parent.ConstRef();
		GameObject*   oldParent      = oldParentXform ? oldParentXform->GetGameObject() : nullptr;

		auto& list = m_pImpl->allObjects[oldParent];
		auto  it   = std::find(list.begin(), list.end(), objectToReparent);

		if (it != list.end())
			list.erase(it);

		GameObject* newParent = CreateObject("New object", oldParent);
		m_pImpl->allObjects[newParent].push_back(objectToReparent);

		objectToReparent->Transform->Parent = &newParent->Transform();
	}

	if (m_pImpl->activeObject)
	{
		m_pImpl->activeObject->RenderUi();
		m_pImpl->activeObject->RenderGizmos();
	}
}

