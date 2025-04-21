/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FactoryManagerBus.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceIndirectBufferSignature.h>
#include <Atom/RHI/DeviceIndirectBufferWriter.h>
#include <Atom/RHI/PhysicalDevice.h>
#include <Atom/RHI/DeviceQueryPool.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/DeviceDispatchRaysIndirectBuffer.h>
#include <Atom/RHI.Reflect/Metal/Base.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/ImagePool.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/Scope.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/SystemComponent.h>
#include <RHI/SwapChain.h>
#include <RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Metal
    {
        void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
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
                serializeContext->Class<SystemComponent, Component>()
                ->Version(1);
            }
        }

        void SystemComponent::Activate()
        {
            RHI::FactoryManagerBus::Broadcast(&RHI::FactoryManagerRequest::RegisterFactory, this);
        }

        void SystemComponent::Deactivate()
        {
            RHI::FactoryManagerBus::Broadcast(&RHI::FactoryManagerRequest::UnregisterFactory, this);
        }

        Name SystemComponent::GetName()
        {
            return m_apiName;
        }

        RHI::APIType SystemComponent::GetType()
        {
            return Metal::RHIType;
        }

        RHI::APIPriority SystemComponent::GetDefaultPriority()
        {
            return RHI::APITopPriority;
        }

        bool SystemComponent::SupportsXR() const
        {
            // Metal RHI does not support any xr api
            return false;
        }
        
        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return PhysicalDevice::Enumerate();
        }

        RHI::Ptr<RHI::DeviceSwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
        }

        RHI::Ptr<RHI::DeviceFence> SystemComponent::CreateFence()
        {
            return FenceImpl::Create();
        }

        RHI::Ptr<RHI::DeviceBuffer> SystemComponent::CreateBuffer()
        {
            return Buffer::Create();
        }

        RHI::Ptr<RHI::DeviceBufferView> SystemComponent::CreateBufferView()
        {
            return BufferView::Create();
        }

        RHI::Ptr<RHI::DeviceBufferPool> SystemComponent::CreateBufferPool()
        {
            return BufferPool::Create();
        }
        
        RHI::Ptr<RHI::Device> SystemComponent::CreateDevice()
        {
            return Device::Create();
        }
        
        RHI::Ptr<RHI::DeviceImage> SystemComponent::CreateImage()
        {
            return Image::Create();
        }

        RHI::Ptr<RHI::DeviceImageView> SystemComponent::CreateImageView()
        {
            return ImageView::Create();
        }
        
        RHI::Ptr<RHI::DeviceImagePool> SystemComponent::CreateImagePool()
        {
            return ImagePool::Create();
        }

        RHI::Ptr<RHI::DeviceStreamingImagePool> SystemComponent::CreateStreamingImagePool()
        {
            return StreamingImagePool::Create();
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroup> SystemComponent::CreateShaderResourceGroup()
        {
            return ShaderResourceGroup::Create();
        }

        RHI::Ptr<RHI::DeviceShaderResourceGroupPool> SystemComponent::CreateShaderResourceGroupPool()
        {
            return ShaderResourceGroupPool::Create();
        }

        RHI::Ptr<RHI::DevicePipelineLibrary> SystemComponent::CreatePipelineLibrary()
        {
            return PipelineLibrary::Create();
        }

        RHI::Ptr<RHI::DevicePipelineState> SystemComponent::CreatePipelineState()
        {
            return PipelineState::Create();
        }

        RHI::Ptr<RHI::FrameGraphCompiler> SystemComponent::CreateFrameGraphCompiler()
        {
            return FrameGraphCompiler::Create();
        }

        RHI::Ptr<RHI::FrameGraphExecuter> SystemComponent::CreateFrameGraphExecuter()
        {
            return FrameGraphExecuter::Create();
        }

        RHI::Ptr<RHI::DeviceTransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::Scope> SystemComponent::CreateScope()
        {
            return Scope::Create();
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
            return RHI::Ptr<RHI::DeviceIndirectBufferSignature>();
        }

        RHI::Ptr<RHI::DeviceIndirectBufferWriter> SystemComponent::CreateIndirectBufferWriter()
        {
            return RHI::Ptr<RHI::DeviceIndirectBufferWriter>();
        }

        RHI::Ptr<RHI::DeviceRayTracingBufferPools> SystemComponent::CreateRayTracingBufferPools()
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingBlas> SystemComponent::CreateRayTracingBlas()
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingTlas> SystemComponent::CreateRayTracingTlas()
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingPipelineState> SystemComponent::CreateRayTracingPipelineState()
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingShaderTable> SystemComponent::CreateRayTracingShaderTable()
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceDispatchRaysIndirectBuffer> SystemComponent::CreateDispatchRaysIndirectBuffer()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingCompactionQueryPool> CreateRayTracingCompactionQueryPool()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }

        RHI::Ptr<RHI::DeviceRayTracingCompactionQuery> CreateRayTracingCompactionQuery()
        {
            AZ_Assert(false, "Not implemented");
            return nullptr;
        }
    }
}
