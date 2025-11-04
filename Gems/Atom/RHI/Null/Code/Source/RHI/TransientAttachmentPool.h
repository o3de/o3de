/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceTransientAttachmentPool.h>

namespace AZ
{
    namespace Null
    {
        class TransientAttachmentPool final
            : public RHI::DeviceTransientAttachmentPool
        {
            using Base = RHI::DeviceTransientAttachmentPool;
        public:
            AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator);
            AZ_RTTI(TransientAttachmentPool, "{37FCCF38-52D0-46E4-B4D1-1CB2B063422D}", Base);

            static RHI::Ptr<TransientAttachmentPool> Create();

        private:
            TransientAttachmentPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceTransientAttachmentPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::TransientAttachmentPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void BeginInternal([[maybe_unused]] const RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override {}
            void EndInternal() override {}
            RHI::DeviceImage* ActivateImage([[maybe_unused]] const RHI::TransientImageDescriptor& descriptor) override { return nullptr;}
            RHI::DeviceBuffer* ActivateBuffer([[maybe_unused]] const RHI::TransientBufferDescriptor& descriptor) override { return nullptr;}
            void DeactivateBuffer([[maybe_unused]] const RHI::AttachmentId& attachmentId) override {}
            void DeactivateImage([[maybe_unused]] const RHI::AttachmentId& attachmentId) override {}
            void ShutdownInternal() override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
