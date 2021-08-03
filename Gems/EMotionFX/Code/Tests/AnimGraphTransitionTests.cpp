
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphTransitionFixture.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>

namespace EMotionFX
{
    class AnimGraphTransitionFixtureParams
        : public AnimGraphTransitionFixture
        , public ::testing::WithParamInterface<decltype(&AnimGraphNodeData::GetLocalWeight)>
    {
    };

    TEST_P(AnimGraphTransitionFixtureParams, TestAnimGraphTransitionWeight)
    {
        static constexpr float fps = 60.0f;
        static constexpr float updateInterval = 1.0f / fps;
        static constexpr int numIterations = static_cast<int>(3.0f * fps);

        // Run the EMotionFX update loop for 3 seconds at 60 fps
        for (int i = 0; i < numIterations; ++i)
        {
            GetEMotionFX().Update(updateInterval);

            const AZStd::vector<AnimGraphNode*>& activeStates = m_stateMachine->GetActiveStates(m_animGraphInstance);

            float weight = 0.0f;
            for (const AnimGraphNode* activeState : activeStates)
            {
                for (int j = 0; j < 2; ++j)
                {
                    AnimGraphMotionNode* motionNode = (j == 0) ? m_motionNodeA : m_motionNodeB;
                    // Check if the node is active
                    if (motionNode == activeState)
                    {
                        AnimGraphNodeData* uniqueData = motionNode->FindOrCreateUniqueNodeData(m_animGraphInstance);
                        weight += (uniqueData->*(GetParam()))();
                    }
                }
            }
            // The combined weights for the active nodes should be close to 1
            EXPECT_FLOAT_EQ(weight, 1.0f) << "frame " << i;
        }
    }

    INSTANTIATE_TEST_CASE_P(TestAnimGraphTransitionWeights, AnimGraphTransitionFixtureParams,
        ::testing::Values(
            &AnimGraphNodeData::GetLocalWeight,
            &AnimGraphNodeData::GetGlobalWeight
            )
        );
} // end namespace EMotionFX
