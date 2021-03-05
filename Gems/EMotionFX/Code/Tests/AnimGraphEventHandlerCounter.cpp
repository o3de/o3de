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

#include <Tests/AnimGraphEventHandlerCounter.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphEventHandlerCounter, AnimGraphEventHandlerAllocator, 0)

    const AZStd::vector<EventTypes> AnimGraphEventHandlerCounter::GetHandledEventTypes() const
    {
        return
        {
            EVENT_TYPE_ON_STATE_ENTER,
            EVENT_TYPE_ON_STATE_ENTERING,
            EVENT_TYPE_ON_STATE_EXIT,
            EVENT_TYPE_ON_STATE_END,
            EVENT_TYPE_ON_START_TRANSITION,
            EVENT_TYPE_ON_END_TRANSITION
        };
    }

    void AnimGraphEventHandlerCounter::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        AZ_UNUSED(animGraphInstance);
        if (!state)
        {
            return;
        }
        m_numStatesEntering++;
    }

    void AnimGraphEventHandlerCounter::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        AZ_UNUSED(animGraphInstance);
        if (!state)
        {
            return;
        }
        m_numStatesEntered++;
    }

    void AnimGraphEventHandlerCounter::OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        AZ_UNUSED(animGraphInstance);
        if (!state)
        {
            return;
        }
        m_numStatesExited++;
    }

    void AnimGraphEventHandlerCounter::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        AZ_UNUSED(animGraphInstance);
        if (!state)
        {
            return;
        }
        m_numStatesEnded++;
    }

    void AnimGraphEventHandlerCounter::OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        AZ_UNUSED(animGraphInstance);
        if (!transition)
        {
            return;
        }
        m_numTransitionsStarted++;
    }

    void AnimGraphEventHandlerCounter::OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        AZ_UNUSED(animGraphInstance);
        if (!transition)
        {
            return;
        }
        m_numTransitionsEnded++;
    }
} // EMotionFX