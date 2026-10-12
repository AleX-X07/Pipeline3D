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
#include <c3d/graphics/splitscreenrenderer.h>

#include <spherecomponent.h>
#include <cubecomponent.h>
#include <raquettecomponent.h>
#include <cylindercomponent.h>
#include <print>

ExampleGame::ExampleGame()
{
}

void ExampleGame::Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu)
{
	Game::Initialize(platform, gpu);

	//m_renderer = std::make_unique<c3d::graphics::SplitScreenRenderer>(gpu, c3d::graphics::EScreenSplitDirection::Vertical);
	//((c3d::graphics::SplitScreenRenderer*)m_renderer.get())->SetSplitLine(4.0f, { 1.0f, 0.0f, 1.0f });
	m_renderer = std::make_unique<c3d::graphics::DefaultRenderer>(gpu);

	auto mesh4 = m_gameScene.CreateObject("Cylindre");
	{
		auto meshComponent = mesh4->AddComponent<c3d::components::rendering::Mesh>();
		auto createCylinderComponent = mesh4->AddComponent<CylinderComponent>();

		createCylinderComponent->CreateProceduralMesh(gpu, 2.0f, 0.5f, 360, 180);
		meshComponent->Asset = createCylinderComponent->GetMyMesh();

	}

	auto mesh1 = m_gameScene.CreateObject("BoulePlay");
	{
		float r = 0.5f;

		auto renderComponent = mesh1->AddComponent<c3d::components::rendering::Mesh>();
		auto createMeshComponent = mesh1->AddComponent<SphereComponent>();

		createMeshComponent->CreateProceduralMesh(gpu, 2.0f, r, 360, 180);
		renderComponent->Asset = createMeshComponent->GetMyMesh();

		mesh1->Transform->Position = { 4.0f ,0.0f + r, 0.0f };

	}

	auto mesh2 = m_gameScene.CreateObject("Box");
	{
		auto mesh2Component = mesh2->AddComponent<c3d::components::rendering::Mesh>();
		mesh2Component->Asset = ga::assets::AssetManager::Global().Load<ga::render::Mesh>("/assets/plane.glb");
		mesh2->Transform->Position = { 0.0f, 0.0f, 0.0f };
	}

	auto mesh3 = m_gameScene.CreateObject("Raquette");
	{
		float width = 2.0f;
		float height = 1.0f;
		float depth = 2.0f;

		float r = 5.0f;
		glm::vec3 center = m_gameScene.GetObject("Cylindre")->Transform->Position;
		float speed = 5.0f;

		mesh3->Transform->Position = { 0.0f, 0.0f, 4.0f };

		auto renderMeshComponent = mesh3->AddComponent<c3d::components::rendering::Mesh>();
		auto createMeshComponent = mesh3->AddComponent<CubeComponent>();
		auto movementComponent = mesh3->AddComponent<RaquetteComponent>(r, center, speed);

		//createMeshComponent->SetVar(r, center, speed);
		createMeshComponent->CreateProceduralMesh(gpu, width, height, depth);
		renderMeshComponent->Asset = createMeshComponent->GetMyMesh();
	}

	auto target = mesh1;

	auto camera0 = m_gameScene.CreateObject("Camera 0");
	{
		auto cameraComponent = camera0->AddComponent<c3d::components::rendering::Camera>();
		cameraComponent->RendererIndex = 0u;
		//cameraComponent->StretchFactor = 0.5f;
		auto lookAtComponent = camera0->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
		camera0->Transform->Position = { 0.0f, 1.0f, 5.0f };

		auto listenerComponent = camera0->AddComponent<c3d::components::audio::AudioListener>();
	}

	//auto camera1 = m_gameScene.CreateObject("Camera 1");
	//{
	//	auto cameraComponent = camera1->AddComponent<c3d::components::rendering::Camera>();
	//	cameraComponent->RendererIndex = 1u;
	//	cameraComponent->StretchFactor = 0.5f;
	//	auto lookAtComponent = camera1->AddComponent<c3d::components::other::LookAt>();
	//	lookAtComponent->Target = mesh1;
	//	camera1->Transform->Position = { 3.0f, 3.0f, 3.0f };
	//}

	auto envmap = m_gameScene.CreateObject("Envmap");
	{
		auto envComponent = camera0->AddComponent<c3d::components::rendering::Environment>();
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
		lookAtComponent->Target = target;
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
		lookAtComponent->Target = target;
	}

	//auto sound = m_gameScene.CreateObject("Audio");
	//{
	//	auto audioComp = sound->AddComponent<c3d::components::audio::AudioSource>();
	//	audioComp->Sound = ga::assets::AssetManager::Global().Load<ga::audio::SoundHandle>("/assets/evoke26.mp3");
	//}
}

void ExampleGame::Deinitialize()
{
	m_renderer.reset();
	m_gameScene.Clear();
	Game::Deinitialize();
}

void ExampleGame::Update(c3d::GameSubsystems& subsystems, float t, float dt)
{
	auto camera = m_gameScene.GetObject("Camera 0");

	glm::vec3 offset = dt * 10.0f * glm::vec3(
		subsystems.platform.Input().IsHeld(ga::platform::EKey::Left) ? -1.0f : (subsystems.platform.Input().IsHeld(ga::platform::EKey::Right) ? 1.0f : 0.0f),
		subsystems.platform.Input().IsHeld(ga::platform::EKey::P) ? -1.0f : (subsystems.platform.Input().IsHeld(ga::platform::EKey::M) ? 1.0f : 0.0f),
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
