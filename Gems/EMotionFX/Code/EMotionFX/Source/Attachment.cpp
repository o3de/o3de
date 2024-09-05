/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Attachment.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Attachment, AttachmentAllocator)

    // constructor
    Attachment::Attachment(ActorInstance* attachToActorInstance, ActorInstance* attachment)
        : MCore::RefCounted()
    {
        m_attachment     = attachment;
        m_actorInstance  = attachToActorInstance;
        if (m_attachment)
        {
            m_attachment->SetSelfAttachment(this);
        }
    }


    // destructor
    Attachment::~Attachment()
    {
    }


    ActorInstance* Attachment::GetAttachmentActorInstance() const
    {
        return m_attachment;
    }


    ActorInstance* Attachment::GetAttachToActorInstance() const
    {
        return m_actorInstance;
    }
}   // namespace
