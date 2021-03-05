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

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Describes a buffer scope attachment, which is the specific usage of a frame graph attachment on a scope.
         */
        struct BufferScopeAttachmentDescriptor
            : public ScopeAttachmentDescriptor
        {
            AZ_TYPE_INFO(BufferScopeAttachmentDescriptor, "{D40FFADC-DDD4-497A-877F-11E84FD96210}");

            static void Reflect(AZ::ReflectContext* context);

            BufferScopeAttachmentDescriptor() = default;

            explicit BufferScopeAttachmentDescriptor(
                const AttachmentId& attachmentId,
                const BufferViewDescriptor& bufferViewDescriptor = BufferViewDescriptor(),
                const AttachmentLoadStoreAction& bufferScopeLoadStoreAction = AttachmentLoadStoreAction());

            /// The buffer view associated with the binding.
            BufferViewDescriptor m_bufferViewDescriptor;
        };
    }
}
