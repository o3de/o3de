/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/AttributeFloat.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <Tests/JackGraphFixture.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>


namespace EMotionFX
{
    struct MotionExtractionTestsData
    {
        std::vector<float> durationMultipliers;
        std::vector<AZ::u32> numOfLoops;
    };

    std::vector<MotionExtractionTestsData> motionExtractionTestData
    {
        {
            {0.001f, 0.01f, 1.0f},
            {1000, 100, 1}
        }
    };

    class MotionExtractionFixtureBase
        : public JackGraphFixture
    {
    public:
        virtual void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            m_jackSkeleton = m_actor->GetSkeleton();
            m_actorInstance->SetMotionExtractionEnabled(true);
            m_actor->AutoSetMotionExtractionNode();

            rootNode = m_jackSkeleton->FindNodeAndIndexByName("jack_root", m_jack_rootIndex);
            hipNode = m_jackSkeleton->FindNodeAndIndexByName("Bip01__pelvis", m_jack_hipIndex);
            m_jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
            AddMotionEntry(TestMotionAssets::GetJackWalkForward(), "jack_walk_forward_aim_zup");

            /*
            +------------+       +---------+
            |m_motionNode+------>+finalNode|
            +------------+       +---------+
            */
            m_motionNode = aznew AnimGraphMotionNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_motionNode->AddMotionId("jack_walk_forward_aim_zup");
            m_motionNode->SetLoop(true);
            m_motionNode->SetMotionExtraction(true);
            
            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_motionNode);
            m_blendTree->AddChildNode(finalNode);
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);
            finalNode->AddConnection(m_motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        }

        void AddMotionEntry(Motion* motion, const AZStd::string& motionId)
        {
            m_motion = motion;
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(m_motion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        AZ::Vector3 ExtractLastFramePos()
        {
            Node* motionExtractionNode = m_actor->GetMotionExtractionNode();
            if (!motionExtractionNode)
            {
                return AZ::Vector3::CreateZero();
            }

            auto motionData = azdynamic_cast<NonUniformMotionData*>(m_motion->GetMotionData());
            auto result = motionData->FindJointIndexByNameId(motionExtractionNode->GetID());
            if (!result.IsSuccess())
            {
                return AZ::Vector3::CreateZero();
            }

            const size_t motionJointIndex = result.GetValue();
            AZ::Vector3 position = AZ::Vector3::CreateZero();
            if (motionData->IsJointPositionAnimated(motionJointIndex))
            {
                const size_t sampleIndex = motionData->GetNumJointPositionSamples(motionJointIndex) - 1;
                position = motionData->GetJointPositionSample(motionJointIndex, sampleIndex).m_value;
            }

            return position;
        }

    protected:
        AZ::u32 m_jack_rootIndex = MCORE_INVALIDINDEX32;
        AZ::u32 m_jack_hipIndex = MCORE_INVALIDINDEX32;
        AnimGraphMotionNode* m_motionNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        Motion* m_motion = nullptr;
        Node* rootNode = nullptr;
        Node* hipNode = nullptr;
        Pose* m_jackPose = nullptr;
        Skeleton* m_jackSkeleton = nullptr;
    };
    

    class MotionExtractionFixture
        : public MotionExtractionFixtureBase
        , public ::testing::WithParamInterface<testing::tuple<bool, MotionExtractionTestsData>>
    {
    public:
        void ConstructGraph() override
        {
            MotionExtractionFixtureBase::ConstructGraph();
            m_reverse = testing::get<0>(GetParam());
            m_param = testing::get<1>(GetParam());
        }

    protected:
        MotionExtractionTestsData m_param;
        bool m_reverse = false;
    };

    class SyncMotionExtractionFixture
        : public MotionExtractionFixture
    {
    public:
        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            m_jackSkeleton = m_actor->GetSkeleton();
            m_actorInstance->SetMotionExtractionEnabled(true);
            m_actor->AutoSetMotionExtractionNode();

            m_jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();

            Motion* motion = TestMotionAssets::GetJackWalkForward();
            AddMotionEntry(motion, "jack_walk_forward_aim_zup");

            /*
                +-------------+        +-------------+
                |m_motionNode1|---o--->+m_motionNode2|
                +-------------+        +-------------+

                Where o = parameter condition, checking if the parameter "Trigger" is set to a value of 1.
            */
            m_motionNode1 = aznew AnimGraphMotionNode();
            m_motionNode1->AddMotionId("jack_walk_forward_aim_zup");

            m_motionNode2 = aznew AnimGraphMotionNode();
            m_motionNode2->AddMotionId("jack_walk_forward_aim_zup");

            m_triggerParameter = static_cast<BoolParameter*>(ParameterFactory::Create(azrtti_typeid<BoolParameter>()));
            m_triggerParameter->SetName("Trigger");
            m_triggerParameter->SetDefaultValue(false);
            m_animGraph->AddParameter(m_triggerParameter);

            m_motion->GetEventTable()->GetSyncTrack()->AddEvent(0.3f, AZStd::make_shared<TwoStringEventData>("SyncA"));
            m_motion->GetEventTable()->GetSyncTrack()->AddEvent(0.6f, AZStd::make_shared<TwoStringEventData>("SyncB"));

            AnimGraphParameterCondition* paramCondition = aznew AnimGraphParameterCondition("Trigger", 1.0f, AnimGraphParameterCondition::EFunction::FUNCTION_EQUAL);
            AnimGraphStateTransition* transition = aznew AnimGraphStateTransition(m_motionNode1, m_motionNode2, {paramCondition}, 0.1f);
            transition->SetSyncMode(AnimGraphObject::ESyncMode::SYNCMODE_CLIPBASED);
            transition->SetExtractionMode(AnimGraphObject::EExtractionMode::EXTRACTIONMODE_TARGETONLY);
            transition->SetEventFilterMode(AnimGraphObject::EEventMode::EVENTMODE_FOLLOWERONLY);
            m_animGraph->GetRootStateMachine()->AddTransition(transition);
            
            m_animGraph->GetRootStateMachine()->AddChildNode(m_motionNode1);
            m_animGraph->GetRootStateMachine()->AddChildNode(m_motionNode2);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_motionNode1);
        }

    protected:
        AnimGraphMotionNode* m_motionNode1 = nullptr;
        AnimGraphMotionNode* m_motionNode2 = nullptr;
        BoolParameter* m_triggerParameter = nullptr;
    };


