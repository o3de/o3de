/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI.Reflect/Size.h>

#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>

#include <AzCore/std/parallel/mutex.h>

#include <MeshletsRenderObject.h>
#include <MultiDispatchComputePass.h>  // for AZ::Meshlets::MeshletsImportedAttachment

namespace AZ
{
    namespace RHI
    {
        struct DrawItem;
        class DrawPacket;
    }

    namespace Meshlets
    {
        class MeshletsRenderObject;
        class MeshletsFeatureProcessor;

        class MeshletsRenderPass
            : public RPI::RasterPass
            , private RPI::ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(MeshletsRenderPass);

        public:
            AZ_RTTI(MeshletsRenderPass, "{753E455B-8E36-4DC3-B315-789F0EF0483C}", RasterPass);
            AZ_CLASS_ALLOCATOR(MeshletsRenderPass, SystemAllocator);

            static RPI::Ptr<MeshletsRenderPass> Create(const RPI::PassDescriptor& descriptor);

            // Adds the lod array of render data
            bool FillDrawRequestData(RHI::DrawPacketBuilder::DrawRequest& drawRequest);
            bool AddDrawPackets(const AZStd::vector<const RHI::DrawPacket*>& drawPackets);

            //! SP1: replaces the per-frame list of attachments that this render
            //! pass will Read (SRV) in this frame. The matching compute pass
            //! imports the same buffers and declares ReadWrite scope usage.
            //! See MeshletsImportedAttachment in MultiDispatchComputePass.h.
            void SetImportedAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments);

            //! PERF: in the shipping (forward-PBR-healthy) path this gem-private pass
            //! has ZERO draw items — meshlets render through the STANDARD ForwardPass via
            //! the "forward" tag. It only draws when the forward shader failed and the
            //! per-instance packet emitted the "MeshletsDrawList" debug-fallback item.
            //! Yet a RasterPass still opens a full-res color+depth scope every frame
            //! (~one whole geometry pass of cost). The feature processor sets this each
            //! frame to whether ANY instance produced a debug item; FrameBeginInternal
            //! skips building the scope entirely when false.
            void SetHasDrawWork(bool hasDrawWork) { m_hasDrawWork = hasDrawWork; }

            Data::Instance<RPI::Shader> GetShader();

            void SetFeatureProcessor(MeshletsFeatureProcessor* featureProcessor)
            {
                m_featureProcessor = featureProcessor;
            }

        protected:
            explicit MeshletsRenderPass(const RPI::PassDescriptor& descriptor);

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            void SetShaderPath(const char* shaderPath) { m_shaderPath = shaderPath; }
            bool LoadShader();
            bool InitializePipelineState();
            bool AcquireFeatureProcessor();
            void BuildShaderAndRenderData();

            // Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            //! SP1: declare Read (SRV) scope usage for the imported attachments
            //! pushed by the feature processor. Together with the compute
            //! pass's ReadWrite declaration this triggers the frame graph's
            //! UAV->SRV barrier between scopes.
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

        protected:
            MeshletsFeatureProcessor* m_featureProcessor = nullptr;

            // The  shader that will be used by the pass
            Data::Instance<RPI::Shader> m_shader = nullptr;

            // Override the following in the inherited class
            AZStd::string m_shaderPath = "dummyShaderPath";

            // To help create the pipeline state 
            RPI::PassDescriptor m_passDescriptor;

            const RHI::PipelineState* m_pipelineState = nullptr;
            RPI::ViewPtr m_currentView = nullptr;

            // SP1 diagnostic: log when the queued-draw-packet count changes from
            // one frame to the next so we can confirm the render pass is actually
            // receiving DrawPackets after the dedicated-buffer refactor.
            int32_t m_lastReportedDrawPacketCount = -1;

            //! Whether this pass has any "MeshletsDrawList" debug-fallback items this
            //! frame (false in the normal forward-healthy path). Set per-frame by the
            //! feature processor; gates the RasterPass scope in FrameBeginInternal.
            bool m_hasDrawWork = false;

            // SP1: per-frame imported attachments (compute output -> render input).
            AZStd::vector<MeshletsImportedAttachment> m_importedAttachments;
            AZStd::mutex m_importedAttachmentsMutex;
            int32_t m_lastReportedImportCount = -1;
        };

    } // namespace Meshlets
} // namespace AZ
