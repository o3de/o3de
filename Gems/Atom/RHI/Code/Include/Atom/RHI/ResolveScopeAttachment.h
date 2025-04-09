/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    //! A specialization of a scope attachment for msaa images. Provides
    //! access to the msaa image view and msaa image scope attachment descriptor.
    class ResolveScopeAttachment final
        : public ImageScopeAttachment
    {
    public:
        AZ_RTTI(ResolveScopeAttachment, "{DC1236D5-69B6-478C-9B7D-3732383A4D61}", ImageScopeAttachment);
        AZ_CLASS_ALLOCATOR(ResolveScopeAttachment, AZ::PoolAllocator);

        ResolveScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            const ResolveScopeAttachmentDescriptor& descriptor);

        const ResolveScopeAttachmentDescriptor& GetDescriptor() const;

    private:
        ResolveScopeAttachmentDescriptor m_descriptor;
    };
}
