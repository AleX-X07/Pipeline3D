#include <c3d/components/physics/spherecollider.h>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>

#include <reactphysics3d/reactphysics3d.h>
#include <imgui/imgui.h>

using namespace c3d::components::physics;

struct SphereCollider::Internal
{
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::SphereShape* sphereShape = nullptr;
};

SphereCollider::SphereCollider(c3d::GameObject* owner)
	: c3d::Component(owner)
	, m_scImpl(new Internal)
{
	Radius = 1.0f;
	Friction = 1.5f;
	RollingResistance = 7.5f;
	Bounciness = 0.5f;
	IsTrigger = false;
	IsDynamic = true;
}

SphereCollider::SphereCollider(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
	, m_scImpl(new Internal)
{
	Deserialize(deserializer);
}

SphereCollider::~SphereCollider() noexcept
{
	if (m_scImpl->sphereShape)
		GetGameObject()->Owner()->UnregisterSphereCollider({}, this);

	delete m_scImpl;
}

void SphereCollider::FixedUpdate(float t, float dt, c3d::GameSubsystems&)
{
	if (IsDynamic.HasChanged() && m_scImpl->sphereShape)
	{
		GetGameObject()->Owner()->UnregisterSphereCollider({}, this);
		m_scImpl->sphereShape = nullptr;
	}

	if (!m_scImpl->sphereShape)
	{
		auto [shape, collider] = GetGameObject()->Owner()->RegisterSphereCollider({}, this);
		m_scImpl->sphereShape = (reactphysics3d::SphereShape*)shape;
		m_scImpl->collider = collider;

		Radius.HasChanged();
	}

	if (Radius.HasChanged())
		m_scImpl->sphereShape->setRadius(Radius);

	if (Friction.HasChanged())
		m_scImpl->collider->getMaterial().setFrictionCoefficient(Friction.ConstRef());

	if (RollingResistance.HasChanged())
		m_scImpl->collider->getMaterial().setRollingResistance(RollingResistance.ConstRef());

	if (Bounciness.HasChanged())
		m_scImpl->collider->getMaterial().setBounciness(Bounciness.ConstRef());

	if (IsTrigger.HasChanged())
		m_scImpl->collider->setIsTrigger(IsTrigger);
}

void SphereCollider::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Radius;
	serializer << IsTrigger;
	serializer << IsDynamic;
}

void SphereCollider::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Radius;
	deserializer >> IsTrigger;
	deserializer >> IsDynamic;
}

void SphereCollider::RenderUi()
{
	Component::RenderUi();

	if (ImGui::Checkbox("Dynamic", &IsDynamic.NonConstRef_Unguarded()))
		IsDynamic.TriggerChanged();
	
	if (ImGui::Checkbox("Trigger", &IsTrigger.NonConstRef_Unguarded()))
		IsTrigger.TriggerChanged();
	
	if (ImGui::DragFloat("Radius", &Radius.NonConstRef_Unguarded(), 0.1f, 0.1f, 1000.0f, "%.2f"))
		Radius.TriggerChanged();
	
	if (ImGui::DragFloat("Friction", &Friction.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		Friction.TriggerChanged();
	
	if (ImGui::DragFloat("Rolling resistance", &RollingResistance.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		RollingResistance.TriggerChanged();
	
	if (ImGui::DragFloat("Bounciness", &Bounciness.NonConstRef_Unguarded(), 0.1f, 0.0f, 1.0f, "%.2f"))
		Bounciness.TriggerChanged();
}
