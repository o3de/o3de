/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/TransientAttachmentPool.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI/DeviceBuffer.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode TransientAttachmentPool::InitInternal(RHI::Device& device, const RHI::TransientAttachmentPoolDescriptor&)
    {
        {
            m_imagePool = RHI::Factory::Get().CreateImagePool();

            RHI::ImagePoolDescriptor desc;
            desc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
            m_imagePool->Init(device, desc);
        }

        {
            m_bufferPool = RHI::Factory::Get().CreateBufferPool();

            RHI::BufferPoolDescriptor desc;
            desc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            m_bufferPool->Init(device, desc);
        }

        return RHI::ResultCode::Success;
    }

    void TransientAttachmentPool::ShutdownInternal()
    {
        m_imagePool = nullptr;
        m_bufferPool = nullptr;
        m_attachments.clear();
    }

    void TransientAttachmentPool::BeginInternal([[maybe_unused]] const RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
    }

    RHI::DeviceImage* TransientAttachmentPool::ActivateImage(
        const RHI::TransientImageDescriptor& descriptor)
    {
        using namespace AZ;
        auto findIt = m_attachments.find(descriptor.m_attachmentId);
        if (findIt != m_attachments.end())
        {
            return azrtti_cast<RHI::DeviceImage*>(findIt->second.get());
        }

        RHI::Ptr<RHI::DeviceImage> image = RHI::Factory::Get().CreateImage();

        RHI::DeviceImageInitRequest request;
        request.m_image = image.get();
        request.m_descriptor = descriptor.m_imageDescriptor;
        m_imagePool->InitImage(request);

        m_attachments.emplace(descriptor.m_attachmentId, image);
        m_activeSet.emplace(descriptor.m_attachmentId);

        return image.get();
    }

    RHI::DeviceBuffer* TransientAttachmentPool::ActivateBuffer(
        const RHI::TransientBufferDescriptor& descriptor)
    {
        using namespace AZ;
        auto findIt = m_attachments.find(descriptor.m_attachmentId);
        if (findIt != m_attachments.end())
        {
            return azrtti_cast<RHI::DeviceBuffer*>(findIt->second.get());
        }

        RHI::Ptr<RHI::DeviceBuffer> buffer = RHI::Factory::Get().CreateBuffer();

        RHI::DeviceBufferInitRequest request;
        request.m_descriptor = descriptor.m_bufferDescriptor;
        request.m_buffer = buffer.get();
        m_bufferPool->InitBuffer(request);

        m_attachments.emplace(descriptor.m_attachmentId, buffer);
        m_activeSet.emplace(descriptor.m_attachmentId);

        return buffer.get();
    }

    void TransientAttachmentPool::DeactivateBuffer(const RHI::AttachmentId& attachmentId)
    {
        AZ_Assert(m_activeSet.find(attachmentId) != m_activeSet.end(), "buffer not in the active set.");
        m_activeSet.erase(attachmentId);
    }

    void TransientAttachmentPool::DeactivateImage(const RHI::AttachmentId& attachmentId)
    {
        AZ_Assert(m_activeSet.find(attachmentId) != m_activeSet.end(), "image not in the active set.");
        m_activeSet.erase(attachmentId);
    }

    void TransientAttachmentPool::EndInternal()
    {
        AZ_Assert(m_currentScope == nullptr, "Scope not properly ended.");
        AZ_Assert(m_activeSet.empty(), "active set is not empty.");
        m_attachments.clear();
    }
}
