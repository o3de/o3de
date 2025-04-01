/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "AttachmentSkin.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "Node.h"
#include "TransformData.h"
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttachmentSkin, AttachmentAllocator)

    AttachmentSkin::AttachmentSkin(ActorInstance* attachToActorInstance, ActorInstance* attachment)
        : Attachment(attachToActorInstance, attachment)
    {
        InitJointMap();
        InitMorphMap();
    }

    AttachmentSkin::~AttachmentSkin()
    {
    }

    AttachmentSkin* AttachmentSkin::Create(ActorInstance* attachToActorInstance, ActorInstance* attachment)
    {
        return aznew AttachmentSkin(attachToActorInstance, attachment);
    }

    void AttachmentSkin::InitMorphMap()
    {
        m_morphMap.clear();
        if (!m_attachment || !m_actorInstance)
        {
            return;
        }

        // Get the morph setups from the first LOD (highest detail level).
        const MorphSetup* sourceMorphSetup = m_actorInstance->GetActor()->GetMorphSetup(0);
        const MorphSetup* targetMorphSetup = m_attachment->GetActor()->GetMorphSetup(0);
        if (!sourceMorphSetup || !targetMorphSetup)
        {
            return;
        }

        // Iterate over the morph targets inside the attachment, and try to locate them inside the actor instance we are attaching to.
        const size_t numTargetMorphs = targetMorphSetup->GetNumMorphTargets();
        m_morphMap.reserve(numTargetMorphs);
        for (size_t i = 0; i < numTargetMorphs; ++i)
        {
            const size_t sourceMorphIndex = sourceMorphSetup->FindMorphTargetNumberByID(targetMorphSetup->GetMorphTarget(i)->GetID());
            if (sourceMorphIndex == InvalidIndex)
            {
                continue;
            }

            MorphMapping mapping;
            mapping.m_sourceMorphIndex = sourceMorphIndex;
            mapping.m_targetMorphIndex = i;
            m_morphMap.emplace_back(mapping);
        }
        m_morphMap.shrink_to_fit();
    }

    void AttachmentSkin::InitJointMap()
    {
        m_jointMap.clear();
        if (!m_attachment || !m_actorInstance)
        {
            return;
        }

        Skeleton* attachmentSkeleton = m_attachment->GetActor()->GetSkeleton();
        Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();

        const size_t numNodes = attachmentSkeleton->GetNumNodes();
        m_jointMap.reserve(numNodes);
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* attachmentNode = attachmentSkeleton->GetNode(i);

            // Try to find the same joint in the actor where we attach to.
            Node* sourceNode = skeleton->FindNodeByID(attachmentNode->GetID());
            if (sourceNode)
            {
                JointMapping mapping;
                mapping.m_sourceJoint = sourceNode->GetNodeIndex();
                mapping.m_targetJoint = i;
                m_jointMap.emplace_back(mapping);
            }
        }
        m_jointMap.shrink_to_fit();
    }

    void AttachmentSkin::Update()
    {
        if (!m_attachment)
        {
            return;
        }

        // Pass the parent's world space transform into the attachment.
        const Transform worldTransform = m_actorInstance->GetWorldSpaceTransform();
        m_attachment->SetParentWorldSpaceTransform(worldTransform);
    }

    void AttachmentSkin::UpdateJointTransforms(Pose& outPose)
    {
        if (!m_attachment || !m_actorInstance)
        {
            return;
        }

        const Pose* actorInstancePose = m_actorInstance->GetTransformData()->GetCurrentPose();
        for (const JointMapping& mapping : m_jointMap)
        {
            outPose.SetModelSpaceTransform(mapping.m_targetJoint, actorInstancePose->GetModelSpaceTransform(mapping.m_sourceJoint));
        }

        // Update the morph target weights.
        for (const MorphMapping& mapping : m_morphMap)
        {
            const float morphWeight = actorInstancePose->GetMorphWeight(mapping.m_sourceMorphIndex);
            outPose.SetMorphWeight(mapping.m_targetMorphIndex, morphWeight);
        }
    }
} // namespace EMotionFX
