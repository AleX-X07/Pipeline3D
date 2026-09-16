#include <c3d/game.h>

#include <gallium/core/messagebus.h>
#include <gallium/core/log.h>

#include <gallium/platform/platform.h>
#include <gallium/platform/timer.h>
#include <gallium/gpu/device.h>
#include <gallium/gpu/commandbuffer.h>
#include <gallium/audio/audiosystem.h>

#include <c3d/componentfactory.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_gallium.h>

#include <c3d/components/audio/audiosourcecomponent.h>
#include <c3d/components/audio/audiolistenercomponent.h>
#include <c3d/components/physics/rigidbody.h>
#include <c3d/components/physics/boxcollider.h>
#include <c3d/components/physics/capsulecollider.h>
#include <c3d/components/physics/spherecollider.h>
#include <c3d/components/rendering/camera.h>
#include <c3d/components/rendering/light.h>
#include <c3d/components/rendering/mesh.h>
#include <c3d/components/other/lookat.h>

using namespace c3d;

int32_t Game::m_Run(Game& game)
{
	auto platform = std::make_unique<ga::platform::Platform>(ga::platform::PlatformInfo {
		.appName              = game.Title(),
		.appVersion           = game.Version(),
		.requestedSurfaceSize = { 1600u, 900u }
	});
	auto gpu = std::make_unique<ga::gpu::Device>(*platform);
	
#ifdef C3D_ENABLE_UI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGallium_Init({
		.platform    = *platform,
		.device      = *gpu,
		.colorFormat = ga::gpu::EFormat::B8G8R8A8_SRGB
	});
#endif /* C3D_ENABLE_UI */

	game.Initialize(*platform, *gpu);

	platform->Timer().Reset();

	std::unique_ptr<ga::gpu::Image> backbuffer;

	GameSubsystems subsystems = { *platform, *gpu, ga::assets::AssetManager::Global(), *game.m_audio };

	do
	{
		platform->PollEvents();
		game.Update(subsystems, float(platform->Timer().ElapsedTime()), float(platform->Timer().DeltaTime()));

		auto currentFrame = gpu->AcquireNextImage();
		auto* frameCb = gpu->AcquireCommandBuffer();

		bool recreateBackbuffer = false;
		if (!backbuffer)
			recreateBackbuffer = true;
		else
			if (backbuffer->Size() != currentFrame.image.Size())
				recreateBackbuffer = true;

		if (recreateBackbuffer)
		{
			gpu->Defer([bb = std::move(backbuffer)] {});

			backbuffer = std::make_unique<ga::gpu::Image>(*gpu, ga::gpu::ImageInfo {
				.type         = ga::gpu::EImageType::Image2D,
				.format       = ga::gpu::EFormat::R16G16B16A16_SFloat,
				.width        = currentFrame.image.Size().x,
				.height       = currentFrame.image.Size().y,
				.usage        = ga::gpu::EImageUsage::ColorAttachment | ga::gpu::EImageUsage::TransferSrc | ga::gpu::EImageUsage::Sampled | ga::gpu::EImageUsage::Storage,
				.samplingMode = ga::gpu::EImageSamplingMode::ClampToEdge,
				.debugName    = "Backbuffer"
			});
		}

#ifdef C3D_ENABLE_UI
		ImGui_ImplGallium_NewFrame();
		ImGui::NewFrame();
#endif /* C3D_ENABLE_UI */

		std::vector<ga::gpu::SemaphoreId> waitSemaphores;

		frameCb->Record([&](const ga::gpu::CommandEncoder& encoder) {
			waitSemaphores = game.Render(subsystems, *backbuffer, encoder);

			encoder.Transition(*backbuffer, ga::gpu::EImageLayout::TransferSrcOptimal);
			encoder.Transition(currentFrame.image, ga::gpu::EImageLayout::TransferDstOptimal);
			encoder.Transfer([&](const ga::gpu::TransferEncoder& transfer) {
				transfer.BlitImage(*backbuffer, currentFrame.image, { 0u, 0u, 0u }, { 0u, 0u, 0u }, backbuffer->Size());
			});
			encoder.Transition(currentFrame.image, ga::gpu::EImageLayout::PresentSrc);

		}, ga::gpu::ECommandBufferUsage::OneTimeSubmit);
		waitSemaphores.push_back(currentFrame.presentComplete);

		auto frameComplete = *gpu->Submit(frameCb, waitSemaphores);
		gpu->ReleaseCommandBuffer(frameCb);

#ifdef C3D_ENABLE_UI
		game.RenderUi();

		ImGui::Render();
		ImGui_ImplGallium_Render(ImGui_ImplGallium_RenderInfo{
			.targetImage = currentFrame.image,
			.waitOps = { frameComplete },
			.signalOp = currentFrame.renderComplete,
			.postRender = [&](const ga::gpu::CommandEncoder& encoder) {
				encoder.Transition(currentFrame.image, ga::gpu::EImageLayout::PresentSrc);
			}
		});
#endif /* C3D_ENABLE_UI */

		gpu->Present({ currentFrame.renderComplete });
	} while (!platform->IsExitRequested());

	gpu->Defer([bb = std::move(backbuffer)] {});
	gpu->WaitIdle();
	game.Deinitialize();

#ifdef C3D_ENABLE_UI
	ImGui_ImplGallium_Shutdown();
	ImGui::DestroyContext();
#endif /* C3D_ENABLE_UI */

	return 0;
}

Game::~Game() = default;

void Game::Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu)
{
	ga::core::MessageBus::Global().Initialize();
	ga::core::LogSink_Console::Global().Initialize();
	ga::core::LogSink_Console::Global().SetMinLogLevel(ga::core::ELogLevel::Info);

	c3d::ComponentFactory::Global().Initialize();

	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::audio::AudioSource>("Audio");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::audio::AudioListener>("Audio");

	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::physics::Rigidbody>("Physics");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::physics::BoxCollider>("Physics");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::physics::CapsuleCollider>("Physics");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::physics::SphereCollider>("Physics");
	//c3d::ComponentFactory::Global().RegisterComponent<c3d::components::physics::MeshCollider>("Physics");

	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::rendering::Camera>("Rendering");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::rendering::Light>("Rendering");
	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::rendering::Mesh>("Rendering");

	c3d::ComponentFactory::Global().RegisterComponent<c3d::components::other::LookAt>("Other");

	m_audio = std::make_unique<ga::audio::AudioSystem>(platform);

	ga::assets::AssetManager::Create(ga::assets::AssetManagerCreateInfo{
		.platform    = platform,
		.gpu         = gpu,
		.audioSystem = *m_audio
	});

	GA_LOG_INFO("Calcium3D", "Initialization completed");
}

void Game::Deinitialize()
{
	c3d::ComponentFactory::Global().Terminate();

	ga::assets::AssetManager::Destroy();
	m_audio.reset();
}
