#include <c3d/components/rendering/light.h>

#include <gallium/render/rendererbase.h>
#include <c3d/gameobject.h>

#include <imgui/imgui.h>

#include <string>

using namespace c3d::components::rendering;

struct Light::Internal
{

};

namespace std
{
	std::string to_string(c3d::components::rendering::ELightType lightType)
	{
		switch (lightType)
		{
		case ELightType::Point:       return "Point";
		case ELightType::Spot:        return "Spot";
		case ELightType::Directional: return "Directional";
		}

		return "(unknown)";
	}
}

Light::Light(GameObject* owner)
	: Component(owner)
	, m_lightImpl(new Internal)
{
	Type       = ELightType::Point;
	Color      = glm::vec3(1.0f);
	Intensity  = 1.0f;
	SpotAngles = { 15.0f, 30.0f };
}

Light::~Light() noexcept
{
}

void Light::Render(ga::render::RendererBase& renderer)
{
	renderer.AddLight({
		.type = ([](const decltype(Type())& type) {
			if (type == c3d::components::rendering::ELightType::Point)       return ga::render::ELightType::Point;
			if (type == c3d::components::rendering::ELightType::Spot)        return ga::render::ELightType::Spot;
			if (type == c3d::components::rendering::ELightType::Directional) return ga::render::ELightType::Directional;
			return ga::render::ELightType::Directional;
		})(Type()),
		.color = Color(),
		.intensity = Intensity(),
		.spotAngles = {
			glm::radians(SpotAngles().x),
			glm::radians(SpotAngles().y)
		}
	}, GetGameObject()->Transform->GlobalMatrix());
}

void Light::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << Type;
	serializer << Color;
	serializer << Intensity;
	serializer << SpotAngles;
}

void Light::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	deserializer >> Type;
	deserializer >> Color;
	deserializer >> Intensity;
	deserializer >> SpotAngles;
}

void Light::RenderUi()
{
	Component::RenderUi();

	std::string sCurrentLightType = std::to_string(Type.ConstRef());
	if (ImGui::BeginCombo("Light type", sCurrentLightType.c_str()))
	{
		for (uint32_t t = (uint32_t)ELightType::Point; t <= (uint32_t)ELightType::Directional; ++t)
		{
			bool isActive = (ELightType(t) == Type.ConstRef());
	
			std::string sType = std::to_string(ELightType(t));
			if (ImGui::Selectable(sType.c_str(), isActive))
				Type = ELightType(t);
	
			if (isActive)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	
	if (ImGui::ColorEdit3("Color", (float*)(&Color.NonConstRef_Unguarded())))
		Color.TriggerChanged();
	
	if (ImGui::DragFloat("Intensity", &Intensity.NonConstRef_Unguarded(), 0.1f, 0.0f, 20.0f, "%.2f"))
		Intensity.TriggerChanged();
}
