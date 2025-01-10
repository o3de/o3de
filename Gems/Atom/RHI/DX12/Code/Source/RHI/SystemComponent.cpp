/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/DX12/Base.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/DispatchRaysIndirectBuffer.h>
#include <RHI/Fence.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/PhysicalDevice.h>
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
    namespace DX12
    {
        void SystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(RHI::Factory::GetPlatformService());
        }

        void SystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(RHI::Factory::GetManagerComponentService());
        }

        void SystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SystemComponent, Component>()
                    ->Version(1);
            }
        }

        void SystemComponent::Activate()
        {
            if (CheckSystemRequirements())
            {
                RHI::FactoryManagerBus::Broadcast(&RHI::FactoryManagerRequest::RegisterFactory, this);
            }
            else
            {
                AZ_Warning("DX12", false, "Current system does not support DX12.");
            }
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
            return DX12::RHIType;
        }

        RHI::APIPriority SystemComponent::GetDefaultPriority()
        {
            // We want dx12 to be default RHI (unless the user choose otherwise).
            return RHI::APITopPriority;
        }

        bool SystemComponent::SupportsXR() const
        {
            //DX12 RHI does not support XR api
            return false;
        }

        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return PhysicalDevice::Enumerate();
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
            return FenceImpl::Create();
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

        RHI::Ptr<RHI::DeviceTransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::DeviceSwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
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
