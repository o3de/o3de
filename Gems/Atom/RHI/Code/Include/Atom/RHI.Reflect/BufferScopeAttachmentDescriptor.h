/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    //! Describes a buffer scope attachment, which is the specific usage of a frame graph attachment on a scope.
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

        /// Returns the descriptor for the BufferView
        const BufferViewDescriptor& GetViewDescriptor() const;

        /// The buffer view associated with the binding.
        BufferViewDescriptor m_bufferViewDescriptor;
    };
}
