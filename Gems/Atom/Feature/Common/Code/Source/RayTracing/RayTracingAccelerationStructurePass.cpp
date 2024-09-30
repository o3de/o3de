/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <RayTracing/RayTracingAccelerationStructurePass.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

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
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices)
            {
                SetEnabled(false);
            }
        }

        void RayTracingAccelerationStructurePass::BuildInternal()
        {
            auto deviceIndex = Pass::GetDeviceIndex();
            InitScope(
                RHI::ScopeId(AZStd::string(GetPathName().GetCStr() + AZStd::to_string(deviceIndex))),
                AZ::RHI::HardwareQueueClass::Compute,
                deviceIndex);
        }

        void RayTracingAccelerationStructurePass::FrameBeginInternal(FramePrepareParams params)
        {
            if (GetScopeId().IsEmpty())
            {
                InitScope(RHI::ScopeId(GetPathName()), RHI::HardwareQueueClass::Compute, Pass::GetDeviceIndex());
            }

            params.m_frameGraphBuilder->ImportScopeProducer(*this);

            ReadbackScopeQueryResults();

            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (rayTracingFeatureProcessor)
            {
                auto revision = rayTracingFeatureProcessor->BeginFrame();
                m_rayTracingRevisionOutDated = revision != m_rayTracingRevision;
                if (m_rayTracingRevisionOutDated)
                {
                    m_rayTracingRevision = revision;
                }
            }
        }

        RHI::Ptr<RPI::Query> RayTracingAccelerationStructurePass::GetQuery(RPI::ScopeQueryType queryType)
        {
            auto typeIndex{ static_cast<uint32_t>(queryType) };
            if (!m_scopeQueries[typeIndex])
            {
                RHI::Ptr<RPI::Query> query;
                switch (queryType)
                {
                case RPI::ScopeQueryType::Timestamp:
                    query = RPI::GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::Timestamp, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
                    break;
                case RPI::ScopeQueryType::PipelineStatistics:
                    query = RPI::GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::PipelineStatistics, RHI::QueryPoolScopeAttachmentType::Global,
                        RHI::ScopeAttachmentAccess::Write);
                    break;
                }

                m_scopeQueries[typeIndex] = query;
            }

            return m_scopeQueries[typeIndex];
        }

        template<typename Func>
        inline void RayTracingAccelerationStructurePass::ExecuteOnTimestampQuery(Func&& func)
        {
            if (IsTimestampQueryEnabled())
            {
                auto query{ GetQuery(RPI::ScopeQueryType::Timestamp) };
                if (query)
                {
                    func(query);
                }
            }
        }

        template<typename Func>
        inline void RayTracingAccelerationStructurePass::ExecuteOnPipelineStatisticsQuery(Func&& func)
        {
            if (IsPipelineStatisticsQueryEnabled())
            {
                auto query{ GetQuery(RPI::ScopeQueryType::PipelineStatistics) };
                if (query)
                {
                    func(query);
                }
            }
        }

        RPI::TimestampResult RayTracingAccelerationStructurePass::GetTimestampResultInternal() const
        {
            return m_timestampResult;
        }

        RPI::PipelineStatisticsResult RayTracingAccelerationStructurePass::GetPipelineStatisticsResultInternal() const
        {
            return m_statisticsResult;
        }

        void RayTracingAccelerationStructurePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (rayTracingFeatureProcessor)
            {
                if (m_rayTracingRevisionOutDated)
                {
                    // create the TLAS buffers based on the descriptor
                    RHI::Ptr<RHI::RayTracingTlas>& rayTracingTlas = rayTracingFeatureProcessor->GetTlas();

                    // import and attach the TLAS buffer
                    const RHI::Ptr<RHI::Buffer>& rayTracingTlasBuffer = rayTracingTlas->GetTlasBuffer();
                    if (rayTracingTlasBuffer && rayTracingFeatureProcessor->HasGeometry())
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

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write, RHI::ScopeAttachmentStage::RayTracingShader);
                    }
                }

                // Attach output data from the skinning pass. This is needed to ensure that this pass is executed after
                // the skinning pass has finished. We assume that the pipeline has a skinning pass with this output available.
                if (rayTracingFeatureProcessor->GetSkinnedMeshCount() > 0)
                {
                    auto skinningPassPtr = FindAdjacentPass(AZ::Name("SkinningPass"));
                    auto skinnedMeshOutputStreamBindingPtr = skinningPassPtr->FindAttachmentBinding(AZ::Name("SkinnedMeshOutputStream"));
                    [[maybe_unused]] auto result = frameGraph.UseShaderAttachment(skinnedMeshOutputStreamBindingPtr->m_unifiedScopeDesc.GetAsBuffer(), RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::RayTracingShader);
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to attach SkinnedMeshOutputStream buffer with error %d", result);
                }

                AddScopeQueryToFrameGraph(frameGraph);
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

            if (!m_rayTracingRevisionOutDated && rayTracingFeatureProcessor->GetSkinnedMeshCount() == 0)
            {
                // TLAS is up to date
                return;
            }

            if (!rayTracingFeatureProcessor->HasGeometry())
            {
                // no ray tracing meshes in the scene
                return;
            }

            BeginScopeQuery(context);

            // build newly added or skinned BLAS objects
            AZStd::vector<const AZ::RHI::DeviceRayTracingBlas*> changedBlasList;
            RayTracingFeatureProcessor::BlasInstanceMap& blasInstances = rayTracingFeatureProcessor->GetBlasInstances();
            for (auto& blasInstance : blasInstances)
            {
                const bool isSkinnedMesh = blasInstance.second.m_isSkinnedMesh;
                const bool buildBlas = (blasInstance.second.m_blasBuilt & RHI::MultiDevice::DeviceMask(1 << context.GetDeviceIndex())) ==
                    RHI::MultiDevice::NoDevices;
                if (buildBlas || isSkinnedMesh)
                {
                    for (auto submeshIndex = 0; submeshIndex < blasInstance.second.m_subMeshes.size(); ++submeshIndex)
                    {
                        auto& submeshBlasInstance = blasInstance.second.m_subMeshes[submeshIndex];
                        changedBlasList.push_back(submeshBlasInstance.m_blas->GetDeviceRayTracingBlas(context.GetDeviceIndex()).get());
                        if (buildBlas)
                        {
                            // Always build the BLAS, if it has not previously been built
                            context.GetCommandList()->BuildBottomLevelAccelerationStructure(*submeshBlasInstance.m_blas->GetDeviceRayTracingBlas(context.GetDeviceIndex()));
                            continue;
                        }

                        // Determine if a skinned mesh BLAS needs to be updated or completely rebuilt. For now, we want to rebuild a BLAS every
                        // SKINNED_BLAS_REBUILD_FRAME_INTERVAL frames, while updating it all other frames. This is based on the assumption that
                        // by adding together the asset ID hash, submesh index, and frame count, we get a value that allows us to uniformly
                        // distribute rebuilding all skinned mesh BLASs over all frames.
                        auto assetGuid = blasInstance.first.m_guid.GetHash();
                        if (isSkinnedMesh && (assetGuid + submeshIndex + m_frameCount) % SKINNED_BLAS_REBUILD_FRAME_INTERVAL != 0)
                        {
                            // Skinned mesh that simply needs an update
                            context.GetCommandList()->UpdateBottomLevelAccelerationStructure(
                                *submeshBlasInstance.m_blas->GetDeviceRayTracingBlas(context.GetDeviceIndex()));
                        }
                        else
                        {
                            // Fall back to building the BLAS in any case
                            context.GetCommandList()->BuildBottomLevelAccelerationStructure(
                                *submeshBlasInstance.m_blas->GetDeviceRayTracingBlas(context.GetDeviceIndex()));
                        }
                    }

                    AZStd::lock_guard lock(rayTracingFeatureProcessor->GetBlasBuiltMutex());
                    blasInstance.second.m_blasBuilt |= RHI::MultiDevice::DeviceMask(1 << context.GetDeviceIndex());
                }
            }

            // build the TLAS object
            context.GetCommandList()->BuildTopLevelAccelerationStructure(*rayTracingFeatureProcessor->GetTlas()->GetDeviceRayTracingTlas(context.GetDeviceIndex()), changedBlasList);

            ++m_frameCount;

            EndScopeQuery(context);
        }

        void RayTracingAccelerationStructurePass::AddScopeQueryToFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            const auto addToFrameGraph = [&frameGraph](RHI::Ptr<RPI::Query> query)
            {
              query->AddToFrameGraph(frameGraph);
            };

            ExecuteOnTimestampQuery(addToFrameGraph);
            ExecuteOnPipelineStatisticsQuery(addToFrameGraph);
        }

        void RayTracingAccelerationStructurePass::BeginScopeQuery(const RHI::FrameGraphExecuteContext& context)
        {
            const auto beginQuery = [&context, this](RHI::Ptr<RPI::Query> query)
            {
              if (query->BeginQuery(context) == RPI::QueryResultCode::Fail)
              {
                  AZ_UNUSED(this); // Prevent unused warning in release builds
                  AZ_WarningOnce(
                      "RayTracingAccelerationStructurePass", false,
                      "BeginScopeQuery failed. Make sure AddScopeQueryToFrameGraph was called in SetupFrameGraphDependencies"
                      " for this pass: %s",
                      this->RTTI_GetTypeName());
              }
            };

            ExecuteOnTimestampQuery(beginQuery);
            ExecuteOnPipelineStatisticsQuery(beginQuery);
        }

        void RayTracingAccelerationStructurePass::EndScopeQuery(const RHI::FrameGraphExecuteContext& context)
        {
            const auto endQuery = [&context](const RHI::Ptr<RPI::Query>& query)
            {
              query->EndQuery(context);
            };

            // This scope query implementation should be replaced by the feature linked below on GitHub:
            // [GHI-16945] Feature Request - Add GPU timestamp and pipeline statistic support for scopes
            ExecuteOnTimestampQuery(endQuery);
            ExecuteOnPipelineStatisticsQuery(endQuery);

            m_lastDeviceIndex = context.GetDeviceIndex();
        }

        void RayTracingAccelerationStructurePass::ReadbackScopeQueryResults()
        {
            ExecuteOnTimestampQuery(
                [this](const RHI::Ptr<RPI::Query>& query)
                {
                    const uint32_t TimestampResultQueryCount{ 2u };
                    uint64_t timestampResult[TimestampResultQueryCount] = { 0 };
                    query->GetLatestResult(&timestampResult, sizeof(uint64_t) * TimestampResultQueryCount, m_lastDeviceIndex);
                    m_timestampResult = RPI::TimestampResult(timestampResult[0], timestampResult[1], RHI::HardwareQueueClass::Compute);
                });

            ExecuteOnPipelineStatisticsQuery(
                [this](const RHI::Ptr<RPI::Query>& query)
                {
                    query->GetLatestResult(&m_statisticsResult, sizeof(RPI::PipelineStatisticsResult), m_lastDeviceIndex);
                });
        }
    }   // namespace RPI
}   // namespace AZ
