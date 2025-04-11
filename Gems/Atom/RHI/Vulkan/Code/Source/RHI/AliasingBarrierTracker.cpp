/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>

namespace AZ
{
    namespace Vulkan
    {
        AliasingBarrierTracker::AliasingBarrierTracker(Device& device, uint64_t heapSize)
            :m_device(device)
        {
            m_pipelineAccess.assign(0, heapSize + 1, PipelineAccessFlags{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE });
        }

        template<class T>
        PipelineAccessFlags CollectPipelineAccess(const AZStd::vector<T>& propertyRanges)
        {
            PipelineAccessFlags flags = {};
            for (const auto& range : propertyRanges)
            {
                flags |= range.m_property;
            }
            return flags;
        }

        // Builds the pipeline and access flags needed when accessing the same memory that this aliased
        // resource is using.
        PipelineAccessFlags GetAliasedResourcePipelineAccessFlags(const RHI::AliasedResource& resource)
        {
            // First find the first usage of the resource
            const auto& transientAttachments = resource.m_beginScope->GetTransientAttachments();
            auto it = AZStd::find_if(
                transientAttachments.begin(),
                transientAttachments.end(),
                [&](const RHI::ScopeAttachment* scopeAttachment)
                {
                    return scopeAttachment->GetFrameAttachment().GetId() == resource.m_attachmentId;
                });

            AZ_Assert(it != transientAttachments.end(), "Failed to find transient attachment %s", resource.m_attachmentId.GetCStr());

            // Need to collect the pipeline and access of all the usages of the resource.
            PipelineAccessFlags transientFlags;
            if (resource.m_type == RHI::AliasedResourceType::Image)
            {
                Image* image = static_cast<Image*>(resource.m_resource);
                // Use an interval map to keep track of the stage and accesses.
                Image::ImagePipelineAccessProperty imagePipelineAccess;
                imagePipelineAccess.Init(image->GetDescriptor());
                for (const RHI::ScopeAttachment* scopeAttachment = *it; scopeAttachment != nullptr;
                     scopeAttachment = scopeAttachment->GetNext())
                {
                    PipelineAccessFlags flags;
                    flags.m_access = GetResourceAccessFlags(*scopeAttachment);
                    flags.m_pipelineStage = GetResourcePipelineStateFlags(*scopeAttachment);
                    const RHI::ImageScopeAttachment* imageScopeAttachment = static_cast<const RHI::ImageScopeAttachment*>(scopeAttachment);
                    auto range = RHI::ImageSubresourceRange(imageScopeAttachment->GetDescriptor().GetViewDescriptor());
                    // If it's a write or read after write access, we override the PipelineAccessFlags values (since we know that the scope will synchronize against it)
                    // If it's a read after read access, the scope will not synchronize, so we combine the PipelineAccessFlags, so future accesses synchronize
                    // against all read accesses.
                    if (IsReadOnlyAccess(flags.m_access))
                    {
                        PipelineAccessFlags currentFlags = CollectPipelineAccess(imagePipelineAccess.Get(range));
                        if (IsReadOnlyAccess(currentFlags.m_access))
                        {
                            flags |= currentFlags;
                        }
                    }
                    imagePipelineAccess.Set(range, flags);
                }
                // Now collect the final PipelineAccessFlags values.
                auto overlap = imagePipelineAccess.Get(RHI::ImageSubresourceRange(image->GetDescriptor()));
                for (auto overlapIt : overlap)
                {
                    transientFlags |= overlapIt.m_property;
                }
            }
            else // Buffer
            {
                Buffer* buffer = static_cast<Buffer*>(resource.m_resource);
                // Use an interval map to keep track of the stage and accesses.
                Buffer::BufferPipelineAccessProperty bufferPipelineAccess;
                bufferPipelineAccess.Init(buffer->GetDescriptor());
                for (const RHI::ScopeAttachment* scopeAttachment = *it; scopeAttachment != nullptr;
                     scopeAttachment = scopeAttachment->GetNext())
                {
                    PipelineAccessFlags flags;
                    flags.m_access = GetResourceAccessFlags(*scopeAttachment);
                    flags.m_pipelineStage = GetResourcePipelineStateFlags(*scopeAttachment);
                    const RHI::BufferScopeAttachment* bufferScopeAttachment =
                        static_cast<const RHI::BufferScopeAttachment*>(scopeAttachment);
                    auto range = RHI::BufferSubresourceRange(bufferScopeAttachment->GetDescriptor().GetViewDescriptor());
                    // If it's a write or read after write access, we override the PipelineAccessFlags values (since we know that the scope
                    // will synchronize against it) If it's a read after read access, the scope will not synchronize, so we combine the
                    // PipelineAccessFlags, so future accesses synchronize against all read accesses.
                    if (IsReadOnlyAccess(flags.m_access))
                    {
                        PipelineAccessFlags currentFlags = CollectPipelineAccess(bufferPipelineAccess.Get(range));
                        if (IsReadOnlyAccess(currentFlags.m_access))
                        {
                            flags |= currentFlags;
                        }
                    }
                    bufferPipelineAccess.Set(range, flags);
                }
                // Now collect the final PipelineAccessFlags values.
                auto overlap = bufferPipelineAccess.Get(RHI::BufferSubresourceRange(buffer->GetDescriptor()));
                for (auto overlapIt : overlap)
                {
                    transientFlags |= overlapIt.m_property;
                }
            }

            return transientFlags;
        }

