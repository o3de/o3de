/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        AZ_CLASS_ALLOCATOR(Factory, AZ::SystemAllocator);

    private:
        const AZ::Name m_platformName{"UnitTest"};

        AZ::Name GetName() override;

        AZ::RHI::APIType GetType() override;

        AZ::RHI::APIPriority GetDefaultPriority() override;

        uint32_t GetAPIUniqueIndex() const override { return 0; }

        bool SupportsXR() const override;

        AZ::RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;

        AZ::RHI::Ptr<AZ::RHI::Device> CreateDevice() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceSwapChain> CreateSwapChain() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceFence> CreateFence() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceBuffer> CreateBuffer() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceBufferView> CreateBufferView() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceBufferPool> CreateBufferPool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceImage> CreateImage() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceImageView> CreateImageView() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceImagePool> CreateImagePool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceStreamingImagePool> CreateStreamingImagePool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceShaderResourceGroupPool> CreateShaderResourceGroupPool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceShaderResourceGroup> CreateShaderResourceGroup() override;

        AZ::RHI::Ptr<AZ::RHI::DevicePipelineLibrary> CreatePipelineLibrary() override;

        AZ::RHI::Ptr<AZ::RHI::DevicePipelineState> CreatePipelineState() override;

        AZ::RHI::Ptr<AZ::RHI::Scope> CreateScope() override;

        AZ::RHI::Ptr<AZ::RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;

        AZ::RHI::Ptr<AZ::RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceTransientAttachmentPool> CreateTransientAttachmentPool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceQueryPool> CreateQueryPool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceQuery> CreateQuery() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceIndirectBufferSignature> CreateIndirectBufferSignature() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceIndirectBufferWriter> CreateIndirectBufferWriter() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingBufferPools> CreateRayTracingBufferPools() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingBlas> CreateRayTracingBlas() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingTlas> CreateRayTracingTlas() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingPipelineState> CreateRayTracingPipelineState() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingShaderTable> CreateRayTracingShaderTable() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceDispatchRaysIndirectBuffer> CreateDispatchRaysIndirectBuffer() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingCompactionQueryPool> CreateRayTracingCompactionQueryPool() override;

        AZ::RHI::Ptr<AZ::RHI::DeviceRayTracingCompactionQuery> CreateRayTracingCompactionQuery() override;
    };
}
