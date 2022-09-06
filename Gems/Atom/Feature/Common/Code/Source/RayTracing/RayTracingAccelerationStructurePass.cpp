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
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <RayTracing/RayTracingAccelerationStructurePass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<RayTracingAccelerationStructurePass> RayTracingAccelerationStructurePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RayTracingAccelerationStructurePass> rayTracingAccelerationStructurePass = aznew RayTracingAccelerationStructurePass(descriptor);
            return AZStd::move(rayTracingAccelerationStructurePass);
        }

        RayTracingAccelerationStructurePass::RayTracingAccelerationStructurePass(const RPI::PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            // disable this pass if we're on a platform that doesn't support raytracing
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                SetEnabled(false);
            }
        }

        void RayTracingAccelerationStructurePass::BuildInternal()
        {
            InitScope(RHI::ScopeId(GetPathName()));
        }

        void RayTracingAccelerationStructurePass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_frameGraphBuilder->ImportScopeProducer(*this);
        }

        void RayTracingAccelerationStructurePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (rayTracingFeatureProcessor)
            {
                if (rayTracingFeatureProcessor->GetRevision() != m_rayTracingRevision)
                {
                    RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();
                    RayTracingFeatureProcessor::SubMeshVector& subMeshes = rayTracingFeatureProcessor->GetSubMeshes();
                    uint32_t rayTracingSubMeshCount = rayTracingFeatureProcessor->GetSubMeshCount();

                    // create the TLAS descriptor
                    RHI::RayTracingTlasDescriptor tlasDescriptor;
                    RHI::RayTracingTlasDescriptor* tlasDescriptorBuild = tlasDescriptor.Build();

                    uint32_t instanceIndex = 0;
                    for (auto& subMesh : subMeshes)
                    {
                        tlasDescriptorBuild->Instance()
                            ->InstanceID(instanceIndex)
                            ->HitGroupIndex(0)
                            ->Blas(subMesh.m_blas)
                            ->Transform(subMesh.m_mesh->m_transform)
                            ->NonUniformScale(subMesh.m_mesh->m_nonUniformScale)
                            ->Transparent(subMesh.m_irradianceColor.GetA() < 1.0f)
                            ;

                        instanceIndex++;
                    }

                    // create the TLAS buffers based on the descriptor
                    RHI::Ptr<RHI::RayTracingTlas>& rayTracingTlas = rayTracingFeatureProcessor->GetTlas();
                    rayTracingTlas->CreateBuffers(*device, &tlasDescriptor, rayTracingBufferPools);

                    // import and attach the TLAS buffer
                    const RHI::Ptr<RHI::Buffer>& rayTracingTlasBuffer = rayTracingTlas->GetTlasBuffer();
                    if (rayTracingTlasBuffer && rayTracingSubMeshCount)
                    {
                        AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                        if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId) == false)
                        {
                            [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                            AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                        }

                        uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingTlasBuffer->GetDescriptor().m_byteCount);
                        RHI::BufferViewDescriptor tlasBufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

                        RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = tlasAttachmentId;
                        desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write);
                    }
                }

                // update and compile the RayTracingSceneSrg and RayTracingMaterialSrg
                // Note: the timing of this update is very important, it needs to be updated after the TLAS is allocated so it can
                // be set on the RayTracingSceneSrg for this frame, and the ray tracing mesh data in the RayTracingSceneSrg must
                // exactly match the TLAS.  Any mismatch in this data may result in a TDR.
                rayTracingFeatureProcessor->UpdateRayTracingSrgs();
            }
        }

        void RayTracingAccelerationStructurePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (!rayTracingFeatureProcessor)
            {
                return;
            }

            if (!rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer())
            {
                return;
            }

            if (rayTracingFeatureProcessor->GetRevision() == m_rayTracingRevision)
            {
                // TLAS is up to date
                return;
            }

            // update the stored revision, even if we don't have any meshes to process
            m_rayTracingRevision = rayTracingFeatureProcessor->GetRevision();

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // no ray tracing meshes in the scene
                return;
            }

            // build newly added BLAS objects
            RayTracingFeatureProcessor::BlasInstanceMap& blasInstances = rayTracingFeatureProcessor->GetBlasInstances();
            for (auto& blasInstance : blasInstances)
            {
                if (blasInstance.second.m_blasBuilt == false)
                {
                    for (auto& blasInstanceSubMesh : blasInstance.second.m_subMeshes)
                    {
                        context.GetCommandList()->BuildBottomLevelAccelerationStructure(*blasInstanceSubMesh.m_blas);
                    }

                    blasInstance.second.m_blasBuilt = true;
                }
            }

            // build the TLAS object
            context.GetCommandList()->BuildTopLevelAccelerationStructure(*rayTracingFeatureProcessor->GetTlas());
        }
    }   // namespace RPI
}   // namespace AZ
