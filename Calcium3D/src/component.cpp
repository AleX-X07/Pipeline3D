#include <c3d/component.h>
#include <c3d/gameobject.h>

using namespace c3d;

struct Component::Internal
{
	GameObject* object;
};

Component::Component(GameObject* owner)
	: m_pImpl(new Internal)
{
	m_pImpl->object = owner;
}

Component::Component(ga::core::CttiDeserializer& deserializer)
	: m_pImpl(new Internal)
{
	Deserialize(deserializer);
}

Component::~Component() noexcept
{
}

GameObject* Component::GetGameObject()
{
	return m_pImpl->object;
}


void Component::Serialize(ga::core::CttiSerializer& serializer) const
{
	serializer << m_pImpl->object;
}

void Component::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	deserializer >> m_pImpl->object;
}

void Component::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	m_pImpl->object = deserializer.ResolvePointer(m_pImpl->object);
}

void Component::RenderUi()
{
}

