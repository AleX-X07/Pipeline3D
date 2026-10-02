#include <imgui/imgui_impl_gallium.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <imgui/mdi_glyphs.h>
#include <imgui/mdi_ttf.h>

#include <gallium/gpu/device.h>
#include <gallium/gpu/buffer.h>
#include <gallium/gpu/image.h>
#include <gallium/gpu/graphicspipeline.h>
#include <gallium/gpu/commandbuffer.h>
#include <gallium/gpu/shadercache.h>
#include <gallium/gpu/shadermodule.h>
#include <gallium/gpu/shadermodule_reflection.h>
#include <gallium/gpu/descriptorregistry.h>
#include <gallium/platform/platform.h>
#include <gallium/platform/vfs.h>
#include <gallium/platform/input.h>
#include <gallium/platform/timer.h>

#include <glm/glm.hpp>
#include <cassert>
#include <iterator>
#include <memory>
#include <vector>

struct ImGui_ImplGallium_Data
{
    ga::gpu::Device* device = nullptr;
    ga::platform::Platform* platform = nullptr;

    // Font atlas
    std::unique_ptr<ga::gpu::Image>  fontImage;
    ImTextureID                      fontTextureId = 0u;

    // Pipeline
    std::unique_ptr<ga::gpu::GraphicsPipeline> pipeline;

    // Per-frame geometry - grown on demand, never shrunk
    struct PerFrameData
    {
        std::unique_ptr<ga::gpu::Buffer> vertexBuffer     = nullptr;
        std::unique_ptr<ga::gpu::Buffer> indexBuffer      = nullptr;
        int                              vertexBufferSize = 0;
        int                              indexBufferSize  = 0;
    };
    std::vector<PerFrameData> perFrameData;
    uint32_t                  currentFrame = 0;

    // Push constant layout - grabbed from shader reflection
    ga::gpu::ShaderStructDesc pcDesc;

    // Input - track previous state for ImGui
    double lastTime = 0.0;

    ImFont*                                    defaultFont;
    std::unordered_map<string_hash_t, ImFont*> fonts;
};

static ImGui_ImplGallium_Data* ImGui_ImplGallium_GetBackendData()
{
    return ImGui::GetCurrentContext()
        ? static_cast<ImGui_ImplGallium_Data*>(ImGui::GetIO().BackendRendererUserData)
        : nullptr;
}

static void ImGui_ImplGallium_CreateFontsTexture(
    const std::optional<ImGui_ImplGallium_InitInfo::FontInfo>& defaultFont,
    const std::unordered_map<string_hash_t, ImGui_ImplGallium_InitInfo::FontInfo>& additionalFonts
) {
    auto* bd = ImGui_ImplGallium_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;

    ImFontConfig config {};
    config.FontDataOwnedByAtlas = false;

    float defaultFontSize = defaultFont.has_value() ? defaultFont->size_pixels : 13.0f;

    if (!defaultFont.has_value())
        bd->defaultFont = io.Fonts->AddFontDefaultVector();
    else
        bd->defaultFont = io.Fonts->AddFontFromMemoryTTF(defaultFont->font_data, defaultFont->font_data_size, defaultFontSize, &config, defaultFont->glyph_ranges);

    {
        constexpr ImWchar mdi_icon_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };

        ImFontConfig mdiConfig {};
        mdiConfig.FontDataOwnedByAtlas = false;
        mdiConfig.PixelSnapH           = true;
        mdiConfig.MergeMode            = true;
        mdiConfig.GlyphMinAdvanceX     = defaultFontSize; // Use if you want to make the icon monospaced
        io.Fonts->AddFontFromMemoryTTF(mdi_ttf_bytes, mdi_ttf_size, defaultFontSize, &mdiConfig, mdi_icon_ranges);
    }

    for (const auto& [id, font] : additionalFonts)
    {
        if (font.monospace)
        {
            config.GlyphMinAdvanceX = font.size_pixels;
            config.GlyphMaxAdvanceX = font.size_pixels;
        }
        else
        {
            config.GlyphMinAdvanceX = 0;
            config.GlyphMaxAdvanceX = FLT_MAX;
        }

        bd->fonts[id] = io.Fonts->AddFontFromMemoryTTF(font.font_data, font.font_data_size, font.size_pixels, &config, font.glyph_ranges);
    }

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    bd->fontImage = std::make_unique<ga::gpu::Image>(*bd->device, ga::gpu::ImageInfo {
        .type        = ga::gpu::EImageType::Image2D,
        .format      = ga::gpu::EFormat::R8G8B8A8_SRGB,
        .width       = size_t(width),
        .height      = size_t(height),
        .usage       = ga::gpu::EImageUsage::Sampled,
        .initialData = pixels,
    });
    bd->fontImage->SetDebugName("ImGui font atlas");

    bd->fontTextureId = bd->fontImage->GetDescriptorIndex().slot;
    io.Fonts->SetTexID(bd->fontTextureId);
}

