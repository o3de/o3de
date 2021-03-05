
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

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    class AnimGraphHubNodeFixture_CircularEntryNodeDependency : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
            motionNode->SetName("Motion0");
            m_rootStateMachine->AddChildNode(motionNode);
            m_rootStateMachine->SetEntryState(motionNode);

            AnimGraphStateMachine* stateMachine = aznew AnimGraphStateMachine();
            stateMachine->SetName("StateMachine0");
            m_rootStateMachine->AddChildNode(stateMachine);

            AnimGraphEntryNode* entryNode = aznew AnimGraphEntryNode();
            entryNode->SetName("EntryNode0");
            stateMachine->AddChildNode(entryNode);
            stateMachine->SetEntryState(entryNode);

            AnimGraphHubNode* hubNode = aznew AnimGraphHubNode();
            hubNode->SetName("HubNode0");
            m_rootStateMachine->AddChildNode(hubNode);

            AddTransitionWithTimeCondition(motionNode, stateMachine, 0.3f, 0.5f);
            AddTransitionWithTimeCondition(stateMachine, hubNode, 0.3f, 0.5f);
        }
    };

    TEST_F(AnimGraphHubNodeFixture_CircularEntryNodeDependency, AnimGraphHubNode_CircularEntryNodeDependency)
    {
        // Simulate for two seconds at 10 fps to make sure to reach the hub node.
        for (size_t i = 0; i < 20; ++i)
        {
            GetEMotionFX().Update(1.0f / 10.0f);
        }
    }
} // namespace EMotionFX
