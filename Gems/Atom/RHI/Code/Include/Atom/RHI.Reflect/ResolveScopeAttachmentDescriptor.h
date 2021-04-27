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

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Describes the binding of a resolve image attachment to a scope.
         */
        struct ResolveScopeAttachmentDescriptor
            : public ImageScopeAttachmentDescriptor
        {
            AZ_TYPE_INFO(ResolveScopeAttachmentDescriptor, "{8ADC8E2B-2221-487B-A13F-E14218292E39}");

            static void Reflect(AZ::ReflectContext* context);

            ResolveScopeAttachmentDescriptor() = default;

            explicit ResolveScopeAttachmentDescriptor(
                const AttachmentId& attachmentId,
                const AttachmentId& resolveAttachmentId,
                const ImageViewDescriptor& imageViewDescriptor = ImageViewDescriptor(),
                const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

            /// The attachment id associated with the attachment to be resolved.
            AttachmentId m_resolveAttachmentId;
        };
    }
}
