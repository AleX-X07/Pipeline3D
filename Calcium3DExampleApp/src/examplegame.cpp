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
#include <behaviourtower.h>

#include <print>
#include <iostream>
#include <vector>

ExampleGame::ExampleGame()
{
}

void ExampleGame::Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu)
{
	Game::Initialize(platform, gpu);

	m_renderer = std::make_unique<c3d::graphics::DefaultRenderer>(gpu);

	auto tower = m_gameScene.CreateObject("Tower");
	{
		// Creating tower
		float hauteur = 5.0f;
		float rayon = 1.0f;
		auto meshComponent = tower->AddComponent<c3d::components::rendering::Mesh>();
		auto createCylinderComponent = tower->AddComponent<CylinderComponent>();
		createCylinderComponent->CreateProceduralMesh(gpu, hauteur, rayon, 360, 180);
		meshComponent->Asset = createCylinderComponent->GetMyMesh();

		// Behaviour
		float hauteurCube = 0.5f;
		auto behaviour = tower->AddComponent<BehaviourTower>();
		tower->Transform->Position->y = hauteur / 2.0f;
		auto posCylinder = tower->Transform->Position;
		behaviour->CreateListOfLineAroundTower(hauteur, gpu, m_gameScene, posCylinder, 4, rayon, hauteurCube);

	}
	auto raquette = m_gameScene.CreateObject("Raquette");
	{
		float width = 2.0f;
		float height = 1.0f;
		float depth = 2.0f;

		float r = 10.0f;
		glm::vec3 center = m_gameScene.GetObject("Tower")->Transform->Position;
		float speed = 1.0f;

		auto renderMeshComponent = raquette->AddComponent<c3d::components::rendering::Mesh>();
		auto createMeshComponent = raquette->AddComponent<CubeComponent>();
		auto movementComponent = raquette->AddComponent<RaquetteComponent>(r, center, speed);

		createMeshComponent->CreateProceduralMesh(gpu, width, height, depth);
		renderMeshComponent->Asset = createMeshComponent->GetMyMesh();
	}

	auto sphere = m_gameScene.CreateObject("Sphere");
	{
		float r = 0.25f;
		float speed = 5.0f;

		auto renderComponent = sphere->AddComponent<c3d::components::rendering::Mesh>();
		auto createMeshComponent = sphere->AddComponent<SphereComponent>(speed);

		createMeshComponent->CreateProceduralMesh(gpu, 2.0f, r, 360, 180, raquette);
		renderComponent->Asset = createMeshComponent->GetMyMesh();

		sphere->Transform->Position = { 3.0f ,0.0f + r, 0.0f };

	}

	auto ground = m_gameScene.CreateObject("Ground");
	{
		auto mesh2Component = ground->AddComponent<c3d::components::rendering::Mesh>();
		auto cube = ground->AddComponent<CubeComponent>();
		cube->CreateProceduralMesh(gpu, 50.0f, 0.50f, 50.0f);
		mesh2Component->Asset = cube->GetMyMesh();
		ground->Transform->Position = { 0.0f, -0.25f, 0.0f };
	}

	// Target de la camera
	auto target = tower;

	auto camera0 = m_gameScene.CreateObject("Camera 0");
	{
		auto cameraComponent = camera0->AddComponent<c3d::components::rendering::Camera>();
		cameraComponent->RendererIndex = 0u;
		auto lookAtComponent = camera0->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
		camera0->Transform->Position = { 0.0f, 1.0f, 5.0f };

		auto listenerComponent = camera0->AddComponent<c3d::components::audio::AudioListener>();
	}

	auto envmap = m_gameScene.CreateObject("Envmap");
	{
		auto envComponent = camera0->AddComponent<c3d::components::rendering::Environment>();
		envComponent->Diffuse = ga::assets::AssetManager::Global().Load<ga::gpu::Image>("/assets/envmap/Tokyo_BigSight_Env.hdr");
		envComponent->Specular = ga::assets::AssetManager::Global().Load<ga::gpu::Image>("/assets/envmap/Tokyo_BigSight_3k.hdr");
		envComponent->Intensity = 0.03f;
	}

	// Light
	float hauterLight = 1.0f;
	auto light1 = m_gameScene.CreateObject("Light1");
	{
		auto lightComp = light1->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(1.0f, 0.6f, 0.4f);
		lightComp->Intensity = 40.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 20.0f, 20.0f };
		light1->Transform->Position = { 0.0f, hauterLight, -5.0f };

		auto lookAtComponent = light1->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
	}

	auto light2 = m_gameScene.CreateObject("Light2");
	{
		auto lightComp = light2->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(1.0f, 0.6f, 0.4f);
		lightComp->Intensity = 40.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 20.0f, 20.0f };
		light2->Transform->Position = { 0.0f, hauterLight, 5.0f };

		auto lookAtComponent = light2->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
	}

	auto light3 = m_gameScene.CreateObject("Light3");
	{
		auto lightComp = light3->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(1.0f, 0.6f, 0.4f);
		lightComp->Intensity = 40.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 20.0f, 20.0f };
		light3->Transform->Position = { 5.0f, hauterLight, 0.0f };

		auto lookAtComponent = light3->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
	}

	auto light4 = m_gameScene.CreateObject("Light4");
	{
		auto lightComp = light4->AddComponent<c3d::components::rendering::Light>();
		lightComp->Color = glm::vec3(1.0f, 0.6f, 0.4f);
		lightComp->Intensity = 40.0f;
		lightComp->Type = c3d::components::rendering::ELightType::Spot;
		lightComp->SpotAngles = { 20.0f, 20.0f };
		light4->Transform->Position = { -5.0f, 5.0f, 0.0f };

		auto lookAtComponent = light4->AddComponent<c3d::components::other::LookAt>();
		lookAtComponent->Target = target;
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
	m_gameScene.Update(subsystems);
	
	// Object
	auto tower = m_gameScene.GetObject("Tower");
	auto sphere = m_gameScene.GetObject("Sphere");
	auto raquette = m_gameScene.GetObject("Raquette");

	// Collision avec la raquette
	auto sphereComp = sphere->GetComponent<SphereComponent>();
	auto cubeComp = raquette->GetComponent<CubeComponent>();

	auto SphereInRaquette = raquette->Transform->DetransformPoint(sphere->Transform->Position());

	bool touchingRaquette = sphereComp->OnCollisionCube(SphereInRaquette, cubeComp->GetSize(), sphereComp->GetRadius());
	sphereComp->HandleRaquetteCollision(touchingRaquette);

	// Collision avec la tour
	auto towerComp = tower->GetComponent<CylinderComponent>();

	bool touchingTower = sphereComp->OnCollisionCylinder(sphere->Transform->Position(), tower->Transform->Position(), sphereComp->GetRadius(), towerComp->GetRadius());
	sphereComp->HandleCylinderCollision(touchingTower,tower);

	// Destruction des bloc
	auto behaviour = tower->GetComponent<BehaviourTower>();
	behaviour->DestroyBloc(m_gameScene, sphere);
	float raquetteAngle = raquette->GetComponent<RaquetteComponent>()->angle;

	// Camera
	auto camera = m_gameScene.GetObject("Camera 0");
	glm::vec3 raquettePos = raquette->Transform->Position();
	camera->Transform->Position = raquettePos * 1.2f + glm::vec3(0.0f, 1.2f, 0.0f);
	camera->Transform->Orientation = glm::quat({ -glm::pi<float>() / 12.0f, glm::pi<float>() / 2.0f - raquetteAngle, 0.0f });
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
