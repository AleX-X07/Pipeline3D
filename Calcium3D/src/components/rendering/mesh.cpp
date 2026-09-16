#include <c3d/components/rendering/mesh.h>
#include <c3d/gameobject.h>

#include <gallium/render/rendererbase.h>
#include <gallium/assets/assetmanager.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace c3d::components::rendering;

Mesh::Mesh(GameObject* owner)
	: Component(owner)
	, Asset(std::nullopt)
	, m_meshInstance(nullptr)
{
}

Mesh::Mesh(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
	Deserialize(deserializer);
}

Mesh::~Mesh()
{
	m_meshInstance.reset();
}

void Mesh::Render(ga::render::RendererBase& renderer)
{
	if (!Asset)
	{
		if (m_meshInstance)
			m_meshInstance.reset();

		return;
	}

	bool recreateInstance = false;
	if (!m_meshInstance)
		recreateInstance = true;
	else if (m_meshInstance->mesh != &Asset->get())
		recreateInstance = true;

	if (recreateInstance)
		m_meshInstance = std::make_unique<ga::render::MeshInstance>(&Asset->get());

	// TODO: update skin if the mesh has one and a valid animation is selected

	renderer.AddGeometry(*m_meshInstance, GetGameObject()->Transform->GlobalMatrix());
}


void Mesh::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << (m_meshInstance
		? ga::assets::AssetManager::Global().PathOf<ga::render::Mesh>(*m_meshInstance->mesh)
		: "");
}

void Mesh::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	std::string assetPath;
	deserializer >> assetPath;
	if (!assetPath.empty())
		Asset = ga::assets::AssetManager::Global().Load<ga::render::Mesh>(assetPath);
}

void Mesh::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	Component::ResolvePointers(deserializer);
}


void Mesh::RenderUi()
{
	Component::RenderUi();
	std::string meshPath = "";

	if (Asset.has_value())
		meshPath = ga::assets::AssetManager::Global().PathOf(Asset->get());
	
	ImGui::InputText("Mesh path", &meshPath, ImGuiInputTextFlags_ReadOnly);
	
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ga__path__obj");
	
		if (payload)
		{
			std::string sObjectPath((char*)(payload->Data), payload->DataSize);
			Asset = ga::assets::AssetManager::Global().Load<ga::render::Mesh>(sObjectPath);
		}
		ImGui::EndDragDropTarget();
	}
}

