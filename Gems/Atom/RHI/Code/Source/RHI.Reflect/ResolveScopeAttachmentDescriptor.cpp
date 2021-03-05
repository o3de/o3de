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
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
    {
        void ResolveScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ResolveScopeAttachmentDescriptor, ImageScopeAttachmentDescriptor>()
                    ->Version(2)
                    ->Field("ResolveAttachmentId", &ResolveScopeAttachmentDescriptor::m_resolveAttachmentId)
                    ;
            }
        }

        ResolveScopeAttachmentDescriptor::ResolveScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const AttachmentId& resolveAttachmentId,
            const ImageViewDescriptor& imageViewDescriptor,
            const AttachmentLoadStoreAction& loadStoreAction)
            : ImageScopeAttachmentDescriptor(attachmentId, imageViewDescriptor, loadStoreAction)
            , m_resolveAttachmentId{ resolveAttachmentId }
        {}
    }
}
