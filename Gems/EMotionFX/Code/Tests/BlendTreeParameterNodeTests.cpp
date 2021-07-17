
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    class BlendTreeParameterNodeTests
        : public CommandRunnerFixture
    {
    public:
        void SetUp() override
        {
            CommandRunnerFixture::SetUp();
            m_animGraph = AnimGraphFactory::Create<OneBlendTreeParameterNodeAnimGraph>();
        }

        void TearDown() override
        {
            m_animGraph.reset();
            CommandRunnerFixture::TearDown();
        }

        OneBlendTreeParameterNodeAnimGraph* GetAnimGraph() const
        {
            return m_animGraph.get();
        }

    private:
        AZStd::unique_ptr < OneBlendTreeParameterNodeAnimGraph> m_animGraph;
    };

    TEST_F(BlendTreeParameterNodeTests, RenameParameter)
    {
        const char* startParameterName = "Parameter0";
        const char* renamedParameterName = "RenamedParameter0";

        BlendTreeParameterNode* parameterNode = GetAnimGraph()->GetParameterNode();

        // Add new parameter to the anim graph and check if a output port got added for the parameter node.
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 0);
        ExecuteCommands({std::string{"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name "} + startParameterName});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), startParameterName);

        // Rename anim graph parameter and check if the output port of the parameter node also got renamed.
        ExecuteCommands({std::string{"AnimGraphAdjustParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name "} + startParameterName + " -newName " + renamedParameterName});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), renamedParameterName);

        // Undo and redo
        ExecuteCommands({"UNDO"});
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), startParameterName);

        ExecuteCommands({"REDO"});
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), renamedParameterName);
    }

    TEST_F(BlendTreeParameterNodeTests, RemoveParameterFromMask)
    {
        RecordProperty("test_case_id", "C20655311");

        using StringVector = AZStd::vector<AZStd::string>;

        BlendTreeParameterNode* parameterNode = GetAnimGraph()->GetParameterNode();

        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 0);
        ExecuteCommands({
            {"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name P0"},
            {"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name P1"},
            {"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name P2"},
        });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 3);
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 0);
        EXPECT_EQ(parameterNode->GetParameterIndex(1), 1);
        EXPECT_EQ(parameterNode->GetParameterIndex(2), 2);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{}));

        parameterNode->SetParameters(StringVector{"P1", "P2"});
        parameterNode->Reinit();
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 2);
        // Port 0 maps to parameter 1, port 1 maps to parameter 2
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 1);
        EXPECT_EQ(parameterNode->GetParameterIndex(1), 2);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{"P1", "P2"}));

        ExecuteCommands({{"AnimGraphRemoveParameter -animGraphID 0 -name P0"}});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 2);
        // All the parameters in the mask shifted, ports and parameter indices line up again
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 0);
        EXPECT_EQ(parameterNode->GetParameterIndex(1), 1);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{"P1", "P2"}));

        ExecuteCommands({{"AnimGraphRemoveParameter -animGraphID 0 -name P1"}});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 0);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{"P2"}));

        ExecuteCommands({{"UNDO"}});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 2);
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 0);
        EXPECT_EQ(parameterNode->GetParameterIndex(1), 1);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{"P1", "P2"}));

        ExecuteCommands({{"UNDO"}});
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 2);
        // Port 0 maps to parameter 1, port 1 maps to parameter 2
        EXPECT_EQ(parameterNode->GetParameterIndex(0), 1);
        EXPECT_EQ(parameterNode->GetParameterIndex(1), 2);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{"P1", "P2"}));
    }

    TEST_F(BlendTreeParameterNodeTests, ParameterMaskExercise0)
    {
        using StringVector = AZStd::vector<AZStd::string>;
        const char* parameterName0 = "Param0";
        const char* parameterName1 = "Param1";
        const char* parameterName2 = "Param2";

        BlendTreeParameterNode* parameterNode = GetAnimGraph()->GetParameterNode();

        // 1. Add two parameters (Param0 and Param1) to the anim graph and check if a output port got added for the parameter node.
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 0);
        ExecuteCommands({ std::string{"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name "} + parameterName0 });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        ExecuteCommands({ std::string{"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name "} + parameterName1 });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 2);

        // 2. Change the parameter mask to contain Param1.
        parameterNode->SetParameters(StringVector{ "Param1" });
        parameterNode->Reinit();
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);

        // 3. Add the 3rd parameter (Param2). The parameter mask should stay the same. 
        ExecuteCommands({ std::string{"AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name "} + parameterName2 });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{ "Param1" }));
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), parameterName1);

        // 4. Undo the 3rd step. The parameter mask should stay the same.
        ExecuteCommands({ "UNDO" });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{ "Param1" }));
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), parameterName1);

        // 5. Undo "Add Param1". Now the mask should be empty, and the output port should contain Param0.
        ExecuteCommands({ "UNDO" });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_TRUE(parameterNode->GetParameters().empty());
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), parameterName0);

        // 6. Redo "Add Param1". Now the mask should restore and contain Param1, and the outport should contain Param1 as well.
        ExecuteCommands({ "REDO" });
        EXPECT_EQ(parameterNode->GetOutputPorts().size(), 1);
        EXPECT_THAT(parameterNode->GetParameters(), ::testing::Pointwise(::testing::Eq(), StringVector{ "Param1" }));
        EXPECT_STREQ(parameterNode->GetOutputPorts()[0].GetName(), parameterName1);
    }
} // namespace EMotionFX
