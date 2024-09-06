/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/WebGPU.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <RHI/TransientAttachmentPool.h>
#include <AzCore/std/limits.h>

namespace AZ::WebGPU
{
    RHI::Ptr<TransientAttachmentPool> TransientAttachmentPool::Create()
    {
        return aznew TransientAttachmentPool();
    }

    RHI::ResultCode TransientAttachmentPool::InitInternal(RHI::Device& device, [[maybe_unused]] const RHI::TransientAttachmentPoolDescriptor& descriptor)
    {
        m_heap = AliasedHeap::Create();
        RHI::AliasedHeapDescriptor heapDesc;
        heapDesc.m_alignment = 1;
        heapDesc.m_budgetInBytes = AZStd::numeric_limits<AZ::u64>::max();
        return m_heap->Init(device, heapDesc);
    }

    void TransientAttachmentPool::ShutdownInternal()
    {
        m_heap = nullptr;
    }

    void TransientAttachmentPool::BeginInternal(
        const RHI::TransientAttachmentPoolCompileFlags flags, [[maybe_unused]] const RHI::TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
        m_heap->Begin(flags);
    }

    void TransientAttachmentPool::EndInternal()
    {
        m_heap->End();
    }

    RHI::DeviceImage* TransientAttachmentPool::ActivateImage(const RHI::TransientImageDescriptor& descriptor)
    {
        RHI::DeviceImage* image = nullptr;
        if (CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            return image;
        }

        auto result = m_heap->ActivateImage(descriptor, *m_currentScope, &image);
        if (result != RHI::ResultCode::Success)
        {
            AZ_Assert(false, "Failed to allocated aliased image");
            return nullptr;
        }
        return image;
    }

    RHI::DeviceBuffer* TransientAttachmentPool::ActivateBuffer(const RHI::TransientBufferDescriptor& descriptor)
    {
        RHI::DeviceBuffer* buffer = nullptr;
        if (CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            return buffer;
        }

        auto result = m_heap->ActivateBuffer(descriptor, *m_currentScope, &buffer);
        if (result != RHI::ResultCode::Success)
        {
            AZ_Assert(false, "Failed to allocated aliased buffer");
            return nullptr;
        }
        return buffer;
    }

    void TransientAttachmentPool::DeactivateBuffer(const RHI::AttachmentId& attachmentId)
    {
        if (!CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            m_heap->DeactivateBuffer(attachmentId, *m_currentScope);
        }
    }

    void TransientAttachmentPool::DeactivateImage(const RHI::AttachmentId& attachmentId)
    {
        if (!CheckBitsAny(GetCompileFlags(), RHI::TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            m_heap->DeactivateBuffer(attachmentId, *m_currentScope);
        }
    }
}

