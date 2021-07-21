/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Factory.h>
#include <Tests/Buffer.h>
#include <Tests/FrameGraph.h>
#include <Tests/Image.h>
#include <Tests/Query.h>
#include <Tests/Scope.h>
#include <Tests/ShaderResourceGroup.h>
#include <Tests/TransientAttachmentPool.h>

namespace UnitTest
{
    class Factory
        : public AZ::RHI::Factory
    {
    public:
        Factory();
        ~Factory();

        AZ_CLASS_ALLOCATOR(Factory, AZ::SystemAllocator, 0);

    private:
        const AZ::Name m_platformName{"UnitTest"};

        AZ::Name GetName() override;

        AZ::RHI::APIType GetType() override;

        AZ::RHI::APIPriority GetDefaultPriority() override;

        uint32_t GetAPIUniqueIndex() const override { return 0; }

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

        AZ::RHI::Ptr<AZ::RHI::RayTracingBufferPools> CreateRayTracingBufferPools() override;

        AZ::RHI::Ptr<AZ::RHI::RayTracingBlas> CreateRayTracingBlas() override;

        AZ::RHI::Ptr<AZ::RHI::RayTracingTlas> CreateRayTracingTlas() override;

        AZ::RHI::Ptr<AZ::RHI::RayTracingPipelineState> CreateRayTracingPipelineState() override;

        AZ::RHI::Ptr<AZ::RHI::RayTracingShaderTable> CreateRayTracingShaderTable() override;
    };
}
