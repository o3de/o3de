/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(DepthOfFieldWriteFocusDepthFromGpuPass, SystemAllocator, 0);

            /// Creates a DepthOfFieldWriteFocusDepthFromGpuPass
            static RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> Create(const RPI::PassDescriptor& descriptor);

            // Set pass parameter interfaces...
            void SetScreenPosition(const AZ::Vector2& screenPosition);
            void SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef);

        private:
            DepthOfFieldWriteFocusDepthFromGpuPass(const RPI::PassDescriptor& descriptor);
            void Init();

            // SRG binding indices...
            RHI::ShaderInputBufferIndex   m_autoFocusDataBufferIndex;
            RHI::ShaderInputConstantIndex m_autoFocusScreenPositionIndex;

            RPI::Ptr<RPI::Buffer> m_bufferRef = nullptr;
            AZ::Vector2 m_autoFocusScreenPosition{ 0.0f, 0.0f };
            bool m_initialized = false;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            // Pass overrides
            void BuildAttachmentsInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
        };
    }   // namespace Render
}   // namespace AZ
