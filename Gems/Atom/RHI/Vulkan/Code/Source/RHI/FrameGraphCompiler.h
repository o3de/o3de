/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Scope.h>
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace RHI
    {
        class BufferFrameAttachment;
        class ImageFrameAttachment;
        class ScopeAttachment;
    }

    namespace Vulkan
    {
        class Scope;
        class BufferView;

        class FrameGraphCompiler final
            : public RHI::FrameGraphCompiler
        {
            using Base = RHI::FrameGraphCompiler;

        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator, 0);

            static RHI::Ptr<FrameGraphCompiler> Create();

        private:
            FrameGraphCompiler() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphCompiler
            RHI::ResultCode InitInternal(RHI::Device& device) override;
            void ShutdownInternal() override;
            RHI::MessageOutcome CompileInternal(const RHI::FrameGraphCompileRequest& request) override;
            //////////////////////////////////////////////////////////////////////////

            void CompileResourceBarriers(const RHI::FrameGraphAttachmentDatabase& attachmentDatabase);
            void CompileBufferBarriers(RHI::BufferFrameAttachment& frameGraphAttachment);
            void CompileImageBarriers(RHI::ImageFrameAttachment& frameGraphAttachment);
            
            // Traverse all uses of the resource as scope attachments and add all the necessary barriers, semaphores and
            // ownership transfers for the resource as needed by the scopes.
            template<class ResourceScopeAttachment, class ResourceType>
            void CompileScopeAttachment(ResourceScopeAttachment& scopeAttachment, ResourceType& resource);

            // Queue the resource buffer barrier into the provided scope.
            void QueueResourceBarrier(
                Scope& scope,
                const RHI::ScopeAttachment& scopeAttachment,
                Buffer& buffer,
                const RHI::BufferSubresourceRange& range,
                const Scope::BarrierSlot slot,
                VkPipelineStageFlags srcPipelineStages,
                VkAccessFlags srcAccess,
                const QueueId& srcQueueId,
                const QueueId& dstQueueId) const;

            // Queue the resource barrier into the provided scope.
            void QueueResourceBarrier(
                Scope& scope,
                const RHI::ScopeAttachment& scopeAttachment,
                Image& image,
                const RHI::ImageSubresourceRange& range,
                const Scope::BarrierSlot slot,
                VkPipelineStageFlags srcPipelineStages,
                VkAccessFlags srcAccess,
                const QueueId& srcQueueId,
                const QueueId& dstQueueId) const;

            // Returns the subresource range.
            RHI::ImageSubresourceRange GetSubresourceRange(const RHI::ImageScopeAttachment& scopeAttachment) const;
            RHI::BufferSubresourceRange GetSubresourceRange(const RHI::BufferScopeAttachment& scopeAttachment) const;

            void CompileAsyncQueueSemaphores(const RHI::FrameGraph& frameGraph);
        };

        template<class ResourceScopeAttachment, class ResourceType>
        void FrameGraphCompiler::CompileScopeAttachment(ResourceScopeAttachment& scopeAttachment, ResourceType& resource)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            Scope& scope = static_cast<Scope&>(scopeAttachment.GetScope());
            RHI::ScopeAttachment* prevScopeAttachment = scopeAttachment.GetPrevious();
            Scope* prevScope = prevScopeAttachment ? &(static_cast<Scope&>(prevScopeAttachment->GetScope())) : nullptr;

            VkPipelineStageFlags srcPipelineStageFlags = prevScopeAttachment ? GetResourcePipelineStateFlags(*prevScopeAttachment) : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkAccessFlags srcAccessFlags = prevScopeAttachment ? GetResourceAccessFlags(*prevScopeAttachment) : 0;

            // Add VK_ACCESS_TRANSFER_WRITE_BIT in case we want to do a clear operation.
            if (HasExplicitClear(scopeAttachment, scopeAttachment.GetDescriptor()))
            {
                srcPipelineStageFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                srcAccessFlags = RHI::FilterBits(srcAccessFlags | VK_ACCESS_TRANSFER_WRITE_BIT, GetSupportedAccessFlags(srcPipelineStageFlags));
            }
        
            auto subresourceRange = GetSubresourceRange(scopeAttachment);
            auto subresourceOwnerList = resource.GetOwnerQueue(&subresourceRange);
            const QueueId destinationQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
            for (const auto& owner : subresourceOwnerList)
            {
                // For each resource we have 2 cases:
                // 1- Resource is owned by somebody in the same queue family. No need for ownership transfer.
                //    A barrier is needed if it's the same queue. Otherwise the semaphore will add the
                //    execution + memory dependency.
                // 2- Resource is owned by another queue family. Need to add barriers for
                //    release and acquire operations. A semaphore will add the
                //    execution + memory dependency. 

                const QueueId& ownerQueueId = owner.m_property;
                if (ownerQueueId.m_familyIndex == destinationQueueId.m_familyIndex)
                {
                    // Same family queue, no need for transfer ownership.
                    // If semaphores are needed they will be inserted during CompileAsyncQueueFences
                    QueueResourceBarrier(
                        scope,
                        scopeAttachment,
                        resource,
                        owner.m_range,
                        Scope::BarrierSlot::Prologue,
                        srcPipelineStageFlags,
                        srcAccessFlags,
                        ownerQueueId,
                        destinationQueueId);
                }
                else
                {
                    // We have different family queues. Need to do ownership transfer.
                    // Find a capable scope to initialize the queue transfer.
                    Scope* ownerScope = prevScope;
                    if (!ownerScope)
                    {
                        for (uint32_t queueIndex = 0; queueIndex < RHI::HardwareQueueClassCount && !ownerScope; ++queueIndex)
                        {
                            auto queueClass = static_cast<RHI::HardwareQueueClass>(queueIndex);
                            if (queueContext.GetQueueFamilyIndex(queueClass) == ownerQueueId.m_familyIndex)
                            {
                                ownerScope = static_cast<Scope*>(scope.FindCrossQueueProducer(queueClass));
                            }
                        }
                    }

                    // If we can't find a proper scope to do the transfer we just use the resource
                    // in the new queue. This will invalidate the previous content.
                    if (ownerScope)
                    {
                        // Add a release request of the resource from the owner queue
                        QueueResourceBarrier(
                            *ownerScope,
                            scopeAttachment,
                            resource,
                            owner.m_range,
                            Scope::BarrierSlot::Epilogue,
                            srcPipelineStageFlags,
                            srcAccessFlags,
                            ownerQueueId,
                            destinationQueueId);

                        // Add a request request of the resource from the destination queue
                        QueueResourceBarrier(
                            scope,
                            scopeAttachment,
                            resource,
                            owner.m_range,
                            Scope::BarrierSlot::Prologue,
                            srcPipelineStageFlags,
                            srcAccessFlags,
                            ownerQueueId,
                            destinationQueueId);
                    }
                }
            }

            resource.SetOwnerQueue(destinationQueueId, &subresourceRange);
        }
    }
}
