/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Null
    {
        class TransientAttachmentPool final
            : public RHI::TransientAttachmentPool
        {
            using Base = RHI::TransientAttachmentPool;
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator, 0);
            AZ_RTTI(TransientAttachmentPool, "{37FCCF38-52D0-46E4-B4D1-1CB2B063422D}", Base);

            static RHI::Ptr<TransientAttachmentPool> Create();

        private:
            TransientAttachmentPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::TransientAttachmentPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::TransientAttachmentPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void BeginInternal([[maybe_unused]] const RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override {}
            void EndInternal() override {}
            RHI::Image* ActivateImage([[maybe_unused]] const RHI::TransientImageDescriptor& descriptor) override { return nullptr;}
            RHI::Buffer* ActivateBuffer([[maybe_unused]] const RHI::TransientBufferDescriptor& descriptor) override { return nullptr;}
            void DeactivateBuffer([[maybe_unused]] const RHI::AttachmentId& attachmentId) override {}
            void DeactivateImage([[maybe_unused]] const RHI::AttachmentId& attachmentId) override {}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
