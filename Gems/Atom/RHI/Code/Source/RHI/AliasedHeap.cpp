/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/AliasedHeap.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        void AliasedHeap::Begin(TransientAttachmentPoolCompileFlags compileFlags)
        {
            m_totalAllocations = 0;
            m_compileFlags = compileFlags;
            m_heapStats.m_watermarkSize = 0;
            m_heapStats.m_attachments.clear();
            m_barrierTracker->Reset();
        }

        void AliasedHeap::End()
        {
            AZ_Assert(m_activeAttachmentLookup.empty() && m_firstFitAllocator.GetAllocationCount() == 0,
                "There are still active allocations.");

            if (RHI::CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::GatherStatistics))
            {
                AZStd::sort(m_heapStats.m_attachments.begin(), m_heapStats.m_attachments.end(),
                    [](
                        const RHI::TransientAttachmentStatistics::Attachment& lhs,
                        const RHI::TransientAttachmentStatistics::Attachment& rhs)
                {
                    if (lhs.m_scopeOffsetMin == rhs.m_scopeOffsetMin)
                    {
                        return lhs.m_heapOffsetMin < rhs.m_heapOffsetMin;
                    }
                    return lhs.m_scopeOffsetMin < rhs.m_scopeOffsetMin;
                });
            }

            m_barrierTracker->End();
        }

        RHI::ResultCode AliasedHeap::Init(Device& device, const AliasedHeapDescriptor& descriptor)
        {
            return Base::Init(
                device, descriptor,
                [this, &device, &descriptor]()
            {
                m_descriptor = descriptor;

                ResultCode result = InitInternal(device, descriptor);
                if (result == ResultCode::Success)
                {
                    m_barrierTracker = CreateBarrierTrackerInternal();

                    HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(HeapMemoryLevel::Device);
                    heapMemoryUsage.m_residentInBytes = descriptor.m_budgetInBytes;
                    heapMemoryUsage.m_reservedInBytes = descriptor.m_budgetInBytes;

                    m_cache.SetCapacity(descriptor.m_cacheSize);

                    {
                        FreeListAllocator::Descriptor allocatorDesc;
                        allocatorDesc.m_capacityInBytes = descriptor.m_budgetInBytes;
                        allocatorDesc.m_garbageCollectLatency = 0;
                        allocatorDesc.m_alignmentInBytes = descriptor.m_alignment;

                        // A first-fit allocator is used because it will keep a low watermark.
                        allocatorDesc.m_policy = FreeListAllocatorPolicy::FirstFit;

                        m_firstFitAllocator.Init(allocatorDesc);
                    }

                    m_heapStats.m_name = GetName();
                    m_heapStats.m_heapSize = descriptor.m_budgetInBytes;
                    m_heapStats.m_resourceTypeFlags = descriptor.m_resourceTypeMask;
                }

                return result;
            });
        }

        void AliasedHeap::ShutdownInternal()
        {
            m_barrierTracker = nullptr;
            m_cache.Clear();
            m_firstFitAllocator.Shutdown();
        }

        ResultCode AliasedHeap::ActivateBuffer(
            const RHI::TransientBufferDescriptor& descriptor,
            Scope& scope,
            Buffer** activatedBuffer)
        {
            ResourceMemoryRequirements memRequirements = GetDevice().GetResourceMemoryRequirements(descriptor.m_bufferDescriptor);
            
            const size_t alignmentInBytes = memRequirements.m_alignmentInBytes;
            RHI::VirtualAddress address = m_firstFitAllocator.Allocate(memRequirements.m_sizeInBytes, alignmentInBytes);
            if (address.IsNull())
            {
                return ResultCode::OutOfMemory;
            }

            const size_t heapOffsetInBytes = address.m_ptr;
            m_heapStats.m_watermarkSize = AZStd::max(m_heapStats.m_watermarkSize, heapOffsetInBytes + static_cast<size_t>(memRequirements.m_sizeInBytes));

            Buffer* buffer = nullptr;
            if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
            {
                AZ_Assert(m_totalAllocations < m_cache.GetCapacity(),
                    "Exceeded the size of the cache! This will destroy actively used resources");
                m_totalAllocations++;

                HashValue64 hash = descriptor.GetHash();
                // [GFX TODO][ATOM-6289] This should be looked into, combining cityhash with AZStd::hash
                AZStd::hash_combine(reinterpret_cast<size_t&>(hash), heapOffsetInBytes);

                // Check the cache for the buffer.
                if (RHI::Resource* attachment = m_cache.Find(static_cast<uint64_t>(hash)))
                {
                    buffer = static_cast<Buffer*>(attachment);
                }
                // buffer is not in cache. Create a new one at the placed address, and add it to the cache.
                else
                {
                    /// Ownership is managed by the cache.
                    RHI::Ptr<Buffer> bufferPtr = Factory::Get().CreateBuffer();
                    buffer = bufferPtr.get();

                    RHI::ResultCode resultCode = Base::InitResource(
                        buffer,
                        [this, buffer, &descriptor, heapOffsetInBytes]
                    {
                        BufferInitRequest request(*buffer, descriptor.m_bufferDescriptor);
                        return InitBufferInternal(request, heapOffsetInBytes);
                    });

                    if (resultCode != RHI::ResultCode::Success)
                    {
                        return resultCode;
                    }

                    buffer->SetName(descriptor.m_attachmentId);
                    m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferPtr));
                }
            }

            const uint32_t attachmentIndex = static_cast<uint32_t>(m_heapStats.m_attachments.size());
            m_activeAttachmentLookup.emplace(descriptor.m_attachmentId, AttachmentData{ buffer, attachmentIndex, &scope });
            m_heapStats.m_attachments.emplace_back();

            RHI::TransientAttachmentStatistics::Attachment& attachment = m_heapStats.m_attachments.back();
            attachment.m_heapOffsetMin = heapOffsetInBytes;
            attachment.m_heapOffsetMax = heapOffsetInBytes + memRequirements.m_sizeInBytes - 1;
            attachment.m_sizeInBytes = memRequirements.m_sizeInBytes;
            attachment.m_id = descriptor.m_attachmentId;
            attachment.m_scopeOffsetMin = scope.GetIndex();
            attachment.m_type = AliasedResourceType::Buffer;

            if (activatedBuffer)
            {
                *activatedBuffer = buffer;
            }

            return ResultCode::Success;
        }

        void AliasedHeap::DeactivateBuffer(const AttachmentId& bufferAttachment, Scope& scope)
        {
            DeactivateResourceInternal(bufferAttachment, scope, AliasedResourceType::Buffer);
        }

        void AliasedHeap::DeactivateResourceInternal(const AttachmentId& attachmentId, Scope& scope, AliasedResourceType type)
        {
            auto findIter = m_activeAttachmentLookup.find(attachmentId);
            if (findIter == m_activeAttachmentLookup.end())
            {
                AZ_Assert(false, "Failed to find transient attachment: %s", attachmentId.GetCStr());
                return;
            }

            const AttachmentData& attachmentData = findIter->second;

            TransientAttachmentStatistics::Attachment& attachment = m_heapStats.m_attachments[attachmentData.m_attachmentIndex];
            attachment.m_scopeOffsetMax = scope.GetIndex();

            if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
            {
                // Now that we have the begin and end scope we can add the resource to the tracker.
                AliasedResource aliasedResource;
                aliasedResource.m_beginScope = attachmentData.m_activateScope;
                aliasedResource.m_endScope = &scope;
                aliasedResource.m_resource = attachmentData.m_resource;
                aliasedResource.m_byteOffsetMin = attachment.m_heapOffsetMin;
                aliasedResource.m_byteOffsetMax = attachment.m_heapOffsetMax;
                aliasedResource.m_type = type;

                m_barrierTracker->AddResource(aliasedResource);
            }
            
            const VirtualAddress heapAddress{attachment.m_heapOffsetMin};
            m_firstFitAllocator.DeAllocate(heapAddress);
            m_firstFitAllocator.GarbageCollectForce();
            m_activeAttachmentLookup.erase(findIter);
        }

        ResultCode AliasedHeap::ActivateImage(const RHI::TransientImageDescriptor& descriptor, Scope& scope, Image** activatedImage)
        {
            ResourceMemoryRequirements memRequirements = GetDevice().GetResourceMemoryRequirements(descriptor.m_imageDescriptor);

            VirtualAddress address = m_firstFitAllocator.Allocate(memRequirements.m_sizeInBytes, memRequirements.m_alignmentInBytes);
            if (address.IsNull())
            {
                return ResultCode::OutOfMemory;
            }

            const size_t heapOffsetInBytes = address.m_ptr;
            m_heapStats.m_watermarkSize = AZStd::max(m_heapStats.m_watermarkSize, heapOffsetInBytes + static_cast<size_t>(memRequirements.m_sizeInBytes));

            Image* image = nullptr;

            if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
            {
                AZ_Assert(m_totalAllocations < m_cache.GetCapacity(),
                    "Exceeded the size of the cache! This will destroy actively used resources");
                m_totalAllocations++;

                HashValue64 hash = descriptor.GetHash();
                // [GFX TODO][ATOM-6289] This should be looked into, combining cityhash with AZStd::hash
                AZStd::hash_combine(reinterpret_cast<size_t&>(hash), heapOffsetInBytes);

                // Check the cache for the image.
                if (RHI::Resource* attachment = m_cache.Find(static_cast<uint64_t>(hash)))
                {
                    image = static_cast<Image*>(attachment);
                }
                // image is not in cache. Create a new one at the placed address, and add it to the cache.
                else
                {
                    RHI::Ptr<Image> imagePtr = Factory::Get().CreateImage();
                    image = imagePtr.get();

                    RHI::ResultCode resultCode = Base::InitResource(
                        image,
                        [this, image, &descriptor, heapOffsetInBytes]
                    {
                        ImageInitRequest request(*image, descriptor.m_imageDescriptor, descriptor.m_optimizedClearValue);
                        return InitImageInternal(request, heapOffsetInBytes);
                    });

                    if (resultCode != RHI::ResultCode::Success)
                    {
                        return resultCode;
                    }

                    image->SetName(descriptor.m_attachmentId);
                    m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(imagePtr));
                }
            }

            const size_t sizeInBytes = memRequirements.m_sizeInBytes;

            const uint32_t attachmentIndex = static_cast<uint32_t>(m_heapStats.m_attachments.size());
            m_activeAttachmentLookup.emplace(descriptor.m_attachmentId, AttachmentData{ image, attachmentIndex, &scope });
            m_heapStats.m_attachments.emplace_back();

            RHI::TransientAttachmentStatistics::Attachment& attachment = m_heapStats.m_attachments.back();
            attachment.m_heapOffsetMin = heapOffsetInBytes;
            attachment.m_heapOffsetMax = heapOffsetInBytes + sizeInBytes - 1;
            attachment.m_sizeInBytes = sizeInBytes;
            attachment.m_id = descriptor.m_attachmentId;
            attachment.m_scopeOffsetMin = scope.GetIndex();
            attachment.m_type = CheckBitsAny(descriptor.m_imageDescriptor.m_bindFlags, ImageBindFlags::Color | ImageBindFlags::DepthStencil) ?
                AliasedResourceType::RenderTarget : AliasedResourceType::Image;

            if (activatedImage)
            {
                *activatedImage = image;
            }
            
            return ResultCode::Success;
        }

        void AliasedHeap::DeactivateImage(const AttachmentId& imageAttachment, Scope& scope)
        {
            DeactivateResourceInternal(imageAttachment, scope, AliasedResourceType::Image);
        }

        const AliasedHeapDescriptor& AliasedHeap::GetDescriptor() const
        {
            return m_descriptor;
        }

        const RHI::TransientAttachmentStatistics::Heap& AliasedHeap::GetStatistics() const
        {
            return m_heapStats;
        }
    }
}