        void AliasingBarrierTracker::AddResourceInternal(const RHI::AliasedResource& resourceNew)
        {
            // No need to emit an aliased barrier on first usage of the resource.
            // We just need to set the proper pipeline and access flags so during first usage, the scope can synchronize properly.
            // That first usage will provide the aliasing synchronization that is needed.
            PipelineAccessFlags srcFlags{};

            const auto& queueContext = m_device.GetCommandQueueContext();
            QueueId newQueueId = queueContext.GetCommandQueue(resourceNew.m_beginScope->GetHardwareQueueClass()).GetId();

            // Get the pipeline and access flags that the memory used by the aliased resource needs.
            // These flags include previous usages of this memory by other resources.
            auto overlap = m_pipelineAccess.overlap(resourceNew.m_byteOffsetMin, resourceNew.m_byteOffsetMax + 1);
            for (auto it = overlap.first; it != overlap.second; ++it)
            {
                srcFlags |= it.value();
            }

            if (resourceNew.m_type == RHI::AliasedResourceType::Image)
            {
                Image* image = static_cast<Image*>(resourceNew.m_resource);
                image->SetLayout(VK_IMAGE_LAYOUT_UNDEFINED);
                image->SetOwnerQueue(newQueueId);
                image->SetPipelineAccess(srcFlags);
            }
            else // Buffer
            {
                Buffer* buffer = static_cast<Buffer*>(resourceNew.m_resource);
                buffer->SetOwnerQueue(newQueueId);
                buffer->SetPipelineAccess(srcFlags);
            }

            // Set the aliased memory with the PipelineAccessFlags from using the aliased resource.
            // This way future resources that use this same memory can synchronize properly.
            PipelineAccessFlags dstFlags = GetAliasedResourcePipelineAccessFlags(resourceNew);
            m_pipelineAccess.assign(resourceNew.m_byteOffsetMin, resourceNew.m_byteOffsetMax + 1, dstFlags);
        }

        void AliasingBarrierTracker::AppendBarrierInternal(const RHI::AliasedResource& beforeResource, const RHI::AliasedResource& afterResource)
        {
            const auto& queueContext = m_device.GetCommandQueueContext();
            QueueId oldQueueId = queueContext.GetCommandQueue(beforeResource.m_endScope->GetHardwareQueueClass()).GetId();
            QueueId newQueueId = queueContext.GetCommandQueue(afterResource.m_beginScope->GetHardwareQueueClass()).GetId();

            // If resources are in different queues then we need to add an execution dependency so
            // we don't start using the aliased resource in the new scope before the old scope finishes.
            // The framegraph does not handle this case because we have to remember that from its perspective
            // these are two different resources that don't need a barrier/semaphore.
            if (oldQueueId != newQueueId)
            {
                PipelineAccessFlags dstFlags = GetAliasedResourcePipelineAccessFlags(afterResource);
                auto semaphore = m_device.GetSemaphoreAllocator().Allocate();
                static_cast<Scope*>(beforeResource.m_endScope)->AddSignalSemaphore(semaphore);
                static_cast<Scope*>(afterResource.m_beginScope)->AddWaitSemaphore(AZStd::make_pair(dstFlags.m_pipelineStage, semaphore));
                // This will not deallocate immediately.
                m_barrierSemaphores.push_back(semaphore);
            }
        }

        void AliasingBarrierTracker::EndInternal()
        {
            m_device.GetSemaphoreAllocator().DeAllocate(m_barrierSemaphores.data(), m_barrierSemaphores.size());
            m_barrierSemaphores.clear();
        }
    }
}
