/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(TransformData, TransformDataAllocator, 0)

    // default constructor
    TransformData::TransformData()
        : BaseObject()
    {
        mSkinningMatrices   = nullptr;
        mBindPose           = nullptr;
        mNumTransforms      = 0;
        mHasUniqueBindPose  = false;
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
        MCore::AlignedFree(mSkinningMatrices);

        if (mHasUniqueBindPose)
        {
            delete mBindPose;
        }

        mPose.Clear();
        mSkinningMatrices = nullptr;
        mBindPose       = nullptr;
        mNumTransforms  = 0;
    }


    // initialize from a given actor
    void TransformData::InitForActorInstance(ActorInstance* actorInstance)
    {
        Release();

        // link to the given actor instance
        mPose.LinkToActorInstance(actorInstance);

        // release all memory if we want to resize to zero nodes
        const uint32 numNodes = actorInstance->GetNumNodes();
        if (numNodes == 0)
        {
            Release();
            return;
        }

        mSkinningMatrices       = (AZ::Matrix3x4*)MCore::AlignedAllocate(sizeof(AZ::Matrix3x4) * numNodes, static_cast<uint16>(AZStd::alignment_of<AZ::Matrix3x4>()), EMFX_MEMCATEGORY_TRANSFORMDATA);
        mNumTransforms          = numNodes;

        if (mHasUniqueBindPose)
        {
            mBindPose = new Pose();
            mBindPose->LinkToActorInstance(actorInstance);
        }
        else
        {
            mBindPose = actorInstance->GetActor()->GetBindPose();
        }

        // now initialize the data with the actor transforms
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mSkinningMatrices[i] = AZ::Matrix3x4::CreateIdentity();
        }
    }


    // make the bind pose transforms unique
    void TransformData::MakeBindPoseTransformsUnique()
    {
        if (mHasUniqueBindPose)
        {
            return;
        }

        const ActorInstance* actorInstance = mPose.GetActorInstance();
        mHasUniqueBindPose = true;
        mBindPose = new Pose();
        mBindPose->LinkToActorInstance(actorInstance);
        *mBindPose = *actorInstance->GetActor()->GetBindPose();
    }


    EMFX_SCALECODE
    (
        // set the scaling value for the node and all child nodes
        void TransformData::SetBindPoseLocalScaleInherit(uint32 nodeIndex, const AZ::Vector3& scale)
        {
            const ActorInstance*  actorInstance   = mPose.GetActorInstance();
            const Actor*          actor           = actorInstance->GetActor();

            // get the node index and the number of children of the given node
            const Node* node        = actor->GetSkeleton()->GetNode(nodeIndex);
            const uint32 numChilds  = node->GetNumChildNodes();

            // set the new scale for the given node
            SetBindPoseLocalScale(nodeIndex, scale);

            // iterate through the children and set their scale recursively
            for (uint32 i = 0; i < numChilds; ++i)
            {
                SetBindPoseLocalScaleInherit(node->GetChildIndex(i), scale);
            }
        }

        // update the local space scale
        void TransformData::SetBindPoseLocalScale(uint32 nodeIndex, const AZ::Vector3& scale)
        {
            Transform newTransform = mBindPose->GetLocalSpaceTransform(nodeIndex);
            newTransform.mScale = scale;
            mBindPose->SetLocalSpaceTransform(nodeIndex, newTransform);
        }
    ) // EMFX_SCALECODE

    // set the number of morph weights
    void TransformData::SetNumMorphWeights(uint32 numMorphWeights)
    {
        mPose.ResizeNumMorphs(numMorphWeights);
    }
}   // namespace EMotionFX
