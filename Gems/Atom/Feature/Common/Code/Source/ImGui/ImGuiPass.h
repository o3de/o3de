/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>

#include <AzFramework/Input/Events/InputTextEventListener.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

#include <imgui/imgui.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for the ImGuiPassData.
        struct ImGuiPassData
            : public RPI::RasterPassData
        {
            AZ_RTTI(ImGuiPassData, "{3E96AF5F-DE1E-4B3B-9833-7164AEAB7C28}", RPI::RasterPassData);
            AZ_CLASS_ALLOCATOR(ImGuiPassData, SystemAllocator, 0);

            ImGuiPassData() = default;
            virtual ~ImGuiPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<ImGuiPassData, RPI::RasterPassData>()
                        ->Version(1)
                        ->Field("isDefaultImGui", &ImGuiPassData::m_isDefaultImGui)
                        ;
                }
            }

            bool m_isDefaultImGui = false;
        };

        //! This pass owns and manages activation of an Imgui context.
        class ImGuiPass
            : public RPI::RenderPass
            , private TickBus::Handler
            , private AzFramework::InputChannelEventListener
            , private AzFramework::InputTextEventListener
        {
            using Base = RPI::RenderPass;
            AZ_RPI_PASS(ImGuiPass);

        public:
            AZ_RTTI(ImGuiPass, "{44EC7CFB-860B-40C8-922D-D54F971E049F}", Base);
            AZ_CLASS_ALLOCATOR(ImGuiPass, SystemAllocator, 0);

            //! Creates a new ImGuiPass
            static RPI::Ptr<ImGuiPass> Create(const RPI::PassDescriptor& descriptor);

            ~ImGuiPass();

            //! Gets the underlying ImGui context.
            ImGuiContext* GetContext();

            //! Allows draw data from other imgui contexts to be rendered on this context.
            void RenderImguiDrawData(const ImDrawData& drawData);

            // TickBus::Handler overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

            // AzFramework::InputTextEventListener overrides...
            bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;

            // AzFramework::InputChannelEventListener overrides...
            bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        protected:
            explicit ImGuiPass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        private:

            struct DrawInfo
            {
                RHI::DrawIndexed m_drawIndexed;
                RHI::Scissor m_scissor;
            };

            //! Updates the index and vertex buffers, and returns the total number of draw items.
            uint32_t UpdateImGuiResources();

            void Init();

            ImGuiContext* m_imguiContext = nullptr;

            RHI::Ptr<RPI::PipelineStateForDraw> m_pipelineState;
            Data::Instance<RPI::Shader> m_shader;

            Data::Instance<RPI::ShaderResourceGroup> m_resourceGroup;
            RHI::ShaderInputNameIndex m_fontImageIndex = "FontImage";
            RHI::ShaderInputNameIndex m_projectionMatrixIndex = "m_projectionMatrix";
            RHI::Viewport m_viewportState;

            RHI::IndexBufferView m_indexBufferView;
            AZStd::array<RHI::StreamBufferView, 1> m_vertexBufferView; // Only 1 vertex stream view needed, but most RHI apis expect an array.
            AZStd::vector<DrawInfo> m_draws;
            Data::Instance<RPI::StreamingImage> m_fontAtlas;

            AZStd::vector<ImDrawData> m_drawData;
            bool m_isDefaultImGuiPass = false;

            // ImGui processes mouse wheel movement on NewFrame(), which could be before input events
            // happen, so save the value to apply the most recent value right before NewFrame().
            float m_lastFrameMouseWheel = 0.0;
            
            uint32_t m_viewportWidth = 0;
            uint32_t m_viewportHeight = 0;

        };
    }   // namespace RPI
}   // namespace AZ
