/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mPoses.reserve(12);
        mFreePoses.reserve(12);
        Resize(8);
        mMaxUsed = 0;
    }


    // destructor
    AnimGraphPosePool::~AnimGraphPosePool()
    {
        // delete all poses
        for (AnimGraphPose* pose : mPoses)
        {
            delete pose;
        }
        mPoses.clear();

        // clear the free array
        mFreePoses.clear();
    }


    // resize the number of poses in the pool
    void AnimGraphPosePool::Resize(size_t numPoses)
    {
        const size_t numOldPoses = mPoses.size();

        // if we will remove poses
        if (numPoses < numOldPoses)
        {
            // remove the last poses
            const size_t numToRemove = numOldPoses - numPoses;
            for (size_t i = 0; i < numToRemove; ++i)
            {
                AnimGraphPose* pose = mPoses.back();
                MCORE_ASSERT(AZStd::find(begin(mFreePoses), end(mFreePoses), pose) == end(mFreePoses)); // make sure the pose is not already in use
                delete pose;
                mPoses.erase(mFreePoses.end() - 1);
            }
        }
        else // we want to add new poses
        {
            const size_t numToAdd = numPoses - numOldPoses;
            for (size_t i = 0; i < numToAdd; ++i)
            {
                AnimGraphPose* newPose = new AnimGraphPose();
                mPoses.emplace_back(newPose);
                mFreePoses.emplace_back(newPose);
            }
        }
    }


    // request a pose
    AnimGraphPose* AnimGraphPosePool::RequestPose(const ActorInstance* actorInstance)
    {
        // if we have no free poses left, allocate a new one
        if (mFreePoses.empty())
        {
            AnimGraphPose* newPose = new AnimGraphPose();
            newPose->LinkToActorInstance(actorInstance);
            mPoses.emplace_back(newPose);
            mMaxUsed = AZStd::max(mMaxUsed, GetNumUsedPoses());
            newPose->SetIsInUse(true);
            return newPose;
        }

        // request the last free pose
        AnimGraphPose* pose = mFreePoses[mFreePoses.size() - 1];
        //if (pose->GetActorInstance() != actorInstance)
        pose->LinkToActorInstance(actorInstance);
        mFreePoses.pop_back(); // remove it from the list of free poses
        mMaxUsed = AZStd::max(mMaxUsed, GetNumUsedPoses());
        pose->SetIsInUse(true);
        return pose;
    }


    // free the pose again
    void AnimGraphPosePool::FreePose(AnimGraphPose* pose)
    {
        //MCORE_ASSERT( mPoses.Contains(pose) );
        mFreePoses.emplace_back(pose);
        pose->SetIsInUse(false);
    }


    // free all poses
    void AnimGraphPosePool::FreeAllPoses()
    {
        for (AnimGraphPose* curPose : mPoses)
        {
            if (curPose->GetIsInUse())
            {
                FreePose(curPose);
            }
        }
    }
}   // namespace EMotionFX
