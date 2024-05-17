/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/AliasedHeap.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/Scope.h>

namespace AZ
{
    namespace DX12
    {
        class Scope;

        using AliasedAttachmentAllocator = RHI::AliasedAttachmentAllocator<AliasedHeap>;

        class TransientAttachmentPool
            : public RHI::DeviceTransientAttachmentPool
        {
            using Base = RHI::DeviceTransientAttachmentPool;
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);
            AZ_RTTI(TransientAttachmentPool, "{2E513E84-0161-4A0C-8148-3364BFFFC5E4}", Base);

            static RHI::Ptr<TransientAttachmentPool> Create();            

        private:
            TransientAttachmentPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceTransientAttachmentPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::TransientAttachmentPoolDescriptor& descriptor) override;
            void BeginInternal(RHI::TransientAttachmentPoolCompileFlags flags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override;
            void EndInternal() override;
            RHI::DeviceImage* ActivateImage(const RHI::TransientImageDescriptor& descriptor) override;
            RHI::DeviceBuffer* ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor) override;
            void DeactivateBuffer(const RHI::AttachmentId& attachmentId) override;
            void DeactivateImage(const RHI::AttachmentId& attachmentId) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            D3D12_RESOURCE_HEAP_TIER m_resourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1;

            AZStd::vector<RHI::Ptr<AliasedAttachmentAllocator>> m_aliasedAllocators;
            AliasedAttachmentAllocator* m_bufferAllocator = nullptr;
            AliasedAttachmentAllocator* m_imageAllocator = nullptr;
            AliasedAttachmentAllocator* m_renderTargetAllocator = nullptr;
            AZStd::unordered_map<RHI::AttachmentId, AliasedAttachmentAllocator*> m_imageToAllocatorMap;
        };
    }
}
