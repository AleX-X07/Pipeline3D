#include <c3d/components/physics/rigidbody.h>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>

#include <reactphysics3d/reactphysics3d.h>
#include <imgui/imgui.h>

using namespace c3d::components::physics;

struct Rigidbody::Internal
{
	reactphysics3d::RigidBody* bodyId = nullptr;
};

Rigidbody::Rigidbody(c3d::GameObject* owner)
	: c3d::Component(owner)
	, m_rbImpl(new Internal)
{
	Velocity        = glm::vec3(0.0f);
	AngularVelocity = glm::vec3(0.0f);
	Mass            = 1.0f;
	IsDynamic       = true;

	m_rbImpl->bodyId = nullptr;
}

Rigidbody::Rigidbody(ga::core::CttiDeserializer& deserializer)
	: c3d::Component(deserializer)
	, m_rbImpl(new Internal)
{
	Deserialize(deserializer);

	m_rbImpl->bodyId = nullptr;
}

Rigidbody::~Rigidbody() noexcept
{
	delete m_rbImpl;
}

void Rigidbody::EnforceTransform()
{
	if (m_rbImpl->bodyId)
	{
		reactphysics3d::Transform xform;
		xform.setToIdentity();
		xform.setPosition(*reinterpret_cast<reactphysics3d::Vector3*>(&GetGameObject()->Transform->Position.NonConstRef_Unguarded()));
		xform.setOrientation(*reinterpret_cast<reactphysics3d::Quaternion*>(&GetGameObject()->Transform->Orientation.NonConstRef_Unguarded()));
		m_rbImpl->bodyId->setTransform(xform);
	}
}

void Rigidbody::FixedUpdate(float t, float dt, c3d::GameSubsystems&)
{
	if (!m_rbImpl->bodyId)
		m_rbImpl->bodyId = GetGameObject()->Owner()->GetRigidbody({}, GetGameObject()); // returns null if there's no rigidbody for this GO

	if (m_rbImpl->bodyId)
	{
		if (IsDynamic.HasChanged())
		{
			if (IsDynamic())
				m_rbImpl->bodyId->setType(reactphysics3d::BodyType::DYNAMIC);
			else
				m_rbImpl->bodyId->setType(reactphysics3d::BodyType::STATIC);
		}

		if (Velocity.HasChanged())
			m_rbImpl->bodyId->setLinearVelocity(*reinterpret_cast<const reactphysics3d::Vector3*>(&Velocity.NonConstRef_Unguarded()));

		if (AngularVelocity.HasChanged())
			m_rbImpl->bodyId->setAngularVelocity(*reinterpret_cast<const reactphysics3d::Vector3*>(&AngularVelocity.NonConstRef_Unguarded()));

		if (Mass.HasChanged())
			m_rbImpl->bodyId->setMass(Mass);
	
		reactphysics3d::Vector3 velocity        = m_rbImpl->bodyId->getLinearVelocity();
		reactphysics3d::Vector3 angularVelocity = m_rbImpl->bodyId->getAngularVelocity();

		Velocity        = *reinterpret_cast<glm::vec3*>(&velocity);        Velocity.HasChanged();
		AngularVelocity = *reinterpret_cast<glm::vec3*>(&angularVelocity); AngularVelocity.HasChanged();
		Mass            = m_rbImpl->bodyId->getMass();                     Mass.HasChanged();
	}
}

void Rigidbody::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Velocity;
	serializer << AngularVelocity;
	serializer << Mass;
	serializer << IsDynamic;
}

void Rigidbody::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Velocity;
	deserializer >> AngularVelocity;
	deserializer >> Mass;
	deserializer >> IsDynamic;
}

void Rigidbody::RenderUi()
{
	Component::RenderUi();

	if (ImGui::Checkbox("Dynamic", &IsDynamic.NonConstRef_Unguarded()))
		IsDynamic.TriggerChanged();

	if (ImGui::DragFloat3("Velocity", (float*)(&Velocity.NonConstRef_Unguarded()), 0.1f, 0.1f, 1000.0f, "%.2f"))
		Velocity.TriggerChanged();
	
	if (ImGui::DragFloat3("Angular velocity", (float*)(&AngularVelocity.NonConstRef_Unguarded()), 0.1f, 0.1f, 1000.0f, "%.2f"))
		AngularVelocity.TriggerChanged();
	
	if (ImGui::DragFloat("Mass", &Mass.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		Mass.TriggerChanged();
}
