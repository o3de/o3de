/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EventHandler.h>


namespace EMotionFX
{
    class AnimGraphEventHandlerCounter
        : public AnimGraphInstanceEventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~AnimGraphEventHandlerCounter() = default;

        const AZStd::vector<EventTypes> GetHandledEventTypes() const override;

        void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
        void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;

        void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
        void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;

        void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition) override;
        void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition) override;

        int m_numStatesEntering = 0;
        int m_numStatesEntered = 0;
        int m_numStatesExited = 0;
        int m_numStatesEnded = 0;
        int m_numTransitionsStarted = 0;
        int m_numTransitionsEnded = 0;
    };
}
