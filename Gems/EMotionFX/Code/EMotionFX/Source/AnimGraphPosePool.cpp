/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AnimGraphPosePool.h"
#include "AnimGraphPose.h"


namespace EMotionFX
{
    // constructor
    AnimGraphPosePool::AnimGraphPosePool()
    {
        mPoses.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL);
        mFreePoses.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL);
        mPoses.Reserve(12);
        mFreePoses.Reserve(12);
        Resize(8);
        mMaxUsed = 0;
    }


    // destructor
    AnimGraphPosePool::~AnimGraphPosePool()
    {
        // delete all poses
        const uint32 numPoses = mPoses.GetLength();
        for (uint32 i = 0; i < numPoses; ++i)
        {
            delete mPoses[i];
        }
        mPoses.Clear();

        // clear the free array
        mFreePoses.Clear();
    }


    // resize the number of poses in the pool
    void AnimGraphPosePool::Resize(uint32 numPoses)
    {
        const uint32 numOldPoses = mPoses.GetLength();

        // if we will remove poses
        int32 difference = numPoses - numOldPoses;
        if (difference < 0)
        {
            // remove the last poses
            difference = abs(difference);
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphPose* pose = mPoses[mFreePoses.GetLength() - 1];
                MCORE_ASSERT(mFreePoses.Contains(pose)); // make sure the pose is not already in use
                delete pose;
                mPoses.Remove(mFreePoses.GetLength() - 1);
            }
        }
        else // we want to add new poses
        {
            for (int32 i = 0; i < difference; ++i)
            {
                AnimGraphPose* newPose = new AnimGraphPose();
                mPoses.Add(newPose);
                mFreePoses.Add(newPose);
            }
        }
    }


    // request a pose
    AnimGraphPose* AnimGraphPosePool::RequestPose(const ActorInstance* actorInstance)
    {
        // if we have no free poses left, allocate a new one
        if (mFreePoses.GetLength() == 0)
        {
            AnimGraphPose* newPose = new AnimGraphPose();
            newPose->LinkToActorInstance(actorInstance);
            mPoses.Add(newPose);
            mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedPoses());
            newPose->SetIsInUse(true);
            return newPose;
        }

        // request the last free pose
        AnimGraphPose* pose = mFreePoses[mFreePoses.GetLength() - 1];
        //if (pose->GetActorInstance() != actorInstance)
        pose->LinkToActorInstance(actorInstance);
        mFreePoses.RemoveLast(); // remove it from the list of free poses
        mMaxUsed = MCore::Max<uint32>(mMaxUsed, GetNumUsedPoses());
        pose->SetIsInUse(true);
        return pose;
    }


    // free the pose again
    void AnimGraphPosePool::FreePose(AnimGraphPose* pose)
    {
        //MCORE_ASSERT( mPoses.Contains(pose) );
        mFreePoses.Add(pose);
        pose->SetIsInUse(false);
    }


    // free all poses
    void AnimGraphPosePool::FreeAllPoses()
    {
        const uint32 numPoses = mPoses.GetLength();
        for (uint32 i = 0; i < numPoses; ++i)
        {
            AnimGraphPose* curPose = mPoses[i];
            if (curPose->GetIsInUse())
            {
                FreePose(curPose);
            }
        }
    }
}   // namespace EMotionFX
