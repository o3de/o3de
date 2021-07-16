/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Scope.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <RHI/AliasedHeap.h>

namespace AZ
{
    namespace Metal
    {
        using AliasedAttachmentAllocator = RHI::AliasedAttachmentAllocator<AliasedHeap>;
    
        class TransientAttachmentPool
            : public RHI::TransientAttachmentPool
        {
            using Base = RHI::TransientAttachmentPool;
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator, 0);
            AZ_RTTI(TransientAttachmentPool, "{7E958929-A44F-4C5F-946D-61C283968C29}", Base);

            static RHI::Ptr<TransientAttachmentPool> Create();

        private:
            TransientAttachmentPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::TransientAttachmentPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::TransientAttachmentPoolDescriptor& descriptor) override;
            void BeginInternal(const RHI::TransientAttachmentPoolCompileFlags flags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override;
            void EndInternal() override;
            RHI::Image* ActivateImage(const RHI::TransientImageDescriptor& descriptor) override;
            RHI::Buffer* ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor) override;
            void DeactivateBuffer(const RHI::AttachmentId& attachmentId) override;
            void DeactivateImage(const RHI::AttachmentId& attachmentId) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////
            
            Device& GetDevice() const;
                        
            AZStd::vector<RHI::Ptr<AliasedAttachmentAllocator>> m_aliasedAllocators;
            AliasedAttachmentAllocator* m_imageAllocator = nullptr;
            AliasedAttachmentAllocator* m_renderTargetAllocator = nullptr;
            AliasedAttachmentAllocator* m_bufferAllocator = nullptr;
            
            AZStd::unordered_map<RHI::AttachmentId, AliasedAttachmentAllocator*> m_imageToAllocatorMap;
        };
    }
}
