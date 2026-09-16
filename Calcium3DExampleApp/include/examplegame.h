#ifndef EXAMPLEGAME_H
#define EXAMPLEGAME_H
#pragma once

#include <c3d/game.h>
#include <c3d/gamescene.h>

#include <gallium/render/rendererbase.h>

class ExampleGame
	: public c3d::Game
{
	c3d::GameScene m_gameScene;
	std::unique_ptr<ga::render::RendererBase> m_renderer;

public:
	ExampleGame();

	inline const char* Title() const override   { return "Example Game"; }
	inline glm::uvec3  Version() const override { return { 1, 0, 0 }; }

	void Initialize(ga::platform::Platform& platform, ga::gpu::Device& gpu) override;
	void Deinitialize() override;
	void Update(c3d::GameSubsystems& subsystems, float t, float dt) override;
	std::vector<ga::gpu::SemaphoreId> Render(c3d::GameSubsystems& subsystems, ga::gpu::Image& frameImage, const ga::gpu::CommandEncoder& encoder) override;

	void RenderUi() override;
};

#endif /* EXAMPLEGAME_H */
