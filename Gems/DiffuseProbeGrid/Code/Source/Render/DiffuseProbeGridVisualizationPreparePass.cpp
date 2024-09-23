/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridVisualizationPreparePass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridVisualizationPreparePass> DiffuseProbeGridVisualizationPreparePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridVisualizationPreparePass> diffuseProbeGridVisualizationPreparePass = aznew DiffuseProbeGridVisualizationPreparePass(descriptor);
            return AZStd::move(diffuseProbeGridVisualizationPreparePass);
        }

        DiffuseProbeGridVisualizationPreparePass::DiffuseProbeGridVisualizationPreparePass(const RPI::PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
            // disable this pass if we're on a platform that doesn't support raytracing
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices || !AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                SetEnabled(false);
            }
            else
            {
                LoadShader();
            }
        }

        void DiffuseProbeGridVisualizationPreparePass::LoadShader()
        {
            // load shaders
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridVisualizationPrepare.azshader";
            m_shader = RPI::LoadCriticalShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            AZ_Assert(m_pipelineState, "Failed to acquire pipeline state");

            m_srgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
            AZ_Assert(m_srgLayout.get(), "Failed to find Srg layout");

            const auto outcome = RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridVisualizationPreparePass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridVisualizationPreparePass::ShouldUpdate(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid) const
        {
            return (diffuseProbeGrid->GetVisualizationEnabled() && diffuseProbeGrid->GetVisualizationTlasUpdateRequired());
        }

        bool DiffuseProbeGridVisualizationPreparePass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (diffuseProbeGridFeatureProcessor)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
                {
                    if (ShouldUpdate(diffuseProbeGrid))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        void DiffuseProbeGridVisualizationPreparePass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                // create the TLAS descriptor by adding an instance entry for each probe in the grid
                RHI::RayTracingTlasDescriptor tlasDescriptor;
                RHI::RayTracingTlasDescriptor* tlasDescriptorBuild = tlasDescriptor.Build();

                // initialize the transform for each probe to Identity(), they will be updated by the compute shader
                AZ::Transform transform = AZ::Transform::Identity();

                uint32_t probeCount = diffuseProbeGrid->GetTotalProbeCount();
                for (uint32_t index = 0; index < probeCount; ++index)
                {
                    tlasDescriptorBuild->Instance()
                        ->InstanceID(index)
                        ->InstanceMask(1)
                        ->HitGroupIndex(0)
                        ->Blas(diffuseProbeGridFeatureProcessor->GetVisualizationBlas())
                        ->Transform(transform)
                    ;
                }

                // create the TLAS buffers from on the descriptor
                RHI::Ptr<RHI::RayTracingTlas>& visualizationTlas = diffuseProbeGrid->GetVisualizationTlas();
                visualizationTlas->CreateBuffers(RHI::MultiDevice::AllDevices, &tlasDescriptor, diffuseProbeGridFeatureProcessor->GetVisualizationBufferPools());
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridVisualizationPreparePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                // import and attach the visualization TLAS and probe data
                RHI::Ptr<RHI::RayTracingTlas>& visualizationTlas = diffuseProbeGrid->GetVisualizationTlas();
                const RHI::Ptr<RHI::Buffer>& tlasBuffer = visualizationTlas->GetTlasBuffer();
                const RHI::Ptr<RHI::Buffer>& tlasInstancesBuffer = visualizationTlas->GetTlasInstancesBuffer();
                if (tlasBuffer && tlasInstancesBuffer)
                {
                    // TLAS buffer
                    {
                        AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetProbeVisualizationTlasAttachmentId();
                        if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId) == false)
                        {
                            [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(attachmentId, tlasBuffer);
                            AZ_Assert(result == RHI::ResultCode::Success, "Failed to import DiffuseProbeGrid visualization TLAS buffer with error %d", result);
                        }

                        uint32_t byteCount = aznumeric_cast<uint32_t>(tlasBuffer->GetDescriptor().m_byteCount);
                        RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(byteCount);

                        RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = attachmentId;
                        desc.m_bufferViewDescriptor = bufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentStage::ComputeShader);
                    }

                    // TLAS Instances buffer
                    {
                        AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetProbeVisualizationTlasInstancesAttachmentId();
                        if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId) == false)
                        {
                            [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(attachmentId, tlasInstancesBuffer);
                            AZ_Assert(result == RHI::ResultCode::Success, "Failed to import DiffuseProbeGrid visualization TLAS Instances buffer with error %d", result);
                        }

                        uint32_t byteCount = aznumeric_cast<uint32_t>(tlasInstancesBuffer->GetDescriptor().m_byteCount);
                        RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, byteCount / RayTracingTlasInstanceElementSize, RayTracingTlasInstanceElementSize);

                        RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = attachmentId;
                        desc.m_bufferViewDescriptor = bufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentStage::ComputeShader);
                    }

                    // grid data
                    {
                        RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                        desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                    }

                    // probe data
                    {
                        AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                        if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId) == false)
                        {
                            [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetProbeDataImage());
                            AZ_Assert(result == RHI::ResultCode::Success, "Failed to import DiffuseProbeGrid probe data buffer with error %d", result);
                        }

                        RHI::ImageScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = attachmentId;
                        desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::ComputeShader);
                    }
                }
            }
        }

        void DiffuseProbeGridVisualizationPreparePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                // the DiffuseProbeGrid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateVisualizationPrepareSrg(m_shader, m_srgLayout);
                if (!diffuseProbeGrid->GetVisualizationPrepareSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetVisualizationPrepareSrg()->Compile();
                }
            }
        }

        void DiffuseProbeGridVisualizationPreparePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                const RHI::DeviceShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetVisualizationPrepareSrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);

                RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = diffuseProbeGrid->GetTotalProbeCount();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }
    }   // namespace RPI
}   // namespace AZ
