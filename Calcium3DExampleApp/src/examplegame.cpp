#include <examplegame.h>

#include <gallium/platform/platform.h>
#include <gallium/platform/input.h>
#include <gallium/gpu/device.h>
#include <gallium/gpu/commandbuffer.h>

#include <c3d/gameobject.h>
#include <c3d/components/audio/audiosourcecomponent.h>
#include <c3d/components/audio/audiolistenercomponent.h>
#include <c3d/components/rendering/camera.h>
#include <c3d/components/rendering/light.h>
#include <c3d/components/rendering/mesh.h>
#include <c3d/components/rendering/environment.h>
#include <c3d/components/other/lookat.h>
#include <c3d/graphics/defaultrenderer.h>

#include <print>

ExampleGame::ExampleGame()
{
}

void ExampleGame::Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu)
{
	Game::Initialize(platform, gpu);

	m_renderer = std::make_unique<c3d::graphics::DefaultRenderer>(gpu);

	auto mesh1 = m_gameScene.CreateObject("Damaged Helmet");
	{
		auto mesh1Component = mesh1->AddComponent<c3d::components::rendering::Mesh>();
		mesh1Component->Asset = ga::assets::AssetManager::Global().Load<ga::render::Mesh>("/assets/DamagedHelmet.glb");
	}

	auto mesh2 = m_gameScene.CreateObject("Box");
	{
		auto mesh2Component = mesh2->AddComponent<c3d::components::rendering::Mesh>();
		mesh2Component->Asset = ga::assets::AssetManager::Global().Load<ga::render::Mesh>("/assets/plane.glb");
		mesh2->Transform->Position = { 0.0f, -1.0f, 0.0f };
	}

	auto camera = m_gameScene.CreateObject("Camera");
	{
		auto cameraComponent = camera->AddComponent<c3d::components::rendering::Camera>();
		auto lookAtComponent = camera->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = mesh1;
		camera->Transform->Position = { 0.0f, 1.0f, 5.0f };

		auto listenerComponent = camera->AddComponent<c3d::components::audio::AudioListener>();
	}

	auto envmap = m_gameScene.CreateObject("Envmap");
	{
		auto envComponent = camera->AddComponent<c3d::components::rendering::Environment>();
		envComponent->Diffuse = ga::assets::AssetManager::Global().Load<ga::gpu::Image>("/assets/envmap/Tokyo_BigSight_Env.hdr");
		envComponent->Specular = ga::assets::AssetManager::Global().Load<ga::gpu::Image>("/assets/envmap/Tokyo_BigSight_3k.hdr");
		envComponent->Intensity = 0.03f;
	}

	auto light1 = m_gameScene.CreateObject("Light1");
	{
		auto lightComp = light1->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(1.0f, 0.6f, 0.4f);
		lightComp->Intensity = 20.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 12.0f, 15.0f };
		light1->Transform->Position = { -3.0f, 3.0f, -3.0f };

		auto lookAtComponent = light1->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = mesh1;
	}

	auto light2 = m_gameScene.CreateObject("Light2");
	{
		auto lightComp = light2->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(0.4f, 0.6f, 1.0f);
		lightComp->Intensity = 20.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 12.0f, 15.0f };
		light2->Transform->Position = { -3.0f, 3.0f, 3.0f };

		auto lookAtComponent = light2->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = mesh1;
	}

	auto sound = m_gameScene.CreateObject("Audio");
	{
		auto audioComp = sound->AddComponent<c3d::components::audio::AudioSource>();
		audioComp->Sound = ga::assets::AssetManager::Global().Load<ga::audio::SoundHandle>("/assets/evoke26.mp3");
	}
}

void ExampleGame::Deinitialize()
{
	m_renderer.reset();
	m_gameScene.Clear();
	Game::Deinitialize();
}

void ExampleGame::Update(c3d::GameSubsystems& subsystems, float t, float dt)
{
	auto camera = m_gameScene.GetObject("Camera");

	glm::vec3 offset = dt * 10.0f * glm::vec3(
		subsystems.platform.Input().IsHeld(ga::platform::EKey::Left) ? -1.0f : (subsystems.platform.Input().IsHeld(ga::platform::EKey::Right) ? 1.0f : 0.0f),
		0.0f,
		subsystems.platform.Input().IsHeld(ga::platform::EKey::Up) ? -1.0f : (subsystems.platform.Input().IsHeld(ga::platform::EKey::Down) ? 1.0f : 0.0f)
	);

	camera->Transform->Position = camera->Transform->Position() + offset;

	m_gameScene.Update(subsystems);

	if (subsystems.platform.Input().IsHeld(ga::platform::EMouseButton::Left))
	{
		auto [ro, rd] = camera->GetComponent<c3d::components::rendering::Camera>()->Deproject(subsystems.platform.Input().MousePosition());
		std::println("ro: ({}, {}, {}) / rd: ({}, {}, {})", ro.x, ro.y, ro.z, rd.x, rd.y, rd.z);
	}
}

std::vector<ga::gpu::SemaphoreId> ExampleGame::Render(c3d::GameSubsystems& subsystems, ga::gpu::Image& frameImage, const ga::gpu::CommandEncoder& encoder)
{
	m_gameScene.Render(*m_renderer);
	return m_renderer->Render(subsystems.gpu, encoder, frameImage);
}

void ExampleGame::RenderUi()
{
	m_gameScene.RenderUi();
}