#ifndef EMFX_SCALE_DISABLED
    TEST_F(MotionExtractionFixtureBase, ScaleTest)
    {
        const float scale = 2.0f;
        m_actorInstance->SetLocalSpaceScale(AZ::Vector3(scale, scale, scale));

        ASSERT_TRUE(m_motionNode->GetIsMotionExtraction()) << "Motion node should use motion extraction effect.";
        ASSERT_NE(m_actor->GetMotionExtractionNode(), nullptr) << "Actor's motion extraction node should not be nullptr.";

        // Move the character forward in 30 steps.
        // Make it so it exactly ends at the end of the motion.
        // The amount we move should be scaled up with the actor instance scale.
        const float expectedY = ExtractLastFramePos().GetY() * scale;
        const float duration = m_motion->GetDuration();
        const AZ::u32 numSteps = 30;
        const float stepSize = duration / static_cast<float>(numSteps);
        for (AZ::u32 i = 0; i < numSteps; ++i)
        {
            GetEMotionFX().Update(stepSize);
        }

        // Make sure we also really end where we expect.
        // Motion extraction will introduce some small inaccuracies, so we can't use AZ::g_fltEps here, but need a slightly larger value in our AZ::IsClose().
        const float yPos = m_actorInstance->GetWorldSpaceTransform().mPosition.GetY();
        EXPECT_TRUE(AZ::IsClose(yPos, expectedY, 0.01f));
    }