static void ImGui_ImplGallium_CreatePipeline(ga::gpu::EFormat colorFormat)
{
    auto* bd = ImGui_ImplGallium_GetBackendData();

    auto& shader = bd->device->GetShaderCache().Get("/shaders/imgui/imgui.slang.spv");

    bd->pcDesc = *shader.GetStruct("PushConstants");

    // ImDrawVert layout: pos(float2) uv(float2) col(uint32 packed as R8G8B8A8)
    bd->pipeline = std::make_unique<ga::gpu::GraphicsPipeline>(*bd->device,
        ga::gpu::GraphicsPipelineInfo {
            .stages = {
                { shader, ga::gpu::EShaderStage::Vertex,   "vs_main" },
                { shader, ga::gpu::EShaderStage::Fragment, "fs_main" },
            },
            .vertexInput = {
                .bindings = {
                    { 0u, sizeof(ImDrawVert), ga::gpu::EVertexBindingRate::Vertex }
                },
                .attributes = {
                    { 0, 0, ga::gpu::EFormat::R32G32_SFloat,  offsetof(ImDrawVert, pos) },
                    { 1, 0, ga::gpu::EFormat::R32G32_SFloat,  offsetof(ImDrawVert, uv)  },
                    { 2, 0, ga::gpu::EFormat::R8G8B8A8_UNorm, offsetof(ImDrawVert, col) }
                }
            },
            .attachmentFormats = {
                .colorFormats = { colorFormat }
            },
            .blending = {
                .enabled        = true,
                .srcColorFactor = ga::gpu::EBlendFactor::SrcAlpha,
                .dstColorFactor = ga::gpu::EBlendFactor::OneMinusSrcAlpha,
                .colorOp        = ga::gpu::EBlendOp::Add,
                .srcAlphaFactor = ga::gpu::EBlendFactor::One,
                .dstAlphaFactor = ga::gpu::EBlendFactor::OneMinusSrcAlpha,
                .alphaOp        = ga::gpu::EBlendOp::Add,
            },
        });

    bd->perFrameData.resize(bd->device->FrameCount());
}

