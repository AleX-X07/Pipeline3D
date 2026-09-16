#include <c3d/gameobject.h>
#include <c3d/gamescene.h>
#include <c3d/component.h>
#include <c3d/componentfactory.h>

#include <gallium/string_utils.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace c3d;

struct GameObject::Internal
{
	bool                                          enabled;
	GameScene*                                    ownerScene;
	std::unordered_map<string_hash_t, Component*> components;
};

GameObject::GameObject()
	: m_pImpl(new Internal)
	, Transform(TransformData({}, this))
{
	m_pImpl->enabled = true;
	RenderOrder = 0;
}

GameObject::GameObject(ga::core::CttiDeserializer& deserializer)
	: GameObject()
{
	Deserialize(deserializer);
}

GameObject::GameObject(const ga::Badge<GameScene>&, GameScene* scene, const std::string& name /* = "" */)
	: GameObject()
{
	Name = name;
	m_pImpl->ownerScene = scene;
}

GameObject::~GameObject()
{
	for (auto& [_, component] : m_pImpl->components)
		delete component;
}

GameScene* GameObject::Owner() const
{
	return m_pImpl->ownerScene;
}

bool GameObject::HasComponent(string_hash_t componentId)
{
	return m_pImpl->components.count(componentId);
}

Component* GameObject::GetComponent(string_hash_t componentId)
{
	if (m_pImpl->components.count(componentId))
		return m_pImpl->components[componentId];

	return nullptr;
}

Component* GameObject::AddComponent(string_hash_t componentId, Component* component)
{
	if (!m_pImpl->components.count(componentId))
		m_pImpl->components[componentId] = component;

	return m_pImpl->components[componentId];
}

void GameObject::RemoveComponent(string_hash_t componentId)
{
	if (m_pImpl->components.count(componentId))
	{
		delete m_pImpl->components[componentId];
		m_pImpl->components.erase(componentId);
	}
}

void GameObject::Serialize(ga::core::CttiSerializer& serializer) const
{
	serializer << m_pImpl->ownerScene;
	serializer << Name;
	serializer << Transform;
	serializer << RenderOrder;
	serializer << m_pImpl->components;
}

void GameObject::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	deserializer >> m_pImpl->ownerScene;
	deserializer >> Name;
	deserializer >> Transform;
	deserializer >> RenderOrder;

	uint64_t numComponents;
	deserializer >> numComponents;
	for (uint64_t i = 0; i < numComponents; ++i)
	{
		string_hash_t componentId;
		deserializer >> componentId;

		AddComponent(componentId, ComponentFactory::Global().CreateComponent(componentId, this));
		uint64_t dummy; deserializer >> dummy; // Components are unique anyway so let's discard the "unexisting pointer" id
		GetComponent(componentId)->Deserialize(deserializer);
	}
}

void GameObject::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	m_pImpl->ownerScene = deserializer.ResolvePointer(m_pImpl->ownerScene);
	Transform->ResolvePointers(deserializer);

	for (auto& [_, component] : m_pImpl->components)
		component->ResolvePointers(deserializer);
}

void GameObject::Enable()
{
	if (!m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnEnable();

	m_pImpl->enabled = true;
}

void GameObject::Disable()
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnDisable();

	m_pImpl->enabled = false;
}

bool GameObject::IsEnabled() const
{
	return m_pImpl->enabled;
}

void GameObject::Start(const ga::Badge<GameScene>&)
{
	for (auto& [type, c] : m_pImpl->components)
		c->Start();
}

void GameObject::Reset(const ga::Badge<GameScene>&)
{
	for (auto& [type, c] : m_pImpl->components)
		c->Reset();
}

void GameObject::FixedUpdate(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->FixedUpdate(t, dt, subsystems);
}

void GameObject::PreUpdate(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->PreUpdate(t, dt, subsystems);
}

void GameObject::Update(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->Update(t, dt, subsystems);
}

void GameObject::LateUpdate(const ga::Badge<GameScene>&, float t, float dt, c3d::GameSubsystems& subsystems)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->LateUpdate(t, dt, subsystems);
}

void GameObject::Render(const ga::Badge<GameScene>&, ga::render::RendererBase& rp)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->Render(rp);
}

void GameObject::OnCollisionEnter(const physics::Collision& collision)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnCollisionEnter(collision);
}

void GameObject::OnCollisionStay(const physics::Collision& collision)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnCollisionStay(collision);
}

void GameObject::OnCollisionLeave(const physics::Collision& collision)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnCollisionLeave(collision);
}

void GameObject::OnTriggerEnter(GameObject* other)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnTriggerEnter(other);
}

void GameObject::OnTriggerStay(GameObject* other)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnTriggerStay(other);
}

void GameObject::OnTriggerLeave(GameObject* other)
{
	if (m_pImpl->enabled)
		for (auto& [type, c] : m_pImpl->components)
			c->OnTriggerLeave(other);
}


void GameObject::RenderUi()
{
	Component* componentToRemove = nullptr;

	if (ImGui::Begin("Inspector"))
	{
		ImGui::Checkbox("Enabled", &m_pImpl->enabled);

		if (ImGui::InputText("Name", &Name.NonConstRef_Unguarded()))
			Name.TriggerChanged();

		ImGui::Separator();

		if (ImGui::DragInt("Render order", &RenderOrder.NonConstRef_Unguarded()))
			RenderOrder.TriggerChanged();

		ImGui::Separator();
		
		Transform.NonConstRef_Unguarded().RenderUi();
		ImGui::Separator();

		for (auto& [type, c] : m_pImpl->components)
		{
			std::string longName = c->GetTypeInfo()->GetTypeName();
			std::string shortName = ga::string_replace(longName, "::", ":");

			std::vector<std::string> tokens;
			ga::string_split(tokens, shortName, ':');
			shortName = tokens.back();

			bool isHeaderOpen = ImGui::CollapsingHeader((shortName + "##" + std::to_string(this) + "_" + shortName).c_str(), ImGuiTreeNodeFlags_DefaultOpen);

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Remove component"))
				{
					componentToRemove = c;
				}

				ImGui::EndPopup();
			}

			if (isHeaderOpen)
				c->RenderUi();
		}

		ImGui::Separator();

		if (ImGui::Button("Add component"))
			ImGui::OpenPopup("Add component menu");

		if (ImGui::BeginPopupContextItem("Add component menu"))
		{
			std::function<void(const ComponentFactory::List&)> recursiveComponentMenu;
			recursiveComponentMenu = [&](const ComponentFactory::List& list) {
				for (auto& a : list.subcategories)
				{
					if (ImGui::BeginMenu(a.first.c_str()))
					{
						recursiveComponentMenu(a.second);
						ImGui::EndMenu();
					}
				}

				for (auto& a : list.components)
					if (ImGui::MenuItem(a.first.c_str()))
						AddComponent(a.second, ComponentFactory::Global().CreateComponent(a.second, this));
			};

			recursiveComponentMenu(ComponentFactory::Global().GetComponents());
			ImGui::EndPopup();
		}
	}
	ImGui::End();

	if (componentToRemove)
		RemoveComponent(componentToRemove->GetType());
}

void GameObject::RenderGizmos()
{
	for (auto& [type, c] : m_pImpl->components)
		c->RenderGizmo();
}

