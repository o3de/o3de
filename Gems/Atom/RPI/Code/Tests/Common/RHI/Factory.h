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

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>

namespace UnitTest
{
    namespace StubRHI
    {
        class Factory final
            : public AZ::RHI::Factory
        {
        public:
            Factory();
            ~Factory();

            AZ_CLASS_ALLOCATOR(Factory, AZ::SystemAllocator, 0);

            static AZ::RHI::Ptr<AZ::RHI::Device> CreateDefaultDevice();

        private:
            const AZ::Name m_platformName;

            AZ::Name GetName() override;

            AZ::RHI::APIType GetType() override;

            AZ::RHI::APIPriority GetDefaultPriority() override;

            uint32_t GetAPIUniqueIndex() const override { return 0; };

            AZ::RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;

            AZ::RHI::Ptr<AZ::RHI::Device> CreateDevice() override;

            AZ::RHI::Ptr<AZ::RHI::SwapChain> CreateSwapChain() override;

            AZ::RHI::Ptr<AZ::RHI::Fence> CreateFence() override;

            AZ::RHI::Ptr<AZ::RHI::Buffer> CreateBuffer() override;

            AZ::RHI::Ptr<AZ::RHI::BufferView> CreateBufferView() override;

            AZ::RHI::Ptr<AZ::RHI::BufferPool> CreateBufferPool() override;

            AZ::RHI::Ptr<AZ::RHI::Image> CreateImage() override;

            AZ::RHI::Ptr<AZ::RHI::ImageView> CreateImageView() override;

            AZ::RHI::Ptr<AZ::RHI::ImagePool> CreateImagePool() override;

            AZ::RHI::Ptr<AZ::RHI::StreamingImagePool> CreateStreamingImagePool() override;

            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupPool> CreateShaderResourceGroupPool() override;

            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroup> CreateShaderResourceGroup() override;

            AZ::RHI::Ptr<AZ::RHI::PipelineLibrary> CreatePipelineLibrary() override;

            AZ::RHI::Ptr<AZ::RHI::PipelineState> CreatePipelineState() override;

            AZ::RHI::Ptr<AZ::RHI::Scope> CreateScope() override;

            AZ::RHI::Ptr<AZ::RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;

            AZ::RHI::Ptr<AZ::RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;

            AZ::RHI::Ptr<AZ::RHI::TransientAttachmentPool> CreateTransientAttachmentPool() override;

            AZ::RHI::Ptr<AZ::RHI::QueryPool> CreateQueryPool() override;

            AZ::RHI::Ptr<AZ::RHI::Query> CreateQuery() override;

            AZ::RHI::Ptr<AZ::RHI::IndirectBufferSignature> CreateIndirectBufferSignature() override;

            AZ::RHI::Ptr<AZ::RHI::IndirectBufferWriter> CreateIndirectBufferWriter() override;

            AZ::RHI::Ptr<AZ::RHI::RayTracingBlas> CreateRayTracingBlas() override;

            AZ::RHI::Ptr<AZ::RHI::RayTracingTlas> CreateRayTracingTlas() override;

            AZ::RHI::Ptr<AZ::RHI::RayTracingPipelineState> CreateRayTracingPipelineState() override;

            AZ::RHI::Ptr<AZ::RHI::RayTracingShaderTable> CreateRayTracingShaderTable() override;
        };
    }
}
