/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Render/DiffuseProbeGridSoftwareScene.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AzCore/std/containers/array.h>

namespace AZ::Render
{
    class DiffuseProbeGridSoftwareRayTracingPass final : public RPI::RenderPass
    {
    public:
        AZ_RPI_PASS(DiffuseProbeGridSoftwareRayTracingPass);
        AZ_RTTI(DiffuseProbeGridSoftwareRayTracingPass, "{AF67D649-436E-4320-BB82-A359BE7EB104}", RPI::RenderPass);
        AZ_CLASS_ALLOCATOR(DiffuseProbeGridSoftwareRayTracingPass, SystemAllocator);
        static RPI::Ptr<DiffuseProbeGridSoftwareRayTracingPass> Create(const RPI::PassDescriptor& descriptor);
        //! Drop Gem-owned acceleration, scratch and shader resources when the backend is no longer requested.
        void ReleaseResources();
        bool IsReadyToTrace() const { return m_ready; }

    private:
        explicit DiffuseProbeGridSoftwareRayTracingPass(const RPI::PassDescriptor& descriptor);
        bool IsEnabled() const override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        RPI::TimestampResult GetTimestampResultInternal() const override { return m_fullPassTimestamp; }

        bool LoadShaders();
        void UploadScene();
        bool PrepareGridScratch();
        void SetupFeedbackStage(RHI::FrameGraphInterface frameGraph, uint32_t stage);
        void CompileFeedbackStage(uint32_t stage);
        void ExecuteFeedbackStage(const RHI::FrameGraphExecuteContext& context, uint32_t stage);
        void UseScratchBuffer(RHI::FrameGraphInterface frameGraph, size_t gridIndex, uint32_t slot,
            RHI::ScopeAttachmentAccess access, bool clear = false);

        void UseSceneBuffers(RHI::FrameGraphInterface frameGraph, bool refit);
        void BindSceneBuffers(const Data::Instance<RPI::ShaderResourceGroup>& srg) const;
        void PrepareRefit(RHI::FrameGraphInterface frameGraph, uint32_t level);
        void CompileRefit(uint32_t level);
        void ExecuteRefit(const RHI::FrameGraphExecuteContext& context, uint32_t level);

        enum BufferSlot : uint32_t { Nodes, Bounds, Primitives, Instances, Work, BufferCount };
        enum ScratchSlot : uint32_t { HitQueries, HitAlbedo, Indirect, ScratchCount };
        enum FeedbackStage : uint32_t { QueryFeedback, IntegrateFeedback, FeedbackStageCount };
        struct GridScratch
        {
            uint32_t m_queryCount = 0;
            Data::Instance<RPI::ShaderResourceGroup> m_queryObjectSrg;
            AZStd::array<Data::Instance<RPI::Buffer>, ScratchCount> m_buffers;
            AZStd::array<RHI::AttachmentId, ScratchCount> m_attachmentIds;
            AZStd::array<Data::Instance<RPI::ShaderResourceGroup>, FeedbackStageCount> m_srgs;
        };
        AZStd::vector<GridScratch> m_gridScratch;
        AZStd::array<AZStd::unique_ptr<RHI::ScopeProducerFunctionNoData>, FeedbackStageCount> m_feedbackScopes;
        AZStd::array<Data::Instance<RPI::Shader>, FeedbackStageCount> m_feedbackShaders;
        AZStd::array<RHI::ConstPtr<RHI::PipelineState>, FeedbackStageCount> m_feedbackPipelines;
        AZStd::array<RHI::Ptr<RPI::Query>, FeedbackStageCount> m_feedbackTimestampQueries;
        RHI::DispatchDirect m_queryArguments;

        struct Range { uint32_t m_first = 0; uint32_t m_count = 0; };
        struct InstanceKey
        {
            AZStd::shared_ptr<DiffuseProbeGridSoftwareScene::Geometry> m_geometry;
            MeshInfoHandle m_meshInfoHandle;
            bool m_skinned;
            Matrix3x4 m_transform;
        };

        DiffuseProbeGridSoftwareScene m_scene;
        DiffuseProbeGridSoftwareScene::GpuData m_gpuData;
        AZStd::vector<InstanceKey> m_keys;
        AZStd::array<Data::Instance<RPI::Buffer>, BufferCount> m_buffers;
        AZStd::array<RHI::AttachmentId, BufferCount> m_attachmentIds;
        AZStd::vector<Range> m_initialRanges;
        AZStd::vector<Range> m_dynamicRanges;
        const AZStd::vector<Range>* m_activeRanges = nullptr;
        AZStd::vector<AZStd::unique_ptr<RHI::ScopeProducerFunctionNoData>> m_refitScopes;
        AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_refitSrgs;
        AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_traceSrgs;
        Data::Instance<RPI::Shader> m_traceShader;
        Data::Instance<RPI::Shader> m_refitShader;
        RHI::ConstPtr<RHI::PipelineState> m_tracePipeline;
        RHI::ConstPtr<RHI::PipelineState> m_refitPipeline;
        RHI::BufferScopeAttachmentDescriptor m_skinningOutput;
        bool m_hasSkinningOutput = false;
        RHI::Ptr<RPI::Query> m_fullPassTimestampQuery;
        AZStd::vector<RHI::Ptr<RPI::Query>> m_refitTimestampQueries;
        RPI::TimestampResult m_fullPassTimestamp;
        int m_timestampDevice = RHI::MultiDevice::InvalidDeviceIndex;
        bool m_ready = false;
        uint32_t m_frameNumber = 0;
    };
}
