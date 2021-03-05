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

// include required headers
#include "Attachment.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Attachment, AttachmentAllocator, 0)

    // constructor
    Attachment::Attachment(ActorInstance* attachToActorInstance, ActorInstance* attachment)
        : BaseObject()
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
