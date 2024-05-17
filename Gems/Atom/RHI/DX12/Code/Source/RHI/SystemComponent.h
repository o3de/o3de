/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Device.h>
#include <RHI/CommandQueueContext.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/DX12/Base.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace DX12
    {
        class SystemComponent
            : public Component
            , public RHI::Factory
        {
        public:
            AZ_COMPONENT(SystemComponent, "{17665B3D-940C-44F5-935C-1FB27EF0FFD7}");

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void Reflect(ReflectContext* context);

            SystemComponent() = default;
            virtual ~SystemComponent() = default;

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
            // This function is implemented by each supported platform in a separate file.
            bool CheckSystemRequirements() const;

            const Name m_apiName{APINameString};
        };
    }
}
