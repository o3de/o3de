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
#include "AnimGraphPose.h"
#include "ActorInstance.h"
#include "Actor.h"
#include "Pose.h"


namespace EMotionFX
{
    // constructor
    AnimGraphPose::AnimGraphPose()
    {
        mFlags = 0;
    }


    // copy constructor
    AnimGraphPose::AnimGraphPose(const AnimGraphPose& other)
    {
        mFlags = 0;
        mPose.InitFromPose(&other.mPose);
        //mFlags = other.mFlags;
    }


    // destructor
    AnimGraphPose::~AnimGraphPose()
    {
    }


    // = operator
    AnimGraphPose& AnimGraphPose::operator=(const AnimGraphPose& other)
    {
        mPose.InitFromPose(&other.mPose);
        //mFlags = other.mFlags;
        return *this;
    }


    // initialize
    void AnimGraphPose::LinkToActorInstance(const ActorInstance* actorInstance)
    {
        // resize the transformation buffer, which contains the local space transformations
        mPose.LinkToActorInstance(actorInstance);
    }


    // init to bind pose
    void AnimGraphPose::InitFromBindPose(const ActorInstance* actorInstance)
    {
        // init for the actor instance
        LinkToActorInstance(actorInstance);

        // fill the local pose with the bind pose
        mPose.InitFromBindPose(actorInstance);
    }

}   // namespace EMotionFX
