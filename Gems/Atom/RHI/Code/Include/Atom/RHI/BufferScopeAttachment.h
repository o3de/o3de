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

#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class BufferFrameAttachment;

        /**
         * A specialization of a scope attachment for buffers. Provides
         * access to the buffer view and buffer scope attachment descriptor.
         */
        class BufferScopeAttachment final
            : public ScopeAttachment
        {
        public:
            AZ_RTTI(BufferScopeAttachment, "{48A21F94-985B-40EE-A75A-8E960E935321}", ScopeAttachment);
            AZ_CLASS_ALLOCATOR(BufferScopeAttachment, AZ::PoolAllocator, 0);

            BufferScopeAttachment(
                Scope& scope,
                FrameAttachment& attachment,
                ScopeAttachmentUsage usage,
                ScopeAttachmentAccess access,
                const BufferScopeAttachmentDescriptor& descriptor);

            const BufferScopeAttachmentDescriptor& GetDescriptor() const;

            /// Returns the parent graph attachment referenced by this scope attachment.
            const BufferFrameAttachment& GetFrameAttachment() const;
            BufferFrameAttachment& GetFrameAttachment();

            /// Returns the previous scope attachment in the linked list.
            const BufferScopeAttachment* GetPrevious() const;
            BufferScopeAttachment* GetPrevious();

            /// Returns the next scope attachment in the linked list.
            const BufferScopeAttachment* GetNext() const;
            BufferScopeAttachment* GetNext();

            /// Returns the buffer view set on the scope attachment.
            const BufferView* GetBufferView() const;

            /// Assigns a buffer view to the scope attachment.
            void SetBufferView(ConstPtr<BufferView> bufferView);

        private:
            BufferScopeAttachmentDescriptor m_descriptor;
        };
    }
}
