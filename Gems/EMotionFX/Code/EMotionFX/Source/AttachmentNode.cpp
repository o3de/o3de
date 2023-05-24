/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "AttachmentNode.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttachmentNode, AttachmentAllocator)

    AttachmentNode::AttachmentNode(ActorInstance* attachToActorInstance, size_t attachToNodeIndex, ActorInstance* attachment, bool managedExternally)
        : Attachment(attachToActorInstance, attachment)
        , m_attachedToNode(attachToNodeIndex)
        , m_isManagedExternally(managedExternally)
    {
        AZ_Assert(attachToNodeIndex < attachToActorInstance->GetNumNodes(), "Node index is out of bounds.");
    }


    AttachmentNode::~AttachmentNode()
    {
    }


    AttachmentNode* AttachmentNode::Create(ActorInstance* attachToActorInstance, size_t attachToNodeIndex, ActorInstance* attachment, bool managedExternally)
    {
        return aznew AttachmentNode(attachToActorInstance, attachToNodeIndex, attachment, managedExternally);
    }


    void AttachmentNode::Update()
    {
        if (!m_attachment)
        {
            return;
        }

        // Pass the parent's world space transform into the attachment.
        if (!m_isManagedExternally)
        {
            const Transform worldTransform = m_actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(m_attachedToNode);
            m_attachment->SetParentWorldSpaceTransform(worldTransform);
        }
    }


    size_t AttachmentNode::GetAttachToNodeIndex() const
    {
        return m_attachedToNode;
    }


    bool AttachmentNode::GetIsManagedExternally() const
    {
        return m_isManagedExternally;
    }


    void AttachmentNode::SetIsManagedExternally(bool managedExternally)
    {
        m_isManagedExternally = managedExternally;
    }
}   // namespace EMotionFX
