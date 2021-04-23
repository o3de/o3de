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

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
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
        { }
    }
}