static ImGuiKey GalliumKeyToImGui(ga::platform::EKey key)
{
    using K = ga::platform::EKey;
    switch (key)
    {
    case K::Tab:          return ImGuiKey_Tab;
    case K::Left:         return ImGuiKey_LeftArrow;
    case K::Right:        return ImGuiKey_RightArrow;
    case K::Up:           return ImGuiKey_UpArrow;
    case K::Down:         return ImGuiKey_DownArrow;
    case K::PageUp:       return ImGuiKey_PageUp;
    case K::PageDown:     return ImGuiKey_PageDown;
    case K::Home:         return ImGuiKey_Home;
    case K::End:          return ImGuiKey_End;
    case K::Insert:       return ImGuiKey_Insert;
    case K::Delete:       return ImGuiKey_Delete;
    case K::Backspace:    return ImGuiKey_Backspace;
    case K::Space:        return ImGuiKey_Space;
    case K::Enter:        return ImGuiKey_Enter;
    case K::Escape:       return ImGuiKey_Escape;
    case K::LeftControl:  return ImGuiKey_LeftCtrl;
    case K::LeftShift:    return ImGuiKey_LeftShift;
    case K::LeftAlt:      return ImGuiKey_LeftAlt;
    case K::LeftSuper:    return ImGuiKey_LeftSuper;
    case K::RightControl: return ImGuiKey_RightCtrl;
    case K::RightShift:   return ImGuiKey_RightShift;
    case K::RightAlt:     return ImGuiKey_RightAlt;
    case K::RightSuper:   return ImGuiKey_RightSuper;
    case K::A: return ImGuiKey_A; case K::B: return ImGuiKey_B;
    case K::C: return ImGuiKey_C; case K::D: return ImGuiKey_D;
    case K::E: return ImGuiKey_E; case K::F: return ImGuiKey_F;
    case K::G: return ImGuiKey_G; case K::H: return ImGuiKey_H;
    case K::I: return ImGuiKey_I; case K::J: return ImGuiKey_J;
    case K::K: return ImGuiKey_K; case K::L: return ImGuiKey_L;
    case K::M: return ImGuiKey_M; case K::N: return ImGuiKey_N;
    case K::O: return ImGuiKey_O; case K::P: return ImGuiKey_P;
    case K::Q: return ImGuiKey_Q; case K::R: return ImGuiKey_R;
    case K::S: return ImGuiKey_S; case K::T: return ImGuiKey_T;
    case K::U: return ImGuiKey_U; case K::V: return ImGuiKey_V;
    case K::W: return ImGuiKey_W; case K::X: return ImGuiKey_X;
    case K::Y: return ImGuiKey_Y; case K::Z: return ImGuiKey_Z;
    case K::Num0: return ImGuiKey_0; case K::Num1: return ImGuiKey_1;
    case K::Num2: return ImGuiKey_2; case K::Num3: return ImGuiKey_3;
    case K::Num4: return ImGuiKey_4; case K::Num5: return ImGuiKey_5;
    case K::Num6: return ImGuiKey_6; case K::Num7: return ImGuiKey_7;
    case K::Num8: return ImGuiKey_8; case K::Num9: return ImGuiKey_9;
    case K::F1:  return ImGuiKey_F1;  case K::F2:  return ImGuiKey_F2;
    case K::F3:  return ImGuiKey_F3;  case K::F4:  return ImGuiKey_F4;
    case K::F5:  return ImGuiKey_F5;  case K::F6:  return ImGuiKey_F6;
    case K::F7:  return ImGuiKey_F7;  case K::F8:  return ImGuiKey_F8;
    case K::F9:  return ImGuiKey_F9;  case K::F10: return ImGuiKey_F10;
    case K::F11: return ImGuiKey_F11; case K::F12: return ImGuiKey_F12;
    default: return ImGuiKey_None;
    }
}

static void ImGui_ImplGallium_UpdateKeyState(ImGuiIO& io, ga::platform::Input& input, ga::platform::EKey key)
{
    const ImGuiKey imkey = GalliumKeyToImGui(key);
    if (imkey == ImGuiKey_None)
        return;

    io.AddKeyEvent(imkey, input.IsHeld(key));
}


