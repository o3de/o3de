/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/Vulkan/Base.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Vulkan
    {
        class SystemComponent final
            : public AZ::Component
            , public RHI::Factory
        {
        public:
            AZ_COMPONENT(SystemComponent, "{63A5BE62-43F4-45B9-93FE-E1C6371C457D}");

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);

            static void Reflect(AZ::ReflectContext* context);

            SystemComponent() = default;
            ~SystemComponent() override = default;

            //////////////////////////////////////////////////////////////////////////
            // Component
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

            const Name m_apiName{APINameString};
        };
    } // namespace Vulkan
} // namespace AZ
