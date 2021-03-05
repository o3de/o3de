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

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Describes the binding of an image attachment to a scope.
         */
        struct ImageScopeAttachmentDescriptor
            : public ScopeAttachmentDescriptor
        {
            AZ_TYPE_INFO(ImageScopeAttachmentDescriptor, "{66523EB6-9D3A-4633-A708-ADD57FDD5CE2}");

            static void Reflect(AZ::ReflectContext* context);

            ImageScopeAttachmentDescriptor() = default;

            explicit ImageScopeAttachmentDescriptor(
                const AttachmentId& attachmentId,
                const ImageViewDescriptor& imageViewDescriptor = ImageViewDescriptor(),
                const AttachmentLoadStoreAction& imageScopeLoadStoreAction = AttachmentLoadStoreAction());

            /// The image view associated with the binding.
            ImageViewDescriptor m_imageViewDescriptor;
        };
    }
}
