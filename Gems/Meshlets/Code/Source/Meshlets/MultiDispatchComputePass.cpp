/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <MultiDispatchComputePass.h>

namespace AZ
{
    namespace Meshlets
    {
        Data::Instance<RPI::Shader> MultiDispatchComputePass::GetShader()
        {
            return m_shader;
        }

        RPI::Ptr<MultiDispatchComputePass> MultiDispatchComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MultiDispatchComputePass> pass = aznew MultiDispatchComputePass(descriptor);
            return pass;
        }

        MultiDispatchComputePass::MultiDispatchComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void MultiDispatchComputePass::BuildInternal()
        {
            ComputePass::BuildInternal();

            // Output
            // This is the buffer that is shared between all objects and dispatches and contains
            // the dynamic data that can be changed between passes.
            Name bufferName = Name{ "MeshletsSharedBuffer" };
            RPI::PassAttachmentBinding* localBinding = FindAttachmentBinding(bufferName);
            if (localBinding && !localBinding->GetAttachment() && Meshlets::SharedBufferInterface::Get())
            {
                AttachBufferToSlot(bufferName, Meshlets::SharedBufferInterface::Get()->GetBuffer());
            }
        }

        void MultiDispatchComputePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            // DON'T call the ComputePass:CompileResources as it will try to compile perDraw srg
            // under the assumption that this is a single dispatch compute. Here we have one
            // dispatch per meshlet object and each has its own perDraw srg.
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();


            }

            // GPU cull: finalize each per-instance cull SRG HERE (inside the frame-graph
            // scope, after SetupFrameGraphDependencies imported the args buffer). Binding
            // the scope-backed view of the args UAV and compiling now means the
            // GPU-writable m_outArgs input validates as a registered frame attachment --
            // doing this in the feature processor (before the import) fails with
            // "DeviceBuffer ... is not an attachment on the frame scheduler".
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_importedAttachmentsMutex);
                // Bind every finalize attachment's scope-backed view to its named UAV
                // input first; a single SRG may own several (compacted-indices + args),
                // so collect the SRGs and Compile each ONCE after all its inputs are
                // bound (compiling with a partially-bound SRG fails validation).
                AZStd::vector<AZ::RPI::ShaderResourceGroup*> srgsToCompile;
                for (const auto& att : m_importedAttachments)
                {
                    if (!att.m_finalizeSrg || att.m_finalizeInputName.IsEmpty())
                    {
                        continue;
                    }
                    const RHI::BufferView* view = context.GetBufferView(att.m_attachmentId);
                    const RHI::ShaderInputBufferIndex idx =
                        att.m_finalizeSrg->FindShaderInputBufferIndex(att.m_finalizeInputName);
                    if (view && idx.IsValid())
                    {
                        att.m_finalizeSrg->SetBufferView(idx, view);
                        AZ::RPI::ShaderResourceGroup* raw = att.m_finalizeSrg.get();
                        if (AZStd::find(srgsToCompile.begin(), srgsToCompile.end(), raw) == srgsToCompile.end())
                        {
                            srgsToCompile.push_back(raw);
                        }
                    }
                }
                for (AZ::RPI::ShaderResourceGroup* srg : srgsToCompile)
                {
                    srg->Compile();
                }
            }

            // Instead of compiling per frame, have everything compiled only once after data initialization!
            // Below is an example of compiling the dispatch if change is required.
            /*
            for (auto& dispatchItem : m_dispatchItems)
            {
                if (dispatchItem)
                {
                    for (RHI::ShaderResourceGroup* srgInDispatch : dispatchItem->m_shaderResourceGroups)
                    {
                        srgInDispatch->Compile()
                    }
                }
            }
            */
        }

        void MultiDispatchComputePass::SetImportedAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments)
        {
            // Replace, not append. Called once per frame from the feature processor.
            AZStd::lock_guard<AZStd::mutex> lock(m_importedAttachmentsMutex);
            m_importedAttachments = attachments;
        }

        void MultiDispatchComputePass::SetIndirectBarrierAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments)
        {
            // Replace, not append. Called once per frame from the feature processor.
            AZStd::lock_guard<AZStd::mutex> lock(m_indirectBarrierAttachmentsMutex);
            m_indirectBarrierAttachments = attachments;
        }

        void MultiDispatchComputePass::SetHiZReadImage(Data::Instance<RPI::AttachmentImage> image)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_hiZReadImageMutex);
            m_hiZReadImage = AZStd::move(image);
        }

        void MultiDispatchComputePass::AddDispatchItems(const AZStd::vector<RHI::DispatchItem*>& dispatchItems)
        {
            // Replace, not append. Called once per frame from the feature processor.
            // Synchronizing here means BuildCommandListInternal sees a consistent snapshot.
            AZStd::lock_guard<AZStd::mutex> lock(m_dispatchItemsMutex);
            m_dispatchItems.clear();
            m_dispatchItems.reserve(dispatchItems.size());
            for (auto* dispatchItem : dispatchItems)
            {
                if (dispatchItem)
                {
                    m_dispatchItems.emplace_back(dispatchItem);
                }
            }

            // SP1 diagnostic: emit a trace each time the queued-dispatch-item count
            // CHANGES from the previously reported count. This stays silent in
            // steady state (e.g. one item every frame) but lets us see whether the
            // per-frame feature-processor -> compute-pass plumbing is actually
            // queuing work. If we never see "queued N>0 items" here the dispatch
            // is being silently dropped upstream.
            const int32_t currentCount = static_cast<int32_t>(m_dispatchItems.size());
            if (currentCount != m_lastReportedDispatchCount)
            {
                AZ_TracePrintf("Meshlets",
                    "MultiDispatchComputePass::AddDispatchItems: queued %d item(s) "
                    "(was %d).\n",
                    currentCount, m_lastReportedDispatchCount);
                m_lastReportedDispatchCount = currentCount;
            }
        }

        // [To Do] Important remark
        //-------------------------
        // When the work load / amount of dispatches is high, the RHI will split work and distribute it
        // between several threads.
        // To avoid repeating the work or possibly corrupting data in such a case, split the work
        // as per Github issue #9899 (https://github.com/o3de/o3de/pull/9899) as an example of how to
        // prevent multiple threads trying to submit the same work.
        // This was not done here yet due to the very limited work required but shuold be changed!
        void MultiDispatchComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::ComputePass::SetupFrameGraphDependencies(frameGraph);

            // SP1: import each per-object meshlet UAV buffer as a frame-graph
            // attachment and declare ReadWrite scope usage on this (compute)
            // scope. The render pass declares Read scope usage on the same
            // attachment id; that combination is what tells the frame graph
            // to emit a UAV->SRV barrier between the compute write and the
            // render read. Without this the dedicated per-object buffers
            // bypass hazard tracking entirely and AMD reads stale/zero data
            // in the vertex shader (every triangle collapses to vertex 0).
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_importedAttachmentsMutex);
                int32_t imported = 0;
                int32_t scoped   = 0;
                int32_t importFails = 0;
                int32_t useFails    = 0;
                for (const auto& att : m_importedAttachments)
                {
                    if (!att.m_rhiBuffer)
                    {
                        continue;
                    }
                    // ImportBuffer is idempotent within a frame: subsequent calls
                    // with the same attachmentId+resource are accepted, so the
                    // render pass importing the same id later (if it ever does)
                    // remains safe.
                    const AZ::RHI::ResultCode importRes =
                        frameGraph.GetAttachmentDatabase().ImportBuffer(
                            att.m_attachmentId, att.m_rhiBuffer);
                    if (importRes == AZ::RHI::ResultCode::Success)
                    {
                        ++imported;
                    }
                    else
                    {
                        ++importFails;
                    }

                    RHI::BufferScopeAttachmentDescriptor scopeDesc(
                        att.m_attachmentId, att.m_viewDescriptor);
                    const AZ::RHI::ResultCode useRes = frameGraph.UseShaderAttachment(
                        scopeDesc,
                        RHI::ScopeAttachmentAccess::ReadWrite,
                        RHI::ScopeAttachmentStage::ComputeShader);
                    if (useRes == AZ::RHI::ResultCode::Success)
                    {
                        ++scoped;
                    }
                    else
                    {
                        ++useFails;
                    }
                }
                if (!m_importedAttachments.empty() || m_lastReportedImportCount != 0)
                {
                    const int32_t total = static_cast<int32_t>(m_importedAttachments.size());
                    if (total != m_lastReportedImportCount)
                    {
                        AZ_TracePrintf("Meshlets",
                            "MultiDispatchComputePass::SetupFrameGraphDependencies: "
                            "received %d attachment(s); imported=%d, scopedUAV=%d "
                            "(importFails=%d useFails=%d) [was %d].\n",
                            total, imported, scoped, importFails, useFails,
                            m_lastReportedImportCount);
                        m_lastReportedImportCount = total;
                    }
                }
            }

            // GPU cull barrier: declare each compute-written args buffer with Indirect
            // (DrawIndirect) scope. When this pass runs after the cull compute pass
            // (which declared the same attachment id ReadWrite), the frame graph emits
            // the UAV->IndirectArgument transition here, before the standard raster
            // passes consume the buffer as indirect arguments. A pass that receives
            // ONLY barrier attachments (and no dispatch items) is a pure barrier.
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_indirectBarrierAttachmentsMutex);
                for (const auto& att : m_indirectBarrierAttachments)
                {
                    if (!att.m_rhiBuffer)
                    {
                        continue;
                    }
                    frameGraph.GetAttachmentDatabase().ImportBuffer(att.m_attachmentId, att.m_rhiBuffer);
                    RHI::BufferScopeAttachmentDescriptor scopeDesc(att.m_attachmentId, att.m_viewDescriptor);
                    // Per-attachment usage: Indirect for the args buffer (consumed by the
                    // indirect draw), Shader/Read for the compacted-index buffer (read by
                    // the vertex shader). Either way this transitions the buffer out of the
                    // compute UAV state before the standard passes consume it.
                    frameGraph.UseAttachment(
                        scopeDesc,
                        RHI::ScopeAttachmentAccess::Read,
                        att.m_barrierUsage,
                        att.m_barrierStage);
                }
            }

            // HiZ cluster cull: import the last-completed persistent HiZ pyramid and
            // declare Read (compute-stage) scope usage. The image was UAV-written by
            // LAST frame's HiZ mip chain and is NOT touched by any pass this frame
            // (the HiZGeneratePass ping-pongs to the other slot), so this is the only
            // scope that puts it in the frame graph -- the declared Read transitions it
            // UAV->shader-read here, before the standard depth/forward/motion passes
            // sample it via the meshlet cull SRGs (bound directly, not as a pass
            // attachment). AttachmentImage carries its own unique attachment id.
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_hiZReadImageMutex);
                if (m_hiZReadImage)
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(
                        m_hiZReadImage->GetAttachmentId(), m_hiZReadImage->GetRHIImage());
                    RHI::ImageScopeAttachmentDescriptor scopeDesc(
                        m_hiZReadImage->GetAttachmentId(), RHI::ImageViewDescriptor{});
                    frameGraph.UseShaderAttachment(
                        scopeDesc,
                        RHI::ScopeAttachmentAccess::Read,
                        RHI::ScopeAttachmentStage::ComputeShader);
                }
            }

            // The scheduler partitions BuildCommandListInternal calls based on this count.
            // Without it, the default is 1 and we'd be submit-ranged past the end of an
            // empty vector (or have a stale single submit). Snapshot the count under the
            // same mutex used by AddDispatchItems for safety.
            uint32_t itemCount = 0;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_dispatchItemsMutex);
                itemCount = static_cast<uint32_t>(m_dispatchItems.size());
            }
            frameGraph.SetEstimatedItemCount(itemCount);
        }

        void MultiDispatchComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            // The following will bind all registered Srgs set in m_shaderResourceGroupsToBind
            // and sends them to the command list ahead of the dispatch.
            // This includes the PerView, PerScene and PerPass srgs.
            SetSrgsForDispatch(context);

            // Vector indexing is O(1), so the previous AZStd::advance() over a set is gone.
            // We do NOT clear m_dispatchItems here: BuildCommandListInternal can be invoked
            // by multiple RHI worker threads (one per submit range) and any of them clearing
            // would race with the others still iterating. AddDispatchItems is responsible
            // for replacing the list at the start of each frame.
            const uint32_t start = context.GetSubmitRange().m_startIndex;
            const uint32_t end = AZStd::min(
                context.GetSubmitRange().m_endIndex,
                static_cast<uint32_t>(m_dispatchItems.size()));

            const int32_t submittedCount = static_cast<int32_t>(end - start);
            if (submittedCount != m_lastReportedSubmitCount)
            {
                // SP1 diagnostic: log when the per-frame number of dispatches the
                // GPU actually executes changes. Combined with the AddDispatchItems
                // trace, this tells us (a) items are queued and (b) the scope is
                // executing them. Silent steady state thereafter.
                AZ_TracePrintf("Meshlets",
                    "MultiDispatchComputePass::BuildCommandListInternal: submitting %d "
                    "dispatch(es) this scope (range %u..%u, total queued %zu) [was %d].\n",
                    submittedCount, start, end, m_dispatchItems.size(),
                    m_lastReportedSubmitCount);
                m_lastReportedSubmitCount = submittedCount;
            }

            for (uint32_t index = start; index < end; ++index)
            {
                commandList->Submit(m_dispatchItems[index]->GetDeviceDispatchItem(context.GetDeviceIndex()), index);
            }
        }

        // [To Do] - implement in order to support hot reloading of the shaders
        void MultiDispatchComputePass::BuildShaderAndRenderData()
        {
        }

        // Before reloading shaders, we want to wait for existing dispatches to finish
        // so shader reloading does not interfere in any way. Because AP reloads are async, there might
        // be a case where dispatch resources are destructed and will most certainly cause a GPU crash.
        // If we flag the need for rebuild, the build will be made at the start of the next frame - at
        // this stage the dispatch items should have been cleared - now we can load the shader and data.
        void MultiDispatchComputePass::OnShaderReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader)
        {
            BuildShaderAndRenderData();
        }

        void MultiDispatchComputePass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset)
        {
            BuildShaderAndRenderData();
        }

        void MultiDispatchComputePass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
        {
            BuildShaderAndRenderData();
        }
    } // namespace Meshlets
}   // namespace AZ
