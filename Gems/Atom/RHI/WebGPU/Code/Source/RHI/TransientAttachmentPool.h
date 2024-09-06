/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <RHI/AliasedHeap.h>

namespace AZ::WebGPU
{
    class TransientAttachmentPool final
        : public RHI::DeviceTransientAttachmentPool
    {
        using Base = RHI::DeviceTransientAttachmentPool;
    public:
        AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);
        AZ_RTTI(TransientAttachmentPool, "{9EA06BFD-035D-442A-B46A-3FE2853813F4}", Base);

        static RHI::Ptr<TransientAttachmentPool> Create();

    private:
        TransientAttachmentPool() = default;
            
        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceTransientAttachmentPool
        RHI::ResultCode InitInternal(RHI::Device& device, const RHI::TransientAttachmentPoolDescriptor& descriptor) override;
        void BeginInternal(const RHI::TransientAttachmentPoolCompileFlags flags, const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override;
        void EndInternal() override;
        RHI::DeviceImage* ActivateImage(const RHI::TransientImageDescriptor& descriptor) override;
        RHI::DeviceBuffer* ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor) override;
        void DeactivateBuffer(const RHI::AttachmentId& attachmentId) override;
        void DeactivateImage(const RHI::AttachmentId& attachmentId) override;
        void ShutdownInternal() override;
        //////////////////////////////////////////////////////////////////////////

        //! Since aliasing is not supported, we use just one heap for all resource allocations.
        RHI::Ptr<AliasedHeap> m_heap;
    };
}
