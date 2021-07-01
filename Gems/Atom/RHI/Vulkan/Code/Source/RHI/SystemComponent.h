/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            const Name m_apiName{APINameString};
        };
    } // namespace Vulkan
} // namespace AZ
