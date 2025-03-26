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
            // This function is implemented by each supported platform in a separate file.
            bool CheckSystemRequirements() const;

            const Name m_apiName{APINameString};
        };
    }
}
