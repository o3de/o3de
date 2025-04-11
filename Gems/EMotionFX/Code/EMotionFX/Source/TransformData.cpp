/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "TransformData.h"
#include "ActorInstance.h"
#include "Actor.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>
#include "Node.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformData, TransformDataAllocator)

    // default constructor
    TransformData::TransformData()
        : MCore::RefCounted()
    {
        m_skinningMatrices   = nullptr;
        m_bindPose           = nullptr;
        m_numTransforms      = 0;
        m_hasUniqueBindPose  = false;
    }


    // destructor
    TransformData::~TransformData()
    {
        Release();
    }


    // create
    TransformData* TransformData::Create()
    {
        return aznew TransformData();
    }


    // get rid of all allocated data
    void TransformData::Release()
    {
        MCore::AlignedFree(m_skinningMatrices);

        if (m_hasUniqueBindPose)
        {
            delete m_bindPose;
        }

        m_pose.Clear();
        m_skinningMatrices = nullptr;
        m_bindPose       = nullptr;
        m_numTransforms  = 0;
    }


    // initialize from a given actor
    void TransformData::InitForActorInstance(ActorInstance* actorInstance)
    {
        Release();

        // link to the given actor instance
        m_pose.LinkToActorInstance(actorInstance);

        // release all memory if we want to resize to zero nodes
        const size_t numNodes = actorInstance->GetNumNodes();
        if (numNodes == 0)
        {
            Release();
            return;
        }

        m_skinningMatrices       = (AZ::Matrix3x4*)MCore::AlignedAllocate(sizeof(AZ::Matrix3x4) * numNodes, static_cast<uint16>(AZStd::alignment_of<AZ::Matrix3x4>()), EMFX_MEMCATEGORY_TRANSFORMDATA);
        m_numTransforms          = numNodes;

        if (m_hasUniqueBindPose)
        {
            m_bindPose = new Pose();
            m_bindPose->LinkToActorInstance(actorInstance);
        }
        else
        {
            m_bindPose = actorInstance->GetActor()->GetBindPose();
        }

        // now initialize the data with the actor transforms
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_skinningMatrices[i] = AZ::Matrix3x4::CreateIdentity();
        }
    }


    // make the bind pose transforms unique
    void TransformData::MakeBindPoseTransformsUnique()
    {
        if (m_hasUniqueBindPose)
        {
            return;
        }

        const ActorInstance* actorInstance = m_pose.GetActorInstance();
        m_hasUniqueBindPose = true;
        m_bindPose = new Pose();
        m_bindPose->LinkToActorInstance(actorInstance);
        *m_bindPose = *actorInstance->GetActor()->GetBindPose();
    }


    EMFX_SCALECODE
    (
        // set the scaling value for the node and all child nodes
        void TransformData::SetBindPoseLocalScaleInherit(size_t nodeIndex, const AZ::Vector3& scale)
        {
            const ActorInstance*  actorInstance   = m_pose.GetActorInstance();
            const Actor*          actor           = actorInstance->GetActor();

            // get the node index and the number of children of the given node
            const Node* node        = actor->GetSkeleton()->GetNode(nodeIndex);
            const size_t numChilds  = node->GetNumChildNodes();

            // set the new scale for the given node
            SetBindPoseLocalScale(nodeIndex, scale);

            // iterate through the children and set their scale recursively
            for (size_t i = 0; i < numChilds; ++i)
            {
                SetBindPoseLocalScaleInherit(node->GetChildIndex(i), scale);
            }
        }

        // update the local space scale
        void TransformData::SetBindPoseLocalScale(size_t nodeIndex, const AZ::Vector3& scale)
        {
            Transform newTransform = m_bindPose->GetLocalSpaceTransform(nodeIndex);
            newTransform.m_scale = scale;
            m_bindPose->SetLocalSpaceTransform(nodeIndex, newTransform);
        }
    ) // EMFX_SCALECODE

    // set the number of morph weights
    void TransformData::SetNumMorphWeights(size_t numMorphWeights)
    {
        m_pose.ResizeNumMorphs(numMorphWeights);
    }
}   // namespace EMotionFX
