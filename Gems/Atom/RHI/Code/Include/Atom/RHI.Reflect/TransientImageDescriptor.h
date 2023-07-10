/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ClearValue.h>

namespace AZ::RHI
{
    struct TransientImageDescriptor
    {
        TransientImageDescriptor() = default;

        TransientImageDescriptor(
            const AttachmentId& attachmentId,
            const ImageDescriptor& imageDescriptor,
            HardwareQueueClassMask supportedQueueMask = HardwareQueueClassMask::All,
            const ClearValue* optimizedClearValue = nullptr);

        /// The attachment id to associate with the transient image.
        AttachmentId m_attachmentId;

        /// The image descriptor used to create the transient image.
        ImageDescriptor m_imageDescriptor;

        /// The set of supported synchronous queues for this transient image.
        HardwareQueueClassMask m_supportedQueueMask = HardwareQueueClassMask::All;

        /// The optimized clear value for the image. If left null, the clear value
        /// from the first clear operation is used.
        const ClearValue* m_optimizedClearValue = nullptr;

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;
    };
}
