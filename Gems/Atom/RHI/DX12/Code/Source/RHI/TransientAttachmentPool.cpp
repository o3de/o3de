/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/TransientAttachmentPool.h>
#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <AzCore/std/sort.h>

//#define DX12_TRANSIENT_ATTACHMENT_POOL_DEBUG_LOG

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<TransientAttachmentPool> TransientAttachmentPool::Create()
        {
            return aznew TransientAttachmentPool();
        }

        RHI::ResultCode TransientAttachmentPool::InitInternal(RHI::Device& deviceBase, const RHI::TransientAttachmentPoolDescriptor& descriptor)
        {
            Device& device = static_cast<Device&>(deviceBase);

            const uint32_t ObjectCacheSize = 256;

            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS options;
                device.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
                m_resourceHeapTier = options.ResourceHeapTier;
            }

            if (m_resourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2)
            {
                AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                heapAllocatorDesc.m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
                heapAllocatorDesc.m_budgetInBytes = descriptor.m_bufferBudgetInBytes + descriptor.m_imageBudgetInBytes + descriptor.m_renderTargetBudgetInBytes;
                heapAllocatorDesc.m_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::All;
                heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;

                RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                allocator->SetName(Name("TransientAttachmentPool [Shared]"));
                allocator->Init(device, heapAllocatorDesc);

                m_bufferAllocator = allocator.get();
                m_imageAllocator = allocator.get();
                m_renderTargetAllocator = allocator.get();

                m_aliasedAllocators.emplace_back(AZStd::move(allocator));
            }
            else
            {
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

                if (descriptor.m_bufferBudgetInBytes || allowNoBudget)
                {
                    AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                    heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                    heapAllocatorDesc.m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
                    heapAllocatorDesc.m_budgetInBytes = descriptor.m_bufferBudgetInBytes;
                    heapAllocatorDesc.m_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                    heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::Buffer;
                    heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;

                    RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                    allocator->SetName(Name("TransientAttachmentPool [Buffers]"));
                    allocator->Init(device, heapAllocatorDesc);

                    m_bufferAllocator = allocator.get();
                    m_aliasedAllocators.emplace_back(AZStd::move(allocator));
                }

                if (descriptor.m_imageBudgetInBytes || allowNoBudget)
                {
                    AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                    heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                    heapAllocatorDesc.m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
                    heapAllocatorDesc.m_budgetInBytes = descriptor.m_imageBudgetInBytes;
                    heapAllocatorDesc.m_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                    heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::Image;
                    heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;

                    RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                    allocator->SetName(Name("TransientAttachmentPool [Images]"));
                    allocator->Init(device, heapAllocatorDesc);

                    m_imageAllocator = allocator.get();

                    m_aliasedAllocators.emplace_back(AZStd::move(m_imageAllocator));
                }

                if (descriptor.m_renderTargetBudgetInBytes || allowNoBudget)
                {
                    AliasedAttachmentAllocator::Descriptor heapAllocatorDesc;
                    heapAllocatorDesc.m_cacheSize = ObjectCacheSize;
                    heapAllocatorDesc.m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
                    heapAllocatorDesc.m_budgetInBytes = descriptor.m_renderTargetBudgetInBytes;
                    heapAllocatorDesc.m_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                    heapAllocatorDesc.m_resourceTypeMask = RHI::AliasedResourceTypeFlags::RenderTarget;
                    heapAllocatorDesc.m_allocationParameters = descriptor.m_heapParameters;

                    RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
                    allocator->SetName(Name("TransientAttachmentPool [Render Targets]"));
                    allocator->Init(device, heapAllocatorDesc);

                    m_renderTargetAllocator = allocator.get();

                    m_aliasedAllocators.emplace_back(AZStd::move(allocator));
                }
            }

            m_statistics.m_heaps.reserve(3);
            m_statistics.m_allocationPolicy = RHI::TransientAttachmentStatistics::AllocationPolicy::HeapPlacement;
            return RHI::ResultCode::Success;
        }

        void TransientAttachmentPool::ShutdownInternal()
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
            {
                allocator->Shutdown();
            }
            m_aliasedAllocators.clear();
            m_bufferAllocator = nullptr;
            m_imageAllocator = nullptr;
            m_renderTargetAllocator = nullptr;
        }

        void TransientAttachmentPool::BeginInternal(RHI::TransientAttachmentPoolCompileFlags compileFlags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint)
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

        void TransientAttachmentPool::EndInternal()
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_aliasedAllocators)
            {
                allocator->End();
            }

            AZ_Assert(m_imageToAllocatorMap.empty(), "Still active images.");

#if defined (DX12_TRANSIENT_ATTACHMENT_POOL_DEBUG_LOG)
            AZ_Printf("TransientAttachmentPool", "Transient Resource Memory Usage:\n");
            if (m_bufferAllocator)
            {
                AZ_Printf("TransientAttachmentPool", "\t       [Buffers]: %d\n", m_bufferAllocator->GetStatistics().m_watermarkSize);
            }
            if (m_imageAllocator)
            {
                AZ_Printf("TransientAttachmentPool", "\t        [Images]: %d\n", m_imageAllocator->GetStatistics().m_watermarkSize);
            }
            if (m_renderTargetAllocator)
            {
                AZ_Printf("TransientAttachmentPool", "\t[Render Targets]: %d\n", m_renderTargetAllocator->GetStatistics().m_watermarkSize);
            }
#endif

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

        RHI::Image* TransientAttachmentPool::ActivateImage(const RHI::TransientImageDescriptor& descriptor)
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
            RHI::Image* image = allocator->ActivateImage(descriptor, *m_currentScope);
            AZ_Assert(
                RHI::CheckBitsAll(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources) || image,
                "Failed to allocate image. Allocator %s is not big enough", allocator->GetName().GetCStr());
            m_imageToAllocatorMap[descriptor.m_attachmentId] = allocator;
            return image;
        }

        void TransientAttachmentPool::DeactivateImage(const RHI::AttachmentId& attachmentId)
        {
            auto findIt = m_imageToAllocatorMap.find(attachmentId);
            AZ_Assert(findIt->second, "Image is not associated with any allocator");
            AliasedAttachmentAllocator* allocator = findIt->second;
            allocator->DeactivateImage(attachmentId, *m_currentScope);
            m_imageToAllocatorMap.erase(findIt);
        }

        RHI::Buffer* TransientAttachmentPool::ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor)
        {
            AZ_Assert(m_bufferAllocator, "No buffer heap allocator to allocate a transient buffer. Make sure you specified one at pool creation time");
            RHI::Buffer* buffer = m_bufferAllocator->ActivateBuffer(descriptor, *m_currentScope);
            AZ_Assert(RHI::CheckBitsAll(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources) || buffer, "Failed to allocate buffer. Allocator is not big enough.");
            return buffer;
        }

        void TransientAttachmentPool::DeactivateBuffer(const RHI::AttachmentId& attachmentId)
        {
            m_bufferAllocator->DeactivateBuffer(attachmentId, *m_currentScope);
        }
    }
}
