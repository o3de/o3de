/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FactoryManagerBus.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferWriter.h>
#include <Atom/RHI/PhysicalDevice.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI.Reflect/Null/Base.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Fence.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/ImagePool.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/RayTracingBufferPools.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <RHI/Scope.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/StreamingImagePool.h>
#include <RHI/SystemComponent.h>
#include <RHI/SwapChain.h>
#include <RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Null
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
            return Null::RHIType;
        }

        RHI::APIPriority SystemComponent::GetDefaultPriority()
        {
            return RHI::APILowPriority;
        }
        
        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return PhysicalDevice::Enumerate();
        }

        RHI::Ptr<RHI::SwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
        }

        RHI::Ptr<RHI::Fence> SystemComponent::CreateFence()
        {
            return Fence::Create();
        }

        RHI::Ptr<RHI::Buffer> SystemComponent::CreateBuffer()
        {
            return Buffer::Create();
        }

        RHI::Ptr<RHI::BufferView> SystemComponent::CreateBufferView()
        {
            return BufferView::Create();
        }

        RHI::Ptr<RHI::BufferPool> SystemComponent::CreateBufferPool()
        {
            return BufferPool::Create();
        }
        
        RHI::Ptr<RHI::Device> SystemComponent::CreateDevice()
        {
            return Device::Create();
        }
        
        RHI::Ptr<RHI::Image> SystemComponent::CreateImage()
        {
            return Image::Create();
        }

        RHI::Ptr<RHI::ImageView> SystemComponent::CreateImageView()
        {
            return ImageView::Create();
        }
        
        RHI::Ptr<RHI::ImagePool> SystemComponent::CreateImagePool()
        {
            return ImagePool::Create();
        }

        RHI::Ptr<RHI::StreamingImagePool> SystemComponent::CreateStreamingImagePool()
        {
            return StreamingImagePool::Create();
        }

        RHI::Ptr<RHI::ShaderResourceGroup> SystemComponent::CreateShaderResourceGroup()
        {
            return ShaderResourceGroup::Create();
        }

        RHI::Ptr<RHI::ShaderResourceGroupPool> SystemComponent::CreateShaderResourceGroupPool()
        {
            return ShaderResourceGroupPool::Create();
        }

        RHI::Ptr<RHI::PipelineLibrary> SystemComponent::CreatePipelineLibrary()
        {
            return PipelineLibrary::Create();
        }

        RHI::Ptr<RHI::PipelineState> SystemComponent::CreatePipelineState()
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

        RHI::Ptr<RHI::TransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::Scope> SystemComponent::CreateScope()
        {
            return Scope::Create();
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
            return RHI::Ptr<RHI::IndirectBufferSignature>();
        }

        RHI::Ptr<RHI::IndirectBufferWriter> SystemComponent::CreateIndirectBufferWriter()
        {
            return RHI::Ptr<RHI::IndirectBufferWriter>();
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
