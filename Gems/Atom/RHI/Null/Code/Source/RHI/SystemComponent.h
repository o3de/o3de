/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/Null/Base.h>
#include <AzCore/Component/Component.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Null
    {
        class SystemComponent
            : public AZ::Component
            , public RHI::Factory
        {
        public:
            AZ_COMPONENT(SystemComponent, "{0A6A246A-CB5B-4F59-99D5-629B7F1C44DD}")

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
            RHI::Ptr<RHI::DeviceBuffer> CreateBuffer() override;
            RHI::Ptr<RHI::DeviceBufferPool> CreateBufferPool() override;
            RHI::Ptr<RHI::DeviceBufferView> CreateBufferView() override;
            RHI::Ptr<RHI::Device> CreateDevice() override;
            RHI::Ptr<RHI::DeviceFence> CreateFence() override;
            RHI::Ptr<RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;
            RHI::Ptr<RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;
            RHI::Ptr<RHI::DeviceImage> CreateImage() override;
            RHI::Ptr<RHI::DeviceImagePool> CreateImagePool() override;
            RHI::Ptr<RHI::DeviceImageView> CreateImageView() override;
            RHI::Ptr<RHI::DeviceStreamingImagePool> CreateStreamingImagePool() override;
            RHI::Ptr<RHI::DevicePipelineLibrary> CreatePipelineLibrary() override;
            RHI::Ptr<RHI::DevicePipelineState> CreatePipelineState() override;
            RHI::Ptr<RHI::Scope> CreateScope() override;
            RHI::Ptr<RHI::DeviceShaderResourceGroup> CreateShaderResourceGroup() override;
            RHI::Ptr<RHI::DeviceShaderResourceGroupPool> CreateShaderResourceGroupPool() override;
            RHI::Ptr<RHI::DeviceSwapChain> CreateSwapChain() override;
            RHI::Ptr<RHI::DeviceTransientAttachmentPool> CreateTransientAttachmentPool() override;
            RHI::Ptr<RHI::DeviceQueryPool> CreateQueryPool() override;
            RHI::Ptr<RHI::DeviceQuery> CreateQuery() override;
            RHI::Ptr<RHI::DeviceIndirectBufferSignature> CreateIndirectBufferSignature() override;
            RHI::Ptr<RHI::DeviceIndirectBufferWriter> CreateIndirectBufferWriter() override;
            RHI::Ptr<RHI::DeviceRayTracingBufferPools> CreateRayTracingBufferPools() override;
            RHI::Ptr<RHI::DeviceRayTracingBlas> CreateRayTracingBlas() override;
            RHI::Ptr<RHI::DeviceRayTracingTlas> CreateRayTracingTlas() override;
            RHI::Ptr<RHI::DeviceRayTracingPipelineState> CreateRayTracingPipelineState() override;
            RHI::Ptr<RHI::DeviceRayTracingShaderTable> CreateRayTracingShaderTable() override;
            RHI::Ptr<RHI::DeviceDispatchRaysIndirectBuffer> CreateDispatchRaysIndirectBuffer() override;
            RHI::Ptr<RHI::DeviceRayTracingCompactionQueryPool> CreateRayTracingCompactionQueryPool() override;
            RHI::Ptr<RHI::DeviceRayTracingCompactionQuery> CreateRayTracingCompactionQuery() override;
            ///////////////////////////////////////////////////////////////////

        private:
            RHI::Ptr<Device> m_device;

            const Name m_apiName{APINameString};
        };
    }
}
