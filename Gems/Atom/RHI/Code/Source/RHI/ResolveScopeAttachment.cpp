/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ResolveScopeAttachment.h>

namespace AZ::RHI
{
    ResolveScopeAttachment::ResolveScopeAttachment(
        Scope& scope,
        FrameAttachment& attachment,
        const ResolveScopeAttachmentDescriptor& descriptor)
        : ImageScopeAttachment(
              scope, attachment, ScopeAttachmentUsage::Resolve, ScopeAttachmentAccess::Write, ScopeAttachmentStage::Copy, descriptor)
        , m_descriptor{ descriptor }
    {
    }

    const ResolveScopeAttachmentDescriptor& ResolveScopeAttachment::GetDescriptor() const
    {
        return m_descriptor;
    }
}
