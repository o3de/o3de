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

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    class LY92748Fixture
        : public CommandRunnerFixture
    {
    };

    TEST_P(LY92748Fixture, ExecuteCommands)
    {
        ExecuteCommands(GetParam());

        const AZStd::vector<AZStd::string>& results = GetResults();

        const AnimGraphConnectionId connectionId = AnimGraphConnectionId::CreateFromString(results.back());

        // Pre checks
        const AnimGraph* animGraph = GetAnimGraphManager().FindAnimGraphByID(0);
        ASSERT_TRUE(animGraph) << "Anim Graph not created";
        AnimGraphNode* motionNode0 = animGraph->GetRootStateMachine()->FindChildNode("Motion0");
        ASSERT_TRUE(motionNode0) << "Motion0 node not created";
        AnimGraphNode* motionNode1 = animGraph->GetRootStateMachine()->FindChildNode("Motion1");
        ASSERT_TRUE(motionNode1) << "Motion1 node not created";
        AnimGraphStateTransition* connection = animGraph->RecursiveFindTransitionById(connectionId);
        ASSERT_TRUE(connection) << "Connection between motion nodes not created";

        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim Graph plugin did not load";
        EMStudio::AnimGraphModel& model = animGraphPlugin->GetAnimGraphModel();
        ASSERT_TRUE(model.FindFirstModelIndex(motionNode0).isValid()) << "Model index for motionNode0 is invalid";
        ASSERT_TRUE(model.FindFirstModelIndex(motionNode1).isValid()) << "Model index for motionNode1 is invalid";
        ASSERT_TRUE(model.FindFirstModelIndex(connection).isValid()) << "Model index for connection is invalid";

        // Select and delete the nodes and connections
        animGraphPlugin->GetGraphWidget()->GetActiveGraph()->SelectNodesInRect(QRect(-10, -10, 1000, 1000));
        animGraphPlugin->GetActionManager().DeleteSelectedNodes();

        // Post checks
        motionNode0 = animGraph->GetRootStateMachine()->FindChildNode("Motion0");
        ASSERT_FALSE(motionNode0) << "Motion0 node not deleted";
        motionNode1 = animGraph->GetRootStateMachine()->FindChildNode("Motion1");
        ASSERT_FALSE(motionNode1) << "Motion1 node not deleted";
        connection = animGraph->RecursiveFindTransitionById(connectionId);
        ASSERT_FALSE(connection) << "Connection between motion nodes not deleted";
        ASSERT_FALSE(model.FindFirstModelIndex(motionNode0).isValid()) << "Model index for motionNode0 is still valid";
        ASSERT_FALSE(model.FindFirstModelIndex(motionNode1).isValid()) << "Model index for motionNode1 is still valid";
        ASSERT_FALSE(model.FindFirstModelIndex(connection).isValid()) << "Model index for connection is still valid";

    }

    INSTANTIATE_TEST_CASE_P(DISABLED_LY92748, LY92748Fixture,
        ::testing::Values(std::vector<std::string> {
            R"str(CreateAnimGraph)str",
            R"str(Select -animGraphID 0)str",
            R"str(AnimGraphCreateNode -animGraphID 0 -type {B8B8AAE6-E532-4BF8-898F-3D40AA41BC82} -parentName Root -xPos 0 -yPos 0 -name Motion0)str",
            R"str(AnimGraphCreateNode -animGraphID 0 -type {B8B8AAE6-E532-4BF8-898F-3D40AA41BC82} -parentName Root -xPos 50 -yPos 0 -name Motion1)str",
            R"str(AnimGraphCreateConnection -animGraphID 0 -sourceNode Motion0 -targetNode Motion1 -sourcePort 0 -targetPort 0 -startOffsetX 98 -startOffsetY 17 -endOffsetX 4 -endOffsetY 17 -transitionType {E69C8C6E-7066-43DD-B1BF-0D2FFBDDF457})str",
            // Don't add more commands here, the result of the CreateConnection command is used to find a pointer to that connection after the commands have been executed
        }
    ));

} // end namespace EMotionFX
