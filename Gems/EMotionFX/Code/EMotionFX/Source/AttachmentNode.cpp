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

// include the required headers
#include "EMotionFXConfig.h"
#include "AttachmentNode.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttachmentNode, AttachmentAllocator, 0)

    AttachmentNode::AttachmentNode(ActorInstance* attachToActorInstance, AZ::u32 attachToNodeIndex, ActorInstance* attachment, bool managedExternally)
        : Attachment(attachToActorInstance, attachment)
        , m_attachedToNode(attachToNodeIndex)
        , m_isManagedExternally(managedExternally)
    {
        AZ_Assert(attachToNodeIndex < attachToActorInstance->GetNumNodes(), "Node index is out of bounds.");
    }


    AttachmentNode::~AttachmentNode()
    {
    }


    AttachmentNode* AttachmentNode::Create(ActorInstance* attachToActorInstance, AZ::u32 attachToNodeIndex, ActorInstance* attachment, bool managedExternally)
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


    uint32 AttachmentNode::GetAttachToNodeIndex() const
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
