/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h"
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
        AZ_CLASS_ALLOCATOR(ParamConditionAnimGraph, AnimGraphAllocator)
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

        EXPECT_TRUE(CommandSystem::BuildRemoveParametersCommandGroup(GetAnimGraph(), {"P0"}));
        EXPECT_EQ(condition->GetParameterType(), azrtti_typeid<EMotionFX::FloatSliderParameter>());

        EXPECT_TRUE(CommandSystem::BuildRemoveParametersCommandGroup(GetAnimGraph(), {"P1"}));
        EXPECT_EQ(condition->GetParameterType(), AZ::TypeId::CreateNull());

        {
            AZStd::string result;
            EXPECT_TRUE(manager.Undo(result)) << result.c_str();
        }
        EXPECT_EQ(condition->GetParameterType(), azrtti_typeid<EMotionFX::FloatSliderParameter>());
    }
} // namespace EMotionFX