void ImGui_ImplGallium_UpdateTexture(ImTextureData* tex)
{
    auto* bd = ImGui_ImplGallium_GetBackendData();

    if (tex->Status == ImTextureStatus_WantCreate)
    {
        auto image = std::make_unique<ga::gpu::Image>(*bd->device, ga::gpu::ImageInfo {
            .type = ga::gpu::EImageType::Image2D,
            .format = ga::gpu::EFormat::R8G8B8A8_UNorm,
            .width = size_t(tex->Width),
            .height = size_t(tex->Height),
            .usage = ga::gpu::EImageUsage::Sampled | ga::gpu::EImageUsage::TransferDst
        });
        image->SetDebugName("ImGui resource");

        auto* previousTexture = (ga::gpu::Image*)(tex->BackendUserData);
        if (previousTexture)
        {
            auto descriptorIndex = previousTexture->GetDescriptorIndex();
            bd->device->GetDescriptorRegistry().Unregister(descriptorIndex);
            delete previousTexture;
        }

        auto imagePtr = image.release();
        imagePtr->SetDescriptorIndex(bd->device->GetDescriptorRegistry().Register(*imagePtr));
        tex->BackendUserData = imagePtr;
        tex->SetTexID(imagePtr->GetDescriptorIndex().slot);
    }

    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
    {
        auto* image = (ga::gpu::Image*)(tex->BackendUserData);

        const int upload_x = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.x;
        const int upload_y = (tex->Status == ImTextureStatus_WantCreate) ? 0 : tex->UpdateRect.y;
        const int upload_w = (tex->Status == ImTextureStatus_WantCreate) ? tex->Width : tex->UpdateRect.w;
        const int upload_h = (tex->Status == ImTextureStatus_WantCreate) ? tex->Height : tex->UpdateRect.h;

        size_t upload_pitch = upload_w * tex->BytesPerPixel;

        const size_t sizeInBytes = upload_w * upload_h * 4;
        std::vector<uint8_t> bytes;
        bytes.resize(upload_w * upload_h * 4);

        for (int y = 0; y < upload_h; y++)
            std::copy_n((char*)tex->GetPixelsAt(upload_x, upload_y + y), upload_pitch, bytes.data() + upload_pitch * y);

        image->Upload(*bd->device, { upload_x, upload_y, 0 }, { upload_w, upload_h, 1 }, tex->GetPixelsAt(upload_x, upload_y));
    }

    if (tex->Status == ImTextureStatus_WantDestroy)
    {
        auto* previousImage = (ga::gpu::Image*)(tex->BackendUserData);
        if (previousImage)
        {
            auto descriptorIndex = previousImage->GetDescriptorIndex();
            bd->device->GetDescriptorRegistry().Unregister(descriptorIndex);
            previousImage->SetDescriptorIndex({});
            delete previousImage;
        }
        tex->SetTexID(~0);
    }

    tex->SetStatus(ImTextureStatus_OK);
}


bool ImGui_ImplGallium_Init(const ImGui_ImplGallium_InitInfo& initInfo)
{
    ImGuiIO& io = ImGui::GetIO();
    assert(io.BackendRendererUserData == nullptr && "Already initialized");

    auto* bd = new ImGui_ImplGallium_Data();
    bd->device = &initInfo.device;
    bd->platform = &initInfo.platform;

    io.BackendRendererUserData = bd;
    io.BackendRendererName = "imgui_impl_gallium";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // Display size
    const auto surfaceSize = initInfo.platform.SurfaceSize();
    io.DisplaySize = ImVec2(float(surfaceSize.x), float(surfaceSize.y));

    ImGui_ImplGallium_CreateFontsTexture(initInfo.defaultFont, initInfo.additionalFonts);
    ImGui_ImplGallium_CreatePipeline(initInfo.colorFormat);

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;            // App is sRGB-aware
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::StyleColorsDark();

    return true;
}

void ImGui_ImplGallium_Shutdown()
{
    auto* bd = ImGui_ImplGallium_GetBackendData();
    assert(bd && "Not initialized or already shut down");

    ImGuiIO& io = ImGui::GetIO();

    bd->device->WaitIdle();

    bd->pipeline.reset();
    bd->fontImage.reset();

    for (auto& pfd : bd->perFrameData)
    {
        pfd.vertexBuffer.reset();
        pfd.indexBuffer.reset();
    }

    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
        {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplGallium_UpdateTexture(tex);
        }

    io.BackendRendererUserData = nullptr;
    io.BackendRendererName = nullptr;
    io.Fonts->SetTexID(0);

    delete bd;
}

