/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <RHI/SystemComponent.h>
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Fence.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/Scope.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/TransientAttachmentPool.h>
#include <RHI/SwapChain.h>
#include <RHI/PipelineState.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/TransientAttachmentPool.h>
#include <RHI/RayTracingBufferPools.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <Atom/RHI.Reflect/Vulkan/Base.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>

namespace AZ
{
    namespace Vulkan
    {
        void SystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(RHI::Factory::GetPlatformService());
        }

        void SystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(RHI::Factory::GetManagerComponentService());
        }

        void SystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SystemComponent, AZ::Component>()
                    ->Version(2);
            }
        }

        void SystemComponent::Activate()
        {
            Instance::Descriptor descriptor;
            descriptor.m_requiredExtensions = RawStringList{ {VK_KHR_SURFACE_EXTENSION_NAME, AZ_VULKAN_SURFACE_EXTENSION_NAME} };
            descriptor.m_optionalExtensions = RawStringList{ VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };

            // First check the command line for validation mode because it has priority over the reflected value.

            AZ::RHI::ValidationMode validationMode = RHI::ValidationMode::Disabled;
            RHI::FactoryManagerBus::BroadcastResult(validationMode, &RHI::FactoryManagerRequest::DetermineValidationMode);
            descriptor.m_validationMode = validationMode;

            if (Instance::GetInstance().Init(descriptor))
            {
                RHI::FactoryManagerBus::Broadcast(&RHI::FactoryManagerRequest::RegisterFactory, this);
            }
            else
            {
                AZ_Warning("Vulkan", false, "Vulkan is not supported on this platform.");
            }
        }

        void SystemComponent::Deactivate()
        {
            RHI::FactoryManagerBus::Broadcast(&RHI::FactoryManagerRequest::UnregisterFactory, this);

            Instance::GetInstance().Shutdown();
            Instance::Reset();
        }

        Name SystemComponent::GetName()
        {            
            return m_apiName;
        }

        RHI::APIPriority SystemComponent::GetDefaultPriority()
        {
            return RHI::APIMiddlePriority;
        }

        RHI::APIType SystemComponent::GetType()
        {
            return Vulkan::RHIType;
        }

        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return Instance::GetInstance().GetSupportedDevices();
        }

        RHI::Ptr<RHI::Buffer> SystemComponent::CreateBuffer()
        {
            return Buffer::Create();
        }

        RHI::Ptr<RHI::BufferPool> SystemComponent::CreateBufferPool()
        {
            return BufferPool::Create();
        }

        RHI::Ptr<RHI::BufferView> SystemComponent::CreateBufferView()
        {
            return BufferView::Create();
        }

        RHI::Ptr<RHI::Device> SystemComponent::CreateDevice()
        {
            return Device::Create();
        }

        RHI::Ptr<RHI::Fence> SystemComponent::CreateFence()
        {
            return Fence::Create();
        }

        RHI::Ptr<RHI::FrameGraphCompiler> SystemComponent::CreateFrameGraphCompiler()
        {
            return FrameGraphCompiler::Create();
        }

        RHI::Ptr<RHI::FrameGraphExecuter> SystemComponent::CreateFrameGraphExecuter()
        {
            return FrameGraphExecuter::Create();
        }

        RHI::Ptr<RHI::Image> SystemComponent::CreateImage()
        {
            return Image::Create();
        }

        RHI::Ptr<RHI::ImagePool> SystemComponent::CreateImagePool()
        {
            return ImagePool::Create();
        }

        RHI::Ptr<RHI::ImageView> SystemComponent::CreateImageView()
        {
            return ImageView::Create();
        }

        RHI::Ptr<RHI::StreamingImagePool> SystemComponent::CreateStreamingImagePool()
        {
            return StreamingImagePool::Create();
        }

        RHI::Ptr<RHI::PipelineLibrary> SystemComponent::CreatePipelineLibrary()
        {
            return PipelineLibrary::Create();
        }

        RHI::Ptr<RHI::PipelineState> SystemComponent::CreatePipelineState()
        {
            return PipelineState::Create();
        }

        RHI::Ptr<RHI::Scope> SystemComponent::CreateScope()
        {
            return Scope::Create();
        }

        RHI::Ptr<RHI::ShaderResourceGroup> SystemComponent::CreateShaderResourceGroup()
        {
            return ShaderResourceGroup::Create();
        }

        RHI::Ptr<RHI::ShaderResourceGroupPool> SystemComponent::CreateShaderResourceGroupPool()
        {
            return ShaderResourceGroupPool::Create();
        }

        RHI::Ptr<RHI::SwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
        }

        RHI::Ptr<RHI::TransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::QueryPool> SystemComponent::CreateQueryPool()
        {
            return QueryPool::Create();
        }

        RHI::Ptr<RHI::Query> SystemComponent::CreateQuery()
        {
            return Query::Create();
        }

        RHI::Ptr<RHI::IndirectBufferSignature> SystemComponent::CreateIndirectBufferSignature()
        {
            return IndirectBufferSignature::Create();
        }

        RHI::Ptr<RHI::IndirectBufferWriter> SystemComponent::CreateIndirectBufferWriter()
        {
            return IndirectBufferWriter::Create();
        }

        RHI::Ptr<RHI::RayTracingBufferPools> SystemComponent::CreateRayTracingBufferPools()
        {
            return RayTracingBufferPools::Create();
        }

        RHI::Ptr<RHI::RayTracingBlas> SystemComponent::CreateRayTracingBlas()
        {
            return RayTracingBlas::Create();
        }

        RHI::Ptr<RHI::RayTracingTlas> SystemComponent::CreateRayTracingTlas()
        {
            return RayTracingTlas::Create();
        }

        RHI::Ptr<RHI::RayTracingPipelineState> SystemComponent::CreateRayTracingPipelineState()
        {
            return RayTracingPipelineState::Create();
        }

        RHI::Ptr<RHI::RayTracingShaderTable> SystemComponent::CreateRayTracingShaderTable()
        {
            return RayTracingShaderTable::Create();
        }
    }
}
