/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom_Feature_Traits_Platform.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridVisualizationAccelerationStructurePass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridVisualizationAccelerationStructurePass> DiffuseProbeGridVisualizationAccelerationStructurePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridVisualizationAccelerationStructurePass> diffuseProbeGridVisualizationAccelerationStructurePass = aznew DiffuseProbeGridVisualizationAccelerationStructurePass(descriptor);
            return AZStd::move(diffuseProbeGridVisualizationAccelerationStructurePass);
        }

        DiffuseProbeGridVisualizationAccelerationStructurePass::DiffuseProbeGridVisualizationAccelerationStructurePass(const RPI::PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            // disable this pass if we're on a platform that doesn't support raytracing
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false || !AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                SetEnabled(false);
            }
        }

        bool DiffuseProbeGridVisualizationAccelerationStructurePass::ShouldUpdate(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid) const
        {
            return (diffuseProbeGrid->GetVisualizationEnabled() && diffuseProbeGrid->GetVisualizationTlasUpdateRequired());
        }

        bool DiffuseProbeGridVisualizationAccelerationStructurePass::IsEnabled() const
        {
            if (!Pass::IsEnabled())
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

        void DiffuseProbeGridVisualizationAccelerationStructurePass::BuildInternal()
        {
            InitScope(RHI::ScopeId(GetPathName()));
        }

        void DiffuseProbeGridVisualizationAccelerationStructurePass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_frameGraphBuilder->ImportScopeProducer(*this);
        }       

        void DiffuseProbeGridVisualizationAccelerationStructurePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                // import and attach the visualization TLAS buffers
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

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write);
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
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);
                    }
                }
            }
        }

        void DiffuseProbeGridVisualizationAccelerationStructurePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // build the visualization BLAS from the DiffuseProbeGridFeatureProcessor
            // Note: the BLAS is used by all DiffuseProbeGrid visualization TLAS objects
            if (m_visualizationBlasBuilt == false)
            {
                context.GetCommandList()->BuildBottomLevelAccelerationStructure(*diffuseProbeGridFeatureProcessor->GetVisualizationBlas());
                m_visualizationBlasBuilt = true;
            }

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }

                if (!diffuseProbeGrid->GetVisualizationTlas()->GetTlasBuffer())
                {
                    continue;
                }

                // build the TLAS object
                context.GetCommandList()->BuildTopLevelAccelerationStructure(*diffuseProbeGrid->GetVisualizationTlas());
            }
        }

        void DiffuseProbeGridVisualizationAccelerationStructurePass::FrameEndInternal()
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!ShouldUpdate(diffuseProbeGrid))
                {
                    continue;
                }
            
                // TLAS is now updated
                diffuseProbeGrid->ResetVisualizationTlasUpdateRequired();
            }
        }
    }   // namespace RPI
}   // namespace AZ
