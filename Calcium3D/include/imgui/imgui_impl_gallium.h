#ifndef IMGUI__IMPL__GALLIUM_H
#define IMGUI__IMPL__GALLIUM_H
#pragma once

#include <imgui/imgui.h>

#include <gallium/string_hash.h>
#include <gallium/gpu/device.h>
#include <gallium/gpu/enums.h>

#include <unordered_map>

namespace ga::gpu      { class Device; class CommandEncoder; class Image; }
namespace ga::platform { class Platform; }

struct ImGui_ImplGallium_InitInfo
{
	ga::platform::Platform& platform;
	ga::gpu::Device&        device;
	ga::gpu::EFormat        colorFormat;

	struct FontInfo
	{
		void*          font_data;
		int            font_data_size;
		float          size_pixels;
		bool           monospace = false;
		const ImWchar* glyph_ranges;
	};

	std::optional<FontInfo>                     defaultFont = std::nullopt;
	std::unordered_map<string_hash_t, FontInfo> additionalFonts = {};
};

bool ImGui_ImplGallium_Init(const ImGui_ImplGallium_InitInfo& initInfo);
void ImGui_ImplGallium_Shutdown();
void ImGui_ImplGallium_NewFrame();
ImFont* ImGui_ImplGallium_GetFont(string_hash_t id);

std::vector<ga::gpu::SemaphoreId> ImGui_ImplGallium_RenderDrawData(ImDrawData* drawData, const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& targetImage);

struct ImGui_ImplGallium_RenderInfo
{
	ga::gpu::Image&                                     targetImage;
	std::vector<ga::gpu::SemaphoreId>                   waitOps;
	std::optional<ga::gpu::SemaphoreId>                 signalOp;
	std::function<void(const ga::gpu::CommandEncoder&)> preRender;
	std::function<void(const ga::gpu::CommandEncoder&)> postRender;
};

std::optional<ga::gpu::SemaphoreId> ImGui_ImplGallium_Render(const ImGui_ImplGallium_RenderInfo& drawData);

#endif /* IMGUI__IMPL__GALLIUM_H */
