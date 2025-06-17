/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/Vulkan/Base.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Device.h>
#include <RHI/DispatchRaysIndirectBuffer.h>
#include <RHI/Fence.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/Instance.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingBufferPools.h>
#include <RHI/RayTracingCompactionQueryPool.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/Scope.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/SwapChain.h>
#include <RHI/SystemComponent.h>
#include <RHI/TransientAttachmentPool.h>


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

        void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("VulkanRequirementsService"));
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

        bool SystemComponent::SupportsXR() const
        {
            // Vulkan RHI supports Openxr
            return true;
        }

        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return Instance::GetInstance().GetSupportedDevices();
        }

        RHI::Ptr<RHI::DeviceBuffer> SystemComponent::CreateBuffer()
        {
            return Buffer::Create();
        }

        RHI::Ptr<RHI::DeviceBufferPool> SystemComponent::CreateBufferPool()
        {
            return BufferPool::Create();
        }

        RHI::Ptr<RHI::DeviceBufferView> SystemComponent::CreateBufferView()
        {
            return BufferView::Create();
        }

        RHI::Ptr<RHI::Device> SystemComponent::CreateDevice()
        {
            return Device::Create();
        }

        RHI::Ptr<RHI::DeviceFence> SystemComponent::CreateFence()
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

        RHI::Ptr<RHI::DeviceImage> SystemComponent::CreateImage()
        {
            return Image::Create();
        }

        RHI::Ptr<RHI::DeviceImagePool> SystemComponent::CreateImagePool()
        {
            return ImagePool::Create();
        }

        RHI::Ptr<RHI::DeviceImageView> SystemComponent::CreateImageView()
        {
            return ImageView::Create();
        }

        RHI::Ptr<RHI::DeviceStreamingImagePool> SystemComponent::CreateStreamingImagePool()
        {
            return StreamingImagePool::Create();
        }

        RHI::Ptr<RHI::DevicePipelineLibrary> SystemComponent::CreatePipelineLibrary()
        {
            return PipelineLibrary::Create();
        }

        RHI::Ptr<RHI::DevicePipelineState> SystemComponent::CreatePipelineState()
        {
            return PipelineState::Create();
        }

        RHI::Ptr<RHI::Scope> SystemComponent::CreateScope()
        {
            return Scope::Create();
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroup> SystemComponent::CreateShaderResourceGroup()
        {
            return ShaderResourceGroup::Create();
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroupPool> SystemComponent::CreateShaderResourceGroupPool()
        {
            return ShaderResourceGroupPool::Create();
        }

        RHI::Ptr<RHI::DeviceSwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
        }

        RHI::Ptr<RHI::DeviceTransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::DeviceQueryPool> SystemComponent::CreateQueryPool()
        {
            return QueryPool::Create();
        }

        RHI::Ptr<RHI::DeviceQuery> SystemComponent::CreateQuery()
        {
            return Query::Create();
        }

        RHI::Ptr<RHI::DeviceIndirectBufferSignature> SystemComponent::CreateIndirectBufferSignature()
        {
            return IndirectBufferSignature::Create();
        }

        RHI::Ptr<RHI::DeviceIndirectBufferWriter> SystemComponent::CreateIndirectBufferWriter()
        {
            return IndirectBufferWriter::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingBufferPools> SystemComponent::CreateRayTracingBufferPools()
        {
            return RayTracingBufferPools::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingBlas> SystemComponent::CreateRayTracingBlas()
        {
            return RayTracingBlas::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingTlas> SystemComponent::CreateRayTracingTlas()
        {
            return RayTracingTlas::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingPipelineState> SystemComponent::CreateRayTracingPipelineState()
        {
            return RayTracingPipelineState::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingShaderTable> SystemComponent::CreateRayTracingShaderTable()
        {
            return RayTracingShaderTable::Create();
        }

        RHI::Ptr<RHI::DeviceDispatchRaysIndirectBuffer> SystemComponent::CreateDispatchRaysIndirectBuffer()
        {
            return DispatchRaysIndirectBuffer::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingCompactionQueryPool> SystemComponent::CreateRayTracingCompactionQueryPool()
        {
            return RayTracingCompactionQueryPool::Create();
        }

        RHI::Ptr<RHI::DeviceRayTracingCompactionQuery> SystemComponent::CreateRayTracingCompactionQuery()
        {
            return RayTracingCompactionQuery::Create();
        }
    }
}
