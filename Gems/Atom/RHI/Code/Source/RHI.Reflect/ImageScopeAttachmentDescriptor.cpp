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
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
    {
        void ImageScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageScopeAttachmentDescriptor, ScopeAttachmentDescriptor>()
                    ->Version(3)
                    ->Field("ImageViewDescriptor", &ImageScopeAttachmentDescriptor::m_imageViewDescriptor)
                    ;
            }
        }

        ImageScopeAttachmentDescriptor::ImageScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const ImageViewDescriptor& imageViewDescriptor,
            const AttachmentLoadStoreAction& imageScopeLoadStoreAction)
            : ScopeAttachmentDescriptor(attachmentId, imageScopeLoadStoreAction)
            , m_imageViewDescriptor{ imageViewDescriptor }
        {}
    }
}
