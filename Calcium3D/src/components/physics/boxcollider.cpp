#include <c3d/components/physics/boxcollider.h>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>

#include <reactphysics3d/reactphysics3d.h>
#include <imgui/imgui.h>

using namespace c3d::components::physics;

struct BoxCollider::Internal
{
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::BoxShape* boxShape = nullptr;
};

BoxCollider::BoxCollider(c3d::GameObject* owner)
	: c3d::Component(owner)
	, m_bcImpl(new Internal)
{
	Size = glm::vec3(1.0f);
	Friction = 1.5f;
	RollingResistance = 7.5f;
	Bounciness = 0.5f;
	IsTrigger = false;
	IsDynamic = true;
}

BoxCollider::BoxCollider(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
	, m_bcImpl(new Internal)
{
	Deserialize(deserializer);
}

BoxCollider::~BoxCollider() noexcept
{
	if (m_bcImpl->boxShape)
		GetGameObject()->Owner()->UnregisterBoxCollider({}, this);

	delete m_bcImpl;
}

void BoxCollider::FixedUpdate(float t, float dt, c3d::GameSubsystems&)
{
	if (IsDynamic.HasChanged() && m_bcImpl->boxShape)
	{
		GetGameObject()->Owner()->UnregisterBoxCollider({}, this);
		m_bcImpl->boxShape = nullptr;
	}

	if (!m_bcImpl->boxShape)
	{
		auto [shape, collider] = GetGameObject()->Owner()->RegisterBoxCollider({}, this);
		m_bcImpl->boxShape = (reactphysics3d::BoxShape*)shape;
		m_bcImpl->collider = collider;
		Size.HasChanged();
	}

	if (Size.HasChanged())
	{
		glm::vec3 halfSize = Size * 0.5f;
		m_bcImpl->boxShape->setHalfExtents(*reinterpret_cast<reactphysics3d::Vector3*>(&halfSize));
	}

	if (Friction.HasChanged())
		m_bcImpl->collider->getMaterial().setFrictionCoefficient(Friction.ConstRef());

	if (RollingResistance.HasChanged())
		m_bcImpl->collider->getMaterial().setRollingResistance(RollingResistance.ConstRef());

	if (Bounciness.HasChanged())
		m_bcImpl->collider->getMaterial().setBounciness(Bounciness.ConstRef());

	if (IsTrigger.HasChanged())
		m_bcImpl->collider->setIsTrigger(IsTrigger);

}

void BoxCollider::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Size;
	serializer << Friction;
	serializer << RollingResistance;
	serializer << Bounciness;
	serializer << IsTrigger;
	serializer << IsDynamic;
}

void BoxCollider::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Size;
	deserializer >> Friction;
	deserializer >> RollingResistance;
	deserializer >> Bounciness;
	deserializer >> IsTrigger;
	deserializer >> IsDynamic;
}

void BoxCollider::RenderUi()
{
	Component::RenderUi();

	if (ImGui::Checkbox("Dynamic", &IsDynamic.NonConstRef_Unguarded()))
		IsDynamic.TriggerChanged();
	
	if (ImGui::Checkbox("Trigger", &IsTrigger.NonConstRef_Unguarded()))
		IsTrigger.TriggerChanged();
	
	if (ImGui::DragFloat3("Size", (float*)(&Size.NonConstRef_Unguarded()), 0.1f, 0.1f, 1000.0f, "%.2f"))
		Size.TriggerChanged();
	
	if (ImGui::DragFloat("Friction", &Friction.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		Friction.TriggerChanged();
	
	if (ImGui::DragFloat("Rolling resistance", &RollingResistance.NonConstRef_Unguarded(), 0.1f, 0.0f, 10.0f, "%.2f"))
		RollingResistance.TriggerChanged();
	
	if (ImGui::DragFloat("Bounciness", &Bounciness.NonConstRef_Unguarded(), 0.1f, 0.0f, 1.0f, "%.2f"))
		Bounciness.TriggerChanged();
}
