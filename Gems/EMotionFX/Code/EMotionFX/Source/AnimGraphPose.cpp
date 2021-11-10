/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AnimGraphPose.h"
#include "ActorInstance.h"
#include "Actor.h"
#include "Pose.h"


namespace EMotionFX
{
    // constructor
    AnimGraphPose::AnimGraphPose()
    {
        m_flags = 0;
    }


    // copy constructor
    AnimGraphPose::AnimGraphPose(const AnimGraphPose& other)
    {
        m_flags = 0;
        m_pose.InitFromPose(&other.m_pose);
    }


    // destructor
    AnimGraphPose::~AnimGraphPose()
    {
    }


    // = operator
    AnimGraphPose& AnimGraphPose::operator=(const AnimGraphPose& other)
    {
        m_pose.InitFromPose(&other.m_pose);
        return *this;
    }


    // initialize
    void AnimGraphPose::LinkToActorInstance(const ActorInstance* actorInstance)
    {
        // resize the transformation buffer, which contains the local space transformations
        m_pose.LinkToActorInstance(actorInstance);
    }


    // init to bind pose
    void AnimGraphPose::InitFromBindPose(const ActorInstance* actorInstance)
    {
        // init for the actor instance
        LinkToActorInstance(actorInstance);

        // fill the local pose with the bind pose
        m_pose.InitFromBindPose(actorInstance);
    }

}   // namespace EMotionFX
