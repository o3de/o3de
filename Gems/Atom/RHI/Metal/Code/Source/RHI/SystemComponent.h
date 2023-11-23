/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/Metal/Base.h>
#include <AzCore/Component/Component.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Metal
    {
        class SystemComponent
            : public AZ::Component
            , public RHI::Factory
        {
        public:
            AZ_COMPONENT(SystemComponent, "{8A5E12D7-5B59-4BE9-BC6E-B063D12A64C6}")

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void Reflect(AZ::ReflectContext* context);

            SystemComponent() = default;
            virtual ~SystemComponent() = default;

           //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////
            
            ///////////////////////////////////////////////////////////////////
            // RHI::Factory
            Name GetName() override;
            RHI::APIType GetType() override;
            RHI::APIPriority GetDefaultPriority() override;
            uint32_t GetAPIUniqueIndex() const override { return APIUniqueIndex; }
            bool SupportsXR() const override;
            RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;
            RHI::Ptr<RHI::SingleDeviceBuffer> CreateBuffer() override;
            RHI::Ptr<RHI::SingleDeviceBufferPool> CreateBufferPool() override;
            RHI::Ptr<RHI::SingleDeviceBufferView> CreateBufferView() override;
            RHI::Ptr<RHI::Device> CreateDevice() override;
            RHI::Ptr<RHI::SingleDeviceFence> CreateFence() override;
            RHI::Ptr<RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;
            RHI::Ptr<RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;
            RHI::Ptr<RHI::SingleDeviceImage> CreateImage() override;
            RHI::Ptr<RHI::SingleDeviceImagePool> CreateImagePool() override;
            RHI::Ptr<RHI::SingleDeviceImageView> CreateImageView() override;
            RHI::Ptr<RHI::SingleDeviceStreamingImagePool> CreateStreamingImagePool() override;
            RHI::Ptr<RHI::SingleDevicePipelineLibrary> CreatePipelineLibrary() override;
            RHI::Ptr<RHI::SingleDevicePipelineState> CreatePipelineState() override;
            RHI::Ptr<RHI::Scope> CreateScope() override;
            RHI::Ptr<RHI::SingleDeviceShaderResourceGroup> CreateShaderResourceGroup() override;
            RHI::Ptr<RHI::SingleDeviceShaderResourceGroupPool> CreateShaderResourceGroupPool() override;
            RHI::Ptr<RHI::SingleDeviceSwapChain> CreateSwapChain() override;
            RHI::Ptr<RHI::SingleDeviceTransientAttachmentPool> CreateTransientAttachmentPool() override;
            RHI::Ptr<RHI::SingleDeviceQueryPool> CreateQueryPool() override;
            RHI::Ptr<RHI::SingleDeviceQuery> CreateQuery() override;
            RHI::Ptr<RHI::SingleDeviceIndirectBufferSignature> CreateIndirectBufferSignature() override;
            RHI::Ptr<RHI::SingleDeviceIndirectBufferWriter> CreateIndirectBufferWriter() override;
            RHI::Ptr<RHI::SingleDeviceRayTracingBufferPools> CreateRayTracingBufferPools() override;
            RHI::Ptr<RHI::SingleDeviceRayTracingBlas> CreateRayTracingBlas() override;
            RHI::Ptr<RHI::SingleDeviceRayTracingTlas> CreateRayTracingTlas() override;
            RHI::Ptr<RHI::SingleDeviceRayTracingPipelineState> CreateRayTracingPipelineState() override;
            RHI::Ptr<RHI::SingleDeviceRayTracingShaderTable> CreateRayTracingShaderTable() override;
            RHI::Ptr<RHI::DispatchRaysIndirectBuffer> CreateDispatchRaysIndirectBuffer() override;
            ///////////////////////////////////////////////////////////////////

        private:
            RHI::Ptr<Device> m_device;

            const Name m_apiName{APINameString};
        };
    }
}
