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

#include <AzCore/RTTI/TypeInfo.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    class ParamConditionAnimGraph
        : public EmptyAnimGraph
    {
    public:
        ParamConditionAnimGraph()
        {
            AddParameter(aznew FloatSliderParameter("P0"));
            AddParameter(aznew FloatSliderParameter("P1"));

            auto bindPoseNode1 = aznew EMotionFX::AnimGraphBindPoseNode();
            auto bindPoseNode2 = aznew EMotionFX::AnimGraphBindPoseNode();

            m_parameterCondition = aznew AnimGraphParameterCondition("P1");
            AnimGraphStateTransition* node1ToNode2 = aznew AnimGraphStateTransition(bindPoseNode1, bindPoseNode2, {m_parameterCondition});

            GetRootStateMachine()->AddChildNode(bindPoseNode1);
            GetRootStateMachine()->AddChildNode(bindPoseNode2);
            GetRootStateMachine()->AddTransition(node1ToNode2);
            GetRootStateMachine()->SetEntryState(bindPoseNode1);

            InitAfterLoading();
        }

        AnimGraphParameterCondition* GetParameterCondition() const
        {
            return m_parameterCondition;
        }

    private:
        AnimGraphParameterCondition* m_parameterCondition;
    };

    class AnimGraphParameterConditionRemoveParameterFixture
        : public SystemComponentFixture
    {
    public:
        void SetUp() override
        {
            SystemComponentFixture::SetUp();
            m_animGraph = AnimGraphFactory::Create<ParamConditionAnimGraph>();
        }

        void TearDown() override
        {
            m_animGraph.reset();
            SystemComponentFixture::TearDown();
        }

        ParamConditionAnimGraph* GetAnimGraph() const
        {
            return m_animGraph.get();
        }

    private:
        AZStd::unique_ptr<ParamConditionAnimGraph> m_animGraph;
    };

    TEST_F(AnimGraphParameterConditionRemoveParameterFixture, ParameterIndexIsUpdatedWhenParameterIsRemoved)
    {
        CommandSystem::CommandManager manager;

        // GetParameterType is a proxy to check that cached m_parameterIndex is updated properly.
        const AnimGraphParameterCondition* condition = GetAnimGraph()->GetParameterCondition();
        EXPECT_EQ(condition->GetParameterType(), azrtti_typeid<EMotionFX::FloatSliderParameter>());

        {
            AZStd::string result;
            EXPECT_TRUE(manager.ExecuteCommand("AnimGraphRemoveParameter -animGraphID 0 -name P0", result)) << result.c_str();
        }

        EXPECT_EQ(condition->GetParameterType(), azrtti_typeid<EMotionFX::FloatSliderParameter>());

        {
            AZStd::string result;
            EXPECT_TRUE(manager.ExecuteCommand("AnimGraphRemoveParameter -animGraphID 0 -name P1", result)) << result.c_str();
        }

        EXPECT_EQ(condition->GetParameterType(), AZ::TypeId::CreateNull());

        // Will be fixed by LY-109269
        //{
        //    AZStd::string result;
        //    EXPECT_TRUE(manager.Undo(result)) << result.c_str();
        //}

        //EXPECT_EQ(condition->GetParameterType(), azrtti_typeid<EMotionFX::FloatSliderParameter>());
    }
} // namespace EMotionFX
