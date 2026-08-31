/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/DispatchItem.h>

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <MeshletsRenderObject.h>

namespace AZ::Meshlets
{
    // SP1: descriptor used to import each per-object meshlet UAV buffer into
    // the frame graph as an attachment. The compute pass imports it and
    // declares ReadWrite scope usage; the render pass declares Read scope
    // usage. Together these let Atom's frame graph insert the UAV->SRV
    // barrier between the compute write and the render read, which dedicated
    // (non-shared) buffers otherwise miss out on and AMD specifically does
    // not transition implicitly. Shared between the compute pass, render
    // pass, and feature processor so all three see the same attachment ids.
    struct MeshletsImportedAttachment
    {
        AZ::RHI::AttachmentId m_attachmentId;
        AZ::RHI::Ptr<AZ::RHI::Buffer> m_rhiBuffer;
        AZ::RHI::BufferViewDescriptor m_viewDescriptor;
        //! GPU cull: optional per-instance SRG whose \ref m_finalizeInputName UAV input
        //! must be bound to THIS attachment's frame-graph (scope-backed) view and then
        //! compiled -- inside the compute pass's CompileResources, so the GPU-writable
        //! input validates as a registered frame attachment (Atom requires every
        //! UAV-bound SRG resource to be a scheduler attachment for hazard tracking).
        //! Null for non-finalize imports. When the same SRG appears on multiple
        //! attachments (e.g. compacted-indices + args), all are bound before one Compile.
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_finalizeSrg;
        AZ::Name m_finalizeInputName;
        //! GPU cull barrier: usage/access/stage to declare on the barrier pass so the
        //! frame graph transitions this buffer out of the compute UAV state into the
        //! state the consumers need (Indirect for the args buffer, Shader/Read for the
        //! compacted-index buffer the vertex shader reads). Defaults to Indirect.
        AZ::RHI::ScopeAttachmentUsage m_barrierUsage = AZ::RHI::ScopeAttachmentUsage::Indirect;
        AZ::RHI::ScopeAttachmentStage m_barrierStage = AZ::RHI::ScopeAttachmentStage::DrawIndirect;
        //! Two-pass occlusion: when set, MeshletsRenderPass declares this attachment
        //! ReadWrite (UAV) instead of its default Read/SRV -- used for the per-cluster
        //! visibility ledger the late-depth pass's AS both reads and writes.
        bool m_renderPassReadWrite = false;
    };
}

namespace AZ
{
    namespace Meshlets
    {
        //! Multi Dispatch Pass - this pass will handle multiple dispatch submission
        //! during each frame - one dispatch per mesh, each represents group of compute
        //! threads that will be working to create meshlets of the given mesh.
        //! This class can be generalized in the future to become a base class for this
        //! dispatch submission.
        //! [To Do] - revisit the 'BuildCommandListInternal' method and refactor to handle
        //! 'under the hood' RHI CPU threads that carries the submissions in parallel
        class MultiDispatchComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(MultiDispatchComputePass);

        public:
            AZ_RTTI(MultiDispatchComputePass, "{13B3BAC7-0F12-4C23-BD9E-F82A7830195E}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MultiDispatchComputePass, SystemAllocator);
            ~MultiDispatchComputePass() = default;

            static RPI::Ptr<MultiDispatchComputePass> Create(const RPI::PassDescriptor& descriptor);

            //! Replaces the frame's dispatch items. Called once per frame from the
            //! feature processor's Render(). Thread-safe with respect to BuildCommandListInternal.
            void AddDispatchItems(const AZStd::vector<RHI::DispatchItem*>& dispatchItems);

            //! SP1: replaces the per-frame list of buffers to import as
            //! frame-graph attachments and declare as ReadWrite (UAV) scope
            //! usage. Called once per frame from the feature processor's
            //! Render(), alongside AddDispatchItems. The corresponding render
            //! pass receives the same list (declared as Read scope usage)
            //! so the frame graph inserts the UAV->SRV barrier between scopes.
            void SetImportedAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments);

            //! GPU cull: replaces the per-frame list of buffers to declare with
            //! Indirect (DrawIndirect) scope usage. Used to drive the UAV->Indirect
            //! transition for compute-written indirect-args buffers. A pass given
            //! only barrier attachments (no dispatch items) acts as a pure barrier:
            //! the prior compute pass wrote the buffer ReadWrite, this pass declares
            //! it Indirect, so the frame graph emits the transition before the
            //! standard raster passes consume it as indirect arguments.
            void SetIndirectBarrierAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments);

            //! HiZ cluster cull: import \p image (the persistent HiZ pyramid slot NOT being
            //! written this frame) and declare Read (ComputeShader-stage) scope usage, so the
            //! frame graph transitions it out of last frame's UAV state into shader-read
            //! before the standard depth/forward/motion passes sample it from the meshlet
            //! cull SRGs (which bind it directly, outside any pass attachment). Pass null to
            //! clear. Called once per frame from the feature processor's Render().
            void SetHiZReadImage(Data::Instance<RPI::AttachmentImage> image);

            // Pass behavior overrides
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            //! Returns the shader held by the ComputePass
            Data::Instance<RPI::Shader> GetShader();

        protected:
            MultiDispatchComputePass(const RPI::PassDescriptor& descriptor);

            // Overriding methods
            void BuildInternal() override;
            //! Tells the frame scheduler the actual number of dispatches we have for
            //! this frame so the submit-range partitioning matches m_dispatchItems.size().
            //! Without this override the scheduler defaults to 1, and BuildCommandListInternal
            //! gets a submit range past the end of our (often empty) vector.
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            void BuildShaderAndRenderData();

        private:
            // Random-access required so BuildCommandListInternal can apply its
            // submit-range slice in O(1). Not a set: deduplication is unnecessary
            // because the feature processor builds the list from unique sources.
            AZStd::vector<RHI::DispatchItem*> m_dispatchItems;
            AZStd::mutex m_dispatchItemsMutex;

            // SP1: imported attachments to register in SetupFrameGraphDependencies.
            AZStd::vector<MeshletsImportedAttachment> m_importedAttachments;
            AZStd::mutex m_importedAttachmentsMutex;

            // GPU cull: attachments to declare with Indirect (DrawIndirect) scope so
            // the frame graph transitions compute-written args buffers to the
            // indirect-argument state before the raster passes consume them.
            AZStd::vector<MeshletsImportedAttachment> m_indirectBarrierAttachments;
            AZStd::mutex m_indirectBarrierAttachmentsMutex;

            // HiZ cluster cull: last-completed persistent pyramid to import + declare
            // Read on this scope (see SetHiZReadImage). Guarded by the same mutex
            // discipline as the buffer lists.
            Data::Instance<RPI::AttachmentImage> m_hiZReadImage;
            AZStd::mutex m_hiZReadImageMutex;

            // SP1 diagnostic: log when the dispatch-item count transitions, and
            // log when the GPU actually runs dispatches. Used to confirm the
            // per-frame compute path is alive after the dedicated-buffer
            // refactor. -1 sentinel = no prior count reported yet.
            int32_t m_lastReportedDispatchCount = -1;
            int32_t m_lastReportedSubmitCount   = -1;
            int32_t m_lastReportedImportCount   = -1;
        };

    }   // namespace Meshlets
}   // namespace AZ

