/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>

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

            AZ_CLASS_ALLOCATOR(Factory, AZ::SystemAllocator);

            static AZ::RHI::Ptr<AZ::RHI::Device> CreateDefaultDevice();

        private:
            const AZ::Name m_platformName;

            AZ::Name GetName() override;

            AZ::RHI::APIType GetType() override;

            AZ::RHI::APIPriority GetDefaultPriority() override;

            uint32_t GetAPIUniqueIndex() const override { return 0; };

            bool SupportsXR() const override;

            AZ::RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;

            AZ::RHI::Ptr<AZ::RHI::Device> CreateDevice() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceSwapChain> CreateSwapChain() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceFence> CreateFence() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceBuffer> CreateBuffer() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceBufferView> CreateBufferView() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceBufferPool> CreateBufferPool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceImage> CreateImage() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceImageView> CreateImageView() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceImagePool> CreateImagePool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceStreamingImagePool> CreateStreamingImagePool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceShaderResourceGroupPool> CreateShaderResourceGroupPool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceShaderResourceGroup> CreateShaderResourceGroup() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDevicePipelineLibrary> CreatePipelineLibrary() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDevicePipelineState> CreatePipelineState() override;

            AZ::RHI::Ptr<AZ::RHI::Scope> CreateScope() override;

            AZ::RHI::Ptr<AZ::RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;

            AZ::RHI::Ptr<AZ::RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceTransientAttachmentPool> CreateTransientAttachmentPool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceQueryPool> CreateQueryPool() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceQuery> CreateQuery() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceIndirectBufferSignature> CreateIndirectBufferSignature() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceIndirectBufferWriter> CreateIndirectBufferWriter() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingBufferPools> CreateRayTracingBufferPools() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingBlas> CreateRayTracingBlas() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingTlas> CreateRayTracingTlas() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingPipelineState> CreateRayTracingPipelineState() override;

            AZ::RHI::Ptr<AZ::RHI::SingleDeviceRayTracingShaderTable> CreateRayTracingShaderTable() override;

            AZ::RHI::Ptr<AZ::RHI::DispatchRaysIndirectBuffer> CreateDispatchRaysIndirectBuffer() override;
        };
    }
}