void ImGui_ImplGallium_NewFrame()
{
    auto* bd = ImGui_ImplGallium_GetBackendData();
    assert(bd && "Not initialized");

    ImGuiIO& io = ImGui::GetIO();
    auto& input = bd->platform->Input();

    // Display size - update every frame in case of resize
    const auto surfaceSize = bd->platform->SurfaceSize();
    io.DisplaySize = ImVec2(float(surfaceSize.x), float(surfaceSize.y));

    // Delta time
    const double now = bd->platform->Timer().ElapsedTime();
    io.DeltaTime = bd->lastTime > 0.0 ? float(now - bd->lastTime) : float(1.0 / 60.0);
    bd->lastTime = now;

    // Mouse position
    const auto mousePos = input.MousePosition();
    io.AddMousePosEvent(mousePos.x, mousePos.y);

    // Mouse buttons
    io.AddMouseButtonEvent(0, input.IsHeld(ga::platform::EMouseButton::Left));
    io.AddMouseButtonEvent(1, input.IsHeld(ga::platform::EMouseButton::Right));
    io.AddMouseButtonEvent(2, input.IsHeld(ga::platform::EMouseButton::Middle));

    for (const auto& c : input.InputChars())
        io.AddInputCharacter(c);

    // Scroll
    const auto scroll = input.ScrollDelta();
    if (scroll.x != 0.f || scroll.y != 0.f)
        io.AddMouseWheelEvent(scroll.x, scroll.y);

    // Keyboard - modifiers first
    io.AddKeyEvent(ImGuiMod_Ctrl, input.IsHeld(ga::platform::EKey::LeftControl)
        || input.IsHeld(ga::platform::EKey::RightControl));
    io.AddKeyEvent(ImGuiMod_Shift, input.IsHeld(ga::platform::EKey::LeftShift)
        || input.IsHeld(ga::platform::EKey::RightShift));
    io.AddKeyEvent(ImGuiMod_Alt, input.IsHeld(ga::platform::EKey::LeftAlt)
        || input.IsHeld(ga::platform::EKey::RightAlt));
    io.AddKeyEvent(ImGuiMod_Super, input.IsHeld(ga::platform::EKey::LeftSuper)
        || input.IsHeld(ga::platform::EKey::RightSuper));

    // All tracked keys
    static constexpr ga::platform::EKey k_TrackedKeys[] = {
        ga::platform::EKey::Tab,
        ga::platform::EKey::Left,    ga::platform::EKey::Right,
        ga::platform::EKey::Up,      ga::platform::EKey::Down,
        ga::platform::EKey::PageUp,  ga::platform::EKey::PageDown,
        ga::platform::EKey::Home,    ga::platform::EKey::End,
        ga::platform::EKey::Insert,  ga::platform::EKey::Delete,
        ga::platform::EKey::Backspace, ga::platform::EKey::Space,
        ga::platform::EKey::Enter,   ga::platform::EKey::Escape,
        ga::platform::EKey::LeftControl,  ga::platform::EKey::LeftShift,
        ga::platform::EKey::LeftAlt,      ga::platform::EKey::LeftSuper,
        ga::platform::EKey::RightControl, ga::platform::EKey::RightShift,
        ga::platform::EKey::RightAlt,     ga::platform::EKey::RightSuper,
        ga::platform::EKey::A, ga::platform::EKey::B, ga::platform::EKey::C,
        ga::platform::EKey::D, ga::platform::EKey::E, ga::platform::EKey::F,
        ga::platform::EKey::G, ga::platform::EKey::H, ga::platform::EKey::I,
        ga::platform::EKey::J, ga::platform::EKey::K, ga::platform::EKey::L,
        ga::platform::EKey::M, ga::platform::EKey::N, ga::platform::EKey::O,
        ga::platform::EKey::P, ga::platform::EKey::Q, ga::platform::EKey::R,
        ga::platform::EKey::S, ga::platform::EKey::T, ga::platform::EKey::U,
        ga::platform::EKey::V, ga::platform::EKey::W, ga::platform::EKey::X,
        ga::platform::EKey::Y, ga::platform::EKey::Z,
        ga::platform::EKey::Num0, ga::platform::EKey::Num1, ga::platform::EKey::Num2,
        ga::platform::EKey::Num3, ga::platform::EKey::Num4, ga::platform::EKey::Num5,
        ga::platform::EKey::Num6, ga::platform::EKey::Num7, ga::platform::EKey::Num8,
        ga::platform::EKey::Num9,
        ga::platform::EKey::F1,  ga::platform::EKey::F2,  ga::platform::EKey::F3,
        ga::platform::EKey::F4,  ga::platform::EKey::F5,  ga::platform::EKey::F6,
        ga::platform::EKey::F7,  ga::platform::EKey::F8,  ga::platform::EKey::F9,
        ga::platform::EKey::F10, ga::platform::EKey::F11, ga::platform::EKey::F12,
    };

    for (auto key : k_TrackedKeys)
        ImGui_ImplGallium_UpdateKeyState(io, input, key);
}

