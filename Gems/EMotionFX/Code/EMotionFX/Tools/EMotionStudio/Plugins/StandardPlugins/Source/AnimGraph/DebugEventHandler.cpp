/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

