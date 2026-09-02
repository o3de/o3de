/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

namespace AZ::Render
{
    //! Reads the GPU-driven cull SurvivorCount buffer (number of instances that passed
    //! frustum/occlusion culling this frame) back to the CPU via a fence-guarded async copy,
    //! so r_gpuDrivenStats can report the actual visible count, not just the registered total.
    //! Modeled on DepthOfFieldCopyFocusDepthToCpuPass, but sources a transient pass attachment.
    class GpuCullStatsReadbackPass final
        : public RPI::Pass
        , public RHI::ScopeProducer
    {
        AZ_RPI_PASS(GpuCullStatsReadbackPass);

    public:
        AZ_RTTI(GpuCullStatsReadbackPass, "{7C3E9A21-4B6D-4F18-9E2A-1D5C8B3F7A60}", RPI::Pass);
        AZ_CLASS_ALLOCATOR(GpuCullStatsReadbackPass, SystemAllocator);

        static RPI::Ptr<GpuCullStatsReadbackPass> Create(const RPI::PassDescriptor& descriptor);
        ~GpuCullStatsReadbackPass() = default;

        //! Most recent survivor (visible instance) count read back from the GPU (a few frames stale).
        uint32_t GetVisibleCount() const { return m_lastVisibleCount; }

    private:
        explicit GpuCullStatsReadbackPass(const RPI::PassDescriptor& descriptor);

        // ScopeProducer overrides
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

        // Pass overrides
        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

        RPI::PassAttachmentBinding* m_survivorCountBinding = nullptr;
        Data::Instance<RPI::Buffer> m_readbackBuffer;
        RHI::CopyBufferDescriptor m_copyDescriptor;
        bool m_needsInitialize = true;
        RHI::Ptr<RHI::Fence> m_fence;
        uint32_t m_lastVisibleCount = 0;
    };
} // namespace AZ::Render
