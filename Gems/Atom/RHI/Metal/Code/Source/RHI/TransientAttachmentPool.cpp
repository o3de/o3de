/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/RHIBus.h>

#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<TransientAttachmentPool> TransientAttachmentPool::Create()
        {
            return aznew TransientAttachmentPool();
        }
        
        Device& TransientAttachmentPool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }
        
        void TransientAttachmentPool::BeginInternal(
            RHI::TransientAttachmentPoolCompileFlags compileFlags,
            const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint)
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
            {
                size_t heapMemoryHint = 0;
                if (memoryHint)
                {
                    auto typeMask = allocator->GetDescriptor().m_resourceTypeMask;
                    if (RHI::CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::Buffer))
                    {
                        heapMemoryHint += memoryHint->m_bufferMemoryInBytes;
                    }

                    if (RHI::CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::Image))
                    {
                        heapMemoryHint += memoryHint->m_imageMemoryInBytes;
                    }

                    if (RHI::CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::RenderTarget))
                    {
                        heapMemoryHint += memoryHint->m_rendertargetMemoryInBytes;
                    }
                }
                allocator->Begin(compileFlags, heapMemoryHint);
            }
        }

        RHI::DeviceImage* TransientAttachmentPool::ActivateImage(const RHI::TransientImageDescriptor& descriptor)
        {
            AliasedAttachmentAllocator* allocator = nullptr;
            if (RHI::CheckBitsAny(descriptor.m_imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil))
            {
                allocator = m_renderTargetAllocator;
            }
            else
            {
                allocator = m_imageAllocator;
            }
            AZ_Assert(allocator, "No image heap allocator to allocate an image. Make sure you specified one at pool creation time");
            RHI::DeviceImage* image = allocator->ActivateImage(descriptor, *m_currentScope);
            AZ_Assert(RHI::CheckBitsAll(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources) || image, "Failed to allocate image. Heap is not big enough");
            m_imageToAllocatorMap[descriptor.m_attachmentId] = allocator;
            return image;
        }

        RHI::DeviceBuffer* TransientAttachmentPool::ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor)
        {
            AZ_Assert(m_bufferAllocator, "No buffer heap allocator to allocate a transient buffer. Make sure you specified one at pool creation time");
            RHI::DeviceBuffer* buffer = m_bufferAllocator->ActivateBuffer(descriptor, *m_currentScope);
            AZ_Assert(RHI::CheckBitsAll(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources) || buffer, "Failed to allocate buffer. Heap is not big enough.");
            return buffer;
        }

        void TransientAttachmentPool::DeactivateBuffer(const RHI::AttachmentId& attachmentId)
        {
            m_bufferAllocator->DeactivateBuffer(attachmentId, *m_currentScope);
        }

        void TransientAttachmentPool::DeactivateImage(const RHI::AttachmentId& attachmentId)
        {
            auto findIt = m_imageToAllocatorMap.find(attachmentId);
            AZ_Assert(findIt->second, "Image is not associated with any allocator");
            AliasedAttachmentAllocator* allocator = findIt->second;
            allocator->DeactivateImage(attachmentId, *m_currentScope);
            m_imageToAllocatorMap.erase(findIt);
        }

        void TransientAttachmentPool::EndInternal()
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
            {
                allocator->End();
            }
            
            AZ_Assert(m_imageToAllocatorMap.empty(), "Still active images.");
            
            if (RHI::CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::GatherStatistics))
            {
                for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
                {
                    size_t statsBegin = m_statistics.m_heaps.size();
                    allocator->GetStatistics(m_statistics.m_heaps);
                    CollectHeapStats(allocator->GetDescriptor().m_resourceTypeMask, { m_statistics.m_heaps.begin() + statsBegin, m_statistics.m_heaps.end() });
                }
            }
        }

        RHI::ResultCode TransientAttachmentPool::InitInternal(RHI::Device& deviceBase, const RHI::TransientAttachmentPoolDescriptor& descriptor)
        {
            static const uint32_t ObjectCacheSize = 256;
            
            auto& device = static_cast<Device&>(deviceBase);
            bool allowNoBudget = false;
            switch (descriptor.m_heapParameters.m_type)
            {
            case RHI::HeapAllocationStrategy::MemoryHint:
                allowNoBudget = true;
                break;
            case RHI::HeapAllocationStrategy::Fixed:
                allowNoBudget = false;
                break;
            case RHI::HeapAllocationStrategy::Paging:
                allowNoBudget = descriptor.m_heapParameters.m_pagingParameters.m_initialAllocationPercentage == 0.f;
                break;
            default:
                AZ_Assert(false, "Invalid heap allocation strategy");
                return RHI::ResultCode::InvalidArgument;
            }
            
            struct
            {
                size_t m_alignment = 0;
                void operator=(size_t value)
                {
                    m_alignment = AZStd::max(m_alignment, value);
                }
            } alignment;
            AZ::RHI::RHIRequirementRequestBus::BroadcastResult(alignment, &AZ::RHI::RHIRequirementsRequest::GetRequiredAlignment, device);

            
            if (descriptor.m_imageBudgetInBytes || allowNoBudget)
            {
                AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                heapAllocatorDesc.m_budgetInBytes = descriptor.m_imageBudgetInBytes;
                heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::Image;
                heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;
                heapAllocatorDesc.m_alignment = AZStd::max(alignment.m_alignment, heapAllocatorDesc.m_alignment);

                
                RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                allocator->SetName(Name("TransientAttachmentPool [Images]"));
                allocator->Init(device, heapAllocatorDesc);
                m_imageAllocator = allocator.get();

                m_aliasedAllocators.emplace_back(AZStd::move(allocator));
            }
            
            if (descriptor.m_renderTargetBudgetInBytes || allowNoBudget)
            {
                //[TODO - ATOM-3804] - Experiment with creating separate heaps for each type - color, depth, stencil, and MSAA.
                AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                heapAllocatorDesc.m_budgetInBytes = descriptor.m_renderTargetBudgetInBytes;
                heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::RenderTarget;
                heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;
                heapAllocatorDesc.m_alignment = AZStd::max(alignment.m_alignment, heapAllocatorDesc.m_alignment);
                
                RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                allocator->SetName(Name("TransientAttachmentPool [Render Targets]"));
                allocator->Init(device, heapAllocatorDesc);
                m_renderTargetAllocator = allocator.get();

                m_aliasedAllocators.emplace_back(AZStd::move(allocator));
            }
            
            if (descriptor.m_bufferBudgetInBytes || allowNoBudget)
            {
                AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                heapAllocatorDesc.m_budgetInBytes = descriptor.m_bufferBudgetInBytes;
                heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::Buffer;
                heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;
                heapAllocatorDesc.m_alignment = AZStd::max(alignment.m_alignment, heapAllocatorDesc.m_alignment);
                
                RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                allocator->SetName(Name("TransientAttachmentPool [Buffers]"));
                allocator->Init(device, heapAllocatorDesc);

                m_bufferAllocator = allocator.get();
                m_aliasedAllocators.emplace_back(AZStd::move(allocator));
            }

            m_statistics.m_heaps.reserve(3);
            m_statistics.m_allocationPolicy = RHI::TransientAttachmentStatistics::AllocationPolicy::HeapPlacement;
            
            return RHI::ResultCode::Success;
        }

        void TransientAttachmentPool::ShutdownInternal()
        {
            m_imageAllocator = nullptr;
            m_renderTargetAllocator = nullptr;
            m_bufferAllocator = nullptr;
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
            {
                allocator->Shutdown();
            }
            m_aliasedAllocators.clear();
        }
    }
}

