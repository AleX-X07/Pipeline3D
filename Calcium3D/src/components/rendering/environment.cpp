#include <c3d/components/rendering/environment.h>
#include <c3d/gameobject.h>

#include <gallium/gpu/descriptorregistry.h>
#include <gallium/render/rendererbase.h>
#include <gallium/assets/assetmanager.h>

using namespace c3d::components::rendering;

Environment::Environment(GameObject* owner)
	: Component(owner)
	, Diffuse(std::nullopt)
	, Specular(std::nullopt)
{
	Intensity = 1.0f;
}

Environment::Environment(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
	Deserialize(deserializer);
}

Environment::~Environment()
{
}

void Environment::Render(ga::render::RendererBase& renderer)
{
	if (!Diffuse && !Specular)
		return;

	uint32_t diffuseId  = Diffuse ? Diffuse->get().GetDescriptorIndex().slot : uint32_t(-1);
	uint32_t specularId = Specular ? Specular->get().GetDescriptorIndex().slot : uint32_t(-1);

	renderer.SetEnvironment(diffuseId, specularId, Intensity());
}


void Environment::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	serializer << (Diffuse
		? ga::assets::AssetManager::Global().PathOf<ga::gpu::Image>(*Diffuse)
		: "");

	serializer << (Specular
		? ga::assets::AssetManager::Global().PathOf<ga::gpu::Image>(*Specular)
		: "");
}

void Environment::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	std::string assetPath;
	deserializer >> assetPath;
	if (!assetPath.empty())
		Diffuse = ga::assets::AssetManager::Global().Load<ga::gpu::Image>(assetPath);

	assetPath.clear();
	deserializer >> assetPath;
	if (!assetPath.empty())
		Specular = ga::assets::AssetManager::Global().Load<ga::gpu::Image>(assetPath);
}

void Environment::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	Component::ResolvePointers(deserializer);
}

void Environment::RenderUi()
{
	Component::RenderUi();

	// TODO not implemented.
}

