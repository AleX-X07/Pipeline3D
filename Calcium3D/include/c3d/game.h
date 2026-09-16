#ifndef CALCIUM3D__GAME_H
#define CALCIUM3D__GAME_H
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/vec3.hpp>

#include <gallium/gpu/enums.h>
#include <gallium/audio/audiosystem.h>
#include <gallium/assets/assetmanager.h>

namespace ga::platform { class Platform; }
namespace ga::gpu      { class Device; class CommandBuffer; class Image; }

#define C3D_ENABLE_UI

namespace c3d
{
	struct GameSubsystems
	{
		ga::platform::Platform&   platform;
		ga::gpu::Device&          gpu;
		ga::assets::AssetManager& assets;
		ga::audio::AudioSystem&   audio;
	};

	class Game
	{
		static int32_t m_Run(Game& game);

	protected:
		std::unique_ptr<ga::audio::AudioSystem> m_audio;

	public:
		virtual ~Game();

		virtual const char* Title() const = 0;
		virtual glm::uvec3  Version() const = 0;

		virtual void Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu);
		virtual void Deinitialize();
		virtual void Update(GameSubsystems& platform, float t, float dt) = 0;
		virtual std::vector<ga::gpu::SemaphoreId> Render(GameSubsystems& gpu, ga::gpu::Image& frameImage, const ga::gpu::CommandEncoder& encoder) = 0;

		virtual void RenderUi() {}

		template<typename T, typename... ARGS>
		static int32_t Run(ARGS... args)
		{
			return m_Run(*std::make_unique<T>(args...));
		}
	};
}

#endif /* CALCIUM3D__GAME_H */
