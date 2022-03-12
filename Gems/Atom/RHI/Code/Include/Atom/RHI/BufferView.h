/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/ResourceView.h>

namespace AZ
{
    namespace RHI
    {
        class Buffer;

        //! BufferView is contains a platform-specific descriptor mapping to a linear sub-region of a specific buffer resource.
        //! It associates 1-to-1 with a BufferViewDescriptor.
        class BufferView
            : public ResourceView
        {
        public:
            AZ_RTTI(BufferView, "{3012F770-1DD7-4CEC-A5D0-E2FC807548C1}", ResourceView);
            virtual ~BufferView() = default;

            //! Initializes the buffer view with the provided buffer and view descriptor.
            ResultCode Init(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor);

            //! Returns the view descriptor used at initialization time.
            const BufferViewDescriptor& GetDescriptor() const;

            //! Returns the buffer associated with this view.
            const Buffer& GetBuffer() const;

            //! Returns whether the view maps to the full buffer.
            bool IsFullView() const override final;

            //! Tells the renderer to ignore any validation related to this buffer's state and scope attachments.
            //! Assumes that the programmer is manually managing the Read/Write state of the buffer correctly.
            bool IgnoreFrameAttachmentValidation() const { return m_descriptor.m_ignoreFrameAttachmentValidation; }

        private:
            bool ValidateForInit(const Buffer& buffer, const BufferViewDescriptor& viewDescriptor) const;

            /// The RHI descriptor for this view.
            BufferViewDescriptor m_descriptor;
        };
    }
}
