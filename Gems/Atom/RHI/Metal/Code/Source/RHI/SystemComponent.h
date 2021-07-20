/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;
            RHI::Ptr<RHI::Buffer> CreateBuffer() override;
            RHI::Ptr<RHI::BufferPool> CreateBufferPool() override;
            RHI::Ptr<RHI::BufferView> CreateBufferView() override;
            RHI::Ptr<RHI::Device> CreateDevice() override;
            RHI::Ptr<RHI::Fence> CreateFence() override;
            RHI::Ptr<RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;
            RHI::Ptr<RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;
            RHI::Ptr<RHI::Image> CreateImage() override;
            RHI::Ptr<RHI::ImagePool> CreateImagePool() override;
            RHI::Ptr<RHI::ImageView> CreateImageView() override;
            RHI::Ptr<RHI::StreamingImagePool> CreateStreamingImagePool() override;
            RHI::Ptr<RHI::PipelineLibrary> CreatePipelineLibrary() override;
            RHI::Ptr<RHI::PipelineState> CreatePipelineState() override;
            RHI::Ptr<RHI::Scope> CreateScope() override;
            RHI::Ptr<RHI::ShaderResourceGroup> CreateShaderResourceGroup() override;
            RHI::Ptr<RHI::ShaderResourceGroupPool> CreateShaderResourceGroupPool() override;
            RHI::Ptr<RHI::SwapChain> CreateSwapChain() override;
            RHI::Ptr<RHI::TransientAttachmentPool> CreateTransientAttachmentPool() override;
            RHI::Ptr<RHI::QueryPool> CreateQueryPool() override;
            RHI::Ptr<RHI::Query> CreateQuery() override;
            RHI::Ptr<RHI::IndirectBufferSignature> CreateIndirectBufferSignature() override;
            RHI::Ptr<RHI::IndirectBufferWriter> CreateIndirectBufferWriter() override;
            RHI::Ptr<RHI::RayTracingBufferPools> CreateRayTracingBufferPools() override;
            RHI::Ptr<RHI::RayTracingBlas> CreateRayTracingBlas() override;
            RHI::Ptr<RHI::RayTracingTlas> CreateRayTracingTlas() override;
            RHI::Ptr<RHI::RayTracingPipelineState> CreateRayTracingPipelineState() override;
            RHI::Ptr<RHI::RayTracingShaderTable> CreateRayTracingShaderTable() override;
            ///////////////////////////////////////////////////////////////////

        private:
            RHI::Ptr<Device> m_device;

            const Name m_apiName{APINameString};
        };
    }
}
