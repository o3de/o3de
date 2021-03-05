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
#include "DebugEventHandler.h"
#include <MCore/Source/LogManager.h>


namespace EMStudio
{
    // constructor
    AnimGraphInstanceDebugEventHandler::AnimGraphInstanceDebugEventHandler()
    {
    }


    // destructor
    AnimGraphInstanceDebugEventHandler::~AnimGraphInstanceDebugEventHandler()
    {
    }


    void AnimGraphInstanceDebugEventHandler::OnStateEnter(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
    {
        MCORE_UNUSED(animGraphInstance);
        MCore::LogError("Entered '%s'", state->GetName());
    }


    void AnimGraphInstanceDebugEventHandler::OnStateEntering(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
    {
        MCORE_UNUSED(animGraphInstance);
        if (state == nullptr)
        {
            return;
        }

        MCore::LogError("Entering '%s'", state->GetName());
    }


    void AnimGraphInstanceDebugEventHandler::OnStateExit(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
    {
        MCORE_UNUSED(animGraphInstance);
        if (state == nullptr)
        {
            return;
        }

        MCore::LogError("Exit '%s'", state->GetName());
    }


    void AnimGraphInstanceDebugEventHandler::OnStateEnd(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)
    {
        MCORE_UNUSED(animGraphInstance);
        if (state == nullptr)
        {
            return;
        }

        MCore::LogError("End '%s'", state->GetName());
    }


    void AnimGraphInstanceDebugEventHandler::OnStartTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition)
    {
        if (transition == nullptr)
        {
            return;
        }

        MCore::LogError("Start transition from '%s' to '%s'", transition->GetSourceNode(animGraphInstance)->GetName(), transition->GetTargetNode()->GetName());
    }


    void AnimGraphInstanceDebugEventHandler::OnEndTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition)
    {
        if (transition == nullptr)
        {
            return;
        }

        MCore::LogError("End transition from '%s' to '%s'", transition->GetSourceNode(animGraphInstance)->GetName(), transition->GetTargetNode()->GetName());
    }
} // namespace EMStudio

