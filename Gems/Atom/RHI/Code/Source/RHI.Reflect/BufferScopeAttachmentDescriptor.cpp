/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    void BufferScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<BufferScopeAttachmentDescriptor, ScopeAttachmentDescriptor>()
                ->Version(3)
                ->Field("BufferViewDescriptor", &BufferScopeAttachmentDescriptor::m_bufferViewDescriptor)
                ;
        }
    }

    BufferScopeAttachmentDescriptor::BufferScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const BufferViewDescriptor& bufferViewDescriptor,
        const AttachmentLoadStoreAction& bufferScopeLoadStoreAction)
        : ScopeAttachmentDescriptor(attachmentId, bufferScopeLoadStoreAction)
        , m_bufferViewDescriptor(bufferViewDescriptor)
    {}

    const BufferViewDescriptor& BufferScopeAttachmentDescriptor::GetViewDescriptor() const
    {
        return m_bufferViewDescriptor;
    }
}
