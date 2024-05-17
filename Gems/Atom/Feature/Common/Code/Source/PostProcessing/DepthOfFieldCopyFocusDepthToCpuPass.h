/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

namespace AZ
{
    namespace Render
    {
        //! This pass is used to read back the depth value written to the buffer.
        class DepthOfFieldCopyFocusDepthToCpuPass final
            : public RPI::Pass
            , public RHI::ScopeProducer
        {
            AZ_RPI_PASS(DepthOfFieldCopyFocusDepthToCpuPass);

        public:
            AZ_RTTI(DepthOfFieldCopyFocusDepthToCpuPass, "{EA00AD76-92FC-4223-AB7D-87F588AB5394}", Pass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldCopyFocusDepthToCpuPass, SystemAllocator);

            //! Creates an DepthOfFieldCopyFocusDepthToCpuPass
            static RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> Create(const RPI::PassDescriptor& descriptor);

            ~DepthOfFieldCopyFocusDepthToCpuPass() = default;

            void SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef);
            float GetFocusDepth();

        private:
            explicit DepthOfFieldCopyFocusDepthToCpuPass(const RPI::PassDescriptor& descriptor);

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            RPI::Ptr<RPI::Buffer> m_bufferRef;
            Data::Instance<RPI::Buffer> m_readbackBuffer;
            RHI::CopyBufferDescriptor m_copyDescriptor;
            bool m_needsInitialize = true;
            RHI::Ptr<RHI::Fence> m_fence;
            float m_lastFocusDepth = 0;
        };
    }   // namespace RPI
}   // namespace AZ
