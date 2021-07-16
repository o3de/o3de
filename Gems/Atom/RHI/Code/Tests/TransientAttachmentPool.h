/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace UnitTest
{
    class TransientAttachmentPool
        : public AZ::RHI::TransientAttachmentPool
    {
    public:
        AZ_CLASS_ALLOCATOR(TransientAttachmentPool, AZ::SystemAllocator, 0);

        TransientAttachmentPool() = default;

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&, const AZ::RHI::TransientAttachmentPoolDescriptor& descriptor) override;

        void ShutdownInternal() override;

        void BeginInternal(const AZ::RHI::TransientAttachmentPoolCompileFlags flags, const AZ::RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint) override;

        AZ::RHI::Image* ActivateImage(const AZ::RHI::TransientImageDescriptor&) override;

        AZ::RHI::Buffer* ActivateBuffer(const AZ::RHI::TransientBufferDescriptor&) override;

        void DeactivateBuffer(const AZ::RHI::AttachmentId&) override;

        void DeactivateImage(const AZ::RHI::AttachmentId&) override;

        void EndInternal() override;

        AZ::RHI::Ptr<AZ::RHI::ImagePool> m_imagePool;
        AZ::RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;
        AZStd::unordered_map<AZ::RHI::AttachmentId, AZ::RHI::Ptr<AZ::RHI::Resource>> m_attachments;

        AZStd::unordered_set<AZ::RHI::AttachmentId> m_activeSet;
    };
}
