/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
