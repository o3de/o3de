/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

namespace AZ
{
    namespace Render
    {
        //! This pass is used to write the depth value of the specified screen coordinates to the buffer.
        class DepthOfFieldWriteFocusDepthFromGpuPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(DepthOfFieldWriteFocusDepthFromGpuPass);

        public:
            AZ_RTTI(DepthOfFieldWriteFocusDepthFromGpuPass, "{60DF04D2-A9FE-4B21-8050-96AFFC46BB87}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldWriteFocusDepthFromGpuPass, SystemAllocator);

            /// Creates a DepthOfFieldWriteFocusDepthFromGpuPass
            static RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> Create(const RPI::PassDescriptor& descriptor);

            // Set pass parameter interfaces...
            void SetScreenPosition(const AZ::Vector2& screenPosition);
            void SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef);

        private:
            DepthOfFieldWriteFocusDepthFromGpuPass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            RHI::ShaderInputNameIndex m_autoFocusDataBufferIndex = "m_outputFocusDepth";
            RHI::ShaderInputNameIndex m_autoFocusScreenPositionIndex = "m_autoFocusScreenPosition";

            RPI::Ptr<RPI::Buffer> m_bufferRef = nullptr;
            AZ::Vector2 m_autoFocusScreenPosition{ 0.0f, 0.0f };

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            // Pass overrides
            void BuildInternal() override;
        };
    }   // namespace Render
}   // namespace AZ