ImFont* ImGui_ImplGallium_GetFont(string_hash_t id)
{
    auto* bd = ImGui_ImplGallium_GetBackendData();
    if (bd->fonts.contains(id))
        return bd->fonts[id];

    return bd->defaultFont;
}

std::vector<ga::gpu::SemaphoreId> ImGui_ImplGallium_RenderDrawData(ImDrawData* drawData, const ga::gpu::CommandEncoder& encoder, ga::gpu::Image& targetImage)
{
    if (drawData->DisplaySize.x <= 0.f || drawData->DisplaySize.y <= 0.f)
        return {};
    if (drawData->TotalVtxCount == 0)
        return {};

    auto* bd = ImGui_ImplGallium_GetBackendData();

    if (drawData->Textures != nullptr)
        for (ImTextureData* tex : *drawData->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplGallium_UpdateTexture(tex);

    // Grow vertex/index buffers if needed
    const int neededVtx = drawData->TotalVtxCount;
    const int neededIdx = drawData->TotalIdxCount;

    auto& frameData = bd->perFrameData[bd->currentFrame];

    if (!frameData.vertexBuffer || frameData.vertexBufferSize < neededVtx)
    {
        frameData.vertexBufferSize = neededVtx + 5000; // headroom
        bd->device->Defer([vb = std::move(frameData.vertexBuffer)] {});

        frameData.vertexBuffer = std::make_unique<ga::gpu::Buffer>(
            *bd->device,
            ga::gpu::BufferInfo {
                .usage                   = ga::gpu::EBufferUsage::VertexBuffer,
                .size                    = size_t(frameData.vertexBufferSize) * sizeof(ImDrawVert),
                .createPersistentStaging = true
            });

        frameData.vertexBuffer->SetDebugName("ImGui: Vertex buffer");
    }

    if (!frameData.indexBuffer || frameData.indexBufferSize < neededIdx)
    {
        frameData.indexBufferSize = neededIdx + 10000;
        bd->device->Defer([ib = std::move(frameData.indexBuffer)] {});

        frameData.indexBuffer = std::make_unique<ga::gpu::Buffer>(
            *bd->device,
            ga::gpu::BufferInfo {
                .usage                   = ga::gpu::EBufferUsage::IndexBuffer,
                .size                    = size_t(frameData.indexBufferSize) * sizeof(ImDrawIdx),
                .createPersistentStaging = true
            });

        frameData.indexBuffer->SetDebugName("ImGui: Index buffer");
    }

    // Upload geometry
    std::vector<ImDrawVert> vtxData; vtxData.reserve(drawData->TotalVtxCount);
    std::vector<ImDrawIdx>  idxData; idxData.reserve(drawData->TotalIdxCount);
    
    for (int n = 0; n < drawData->CmdListsCount; ++n)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        std::copy_n(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size, std::back_inserter(vtxData));
        std::copy_n(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size, std::back_inserter(idxData));
    }

    const auto vtxReady = frameData.vertexBuffer->UploadAsync(vtxData);
    const auto idxReady = frameData.indexBuffer->UploadAsync(idxData);

    const auto beginInfo = ga::gpu::BeginRenderingInfo {
        .pipeline = *bd->pipeline,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = targetImage.Size(),
        },
        .colorAttachments = {
            {
                .image   = targetImage,
                .loadOp  = ga::gpu::ELoadOp::Load,
                .storeOp = ga::gpu::EStoreOp::Store,
            }
        }
    };

    encoder.Render(beginInfo, [&](const ga::gpu::RenderEncoder& render) {
        render.SetViewport({ 0, 0 }, { drawData->DisplaySize.x, drawData->DisplaySize.y });

        render.BindVertexBuffers(0, { { frameData.vertexBuffer.get(), 0 } });
        render.BindIndexBuffer(frameData.indexBuffer.get());

        const float scaleX = 2.f / drawData->DisplaySize.x;
        const float scaleY = 2.f / drawData->DisplaySize.y;
        const float transX = -1.f - drawData->DisplayPos.x * scaleX;
        const float transY = -1.f - drawData->DisplayPos.y * scaleY;

        int globalVtxOffset = 0;
        int globalIdxOffset = 0;

        for (int n = 0; n < drawData->CmdListsCount; ++n)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; ++cmdIdx)
            {
                const ImDrawCmd& cmd = cmdList->CmdBuffer[cmdIdx];

                if (cmd.UserCallback)
                {
                    if (cmd.UserCallback == ImDrawCallback_ResetRenderState)
                    {
                        // Re-bind if needed - for now a no-op since we bind once
                    }
                    else
                    {
                        cmd.UserCallback(cmdList, &cmd);
                    }
                    continue;
                }

                // Scissor - clip to display bounds
                const ImVec2 clipMin(
                    cmd.ClipRect.x - drawData->DisplayPos.x,
                    cmd.ClipRect.y - drawData->DisplayPos.y);
                const ImVec2 clipMax(
                    cmd.ClipRect.z - drawData->DisplayPos.x,
                    cmd.ClipRect.w - drawData->DisplayPos.y);

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    continue;

                render.SetScissor(
                    { int32_t(clipMin.x), int32_t(clipMin.y) },
                    { uint32_t(clipMax.x - clipMin.x), uint32_t(clipMax.y - clipMin.y) });

                // Push constants - scale, translate, textureId per draw
                const uint32_t textureSlot = cmd.GetTexID();

                // Pack into a raw array matching the shader layout:
                //   float2 scale, float2 translate, uint32_t textureId
                struct ImGuiPC
                {
                    float    scaleX, scaleY;
                    float    translateX, translateY;
                    uint32_t textureId;
                } pc{ scaleX, scaleY, transX, transY, textureSlot };

                render.PushConstants(0, sizeof(pc), &pc);

                render.DrawIndexed(
                    cmd.ElemCount,
                    1,
                    cmd.IdxOffset + uint32_t(globalIdxOffset),
                    int32_t(cmd.VtxOffset) + globalVtxOffset
                );
            }

            globalVtxOffset += cmdList->VtxBuffer.Size;
            globalIdxOffset += cmdList->IdxBuffer.Size;
        }
    });

    ++bd->currentFrame %= bd->perFrameData.size();
    return { vtxReady, idxReady };
}

