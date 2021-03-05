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

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>

namespace AZ
{
    namespace RHI
    {
        /**
         *  Describes what rules to apply when the image or buffer attachment is loaded and stored
         */
        struct AttachmentLoadStoreAction
        {
            AZ_TYPE_INFO(AttachmentLoadStoreAction, "{B41084F9-ED52-49F1-A2FA-8F648B0EC0D4}");

            static void Reflect(AZ::ReflectContext* context);

            explicit AttachmentLoadStoreAction(
                const ClearValue& clearValue = ClearValue(),
                AttachmentLoadAction loadAction = AttachmentLoadAction::Load,
                AttachmentStoreAction storeAction = AttachmentStoreAction::Store,
                AttachmentLoadAction loadActionStencil = AttachmentLoadAction::Load,
                AttachmentStoreAction storeActionStencil = AttachmentStoreAction::Store);

            bool operator==(const AttachmentLoadStoreAction& other) const;
            
            /// The clear value if using a Clear load action. Ignored otherwise.
            ClearValue m_clearValue;

            /// The load action applied when the attachment is bound.
            AttachmentLoadAction m_loadAction = AttachmentLoadAction::Load;

            /// The store action applied when the attachment is bound.
            AttachmentStoreAction m_storeAction = AttachmentStoreAction::Store;

            /// The stencil load action. Applies only to depth-stencil image attachments.
            AttachmentLoadAction m_loadActionStencil = AttachmentLoadAction::Load;

            /// The stencil store action. Applies only to depth-stencil image attachments.
            AttachmentStoreAction m_storeActionStencil = AttachmentStoreAction::Store;
        };
    }
}
