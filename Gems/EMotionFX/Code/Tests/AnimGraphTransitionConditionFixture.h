/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SystemComponentFixture.h"
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphInstance;
    class AnimGraphMotionNode;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphObject;
    class MotionSet;

    class AnimGraphTransitionConditionFixture
        : public SystemComponentFixture
    {
    public:
        void SetUp() override;

        void TearDown() override;

        virtual void AddNodesToAnimGraph()
        {
        }

        TwoMotionNodeAnimGraph* GetAnimGraph() const
        {
            return m_animGraph.get();
        }

        AnimGraphInstance* GetAnimGraphInstance() const
        {
            return m_animGraphInstance;
        }

    protected:
        AnimGraphStateMachine* m_stateMachine = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        AnimGraphMotionNode* m_motionNodeA = nullptr;
        AnimGraphMotionNode* m_motionNodeB = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
        AZStd::unique_ptr<Actor> m_actor;
        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_animGraph;
        MotionSet* m_motionSet = nullptr;
        ActorInstance* m_actorInstance = nullptr;
    };
}
