#include <c3d/components/rendering/text2d.h>

#include <gallium/assets/assetmanager.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include <c3d/graphics/renderer2d.h>
#include <c3d/gameobject.h>
#include <c3d/transformdata.h>

#include <glm/gtx/matrix_transform_2d.hpp>

using namespace c3d::components::rendering;

Text2D::Text2D(GameObject* owner)
	: Component(owner)
	, Color({ 1.0f, 1.0f, 1.0f, 1.0f })
{
}

Text2D::Text2D(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
}

Text2D::~Text2D()
{
}

void Text2D::Render2D(c3d::graphics::Renderer2D& renderer)
{
	if (!Font.has_value())
		return;

	glm::vec2 position2d {
		GetGameObject()->Transform->Position().x,
		GetGameObject()->Transform->Position().y
	};

	renderer.AddText(c3d::graphics::Text2D{
		.font      = Font->get(),
		.text      = Text(),
		.transform = glm::translate(glm::identity<glm::mat3>(), position2d),
		.color     = glm::vec4(1.0f)
	});
}

void Text2D::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Text;
	serializer << Color;

	serializer << (Font
		? ga::assets::AssetManager::Global().PathOf<ga::render::Font>(*Font)
		: "");
}

void Text2D::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Text;
	deserializer >> Color;

	std::string assetPath;
	deserializer >> assetPath;
	if (!assetPath.empty())
		Font = ga::assets::AssetManager::Global().Load<ga::render::Font>(assetPath);
}

void Text2D::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
}

void Text2D::RenderUi()
{
	Component::RenderUi();

	if (ImGui::InputText("Text", &Text.NonConstRef_Unguarded()))
		Text.TriggerChanged();
}
