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
        m_poses.reserve(12);
        m_freePoses.reserve(12);
        Resize(8);
        m_maxUsed = 0;
    }


    // destructor
    AnimGraphPosePool::~AnimGraphPosePool()
    {
        // delete all poses
        for (AnimGraphPose* pose : m_poses)
        {
            delete pose;
        }
        m_poses.clear();

        // clear the free array
        m_freePoses.clear();
    }


    // resize the number of poses in the pool
    void AnimGraphPosePool::Resize(size_t numPoses)
    {
        const size_t numOldPoses = m_poses.size();

        // if we will remove poses
        if (numPoses < numOldPoses)
        {
            // remove the last poses
            const size_t numToRemove = numOldPoses - numPoses;
            for (size_t i = 0; i < numToRemove; ++i)
            {
                AnimGraphPose* pose = m_poses.back();
                MCORE_ASSERT(AZStd::find(begin(m_freePoses), end(m_freePoses), pose) == end(m_freePoses)); // make sure the pose is not already in use
                delete pose;
                m_poses.erase(m_freePoses.end() - 1);
            }
        }
        else // we want to add new poses
        {
            const size_t numToAdd = numPoses - numOldPoses;
            for (size_t i = 0; i < numToAdd; ++i)
            {
                AnimGraphPose* newPose = new AnimGraphPose();
                m_poses.emplace_back(newPose);
                m_freePoses.emplace_back(newPose);
            }
        }
    }


    // request a pose
    AnimGraphPose* AnimGraphPosePool::RequestPose(const ActorInstance* actorInstance)
    {
        // if we have no free poses left, allocate a new one
        if (m_freePoses.empty())
        {
            AnimGraphPose* newPose = new AnimGraphPose();
            newPose->LinkToActorInstance(actorInstance);
            m_poses.emplace_back(newPose);
            m_maxUsed = AZStd::max(m_maxUsed, GetNumUsedPoses());
            newPose->SetIsInUse(true);
            return newPose;
        }

        // request the last free pose
        AnimGraphPose* pose = m_freePoses[m_freePoses.size() - 1];
        //if (pose->GetActorInstance() != actorInstance)
        pose->LinkToActorInstance(actorInstance);
        m_freePoses.pop_back(); // remove it from the list of free poses
        m_maxUsed = AZStd::max(m_maxUsed, GetNumUsedPoses());
        pose->SetIsInUse(true);
        return pose;
    }


    // free the pose again
    void AnimGraphPosePool::FreePose(AnimGraphPose* pose)
    {
        m_freePoses.emplace_back(pose);
        pose->SetIsInUse(false);
    }


    // free all poses
    void AnimGraphPosePool::FreeAllPoses()
    {
        for (AnimGraphPose* curPose : m_poses)
        {
            if (curPose->GetIsInUse())
            {
                FreePose(curPose);
            }
        }
    }
}   // namespace EMotionFX
