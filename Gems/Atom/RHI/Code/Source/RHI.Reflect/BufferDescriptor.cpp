/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    BufferBindFlags GetBufferBindFlags(ScopeAttachmentUsage usage, ScopeAttachmentAccess access)
    {
        switch (usage)
        {
        case ScopeAttachmentUsage::RenderTarget:
            break;
        case ScopeAttachmentUsage::DepthStencil:
            break;
        case ScopeAttachmentUsage::Shader:
            switch (access)
            {
            case ScopeAttachmentAccess::ReadWrite:
                return BufferBindFlags::ShaderReadWrite;
            case ScopeAttachmentAccess::Read:
                return BufferBindFlags::ShaderRead;
            case ScopeAttachmentAccess::Write:
                return BufferBindFlags::ShaderWrite;
            }
            break;
        case ScopeAttachmentUsage::Copy:
            switch (access)
            {
            case ScopeAttachmentAccess::Read:
                return BufferBindFlags::CopyRead;
            case ScopeAttachmentAccess::Write:
                return BufferBindFlags::CopyWrite;
            }
            break;
        case ScopeAttachmentUsage::Resolve:
            break;
        case ScopeAttachmentUsage::Predication:
            return BufferBindFlags::Predication;
        case ScopeAttachmentUsage::Indirect:
            return BufferBindFlags::Indirect;
        case ScopeAttachmentUsage::SubpassInput:
            break;
        case ScopeAttachmentUsage::InputAssembly:
            return BufferBindFlags::InputAssembly;
        case ScopeAttachmentUsage::ShadingRate:
            break;
        case ScopeAttachmentUsage::Uninitialized:
            break;
        default:
            break;
        }
        return BufferBindFlags::None;
    }

    BufferDescriptor::BufferDescriptor(BufferBindFlags bindFlags, size_t byteCount)
        : m_bindFlags{bindFlags}
        , m_byteCount{byteCount}
    {}

    AZ::HashValue64 BufferDescriptor::GetHash(AZ::HashValue64 seed /* = 0 */) const
    {
        // We can't just call TypeHash64 here as in most other classes:
        // The m_ownerDeviceIndex member is an optional. The value held by the optional is in a union which is not zero-initialized.
        // TypeHash64 just hashes the bytes of this struct, including the uninitialized bytes of the optional.
        auto hash = TypeHash64(m_byteCount);
        hash = TypeHash64(m_alignment, hash);
        hash = TypeHash64(m_bindFlags, hash);
        hash = TypeHash64(m_sharedQueueMask, hash);
        if (m_ownerDeviceIndex)
        {
            hash = TypeHash64(m_ownerDeviceIndex, hash);
        }
        return hash;
    }

    void BufferDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BufferDescriptor>()
                ->Version(0)
                ->Field("m_bindFlags", &BufferDescriptor::m_bindFlags)
                ->Field("m_byteCount", &BufferDescriptor::m_byteCount)
                ->Field("m_alignment", &BufferDescriptor::m_alignment)
                ->Field("m_ownerDeviceIndex", &BufferDescriptor::m_ownerDeviceIndex);
        }
    }
}
