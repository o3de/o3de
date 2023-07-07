/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    void ScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ScopeAttachmentDescriptor>()
                ->Version(0)
                ->Field("AttachmentId", &ScopeAttachmentDescriptor::m_attachmentId)
                ->Field("LoadStoreAction", &ScopeAttachmentDescriptor::m_loadStoreAction)
                ;
        }
    }

    ScopeAttachmentDescriptor::ScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const AttachmentLoadStoreAction& loadStoreAction)
        : m_attachmentId(attachmentId)
        , m_loadStoreAction(loadStoreAction)
    {}
}
