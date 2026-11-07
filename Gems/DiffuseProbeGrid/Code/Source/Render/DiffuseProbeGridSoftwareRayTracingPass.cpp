/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/DiffuseProbeGridSoftwareRayTracingPass.h>
#include <Render/DiffuseProbeGridTracing.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDispatchItem.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/GpuQuery/GpuQuerySystemInterface.h>
#include <Atom/RPI.Public/GpuQuery/Query.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/limits.h>
#include <DiffuseProbeGrid_Traits_Platform.h>

AZ_CVAR(AZ::CVarFixedString, r_diffuseGI_mode, "Hardware", nullptr, AZ::ConsoleFunctorFlags::Null,
    "Project-wide diffuse GI mode: Hardware, Software, Baked, or Auto. Set through the quality registry.");

AZ_CVAR(bool, r_diffuseGI_updateOffscreenSkinnedMeshes, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Update off-screen character poses and lowest-LOD skinning for hardware GI. Software tracing enables these updates automatically.");

namespace AZ::Render
{
    namespace
    {
        constexpr const char* BufferNames[] = {"m_nodes", "m_bounds", "m_primitiveIndices", "m_instances", "m_work"};

        void SubmitCompute(const RHI::FrameGraphExecuteContext& context, const RHI::PipelineState* pipeline,
            const RHI::ShaderResourceGroup* sceneSrg, const RHI::ShaderResourceGroup* passSrg,
            uint32_t x, uint32_t y, uint16_t groupX, uint16_t groupY, uint32_t item = 0)
        {
            auto* commands = context.GetCommandList();
            if (sceneSrg)
            {
                commands->SetShaderResourceGroupForDispatch(*sceneSrg->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));
            }
            commands->SetShaderResourceGroupForDispatch(*passSrg->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));
            RHI::DeviceDispatchItem dispatch;
            dispatch.m_pipelineState = pipeline->GetDevicePipelineState(context.GetDeviceIndex()).get();
            dispatch.m_arguments.m_direct.m_totalNumberOfThreadsX = x;
            dispatch.m_arguments.m_direct.m_totalNumberOfThreadsY = y;
            dispatch.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            dispatch.m_arguments.m_direct.m_threadsPerGroupX = groupX;
            dispatch.m_arguments.m_direct.m_threadsPerGroupY = groupY;
            dispatch.m_arguments.m_direct.m_threadsPerGroupZ = 1;
            commands->Submit(dispatch, item);
        }
    }

    bool IsDiffuseProbeGridOffscreenSkinningRequested()
    {
        return r_diffuseGI_updateOffscreenSkinnedMeshes;
    }

    bool IsDiffuseProbeGridSoftwareTracingReady(const RPI::Pass& consumer)
    {
        if (const auto* parent = consumer.GetParent())
        {
            for (const auto& child : parent->GetChildren())
            {
                if (const auto* software = azrtti_cast<const DiffuseProbeGridSoftwareRayTracingPass*>(child.get()))
                {
                    return software->IsReadyToTrace();
                }
            }
        }
        return false;
    }

    DiffuseProbeGridMode GetDiffuseProbeGridMode()
    {
        const auto mode = static_cast<AZ::CVarFixedString>(r_diffuseGI_mode);
        if (mode == "Software")
        {
            return DiffuseProbeGridMode::RealTimeSoftware;
        }
        if (mode == "Baked")
        {
            return DiffuseProbeGridMode::Baked;
        }
        if (mode == "Auto")
        {
            return DiffuseProbeGridMode::AutoSelect;
        }
        AZ_WarningOnce("DiffuseProbeGrid", mode == "Hardware",
            "Invalid r_diffuseGI_mode; expected Hardware, Software, Baked, or Auto. Using Hardware.");
        return DiffuseProbeGridMode::RealTimeHardware;
    }

    bool IsDiffuseProbeGridSoftwareTracing()
    {
        const auto mode = GetDiffuseProbeGridMode();
        return mode == DiffuseProbeGridMode::RealTimeSoftware ||
            (mode == DiffuseProbeGridMode::AutoSelect &&
                RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices);
    }

    RPI::Ptr<DiffuseProbeGridSoftwareRayTracingPass> DiffuseProbeGridSoftwareRayTracingPass::Create(const RPI::PassDescriptor& descriptor)
    {
        return aznew DiffuseProbeGridSoftwareRayTracingPass(descriptor);
    }

    DiffuseProbeGridSoftwareRayTracingPass::DiffuseProbeGridSoftwareRayTracingPass(const RPI::PassDescriptor& descriptor)
        : RPI::RenderPass(descriptor)
    {
        const auto id = Uuid::CreateRandom().ToString<AZStd::string>();
        for (uint32_t slot = 0; slot < BufferCount; ++slot)
        {
            m_attachmentIds[slot] = Name(AZStd::string::format("DiffuseProbeGridSoftware_%s_%u", id.c_str(), slot));
        }
    }

    bool DiffuseProbeGridSoftwareRayTracingPass::IsEnabled() const
    {
        if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED || !RenderPass::IsEnabled() || !IsDiffuseProbeGridSoftwareTracing() || !m_pipeline->GetScene())
        {
            return false;
        }
        auto* grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
        return grids && !grids->GetVisibleRealTimeProbeGrids().empty();
    }

    void DiffuseProbeGridSoftwareRayTracingPass::ReleaseResources()
    {
        m_ready = false;
        m_scene.Clear();
        m_keys.clear();
        m_gpuData = {};
        m_buffers = {};
        m_gridScratch.clear();
        m_initialRanges.clear();
        m_dynamicRanges.clear();
        m_activeRanges = nullptr;
        m_refitScopes.clear();
        m_refitSrgs.clear();
        m_traceSrgs.clear();
        m_feedbackScopes = {};
        m_traceShader = {};
        m_refitShader = {};
        m_tracePipeline = {};
        m_refitPipeline = {};
        m_feedbackShaders = {};
        m_feedbackPipelines = {};
        m_fullPassTimestampQuery = {};
        m_refitTimestampQueries.clear();
        m_feedbackTimestampQueries = {};
        m_fullPassTimestamp = {};
        m_timestampDevice = RHI::MultiDevice::InvalidDeviceIndex;
    }

    bool DiffuseProbeGridSoftwareRayTracingPass::LoadShaders()
    {
        if (m_tracePipeline && m_refitPipeline && m_feedbackPipelines[QueryFeedback] && m_feedbackPipelines[IntegrateFeedback])
        {
            return true;
        }
        m_traceShader = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridSoftwareRayTracing.azshader");
        m_refitShader = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridSoftwareRefit.azshader");
        if (!m_traceShader || !m_refitShader)
        {
            return false;
        }
        RHI::PipelineStateDescriptorForDispatch descriptor;
        m_traceShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(descriptor);
        m_tracePipeline = m_traceShader->AcquirePipelineState(descriptor);
        descriptor = {};
        m_refitShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(descriptor);
        m_refitPipeline = m_refitShader->AcquirePipelineState(descriptor);
        // Reuse Atom's shipped irradiance query implementation rather than duplicating the retained DDGI sampler.
        m_feedbackShaders[QueryFeedback] = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridQuery.azshader");
        m_feedbackShaders[IntegrateFeedback] = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridSoftwareIntegrate.azshader");
        for (uint32_t stage = 0; stage < FeedbackStageCount; ++stage)
        {
            if (!m_feedbackShaders[stage])
            {
                return false;
            }
            descriptor = {};
            m_feedbackShaders[stage]->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(descriptor);
            m_feedbackPipelines[stage] = m_feedbackShaders[stage]->AcquirePipelineState(descriptor);
            if (!m_feedbackPipelines[stage])
            {
                return false;
            }
        }
        if (!RPI::GetComputeShaderNumThreads(m_feedbackShaders[QueryFeedback]->GetAsset(), m_queryArguments).IsSuccess())
        {
            return false;
        }
        return m_tracePipeline && m_refitPipeline;
    }

    void DiffuseProbeGridSoftwareRayTracingPass::UploadScene()
    {
        m_gpuData = m_scene.BuildGpuData();
        m_initialRanges.clear();
        m_dynamicRanges.clear();
        AZStd::vector<DiffuseProbeGridSoftwareScene::RefitWork> work;
        auto flatten = [&](const auto& levels, auto& ranges)
        {
            for (const auto& level : levels)
            {
                ranges.push_back({aznumeric_cast<uint32_t>(work.size()), aznumeric_cast<uint32_t>(level.size())});
                work.insert(work.end(), level.begin(), level.end());
            }
        };
        flatten(m_gpuData.m_initialRefit, m_initialRanges);
        flatten(m_gpuData.m_dynamicRefit, m_dynamicRanges);
        auto upload = [&](BufferSlot slot, auto& elements, bool readWrite)
        {
            using Element = typename AZStd::remove_reference_t<decltype(elements)>::value_type;
            const Element empty{};
            RPI::CommonBufferDescriptor descriptor;
            descriptor.m_bufferName = m_attachmentIds[slot].GetCStr();
            descriptor.m_poolType = readWrite ? RPI::CommonBufferPoolType::ReadWrite : RPI::CommonBufferPoolType::ReadOnly;
            descriptor.m_elementSize = sizeof(Element);
            descriptor.m_byteCount = AZStd::max(size_t(1), elements.size()) * sizeof(Element);
            descriptor.m_bufferData = elements.empty() ? &empty : elements.data();
            m_buffers[slot] = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
        };
        upload(Nodes, m_gpuData.m_nodes, false);
        upload(Bounds, m_gpuData.m_bounds, true);
        upload(Primitives, m_gpuData.m_primitiveIndices, false);
        upload(Instances, m_gpuData.m_instances, false);
        upload(Work, work, false);
    }

    void DiffuseProbeGridSoftwareRayTracingPass::FrameBeginInternal(FramePrepareParams params)
    {
        m_ready = false;
        auto* scene = m_pipeline->GetScene();
        auto* meshes = scene->GetFeatureProcessor<MeshFeatureProcessorInterface>();
        auto* transforms = scene->GetFeatureProcessor<TransformServiceFeatureProcessorInterface>();
        if (!meshes || !transforms || !m_pipeline->GetDefaultView() || !LoadShaders())
        {
            return;
        }
        ++m_frameNumber;
        m_scene.Update(*meshes, *transforms);
        const auto& instances = m_scene.GetInstances();
        bool changed = instances.size() != m_keys.size() ||
            AZStd::any_of(m_buffers.begin(), m_buffers.end(), [](const auto& buffer) { return !buffer; });
        for (size_t i = 0; !changed && i < instances.size(); ++i)
        {
            changed = instances[i].m_geometry != m_keys[i].m_geometry ||
                instances[i].m_meshInfoHandle != m_keys[i].m_meshInfoHandle || instances[i].m_skinned != m_keys[i].m_skinned;
        }
        bool relocationRequired = changed || AZStd::any_of(instances.begin(), instances.end(),
            [](const auto& instance) { return instance.m_skinned; });
        for (size_t i = 0; !changed && i < instances.size(); ++i)
        {
            relocationRequired |= !instances[i].m_transform.IsClose(m_keys[i].m_transform, 0.0f);
            m_keys[i].m_transform = instances[i].m_transform;
        }
        if (relocationRequired)
        {
            for (auto& grid : scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids())
            {
                grid->ResetRemainingRelocationIterations();
            }
        }
        if (changed)
        {
            UploadScene();
            m_keys.clear();
            for (const auto& instance : instances)
            {
                m_keys.push_back({instance.m_geometry, instance.m_meshInfoHandle, instance.m_skinned, instance.m_transform});
            }
        }
        m_activeRanges = changed ? &m_initialRanges : &m_dynamicRanges;
        if (!PrepareGridScratch())
        {
            return;
        }
        if (AZStd::any_of(m_buffers.begin(), m_buffers.end(), [](const auto& buffer) { return !buffer; }))
        {
            return;
        }
        m_hasSkinningOutput = false;
        if (AZStd::any_of(instances.begin(), instances.end(), [](const auto& instance) { return instance.m_skinned; }))
        {
            const auto skinning = m_pipeline->FindFirstPass(Name("SkinningPass"));
            const auto* output = skinning ? skinning->FindAttachmentBinding(Name("SkinnedMeshOutputStream")) : nullptr;
            if (!output)
            {
                AZ_ErrorOnce("DiffuseProbeGrid", false, "Software GI requires the pipeline's skinned output attachment before refitting animated geometry");
                return;
            }
            m_skinningOutput = output->m_unifiedScopeDesc.GetAsBuffer();
            m_hasSkinningOutput = true;
        }
        while (m_refitScopes.size() < m_activeRanges->size())
        {
            const auto level = aznumeric_cast<uint32_t>(m_refitScopes.size());
            const auto scopeId = Name(AZStd::string::format("%s_Refit%u", m_attachmentIds[Bounds].GetCStr(), level));
            m_refitScopes.push_back(AZStd::make_unique<RHI::ScopeProducerFunctionNoData>(scopeId,
                [this, level](RHI::FrameGraphInterface graph) { PrepareRefit(graph, level); },
                [this, level](const RHI::FrameGraphCompileContext&) { CompileRefit(level); },
                [this, level](const RHI::FrameGraphExecuteContext& context) { ExecuteRefit(context, level); },
                RHI::HardwareQueueClass::Graphics, Pass::GetDeviceIndex()));
            m_refitSrgs.push_back(RPI::ShaderResourceGroup::Create(m_refitShader->GetAsset(), Name("PassSrg")));
        }
        m_fullPassTimestamp = {};
        m_refitTimestampQueries.resize(m_refitScopes.size());
        if (IsTimestampQueryEnabled())
        {
            auto createQuery = []()
            {
                return RPI::GpuQuerySystemInterface::Get()->CreateQuery(
                    RHI::QueryType::Timestamp, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            };
            if (!m_fullPassTimestampQuery)
            {
                m_fullPassTimestampQuery = createQuery();
            }
            bool hasTimestamp = false;
            auto includeResult = [&](const RHI::Ptr<RPI::Query>& query)
            {
                uint64_t ticks[2] = {};
                if (query && m_timestampDevice != RHI::MultiDevice::InvalidDeviceIndex &&
                    query->GetLatestResult(ticks, m_timestampDevice) == RPI::QueryResultCode::Success && ticks[0] && ticks[1] >= ticks[0])
                {
                    const RPI::TimestampResult result(ticks[0], ticks[1], RHI::HardwareQueueClass::Graphics);
                    if (hasTimestamp)
                    {
                        m_fullPassTimestamp.Add(result);
                    }
                    else
                    {
                        m_fullPassTimestamp = result;
                        hasTimestamp = true;
                    }
                }
            };
            includeResult(m_fullPassTimestampQuery);
            for (auto& query : m_feedbackTimestampQueries)
            {
                if (!query)
                {
                    query = createQuery();
                }
                includeResult(query);
            }
            for (uint32_t level = 0; level < m_activeRanges->size(); ++level)
            {
                if ((*m_activeRanges)[level].m_count)
                {
                    if (!m_refitTimestampQueries[level])
                    {
                        m_refitTimestampQueries[level] = createQuery();
                    }
                    includeResult(m_refitTimestampQueries[level]);
                }
            }
        }
        m_ready = true;
        for (uint32_t level = 0; level < m_activeRanges->size(); ++level)
        {
            if ((*m_activeRanges)[level].m_count)
            {
                params.m_frameGraphBuilder->ImportScopeProducer(*m_refitScopes[level]);
            }
        }
        RenderPass::FrameBeginInternal(params);
        for (uint32_t stage = 0; stage < FeedbackStageCount; ++stage)
        {
            if (!m_feedbackScopes[stage])
            {
                const auto id = Name(AZStd::string::format("%s_Feedback%u", m_attachmentIds[Bounds].GetCStr(), stage));
                m_feedbackScopes[stage] = AZStd::make_unique<RHI::ScopeProducerFunctionNoData>(id,
                    [this, stage](RHI::FrameGraphInterface graph) { SetupFeedbackStage(graph, stage); },
                    [this, stage](const RHI::FrameGraphCompileContext&) { CompileFeedbackStage(stage); },
                    [this, stage](const RHI::FrameGraphExecuteContext& context) { ExecuteFeedbackStage(context, stage); },
                    RHI::HardwareQueueClass::Graphics, Pass::GetDeviceIndex());
            }
            params.m_frameGraphBuilder->ImportScopeProducer(*m_feedbackScopes[stage]);
        }
    }

    bool DiffuseProbeGridSoftwareRayTracingPass::PrepareGridScratch()
    {
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        m_gridScratch.resize(grids.size());
        if (m_traceSrgs.size() > grids.size())
        {
            m_traceSrgs.resize(grids.size());
        }
        for (size_t index = 0; index < grids.size(); ++index)
        {
            auto& scratch = m_gridScratch[index];
            const uint64_t queryCount = uint64_t(grids[index]->GetNumRaysPerProbe().m_rayCount) *
                AZ::DivideAndRoundUp(grids[index]->GetTotalProbeCount(), grids[index]->GetFrameUpdateCount());
            if (queryCount > AZStd::numeric_limits<uint32_t>::max())
            {
                AZ_ErrorOnce("DiffuseProbeGrid", false, "Software GI hit queries exceed 32-bit addressing");
                return false;
            }
            if (scratch.m_queryCount != queryCount || AZStd::any_of(scratch.m_buffers.begin(), scratch.m_buffers.end(),
                [](const auto& buffer) { return !buffer; }))
            {
                scratch.m_queryCount = aznumeric_cast<uint32_t>(queryCount);
                for (uint32_t slot = 0; slot < ScratchCount; ++slot)
                {
                    scratch.m_attachmentIds[slot] = Name(AZStd::string::format("%s_Grid%zu_Scratch%u", m_attachmentIds[Nodes].GetCStr(), index, slot));
                    RPI::CommonBufferDescriptor descriptor;
                    descriptor.m_bufferName = scratch.m_attachmentIds[slot].GetCStr();
                    descriptor.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
                    descriptor.m_elementSize = slot == HitQueries ? 32 : 16;
                    descriptor.m_elementFormat = slot == Indirect ? RHI::Format::R32G32B32A32_FLOAT : RHI::Format::Unknown;
                    descriptor.m_byteCount = AZStd::max(uint64_t(1), queryCount) * descriptor.m_elementSize;
                    scratch.m_buffers[slot] = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
                    if (!scratch.m_buffers[slot])
                    {
                        return false;
                    }
                }
            }
            if (!scratch.m_queryObjectSrg)
            {
                const auto layout = m_feedbackShaders[QueryFeedback]->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object);
                scratch.m_queryObjectSrg = RPI::ShaderResourceGroup::Create(m_feedbackShaders[QueryFeedback]->GetAsset(), layout->GetName());
            }
            for (uint32_t stage = 0; stage < FeedbackStageCount; ++stage)
            {
                if (!scratch.m_srgs[stage])
                {
                    const auto layout = m_feedbackShaders[stage]->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
                    scratch.m_srgs[stage] = RPI::ShaderResourceGroup::Create(m_feedbackShaders[stage]->GetAsset(), layout->GetName());
                }
            }
        }
        return true;
    }

    void DiffuseProbeGridSoftwareRayTracingPass::UseScratchBuffer(
        RHI::FrameGraphInterface frameGraph, size_t gridIndex, uint32_t slot, RHI::ScopeAttachmentAccess access, bool clear)
    {
        const auto& scratch = m_gridScratch[gridIndex];
        const auto& id = scratch.m_attachmentIds[slot];
        if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
        {
            frameGraph.GetAttachmentDatabase().ImportBuffer(id, scratch.m_buffers[slot]->GetRHIBuffer());
        }
        RHI::BufferScopeAttachmentDescriptor descriptor;
        descriptor.m_attachmentId = id;
        descriptor.m_bufferViewDescriptor = scratch.m_buffers[slot]->GetBufferViewDescriptor();
        descriptor.m_loadStoreAction.m_loadAction = clear ? RHI::AttachmentLoadAction::Clear : RHI::AttachmentLoadAction::Load;
        if (clear)
        {
            descriptor.m_loadStoreAction.m_clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
        }
        frameGraph.UseShaderAttachment(descriptor, access, RHI::ScopeAttachmentStage::ComputeShader);
    }

    void DiffuseProbeGridSoftwareRayTracingPass::SetupFeedbackStage(RHI::FrameGraphInterface frameGraph, uint32_t stage)
    {
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(grids.size()));
        if (IsTimestampQueryEnabled() && m_feedbackTimestampQueries[stage])
        {
            m_feedbackTimestampQueries[stage]->AddToFrameGraph(frameGraph);
        }
        for (size_t index = 0; index < grids.size(); ++index)
        {
            const auto& grid = grids[index];
            if (stage == QueryFeedback)
            {
                UseScratchBuffer(frameGraph, index, HitQueries, RHI::ScopeAttachmentAccess::Read);
                UseScratchBuffer(frameGraph, index, Indirect, RHI::ScopeAttachmentAccess::ReadWrite, true);
                RHI::BufferScopeAttachmentDescriptor data;
                data.m_attachmentId = grid->GetGridDataBufferAttachmentId();
                data.m_bufferViewDescriptor = grid->GetRenderData()->m_gridDataBufferViewDescriptor;
                data.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(data, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
            }
            else
            {
                UseScratchBuffer(frameGraph, index, HitAlbedo, RHI::ScopeAttachmentAccess::Read);
                UseScratchBuffer(frameGraph, index, Indirect, RHI::ScopeAttachmentAccess::Read);
            }
            auto useImage = [&](RHI::AttachmentId id, const RHI::Ptr<RHI::Image>& resource,
                const RHI::ImageViewDescriptor& view, RHI::ScopeAttachmentAccess access)
            {
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(id, resource);
                }
                RHI::ImageScopeAttachmentDescriptor descriptor;
                descriptor.m_attachmentId = id;
                descriptor.m_imageViewDescriptor = view;
                descriptor.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(descriptor, access, RHI::ScopeAttachmentStage::ComputeShader);
            };
            const auto* render = grid->GetRenderData();
            if (stage == QueryFeedback)
            {
                useImage(grid->GetIrradianceImageAttachmentId(), grid->GetIrradianceImage(), render->m_probeIrradianceImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
                useImage(grid->GetDistanceImageAttachmentId(), grid->GetDistanceImage(), render->m_probeDistanceImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
                useImage(grid->GetProbeDataImageAttachmentId(), grid->GetProbeDataImage(), render->m_probeDataImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
            }
            else
            {
                useImage(grid->GetRayTraceImageAttachmentId(), grid->GetRayTraceImage(), render->m_probeRayTraceImageViewDescriptor, RHI::ScopeAttachmentAccess::ReadWrite);
            }
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::CompileFeedbackStage(uint32_t stage)
    {
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        for (size_t index = 0; index < grids.size(); ++index)
        {
            const auto& grid = grids[index];
            const auto& scratch = m_gridScratch[index];
            const auto& srg = scratch.m_srgs[stage];
            if (stage == QueryFeedback)
            {
                srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_irradianceQueries")), scratch.m_buffers[HitQueries]->GetBufferView());
                srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_output")), scratch.m_buffers[Indirect]->GetBufferView());
                // Pass-private bindings avoid competing with standalone queries or another render pipeline.
                grid->UpdateQuerySrgBindings(scratch.m_queryObjectSrg);
                scratch.m_queryObjectSrg->Compile();
            }
            else
            {
                srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_indirect")), scratch.m_buffers[Indirect]->GetBufferView());
                srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_hitAlbedo")), scratch.m_buffers[HitAlbedo]->GetBufferView());
                srg->SetImageView(srg->FindShaderInputImageIndex(Name("m_output")), grid->GetRayTraceImage()->GetImageView(grid->GetRenderData()->m_probeRayTraceImageViewDescriptor).get());
                srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_rayCount")), grid->GetNumRaysPerProbe().m_rayCount);
            }
            srg->Compile();
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::ExecuteFeedbackStage(const RHI::FrameGraphExecuteContext& context, uint32_t stage)
    {
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        const auto& query = m_feedbackTimestampQueries[stage];
        if (IsTimestampQueryEnabled() && query && context.GetSubmitRange().m_startIndex == 0)
        {
            query->BeginQuery(context);
        }
        for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
        {
            const auto& scratch = m_gridScratch[index];
            if (stage == QueryFeedback)
            {
                context.GetCommandList()->SetShaderResourceGroupForDispatch(
                    *scratch.m_queryObjectSrg->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));
            }
            const uint16_t groupX = stage == QueryFeedback ? m_queryArguments.m_threadsPerGroupX : 64;
            SubmitCompute(context, m_feedbackPipelines[stage].get(), nullptr, scratch.m_srgs[stage]->GetRHIShaderResourceGroup(),
                scratch.m_queryCount, 1, groupX, 1, index);
        }
        if (IsTimestampQueryEnabled() && query && context.GetSubmitRange().m_endIndex == grids.size())
        {
            query->EndQuery(context);
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::UseSceneBuffers(RHI::FrameGraphInterface frameGraph, bool refit)
    {
        for (uint32_t slot = 0; slot < BufferCount; ++slot)
        {
            if (!refit && slot == Work)
            {
                continue;
            }
            if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(m_attachmentIds[slot]))
            {
                frameGraph.GetAttachmentDatabase().ImportBuffer(m_attachmentIds[slot], m_buffers[slot]->GetRHIBuffer());
            }
            RHI::BufferScopeAttachmentDescriptor descriptor;
            descriptor.m_attachmentId = m_attachmentIds[slot];
            descriptor.m_bufferViewDescriptor = m_buffers[slot]->GetBufferViewDescriptor();
            descriptor.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
            frameGraph.UseShaderAttachment(descriptor, refit && slot == Bounds ? RHI::ScopeAttachmentAccess::ReadWrite : RHI::ScopeAttachmentAccess::Read,
                RHI::ScopeAttachmentStage::ComputeShader);
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::BindSceneBuffers(const Data::Instance<RPI::ShaderResourceGroup>& srg) const
    {
        for (uint32_t slot = 0; slot < BufferCount; ++slot)
        {
            const auto index = srg->FindShaderInputBufferIndex(Name(BufferNames[slot]));
            if (index.IsValid())
            {
                srg->SetBufferView(index, m_buffers[slot]->GetBufferView());
            }
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::PrepareRefit(RHI::FrameGraphInterface frameGraph, uint32_t level)
    {
        UseSceneBuffers(frameGraph, true);
        if (IsTimestampQueryEnabled() && m_refitTimestampQueries[level])
        {
            m_refitTimestampQueries[level]->AddToFrameGraph(frameGraph);
        }
        if (m_hasSkinningOutput)
        {
            frameGraph.UseShaderAttachment(m_skinningOutput, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
        }
        frameGraph.SetEstimatedItemCount(1);
    }

    void DiffuseProbeGridSoftwareRayTracingPass::CompileRefit(uint32_t level)
    {
        const auto& srg = m_refitSrgs[level];
        BindSceneBuffers(srg);
        srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_firstWork")), (*m_activeRanges)[level].m_first);
        srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_workCount")), (*m_activeRanges)[level].m_count);
        srg->Compile();
    }

    void DiffuseProbeGridSoftwareRayTracingPass::ExecuteRefit(const RHI::FrameGraphExecuteContext& context, uint32_t level)
    {
        // Each scope needs its own query: Vulkan resets a writable query interval at the start of that scope.
        if (IsTimestampQueryEnabled() && m_refitTimestampQueries[level])
        {
            m_refitTimestampQueries[level]->BeginQuery(context);
        }
        SubmitCompute(context, m_refitPipeline.get(), m_pipeline->GetScene()->GetRHIShaderResourceGroup(),
            m_refitSrgs[level]->GetRHIShaderResourceGroup(), (*m_activeRanges)[level].m_count, 1, 64, 1);
        if (IsTimestampQueryEnabled() && m_refitTimestampQueries[level])
        {
            m_refitTimestampQueries[level]->EndQuery(context);
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RenderPass::SetupFrameGraphDependencies(frameGraph);
        if (!m_ready)
        {
            return;
        }
        UseSceneBuffers(frameGraph, false);
        if (IsTimestampQueryEnabled() && m_fullPassTimestampQuery)
        {
            m_fullPassTimestampQuery->AddToFrameGraph(frameGraph);
        }
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(grids.size()));
        for (size_t gridIndex = 0; gridIndex < grids.size(); ++gridIndex)
        {
            const auto& grid = grids[gridIndex];
            UseScratchBuffer(frameGraph, gridIndex, HitQueries, RHI::ScopeAttachmentAccess::ReadWrite);
            UseScratchBuffer(frameGraph, gridIndex, HitAlbedo, RHI::ScopeAttachmentAccess::ReadWrite);
            RHI::BufferScopeAttachmentDescriptor data;
            data.m_attachmentId = grid->GetGridDataBufferAttachmentId();
            data.m_bufferViewDescriptor = grid->GetRenderData()->m_gridDataBufferViewDescriptor;
            data.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
            frameGraph.UseShaderAttachment(data, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
            auto image = [&](RHI::AttachmentId id, const RHI::Ptr<RHI::Image>& resource,
                const RHI::ImageViewDescriptor& view, RHI::ScopeAttachmentAccess access)
            {
                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(id))
                {
                    frameGraph.GetAttachmentDatabase().ImportImage(id, resource);
                }
                RHI::ImageScopeAttachmentDescriptor descriptor;
                descriptor.m_attachmentId = id;
                descriptor.m_imageViewDescriptor = view;
                descriptor.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;
                frameGraph.UseShaderAttachment(descriptor, access, RHI::ScopeAttachmentStage::ComputeShader);
            };
            const auto* render = grid->GetRenderData();
            image(grid->GetRayTraceImageAttachmentId(), grid->GetRayTraceImage(), render->m_probeRayTraceImageViewDescriptor, RHI::ScopeAttachmentAccess::ReadWrite);
            image(grid->GetIrradianceImageAttachmentId(), grid->GetIrradianceImage(), render->m_probeIrradianceImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
            image(grid->GetDistanceImageAttachmentId(), grid->GetDistanceImage(), render->m_probeDistanceImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
            image(grid->GetProbeDataImageAttachmentId(), grid->GetProbeDataImage(), render->m_probeDataImageViewDescriptor, RHI::ScopeAttachmentAccess::Read);
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::CompileResources(const RHI::FrameGraphCompileContext&)
    {
        if (!m_ready)
        {
            return;
        }
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        for (size_t index = 0; index < grids.size(); ++index)
        {
            if (index == m_traceSrgs.size())
            {
                m_traceSrgs.push_back(RPI::ShaderResourceGroup::Create(m_traceShader->GetAsset(), Name("PassSrg")));
            }
            const auto& srg = m_traceSrgs[index];
            const auto& grid = grids[index];
            BindSceneBuffers(srg);
            srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_hitQueries")), m_gridScratch[index].m_buffers[HitQueries]->GetBufferView());
            srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_hitAlbedo")), m_gridScratch[index].m_buffers[HitAlbedo]->GetBufferView());
            srg->SetBufferView(srg->FindShaderInputBufferIndex(Name("m_gridData")), grid->GetGridDataBuffer()->GetBufferView(grid->GetRenderData()->m_gridDataBufferViewDescriptor).get());
            srg->SetImageView(srg->FindShaderInputImageIndex(Name("m_output")), grid->GetRayTraceImage()->GetImageView(grid->GetRenderData()->m_probeRayTraceImageViewDescriptor).get());
            srg->SetImageView(srg->FindShaderInputImageIndex(Name("m_probeData")), grid->GetProbeDataImage()->GetImageView(grid->GetRenderData()->m_probeDataImageViewDescriptor).get());
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_instanceCount")), aznumeric_cast<uint32_t>(m_gpuData.m_instances.size()));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_tlasTopologyBase")), m_gpuData.m_tlasTopologyBase);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_tlasBoundsBase")), m_gpuData.m_tlasBoundsBase);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_tlasPrimitiveBase")), m_gpuData.m_tlasPrimitiveBase);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_frameUpdateCount")), grid->GetFrameUpdateCount());
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_frameUpdateIndex")), grid->GetFrameUpdateIndex());
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_ambientMultiplier")), grid->GetAmbientMultiplier());
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_emissiveMultiplier")), grid->GetEmissiveMultiplier());
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_frameNumber")), m_frameNumber);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_transparencyMode")), uint32_t(grid->GetTransparencyMode()));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_giShadows")), uint32_t(grid->GetGIShadows()));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_useDiffuseIbl")), uint32_t(grid->GetUseDiffuseIbl()));
            srg->Compile();
        }
    }

    void DiffuseProbeGridSoftwareRayTracingPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_ready)
        {
            return;
        }
        const auto& grids = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>()->GetVisibleRealTimeProbeGrids();
        if (IsTimestampQueryEnabled() && m_fullPassTimestampQuery && context.GetSubmitRange().m_startIndex == 0)
        {
            m_fullPassTimestampQuery->BeginQuery(context);
        }
        for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
        {
            const auto& grid = grids[index];
            const auto view = m_pipeline->GetDefaultView();
            if (!view)
            {
                continue;
            }
            context.GetCommandList()->SetShaderResourceGroupForDispatch(
                *view->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));
            SubmitCompute(context, m_tracePipeline.get(), m_pipeline->GetScene()->GetRHIShaderResourceGroup(),
                m_traceSrgs[index]->GetRHIShaderResourceGroup(), grid->GetNumRaysPerProbe().m_rayCount,
                AZ::DivideAndRoundUp(grid->GetTotalProbeCount(), grid->GetFrameUpdateCount()), 8, 8, index);
        }
        if (IsTimestampQueryEnabled() && m_fullPassTimestampQuery && context.GetSubmitRange().m_endIndex == grids.size())
        {
            m_fullPassTimestampQuery->EndQuery(context);
            m_timestampDevice = context.GetDeviceIndex();
        }
    }
}