#endif

    TEST_P(MotionExtractionFixture, ReverseRotationMotionExtractionOutputsCorrectDelta)
    {
        // Test motion extraction with reverse effect on and off, rotation to 90 degrees left and right
        m_motionNode->FindMotionInstance(m_animGraphInstance)->SetMotionExtractionEnabled(true);
        m_motionNode->SetReverse(m_reverse);
        GetEMotionFX().Update(0.0f);
        EXPECT_TRUE(m_motionNode->GetIsMotionExtraction()) << "Motion node should use motion extraction effect.";
        EXPECT_NE(m_actor->GetMotionExtractionNode(), nullptr) << "Actor's motion extraction node should not be nullptr.";

        // The expected delta used is the distance of the jack walk forward motion will move in 1 complete duration
        const float expectedDelta = ExtractLastFramePos().GetY();
        for (AZ::u32 paramIndex = 0; paramIndex < m_param.durationMultipliers.size(); paramIndex++)
        {
            // Test motion extraction under different durations/time deltas
            const float motionDuration = 1.066f * m_param.durationMultipliers[paramIndex];
            const float originalPositionY = m_actorInstance->GetWorldSpaceTransform().mPosition.GetY();
            for (AZ::u32 i = 0; i < m_param.numOfLoops[paramIndex]; i++)
            {
                GetEMotionFX().Update(motionDuration);
            }
            const float updatedPositionY = m_actorInstance->GetWorldSpaceTransform().mPosition.GetY();
            const float actualDeltaY = AZ::GetAbs(updatedPositionY - originalPositionY);
            EXPECT_TRUE(AZ::GetAbs(actualDeltaY - expectedDelta) < 0.002f)
                << "The absolute difference between actual delta and expected delta of Y-axis should be less than 0.002f.";
        }

        // Test motion extraction with rotation
        const AZ::Quaternion actorRotation(0.0f, 0.0f, -1.0f, 1.0f);
        m_actorInstance->SetLocalSpaceRotation(actorRotation.GetNormalized());
        GetEMotionFX().Update(0.0f);
        for (AZ::u32 paramIndex = 0; paramIndex < m_param.durationMultipliers.size(); paramIndex++)
        {
            const float motionDuration = 1.066f * m_param.durationMultipliers[paramIndex];
            const float originalPositionX = m_actorInstance->GetWorldSpaceTransform().mPosition.GetX();
            for (AZ::u32 i = 0; i < m_param.numOfLoops[paramIndex]; i++)
            {
                GetEMotionFX().Update(motionDuration);
            }
            const float updatedPositionX = m_actorInstance->GetWorldSpaceTransform().mPosition.GetX();
            const float actualDeltaX = AZ::GetAbs(updatedPositionX - originalPositionX);
            EXPECT_TRUE(AZ::GetAbs(actualDeltaX - expectedDelta) < 0.002f)
                << "The absolute difference between actual delta and expected delta of X-axis should be less than 0.002f.";
        }
    }

    TEST_P(MotionExtractionFixture, DiagonalRotationMotionExtractionOutputsCorrectDelta)
    {
        // Test motion extraction with diagonal rotation
        m_motionNode->FindMotionInstance(m_animGraphInstance)->SetMotionExtractionEnabled(true);
        GetEMotionFX().Update(0.0f);

        const float expectedDeltaX = 1.30162f;
        const float expectedDeltaY = 0.97622f;

        // Use m_reverse to decide rotating diagonally to the left(0.5) or right(-0.5)
        const AZ::Quaternion diagonalRotation = m_reverse ? AZ::Quaternion(0.0f, 0.0f, 0.5f, 1.0f) : AZ::Quaternion(0.0f, 0.0f, -0.5f, 1.0f);
        m_actorInstance->SetLocalSpaceRotation(diagonalRotation.GetNormalized());
        GetEMotionFX().Update(0.0f);
        for (AZ::u32 paramIndex = 0; paramIndex < m_param.durationMultipliers.size(); paramIndex++)
        {
            const float originalPositionX = m_actorInstance->GetWorldSpaceTransform().mPosition.GetX();
            const float originalPositionY = m_actorInstance->GetWorldSpaceTransform().mPosition.GetY();
            const float motionDuration = 1.066f * m_param.durationMultipliers[paramIndex];
            for (AZ::u32 i = 0; i < m_param.numOfLoops[paramIndex]; i++)
            {
                GetEMotionFX().Update(motionDuration);
            }
            const float updatedPositionX = m_actorInstance->GetWorldSpaceTransform().mPosition.GetX();
            const float updatedPositionY = m_actorInstance->GetWorldSpaceTransform().mPosition.GetY();
            const float actualDeltaX = AZ::GetAbs(updatedPositionX - originalPositionX);
            const float actualDeltaY = AZ::GetAbs(updatedPositionY - originalPositionY);
            EXPECT_NEAR(actualDeltaX, expectedDeltaX, 0.001f)
                << "Diagonal Rotation: The absolute difference between actual delta and expected delta of X-axis should be less than 0.001f.";
            EXPECT_NEAR(actualDeltaY, expectedDeltaY, 0.001f)
                << "Diagonal Rotation: The absolute difference between actual delta and expected delta of Y-axis should be less than 0.001f.";
        }
    }

    TEST_F(SyncMotionExtractionFixture, VerifyFirstFrameSync)
    {
        ASSERT_NE(m_motionNode1->FindMotionInstance(m_animGraphInstance), nullptr);
        ASSERT_NE(m_motionNode2->FindMotionInstance(m_animGraphInstance), nullptr);
        GetEMotionFX().Update(0.0f);

        // Make sure we're out of sync first.
        m_motionNode1->SetCurrentPlayTimeNormalized(m_animGraphInstance, 0.75f);
        m_motionNode2->SetCurrentPlayTimeNormalized(m_animGraphInstance, 0.2f);
        auto* param = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeBool>(0); 
        param->SetValue(true); // Trigger the transition into motion 2.

        // Update one frame, which is the first frame during the synced transition.
        // We currently expect the motion extraction delta to be zero here. This is in order to prevent possible teleports which can happen.
        // This is because the presync time value of the second motion node is from the unsynced playback.
        // When we improve our syncing system we can handle this differently and we won't expect a zero trajectory delta anymore.
        GetEMotionFX().Update(0.15f);
        EXPECT_FLOAT_EQ(m_actorInstance->GetTrajectoryDeltaTransform().mPosition.GetLength(), 0.0f);
        EXPECT_FLOAT_EQ(m_motionNode1->GetCurrentPlayTime(m_animGraphInstance), m_motionNode2->GetCurrentPlayTime(m_animGraphInstance));
        EXPECT_EQ(m_animGraphInstance->GetEventBuffer().GetNumEvents(), 0);

        // The second frame should be as normal.
        GetEMotionFX().Update(0.15f);
        EXPECT_GT(m_actorInstance->GetTrajectoryDeltaTransform().mPosition.GetLength(), 0.0f);
        EXPECT_LE(m_actorInstance->GetTrajectoryDeltaTransform().mPosition.GetLength(), 0.3f);
        EXPECT_FLOAT_EQ(m_motionNode1->GetCurrentPlayTime(m_animGraphInstance), m_motionNode2->GetCurrentPlayTime(m_animGraphInstance));
        EXPECT_EQ(m_animGraphInstance->GetEventBuffer().GetNumEvents(), 0);
    }
    
    INSTANTIATE_TEST_CASE_P(MotionExtraction_OutputTests,
        MotionExtractionFixture,
        ::testing::Combine(
            ::testing::Bool(),
            ::testing::ValuesIn(motionExtractionTestData)
        )
    );
    
} // end namespace EMotionFX
