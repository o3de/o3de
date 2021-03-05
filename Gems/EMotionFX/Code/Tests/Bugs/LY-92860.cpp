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

#include <Tests/UI/CommandRunnerFixture.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>


namespace EMotionFX
{
    std::vector<std::string> prepareLY92860Commands
    {
        // Create a blend tree with one parameter and three smoothing nodes in it.
        R"str(CreateAnimGraph)str",
        R"str(Unselect -animGraphIndex SELECT_ALL)str",
        R"str(Select -animGraphID 0)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2} -parentName Root -xPos 411 -yPos 238 -name GENERATE -namePrefix BlendTree)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {1A755218-AD9D-48EA-86FC-D571C11ECA4D} -parentName BlendTree0 -xPos 0 -yPos 0 -name GENERATE -namePrefix FinalNode)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {4510529A-323F-40F6-B773-9FA8FC4DE53D} -parentName BlendTree0 -xPos -534 -yPos -15 -name GENERATE -namePrefix Parameters)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {80D8C793-3CD4-4216-B804-CC00EAD20FAA} -parentName BlendTree0 -xPos -230 -yPos -121 -name GENERATE -namePrefix Smoothing)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {80D8C793-3CD4-4216-B804-CC00EAD20FAA} -parentName BlendTree0 -xPos -150 -yPos 12 -name GENERATE -namePrefix Smoothing)str",
        R"str(AnimGraphCreateNode -animGraphID 0 -type {80D8C793-3CD4-4216-B804-CC00EAD20FAA} -parentName BlendTree0 -xPos -171 -yPos 157 -name GENERATE -namePrefix Smoothing)str",
        R"str(AnimGraphAdjustNode -animGraphID 0 -name Smoothing1 -xPos -229 -yPos 10 -updateAttributes false)str",
        R"str(AnimGraphAdjustNode -animGraphID 0 -name Smoothing2 -xPos -229 -yPos 120 -updateAttributes false)str",
        // Create three float parameters.
        R"str(AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name Parameter0 -contents <ObjectStream version = "3">
            <Class name = "FloatSliderParameter" version = "1" type = "{2ED6BBAF-5C82-4EAA-8678-B220667254F2}">
            <Class name = "FloatParameter" field = "BaseClass1" version = "1" type = "{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}">
            <Class name = "(RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt;" field = "BaseClass1" version = "1" type = "{01CABBF8-9500-5ABB-96BD-9989198146C2}">
            <Class name = "(DefaultValueParameter&lt;ValueType, Derived&gt;)&lt;float (RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt; &gt;" field = "BaseClass1" version = "1" type = "{3221F118-9372-5BA3-BD8B-E88267CB356B}">
            <Class name = "ValueParameter" field = "BaseClass1" version = "1" type = "{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}">
            <Class name = "Parameter" field = "BaseClass1" version = "1" type = "{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
            <Class name = "AZStd::string" field = "name" value = "Parameter0" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            <Class name = "AZStd::string" field = "description" value = "" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            </Class>
            </Class>
            <Class name = "float" field = "defaultValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            <Class name = "bool" field = "hasMinValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "minValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name = "bool" field = "hasMaxValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "maxValue" value = "1.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            </Class>
            </Class>
            </ObjectStream>)str",
        R"str(AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name Parameter1 -contents <ObjectStream version = "3">
            <Class name = "FloatSliderParameter" version = "1" type = "{2ED6BBAF-5C82-4EAA-8678-B220667254F2}">
            <Class name = "FloatParameter" field = "BaseClass1" version = "1" type = "{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}">
            <Class name = "(RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt;" field = "BaseClass1" version = "1" type = "{01CABBF8-9500-5ABB-96BD-9989198146C2}">
            <Class name = "(DefaultValueParameter&lt;ValueType, Derived&gt;)&lt;float (RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt; &gt;" field = "BaseClass1" version = "1" type = "{3221F118-9372-5BA3-BD8B-E88267CB356B}">
            <Class name = "ValueParameter" field = "BaseClass1" version = "1" type = "{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}">
            <Class name = "Parameter" field = "BaseClass1" version = "1" type = "{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
            <Class name = "AZStd::string" field = "name" value = "Parameter1" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            <Class name = "AZStd::string" field = "description" value = "" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            </Class>
            </Class>
            <Class name = "float" field = "defaultValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            <Class name = "bool" field = "hasMinValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "minValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name = "bool" field = "hasMaxValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "maxValue" value = "1.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            </Class>
            </Class>
            </ObjectStream>)str",
        R"str(AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name Parameter2 -contents <ObjectStream version = "3">
            <Class name = "FloatSliderParameter" version = "1" type = "{2ED6BBAF-5C82-4EAA-8678-B220667254F2}">
            <Class name = "FloatParameter" field = "BaseClass1" version = "1" type = "{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}">
            <Class name = "(RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt;" field = "BaseClass1" version = "1" type = "{01CABBF8-9500-5ABB-96BD-9989198146C2}">
            <Class name = "(DefaultValueParameter&lt;ValueType, Derived&gt;)&lt;float (RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt; &gt;" field = "BaseClass1" version = "1" type = "{3221F118-9372-5BA3-BD8B-E88267CB356B}">
            <Class name = "ValueParameter" field = "BaseClass1" version = "1" type = "{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}">
            <Class name = "Parameter" field = "BaseClass1" version = "1" type = "{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
            <Class name = "AZStd::string" field = "name" value = "Parameter2" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            <Class name = "AZStd::string" field = "description" value = "" type = "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            </Class>
            </Class>
            <Class name = "float" field = "defaultValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            <Class name = "bool" field = "hasMinValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "minValue" value = "0.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            <Class name = "bool" field = "hasMaxValue" value = "true" type = "{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name = "float" field = "maxValue" value = "1.0000000" type = "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            </Class>
            </Class>
            </ObjectStream>)str",
        // Connect the parameter node output ports with the smoothing node input ports.
        R"str(AnimGraphCreateConnection -animGraphID 0 -sourceNode Parameters0 -targetNode Smoothing0 -sourcePort 0 -targetPort 0 -startOffsetX 119 -startOffsetY 42 -endOffsetX -2 -endOffsetY 38)str",
        R"str(AnimGraphCreateConnection -animGraphID 0 -sourceNode Parameters0 -targetNode Smoothing1 -sourcePort 1 -targetPort 0 -startOffsetX 121 -startOffsetY 55 -endOffsetX -3 -endOffsetY 40)str",
        R"str(AnimGraphCreateConnection -animGraphID 0 -sourceNode Parameters0 -targetNode Smoothing2 -sourcePort 2 -targetPort 0 -startOffsetX 119 -startOffsetY 70 -endOffsetX -2 -endOffsetY 40)str"
    };

    class LY92860Fixture
        : public CommandRunnerFixture
    {
    public:
        BlendTreeParameterNode* GetSingleParameterNode()
        {
            AnimGraphManager* animGraphManager = GetEMotionFX().GetAnimGraphManager();
            const size_t numAnimGraphs = animGraphManager->GetNumAnimGraphs();
            if (numAnimGraphs == 1)
            {
                AnimGraph* animGraph = animGraphManager->GetAnimGraph(0);

                AZStd::vector<AnimGraphObject*> parameterNodes;
                animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<BlendTreeParameterNode>(), parameterNodes);

                if (parameterNodes.size() == 1)
                {
                    return static_cast<BlendTreeParameterNode*>(parameterNodes[0]);
                }
            }

            return nullptr;
        }

        void Run()
        {
            ExecuteCommands(GetParam());

            BlendTreeParameterNode* parameterNode = GetSingleParameterNode();
            ASSERT_TRUE(parameterNode) << "Expected exactly one parameter node.";

            // Pre-clear checks
            {
                ASSERT_EQ(parameterNode->GetParameters().size(), 0) << "Expected empty parameter mask as we did not adjust it.";

                const AZStd::vector<AnimGraphNode::Port>& outputPorts = parameterNode->GetOutputPorts();
                ASSERT_EQ(outputPorts.size(), 3) << "Expected 3 output ports, one for each smoothing node that got created.";

                for (const AnimGraphNode::Port& outputPort : outputPorts)
                {
                    ASSERT_TRUE(outputPort.mConnection) << "Expected a valid connection at the output port.";
                }
            }

            // Clear the three parameters the same way the UI does it to make sure the exact same command group gets created and we can undo it with a single call.
            // This should also automatically remove the connections to the smoothing nodes.
            EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
            EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
            ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin is not available.";
            animGraphPlugin->GetParameterWindow()->ClearParameters(/*askBefore*/false);

            // Post-clear checks
            {
                ASSERT_EQ(parameterNode->GetParameters().size(), 0) << "Expected empty parameter mask as we did not adjust it.";

                const AZStd::vector<AnimGraphNode::Port>& outputPorts = parameterNode->GetOutputPorts();
                ASSERT_EQ(outputPorts.size(), 0) << "Expected no output ports as we cleared all parameters.";
            }

            AZStd::string undoResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(undoResult)) << "Undo: " << undoResult.c_str();

            // Post-undo checks
            {
                ASSERT_EQ(parameterNode->GetParameters().size(), 0) << "Expected empty parameter mask as we did not adjust it.";

                const AZStd::vector<AnimGraphNode::Port>& outputPorts = parameterNode->GetOutputPorts();
                ASSERT_EQ(outputPorts.size(), 3) << "Expected 3 output ports, as the undo call should have recreated all of them.";

                for (const AnimGraphNode::Port& outputPort : outputPorts)
                {
                    ASSERT_TRUE(outputPort.mConnection) << "Expected a valid connection at the output port.";
                }
            }
        }
    };

    TEST_P(LY92860Fixture, ExecuteCommands)
    {
        Run();
    };

    INSTANTIATE_TEST_CASE_P(LY92860, LY92860Fixture, ::testing::Values(prepareLY92860Commands));
} // EMotionFX
