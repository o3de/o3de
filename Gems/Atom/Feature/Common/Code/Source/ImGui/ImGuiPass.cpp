/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImGui/ImGuiPass.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/Feature/ImGui/SystemBus.h>

#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            [[maybe_unused]] static const char* PassName = "ImGuiPass";
            static const char* ImguiShaderFilePath = "Shaders/imgui/imgui.azshader";
        }

        class ImguiContextScope
        {
        public:
            explicit ImguiContextScope(ImGuiContext* newContext = nullptr)
                : m_savedContext(ImGui::GetCurrentContext())
            {
                ImGui::SetCurrentContext(newContext);
            }

            ~ImguiContextScope()
            {
                ImGui::SetCurrentContext(m_savedContext);
            }

        private:
            ImGuiContext* m_savedContext = nullptr;
        };

        RPI::Ptr<Render::ImGuiPass> ImGuiPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew ImGuiPass(descriptor);
        }

        ImGuiPass::ImGuiPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
            , AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDebugUI() - 1) // Give ImGui manager priority over the pass
            , AzFramework::InputTextEventListener(AzFramework::InputTextEventListener::GetPriorityDebugUI() - 1) // Give ImGui manager priority over the pass
        {

            const ImGuiPassData* imguiPassData = RPI::PassUtils::GetPassData<ImGuiPassData>(descriptor);
            if (imguiPassData)
            {
                // check if this is the default ImGui pass.
                if (imguiPassData->m_isDefaultImGui)
                {
                    // Check to see if another default is already set.
                    ImGuiPass* currentDefaultPass = nullptr;
                    ImGuiSystemRequestBus::BroadcastResult(currentDefaultPass, &ImGuiSystemRequestBus::Events::GetDefaultImGuiPass);

                    if (currentDefaultPass != nullptr && currentDefaultPass->GetRenderPipeline() == GetRenderPipeline())
                    {
                        // Only error when the pipelines match, meaning the default was set multiple times for the same pipeline. When the pipelines differ,
                        // it's possible that multiple default ImGui passes are intentional, and only the first one to load should actually be set as default.
                        AZ_Error("ImGuiPass", false, "Default ImGui pass is already set on this pipeline, ignoring request to set this pass as default. Only one ImGui pass should be marked as default in the pipeline.");
                    }
                    else
                    {
                        m_isDefaultImGuiPass = true;
                        ImGuiSystemRequestBus::Broadcast(&ImGuiSystemRequestBus::Events::PushDefaultImGuiPass, this);
                    }
                }
            }

            auto imguiContextScope = ImguiContextScope(nullptr);

            m_imguiContext = ImGui::CreateContext();
            ImGui::StyleColorsDark();

            Init();
            ImGui::NewFrame();

            TickBus::Handler::BusConnect();
            AzFramework::InputChannelEventListener::Connect();
            AzFramework::InputTextEventListener::Connect();
        }

        ImGuiPass::~ImGuiPass()
        {
            if (m_isDefaultImGuiPass)
            {
                ImGuiSystemRequestBus::Broadcast(&ImGuiSystemRequestBus::Events::RemoveDefaultImGuiPass, this);
            }

            ImGuiContext* contextToRestore = ImGui::GetCurrentContext();
            if (contextToRestore == m_imguiContext)
            {
                contextToRestore = nullptr; // Don't restore this context since it's being deleted.
            }

            ImGui::SetCurrentContext(m_imguiContext);
            ImGui::DestroyContext(m_imguiContext);
            m_imguiContext = nullptr;
            ImGui::SetCurrentContext(contextToRestore);

            AzFramework::InputTextEventListener::BusDisconnect();
            AzFramework::InputChannelEventListener::BusDisconnect();
            TickBus::Handler::BusDisconnect();
        }

        ImGuiContext* ImGuiPass::GetContext()
        {
            return m_imguiContext;
        }

        void ImGuiPass::RenderImguiDrawData(const ImDrawData& drawData)
        {
            m_drawData.push_back(drawData);
        }

        void ImGuiPass::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            auto& io = ImGui::GetIO();
            io.DeltaTime = deltaTime;
        }

        bool ImGuiPass::OnInputTextEventFiltered(const AZStd::string& textUTF8)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            auto& io = ImGui::GetIO();
            io.AddInputCharactersUTF8(textUTF8.c_str());
            return io.WantTextInput;
        }

        bool ImGuiPass::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
        {
            if (!IsEnabled() || GetRenderPipeline()->GetScene() == nullptr)
            {
                return false;
            }

            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            auto& io = ImGui::GetIO();

            bool shouldCaptureEvent = false;

            // Matches ImGuiKey_ enum
            static const AzFramework::InputChannelId ImGuiKeyChannels[] =
            {
                AzFramework::InputDeviceKeyboard::Key::EditTab, // ImGuiKey_Tab
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft, // ImGuiKey_LeftArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight, // ImGuiKey_RightArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp, // ImGuiKey_UpArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown, // ImGuiKey_DownArrow
                AzFramework::InputDeviceKeyboard::Key::NavigationPageUp, // ImGuiKey_PageUp
                AzFramework::InputDeviceKeyboard::Key::NavigationPageDown, // ImGuiKey_PageDown
                AzFramework::InputDeviceKeyboard::Key::NavigationHome, // ImGuiKey_Home
                AzFramework::InputDeviceKeyboard::Key::NavigationEnd, // ImGuiKey_End
                AzFramework::InputDeviceKeyboard::Key::NavigationInsert, // ImGuiKey_Insert
                AzFramework::InputDeviceKeyboard::Key::NavigationDelete, // ImGuiKey_Delete
                AzFramework::InputDeviceKeyboard::Key::EditBackspace, // ImGuiKey_Backspace
                AzFramework::InputDeviceKeyboard::Key::EditSpace, // ImGuiKey_Space
                AzFramework::InputDeviceKeyboard::Key::EditEnter, // ImGuiKey_Enter
                AzFramework::InputDeviceKeyboard::Key::Escape, // ImGuiKey_Escape
                AzFramework::InputDeviceKeyboard::Key::NumPadEnter, // ImGuiKey_KeyPadEnter
                AzFramework::InputDeviceKeyboard::Key::AlphanumericA, // ImGuiKey_A
                AzFramework::InputDeviceKeyboard::Key::AlphanumericC, // ImGuiKey_C
                AzFramework::InputDeviceKeyboard::Key::AlphanumericV, // ImGuiKey_V
                AzFramework::InputDeviceKeyboard::Key::AlphanumericX, // ImGuiKey_X
                AzFramework::InputDeviceKeyboard::Key::AlphanumericY, // ImGuiKey_Y
                AzFramework::InputDeviceKeyboard::Key::AlphanumericZ  // ImGuiKey_Z
            };

            static_assert(AZ_ARRAY_SIZE(ImGuiKeyChannels) == ImGuiKey_COUNT, "ImGui key input enum does not match input channels array.");

            // Matches ImGuiNavInput_ enum
            static const AzFramework::InputChannelId ImGuiNavChannels[] =
            {
                AzFramework::InputDeviceGamepad::Button::A, // ImGuiNavInput_Activate
                AzFramework::InputDeviceGamepad::Button::B, // ImGuiNavInput_Cancel
                AzFramework::InputDeviceGamepad::Button::Y, // ImGuiNavInput_Input
                AzFramework::InputDeviceGamepad::Button::X, // ImGuiNavInput_Menu
                AzFramework::InputDeviceGamepad::Button::DL, // ImGuiNavInput_DpadLeft
                AzFramework::InputDeviceGamepad::Button::DR, // ImGuiNavInput_DpadRight
                AzFramework::InputDeviceGamepad::Button::DU, // ImGuiNavInput_DpadUp
                AzFramework::InputDeviceGamepad::Button::DD, // ImGuiNavInput_DpadDown
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LL, // ImGuiNavInput_LStickLeft
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LR, // ImGuiNavInput_LStickRight
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LU, // ImGuiNavInput_LStickUp
                AzFramework::InputDeviceGamepad::ThumbStickDirection::LD, // ImGuiNavInput_LStickDown
                AzFramework::InputDeviceGamepad::Button::L1, // ImGuiNavInput_FocusPrev
                AzFramework::InputDeviceGamepad::Button::R1, // ImGuiNavInput_FocusNext
                AzFramework::InputDeviceGamepad::Trigger::L2, // ImGuiNavInput_TweakSlow
                AzFramework::InputDeviceGamepad::Trigger::R2, // ImGuiNavInput_TweakFast
            };

            static_assert(AZ_ARRAY_SIZE(ImGuiNavChannels) == (ImGuiNavInput_InternalStart_), "ImGui nav input enum does not match input channels array.");

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            switch (inputChannel.GetState())
            {
            case AzFramework::InputChannel::State::Began:
            case AzFramework::InputChannel::State::Updated: // update the camera rotation
            {
                /// Mouse Events
                if (inputChannelId == AzFramework::InputDeviceMouse::SystemCursorPosition)
                {
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();

                    AZ_Assert(position, "Expected positiondata2d but found nullptr");

                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Left ||
                    inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    io.MouseDown[0] = true;
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ_Assert(position, "Expected positiondata2d but found nullptr");
                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    io.MouseDown[1] = true;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    io.MouseDown[2] = true;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    const float MouseWheelDeltaScale = 1.0f / 120.0f; // based on WHEEL_DELTA in WinUser.h
                    m_lastFrameMouseWheel += inputChannel.GetValue() * MouseWheelDeltaScale;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                /// Keyboard Modifiers
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftR)
                {
                    io.KeyShift = true;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltR)
                {
                    io.KeyAlt = true;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
                {
                    io.KeyCtrl = true;
                }

                /// Specific Key & Gamepad Events
                else
                {
                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiKeyChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.KeysDown[i] = true;
                            shouldCaptureEvent = io.WantCaptureKeyboard;
                            break;
                        }
                    }

                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiNavChannels); ++i)
                    {
                        if (inputChannelId == ImGuiNavChannels[i])
                        {
                            io.NavInputs[i] = true;
                            break;
                        }
                    }
                }
                break;
            }

            case AzFramework::InputChannel::State::Ended:
            {
                /// Mouse Events
                if (inputChannelId == AzFramework::InputDeviceMouse::Button::Left ||
                    inputChannelId == AzFramework::InputDeviceTouch::Touch::Index0)
                {
                    io.MouseDown[0] = false;
                    const auto* position = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                    AZ_Assert(position, "Expected positiondata2d but found nullptr");
                    io.MousePos.x = position->m_normalizedPosition.GetX() * static_cast<float>(io.DisplaySize.x);
                    io.MousePos.y = position->m_normalizedPosition.GetY() * static_cast<float>(io.DisplaySize.y);
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Right)
                {
                    io.MouseDown[1] = false;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Button::Middle)
                {
                    io.MouseDown[2] = false;
                    shouldCaptureEvent = io.WantCaptureMouse;
                }

                /// Keyboard Modifiers
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierShiftR)
                {
                    io.KeyShift = false;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierAltR)
                {
                    io.KeyAlt = false;
                }
                else if (
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL ||
                    inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
                {
                    io.KeyCtrl = false;
                }

                /// Specific Key & Gamepad Events
                else
                {
                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiKeyChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.KeysDown[i] = false;
                            shouldCaptureEvent = io.WantCaptureKeyboard;
                            break;
                        }
                    }

                    for (size_t i = 0; i < AZ_ARRAY_SIZE(ImGuiNavChannels); ++i)
                    {
                        if (inputChannelId == ImGuiKeyChannels[i])
                        {
                            io.NavInputs[i] = false;
                            break;
                        }
                    }
                }
                break;
            }
            case AzFramework::InputChannel::State::Idle:
                break;
            }

            return shouldCaptureEvent;
        }

        void ImGuiPass::FrameBeginInternal(FramePrepareParams params)
        {
            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            m_viewportWidth = static_cast<uint32_t>(params.m_viewportState.m_maxX - params.m_viewportState.m_minX);
            m_viewportHeight = static_cast<uint32_t>(params.m_viewportState.m_maxY - params.m_viewportState.m_minY);

            auto& io = ImGui::GetIO();
            io.DisplaySize.x = AZStd::max<float>(1.0f, static_cast<float>(m_viewportWidth));
            io.DisplaySize.y = AZStd::max<float>(1.0f, static_cast<float>(m_viewportHeight));

            Matrix4x4 projectionMatrix =
                Matrix4x4::CreateFromRows(
                    AZ::Vector4(2.0f / m_viewportWidth, 0.0f, 0.0f, -1.0f),
                    AZ::Vector4(0.0f, -2.0f / m_viewportHeight, 0.0f, 1.0f),
                    AZ::Vector4(0.0f, 0.0f, 0.5f, 0.5f),
                    AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

            m_resourceGroup->SetConstant(m_projectionMatrixIndex, projectionMatrix);

            m_viewportState = params.m_viewportState;

            Base::FrameBeginInternal(params);
        }

        void ImGuiPass::Init()
        {
            auto& io = ImGui::GetIO();

            // ImGui IO Setup
            {
                for (size_t i = 0; i < ImGuiKey_COUNT; ++i)
                {
                    io.KeyMap[static_cast<ImGuiKey_>(i)] = static_cast<int>(i);
                }
                io.NavActive = true;

                // Touch input
                const AzFramework::InputDevice* inputDevice = nullptr;
                AzFramework::InputDeviceRequestBus::EventResult(inputDevice,
                    AzFramework::InputDeviceTouch::Id,
                    &AzFramework::InputDeviceRequests::GetInputDevice);

                if (inputDevice && inputDevice->IsSupported())
                {
                    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
                }

                // Set initial display size to something reasonable (this will be updated in FramePrepare)
                io.DisplaySize.x = 1920;
                io.DisplaySize.y = 1080;
            }

            {
                m_shader = RPI::LoadCriticalShader(ImguiShaderFilePath);

                m_pipelineState = aznew RPI::PipelineStateForDraw;
                m_pipelineState->Init(m_shader);

                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()
                    ->Channel("POSITION", RHI::Format::R32G32_FLOAT)
                    ->Channel("UV", RHI::Format::R32G32_FLOAT)
                    ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);
                m_pipelineState->InputStreamLayout() = layoutBuilder.End();

            }

            // Get shader resource group
            {
                auto perObjectSrgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object);
                if (!perObjectSrgLayout)
                {
                    AZ_Error(PassName, false, "Failed to get shader resource group layout");
                    return;
                }

                m_resourceGroup = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), perObjectSrgLayout->GetName());
                if (!m_resourceGroup)
                {
                    AZ_Error(PassName, false, "Failed to create shader resource group");
                    return;
                }
            }

            // Find or create font atlas
            const char* FontAtlasName = "ImGuiFontAtlas";
            m_fontAtlas = Data::InstanceDatabase<RPI::StreamingImage>::Instance().Find(Data::InstanceId::CreateName(FontAtlasName));
            if (!m_fontAtlas)
            {
                uint8_t* pixels;
                int32_t width = 0;
                int32_t height = 0;

                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

                const uint32_t pixelDataSize = width * height * 4;

                RHI::Size imageSize;
                imageSize.m_width = aznumeric_cast<uint32_t>(width);
                imageSize.m_height = aznumeric_cast<uint32_t>(height);

                Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();

                // CreateFromCpuData will add the image to the instance database.
                m_fontAtlas = RPI::StreamingImage::CreateFromCpuData(*streamingImagePool, RHI::ImageDimension::Image2D, imageSize, RHI::Format::R8G8B8A8_UNORM_SRGB, pixels, pixelDataSize, Uuid::CreateName(FontAtlasName));
                AZ_Error(PassName, m_fontAtlas, "Failed to initialize the ImGui font image!");
            }
            else
            {
                // GetTexDataAsRGBA32() sets the font default internally, but if a m_fontAtlas already has been retrieved it needs to be done manually.
                io.Fonts->AddFontDefault();
                io.Fonts->Build();
            }
            m_resourceGroup->SetImage(m_fontImageIndex, m_fontAtlas);
            io.Fonts->TexID = reinterpret_cast<ImTextureID>(m_fontAtlas.get());
        }

        void ImGuiPass::InitializeInternal()
        {
            // Set output format and finalize pipeline state
            m_pipelineState->SetOutputFromPass(this);
            m_pipelineState->Finalize();

            Base::InitializeInternal();
        }

        void ImGuiPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            Base::SetupFrameGraphDependencies(frameGraph);
            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            ImGui::Render();
            uint32_t drawCount = UpdateImGuiResources();
            frameGraph.SetEstimatedItemCount(drawCount);

            m_draws.clear();
            m_draws.reserve(drawCount);
        }

        void ImGuiPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            m_resourceGroup->Compile();

            // Create all the DrawIndexeds so they can be submitted in parallel on BuildCommandListInternal()
            uint32_t vertexOffset = 0;
            uint32_t indexOffset = 0;

            for (ImDrawData& drawData : m_drawData)
            {
                for (int32_t cmdListIdx = 0; cmdListIdx < drawData.CmdListsCount; cmdListIdx++)
                {
                    const ImDrawList* drawList = drawData.CmdLists[cmdListIdx];
                    for (const ImDrawCmd& drawCmd : drawList->CmdBuffer)
                    {
                        AZ_Assert(drawCmd.UserCallback == nullptr, "ImGui UserCallbacks are not supported by the ImGui Pass");
                        uint32_t scissorMaxX = static_cast<uint32_t>(drawCmd.ClipRect.z);
                        uint32_t scissorMaxY = static_cast<uint32_t>(drawCmd.ClipRect.w);
                        
                        //scissorMaxX/scissorMaxY can be a frame stale from imgui (ImGui::NewFrame runs after this) hence we clamp it to viewport bounds
                        //otherwise it is possible to have a frame where scissor bounds can be bigger than window's bounds if we resize the window
                        scissorMaxX = AZStd::min(scissorMaxX, m_viewportWidth);
                        scissorMaxY = AZStd::min(scissorMaxY, m_viewportHeight);
                        
                        m_draws.push_back(
                            {
                                RHI::DrawIndexed(1, 0, vertexOffset, drawCmd.ElemCount, indexOffset),
                                RHI::Scissor(
                                    static_cast<int32_t>(drawCmd.ClipRect.x),
                                    static_cast<int32_t>(drawCmd.ClipRect.y),
                                             scissorMaxX,
                                             scissorMaxY
                                )
                            }
                        );

                        indexOffset += drawCmd.ElemCount;
                    }
                    vertexOffset += drawList->VtxBuffer.size();
                }
            }
            m_drawData.clear();

            auto imguiContextScope = ImguiContextScope(m_imguiContext);
            ImGui::GetIO().MouseWheel = m_lastFrameMouseWheel;
            m_lastFrameMouseWheel = 0.0;
            ImGui::NewFrame();
        }

        void ImGuiPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_PROFILE_SCOPE(AzRender, "ImGuiPass: BuildCommandListInternal");

            context.GetCommandList()->SetViewport(m_viewportState);

            const RHI::ShaderResourceGroup* shaderResourceGroups[] = { m_resourceGroup->GetRHIShaderResourceGroup() };

            uint32_t numDraws = aznumeric_cast<uint32_t>(m_draws.size());
            uint32_t firstIndex = (context.GetCommandListIndex() * numDraws) / context.GetCommandListCount();
            uint32_t lastIndex = ((context.GetCommandListIndex() + 1) * numDraws) / context.GetCommandListCount();

            for (uint32_t i = firstIndex; i < lastIndex; ++i)
            {
                RHI::DrawItem drawItem;
                drawItem.m_arguments = m_draws.at(i).m_drawIndexed;
                drawItem.m_pipelineState = m_pipelineState->GetRHIPipelineState();
                drawItem.m_indexBufferView = &m_indexBufferView;
                drawItem.m_shaderResourceGroupCount = 1;
                drawItem.m_shaderResourceGroups = shaderResourceGroups;
                drawItem.m_streamBufferViewCount = 1;
                drawItem.m_streamBufferViews = m_vertexBufferView.data();
                drawItem.m_scissorsCount = 1;
                drawItem.m_scissors = &m_draws.at(i).m_scissor;

                context.GetCommandList()->Submit(drawItem);
            }
        }

        uint32_t ImGuiPass::UpdateImGuiResources()
        {
            AZ_PROFILE_SCOPE(AzRender, "ImGuiPass: UpdateImGuiResources");

            auto imguiContextScope = ImguiContextScope(m_imguiContext);

            constexpr uint32_t indexSize = aznumeric_cast<uint32_t>(sizeof(ImDrawIdx));
            constexpr uint32_t vertexSize = aznumeric_cast<uint32_t>(sizeof(ImDrawVert));

            if (ImGui::GetDrawData())
            {
                m_drawData.push_back(*ImGui::GetDrawData());
            }

            uint32_t totalIdxBufferSize = 0;
            uint32_t totalVtxBufferSize = 0;
            for (ImDrawData& drawData : m_drawData)
            {
                totalIdxBufferSize += drawData.TotalIdxCount * indexSize;
                totalVtxBufferSize += drawData.TotalVtxCount * vertexSize;
            }

            if (totalIdxBufferSize == 0)
            {
                return 0; // Nothing to draw.
            }

            auto vertexBuffer = RPI::DynamicDrawInterface::Get()->GetDynamicBuffer(totalVtxBufferSize, RHI::Alignment::InputAssembly);
            auto indexBuffer = RPI::DynamicDrawInterface::Get()->GetDynamicBuffer(totalIdxBufferSize, RHI::Alignment::InputAssembly);

            if (!vertexBuffer || !indexBuffer)
            {
                return 0;
            }

            ImDrawIdx* indexBufferData = static_cast<ImDrawIdx*>(indexBuffer->GetBufferAddress());
            ImDrawVert* vertexBufferData = static_cast<ImDrawVert*>(vertexBuffer->GetBufferAddress());

            uint32_t drawCount = 0;
            uint32_t indexBufferOffset = 0;
            uint32_t vertexBufferOffset = 0;

            for (ImDrawData& drawData : m_drawData)
            {
                for (int32_t cmdListIndex = 0; cmdListIndex < drawData.CmdListsCount; cmdListIndex++)
                {
                    const ImDrawList* drawList = drawData.CmdLists[cmdListIndex];

                    const uint32_t indexBufferByteSize = drawList->IdxBuffer.size() * indexSize;
                    memcpy(indexBufferData + indexBufferOffset, drawList->IdxBuffer.Data, indexBufferByteSize);
                    indexBufferOffset += drawList->IdxBuffer.size();

                    const uint32_t vertexBufferByteSize = drawList->VtxBuffer.size() * vertexSize;
                    memcpy(vertexBufferData + vertexBufferOffset, drawList->VtxBuffer.Data, vertexBufferByteSize);
                    vertexBufferOffset += drawList->VtxBuffer.size();

                    ++drawCount;
                }
            }

            static_assert(indexSize == 2, "Expected index size from ImGui to be 2 to match RHI::IndexFormat::Uint16");
            m_indexBufferView = indexBuffer->GetIndexBufferView(RHI::IndexFormat::Uint16);
            m_vertexBufferView[0] = vertexBuffer->GetStreamBufferView(vertexSize);

            RHI::ValidateStreamBufferViews(m_pipelineState->ConstDescriptor().m_inputStreamLayout, m_vertexBufferView);

            return drawCount;
        }

    } // namespace Render
} // namespace AZ
