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

#include <Atom/RHI/ResolveScopeAttachment.h>

namespace AZ
{
    namespace RHI
    {
        ResolveScopeAttachment::ResolveScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            const ResolveScopeAttachmentDescriptor& descriptor)
            : ImageScopeAttachment(scope, attachment, ScopeAttachmentUsage::Resolve, ScopeAttachmentAccess::Write, descriptor)
            , m_descriptor{ descriptor }
        {
        }

        const ResolveScopeAttachmentDescriptor& ResolveScopeAttachment::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}