std::optional<ga::gpu::SemaphoreId> ImGui_ImplGallium_Render(const ImGui_ImplGallium_RenderInfo& renderInfo)
{
    auto* gpu = ImGui_ImplGallium_GetBackendData()->device;
    auto imguiCb = gpu->AcquireCommandBuffer();

    std::vector<ga::gpu::SemaphoreId> imguiWaitOps;
    
    imguiCb->Record([&](const ga::gpu::CommandEncoder& encoder) {
        encoder.PushLabel("ImGui");
        if (renderInfo.preRender)
            renderInfo.preRender(encoder);
        encoder.Transition(renderInfo.targetImage, ga::gpu::EImageLayout::ColorAttachmentOptimal);
        imguiWaitOps = ImGui_ImplGallium_RenderDrawData(ImGui::GetDrawData(), encoder, renderInfo.targetImage);
        if (renderInfo.postRender)
            renderInfo.postRender(encoder);
        encoder.PopLabel();
    }, ga::gpu::ECommandBufferUsage::OneTimeSubmit);
    
    for (auto& w : renderInfo.waitOps)
        imguiWaitOps.push_back(w);

    auto result = gpu->Submit(imguiCb, imguiWaitOps, renderInfo.signalOp);
    
    gpu->ReleaseCommandBuffer(imguiCb);
    return result;
}
