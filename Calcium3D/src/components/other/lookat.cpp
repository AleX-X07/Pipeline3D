#include <c3d/components/other/lookat.h>
#include <c3d/gameobject.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace c3d::components::other;

LookAt::LookAt(c3d::GameObject* owner)
	: Component(owner)
{
}

LookAt::LookAt(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
	Deserialize(deserializer);
}

LookAt::~LookAt() noexcept
{
}

void LookAt::Update(float t, float dt, c3d::GameSubsystems&)
{
	if (Target.ConstRef())
		GetGameObject()->Transform().LookAt(Target.ConstRef()->Transform.ConstRef(), glm::vec3(0.0f, 1.0f, 0.0f));
}

void LookAt::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Target;
}

void LookAt::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);
	deserializer >> Target;
}

void LookAt::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	Component::ResolvePointers(deserializer);
	Target = deserializer.ResolvePointer(Target.NonConstRef_Unguarded());
}

void LookAt::RenderUi()
{
	Component::RenderUi();

	if (ImGui::Button("X"))
		Target = nullptr;
	ImGui::SameLine();
	
	ImGui::PushItemWidth(-97.0f);
	
	std::string targetName;
	if (Target.ConstRef())
		targetName = Target.ConstRef()->Name.ConstRef();
	ImGui::InputText("Target", &targetName, ImGuiInputTextFlags_ReadOnly);
	
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("c3d__GameObject");
	
		if (payload)
			Target = *reinterpret_cast<c3d::GameObject**>(payload->Data);
	
		ImGui::EndDragDropTarget();
	}
}
