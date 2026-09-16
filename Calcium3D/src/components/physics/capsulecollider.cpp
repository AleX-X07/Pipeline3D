#include <c3d/components/physics/CapsuleCollider.h>

#include <gallium/core/log.h>
#include <c3d/gamescene.h>
#include <c3d/gameobject.h>

#include <reactphysics3d/reactphysics3d.h>
#include <imgui/imgui.h>

using namespace c3d::components::physics;

struct CapsuleCollider::Internal
{
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::CapsuleShape* capsuleShape = nullptr;
};

CapsuleCollider::CapsuleCollider(c3d::GameObject* owner)
	: c3d::Component(owner)
	, m_ccImpl(new Internal)
{
	Radius = 1.0f;
	Height = 2.0f;
	Friction = 1.5f;
	RollingResistance = 7.5f;
	Bounciness = 0.5f;
	IsTrigger = false;
	IsDynamic = true;
}

CapsuleCollider::CapsuleCollider(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
	, m_ccImpl(new Internal)
{
	Deserialize(deserializer);
}

CapsuleCollider::~CapsuleCollider() noexcept
{
	if (m_ccImpl->capsuleShape)
		GetGameObject()->Owner()->UnregisterCapsuleCollider({}, this);

	delete m_ccImpl;
}

void CapsuleCollider::FixedUpdate(float t, float dt, c3d::GameSubsystems&)
{
	if (IsDynamic.HasChanged() && m_ccImpl->capsuleShape)
	{
		GetGameObject()->Owner()->UnregisterCapsuleCollider({}, this);
		m_ccImpl->capsuleShape = nullptr;
	}

	if (!m_ccImpl->capsuleShape)
	{
		auto [shape, collider] = GetGameObject()->Owner()->RegisterCapsuleCollider({}, this);
		m_ccImpl->capsuleShape = (reactphysics3d::CapsuleShape*)shape;
		m_ccImpl->collider = collider;

		Radius.HasChanged();
		Height.HasChanged();
	}
	
	if (Radius.HasChanged() || Height.HasChanged())
	{
		m_ccImpl->capsuleShape->setHeight(Height);
		m_ccImpl->capsuleShape->setRadius(Radius);
	}

	if (IsTrigger.HasChanged())
		m_ccImpl->collider->setIsTrigger(IsTrigger);

	if (Friction.HasChanged())
		m_ccImpl->collider->getMaterial().setFrictionCoefficient(Friction.ConstRef());

	if (RollingResistance.HasChanged())
		m_ccImpl->collider->getMaterial().setRollingResistance(RollingResistance.ConstRef());

	if (Bounciness.HasChanged())
		m_ccImpl->collider->getMaterial().setBounciness(Bounciness.ConstRef());
}

void CapsuleCollider::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Height;
	serializer << Radius;
	serializer << IsTrigger;
	serializer << IsDynamic;
}

void CapsuleCollider::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Height;
	deserializer >> Radius;
	deserializer >> IsTrigger;
	deserializer >> IsDynamic;
}

void CapsuleCollider::RenderUi()
{
	Component::RenderUi();

	if (ImGui::Checkbox("Dynamic", &IsDynamic.NonConstRef_Unguarded()))
		IsDynamic.TriggerChanged();
	
	if (ImGui::Checkbox("Trigger", &IsTrigger.NonConstRef_Unguarded()))
		IsTrigger.TriggerChanged();
	
	if (ImGui::DragFloat("Height", &Height.NonConstRef_Unguarded(), 0.1f, 0.1f, 1000.0f, "%.2f"))
		Height.TriggerChanged();
	
	if (ImGui::DragFloat("Radius", &Radius.NonConstRef_Unguarded(), 0.1f, 0.1f, 1000.0f, "%.2f"))
		Radius.TriggerChanged();
	
	if (ImGui::DragFloat("Friction", &Friction.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		Friction.TriggerChanged();
	
	if (ImGui::DragFloat("Rolling resistance", &RollingResistance.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		RollingResistance.TriggerChanged();
	
	if (ImGui::DragFloat("Bounciness", &Bounciness.NonConstRef_Unguarded(), 0.1f, 0.0f, 1.0f, "%.2f"))
		Bounciness.TriggerChanged();
}
