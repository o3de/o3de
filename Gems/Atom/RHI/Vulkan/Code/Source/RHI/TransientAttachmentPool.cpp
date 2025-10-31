/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/RHIBus.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<TransientAttachmentPool> TransientAttachmentPool::Create()
        {
            return aznew TransientAttachmentPool();
        }

        void TransientAttachmentPool::BeginInternal(const RHI::TransientAttachmentPoolCompileFlags compileFlags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint)
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_allocators)
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
            AliasedAttachmentAllocator* allocator = GetImageAllocator(descriptor.m_imageDescriptor);
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
            auto findIter = m_imageToAllocatorMap.find(attachmentId);
            AZ_Assert(findIter != m_imageToAllocatorMap.end(), "Could not find attachment %s", attachmentId.GetCStr());
            AliasedAttachmentAllocator* allocator = findIter->second;
            allocator->DeactivateImage(attachmentId, *m_currentScope);
        }

        void TransientAttachmentPool::EndInternal()
        {
            for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_allocators)
            {
                allocator->End();
            }

            if (RHI::CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::GatherStatistics))
            {
                for (RHI::Ptr<AliasedAttachmentAllocator>& allocator : m_allocators)
                {
                    size_t statsBegin = m_statistics.m_heaps.size();
                    allocator->GetStatistics(m_statistics.m_heaps);
                    CollectHeapStats(allocator->GetDescriptor().m_resourceTypeMask, { m_statistics.m_heaps.begin() + statsBegin, m_statistics.m_heaps.end() });
                }
            }
        }

        RHI::ResultCode TransientAttachmentPool::InitInternal(RHI::Device& deviceBase, const RHI::TransientAttachmentPoolDescriptor& descriptor)
        {
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

            if (descriptor.m_bufferBudgetInBytes || allowNoBudget)
            {
                // Use a buffer descriptor of size 1 to get the memory requirements.
                RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant | RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::Predication | RHI::BufferBindFlags::Indirect;
                bufferDescriptor.m_byteCount = 1;
                VkMemoryRequirements memRequirements = device.GetBufferMemoryRequirements(bufferDescriptor);

                m_bufferAllocator = CreateAllocator(
                    device,
                    descriptor.m_heapParameters,
                    memRequirements,
                    descriptor.m_bufferBudgetInBytes,
                    RHI::AliasedResourceTypeFlags::Buffer,
                    "TransientAttachmentPool [Buffers]");

                if (!m_bufferAllocator)
                {
                    return RHI::ResultCode::Fail;
                }                
            }

            if (descriptor.m_renderTargetBudgetInBytes || allowNoBudget)
            {
                // Use an image descriptor of size 1x1 to get the memory requirements.
                RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead;
                RHI::ImageDescriptor imageDescriptor = RHI::ImageDescriptor::Create2D(bindFlags, 1, 1, RHI::Format::R8G8B8A8_UNORM);
                VkMemoryRequirements memRequirements = device.GetImageMemoryRequirements(imageDescriptor);

                m_renderTargetAllocator = CreateAllocator(
                    device,
                    descriptor.m_heapParameters,
                    memRequirements,
                    descriptor.m_renderTargetBudgetInBytes,
                    RHI::AliasedResourceTypeFlags::RenderTarget,
                    "TransientAttachmentPool [Rendertargets]");

                if (!m_renderTargetAllocator)
                {
                    return RHI::ResultCode::Fail;
                }
            }

            if (descriptor.m_imageBudgetInBytes || allowNoBudget)
            {
                // Use an image descriptor of size 1x1 to get the memory requirements.
                RHI::ImageBindFlags bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
                RHI::ImageDescriptor imageDescriptor = RHI::ImageDescriptor::Create2D(bindFlags, 1, 1, RHI::Format::R8G8B8A8_UNORM);
                VkMemoryRequirements memRequirements = device.GetImageMemoryRequirements(imageDescriptor);                

                m_imageAllocator = CreateAllocator(
                    device,
                    descriptor.m_heapParameters,
                    memRequirements,
                    descriptor.m_imageBudgetInBytes,
                    RHI::AliasedResourceTypeFlags::Image,
                    "TransientAttachmentPool [Images]");

                if (!m_imageAllocator)
                {
                    return RHI::ResultCode::Fail;
                }
            }

            m_statistics.m_heaps.reserve(3);
            m_statistics.m_allocationPolicy = RHI::TransientAttachmentStatistics::AllocationPolicy::HeapPlacement;

            return RHI::ResultCode::Success;
        }

        void TransientAttachmentPool::ShutdownInternal()
        {
            m_allocators.clear();
            m_bufferAllocator = nullptr;
            m_imageAllocator = nullptr;
            m_renderTargetAllocator = nullptr;
        }

        AliasedAttachmentAllocator* TransientAttachmentPool::GetImageAllocator(const RHI::ImageDescriptor& imageDescriptor) const
        {
            if (RHI::CheckBitsAny(imageDescriptor.m_bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil))
            {
                return m_renderTargetAllocator;
            }
            else
            {
                return m_imageAllocator;
            }
        }       

        AliasedAttachmentAllocator* TransientAttachmentPool::CreateAllocator(
            Device& device,
            const RHI::HeapAllocationParameters& heapParameters,
            const VkMemoryRequirements& memRequirements,
            size_t budgetInBytes,
            RHI::AliasedResourceTypeFlags resourceTypeMask,
            const char* name)
        {
            const uint32_t ObjectCacheSize = 256;

            RHI::Ptr<AliasedAttachmentAllocator> allocator = AliasedAttachmentAllocator::Create();
            AliasedAttachmentAllocator::Descriptor heapDesc;
            heapDesc.m_cacheSize = ObjectCacheSize;
            heapDesc.m_memoryRequirements = memRequirements;
            heapDesc.m_budgetInBytes = budgetInBytes;
            heapDesc.m_resourceTypeMask = resourceTypeMask;
            heapDesc.m_allocationParameters = heapParameters;
            struct
            {
                size_t m_alignment = 0;
                void operator=(size_t value)
                {
                    m_alignment = AZStd::max(m_alignment, value);
                }
            } alignment;
            RHI::RHIRequirementRequestBus::BroadcastResult(alignment, &RHI::RHIRequirementsRequest::GetRequiredAlignment, device);
            heapDesc.m_alignment = AZStd::max(alignment.m_alignment, heapDesc.m_alignment);

            auto result = allocator->Init(device, heapDesc);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to initialize transient attachment allocator %s", name);
                return nullptr;
            }

            allocator->SetName(Name{ name });
            m_allocators.push_back(allocator);
            return allocator.get();
        }
    }
}
