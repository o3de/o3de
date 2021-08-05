/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <RHI/SystemComponent.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/Fence.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/FrameGraphExecuter.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>
#include <RHI/PhysicalDevice.h>
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
#include <RHI/SwapChain.h>
#include <RHI/TransientAttachmentPool.h>
#include <Atom/RHI.Reflect/DX12/Base.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Serialization/SerializeContext.h>

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

        RHI::PhysicalDeviceList SystemComponent::EnumeratePhysicalDevices()
        {
            return PhysicalDevice::Enumerate();
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

        RHI::Ptr<RHI::TransientAttachmentPool> SystemComponent::CreateTransientAttachmentPool()
        {
            return TransientAttachmentPool::Create();
        }

        RHI::Ptr<RHI::SwapChain> SystemComponent::CreateSwapChain()
        {
            return SwapChain::Create();
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
