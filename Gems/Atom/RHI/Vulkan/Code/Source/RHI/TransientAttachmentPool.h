/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <RHI/AliasedHeap.h>

namespace AZ
{
    namespace Vulkan
    {
        class Scope;

        using AliasedAttachmentAllocator = RHI::AliasedAttachmentAllocator<AliasedHeap>;

        class TransientAttachmentPool final
            : public RHI::DeviceTransientAttachmentPool
        {
            using Base = RHI::DeviceTransientAttachmentPool;

        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);
            AZ_RTTI(TransientAttachmentPool, "5440AD98-D7EF-4C73-BA79-D281253BC048", Base);

            static RHI::Ptr<TransientAttachmentPool> Create();
            ~TransientAttachmentPool() = default;

        private:
            TransientAttachmentPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceTransientAttachmentPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::TransientAttachmentPoolDescriptor& descriptor) override;
            void BeginInternal(const RHI::TransientAttachmentPoolCompileFlags compileFlags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override;
            void EndInternal() override;
            RHI::DeviceImage* ActivateImage(const RHI::TransientImageDescriptor& descriptor) override;
            RHI::DeviceBuffer* ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor) override;
            void DeactivateBuffer(const RHI::AttachmentId& attachmentId) override;
            void DeactivateImage(const RHI::AttachmentId& attachmentId) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            AliasedAttachmentAllocator* GetImageAllocator(const RHI::ImageDescriptor& imageDescriptor) const;
            AliasedAttachmentAllocator* CreateAllocator(
                Device& device,
                const RHI::HeapAllocationParameters& heapParameters,
                const VkMemoryRequirements& memoryRequirements,
                size_t budgetInBytes,
                RHI::AliasedResourceTypeFlags resourceTypeMask,
                const char* name);


            AZStd::vector<RHI::Ptr<AliasedAttachmentAllocator>> m_allocators;
            AliasedAttachmentAllocator* m_bufferAllocator = nullptr;
            AliasedAttachmentAllocator* m_imageAllocator = nullptr;
            AliasedAttachmentAllocator* m_renderTargetAllocator = nullptr;

            AZStd::unordered_map<RHI::AttachmentId, AliasedAttachmentAllocator*> m_imageToAllocatorMap;
        };
    }
}
