/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <Tests/JackGraphFixture.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>


namespace EMotionFX
{
    class AnimGraphMotionNodeFixture
        : public JackGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            m_jackSkeleton = m_actor->GetSkeleton();
            SetupIndices();
            SetupMirrorNodes();
            m_jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();

            // Motion of Jack walking forward (Y-axis change) with right arm aiming towards front.
            AddMotionData(TestMotionAssets::GetJackWalkForward(), "jack_walk_forward_aim_zup");

            /*
              Blend tree in animgraph:
              +---------------+                         
              |m_parameterNode|---+                     
              +---------------+   |    +------------+       +---------+
                                  +--->|m_motionNode|------>|finalNode|
                                  +--->|            |       +---------+
              +---------------+   |    +------------+
              |m_fltConstNode |---+                  
              +---------------+                      
            */
            AddParameter<BoolParameter>("InPlace", false);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_fltConstNode = aznew BlendTreeFloatConstantNode();
            m_paramNode = aznew BlendTreeParameterNode();
            m_motionNode = aznew AnimGraphMotionNode();

            // Control motion and effects to be used.
            m_motionNode->AddMotionId("jack_walk_forward_aim_zup");
            m_motionNode->SetLoop(false);
            m_motionNode->SetRetarget(false);
            m_motionNode->SetReverse(false);
            m_motionNode->SetEmitEvents(false);
            m_motionNode->SetMirrorMotion(false);
            m_motionNode->SetMotionExtraction(false);

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_motionNode);
            m_blendTree->AddChildNode(m_paramNode);
            m_blendTree->AddChildNode(m_fltConstNode);
            m_blendTree->AddChildNode(finalNode);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            finalNode->AddConnection(m_motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        }

        void AddMotionData(Motion* newMotion, const AZStd::string& motionId)
        {
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(newMotion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        template <class paramType, class inputType>
        void ParamSetValue(const AZStd::string& paramName, const inputType& value)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
            paramType* typeParam = static_cast<paramType*>(param);
            typeParam->SetValue(value);
        }

        bool PositionsAreMirrored(const AZ::Vector3& leftPos, const AZ::Vector3& rightPos, float tolerance)
        {
            if (!AZ::IsClose(leftPos.GetX(), AZ::GetAbs(rightPos.GetX()), tolerance))
            {
                return false;
            }
            if (!AZ::IsClose(leftPos.GetY(), rightPos.GetY(), tolerance))
            {
                return false;
            }
            if (!AZ::IsClose(leftPos.GetZ(), rightPos.GetZ(), tolerance))
            {
                return false;
            }
            return true;
        }

    protected:
        size_t m_lHandIndex = InvalidIndex;
        size_t m_lLoArmIndex = InvalidIndex;
        size_t m_lLoLegIndex = InvalidIndex;
        size_t m_lAnkleIndex = InvalidIndex;
        size_t m_rHandIndex = InvalidIndex;
        size_t m_rLoArmIndex = InvalidIndex;
        size_t m_rLoLegIndex = InvalidIndex;
        size_t m_rAnkleIndex = InvalidIndex;
        size_t m_jackRootIndex = InvalidIndex;
        size_t m_bip01PelvisIndex = InvalidIndex;
        AnimGraphMotionNode* m_motionNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_fltConstNode = nullptr;
        BlendTreeParameterNode* m_paramNode = nullptr;
        Pose * m_jackPose = nullptr;
        Skeleton* m_jackSkeleton = nullptr;

    private:
        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string& name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void SetupIndices()
        {
            Node* rootNode = m_jackSkeleton->FindNodeAndIndexByName("jack_root", m_jackRootIndex);
            Node* pelvisNode = m_jackSkeleton->FindNodeAndIndexByName("Bip01__pelvis", m_bip01PelvisIndex);
            Node* lHandNode = m_jackSkeleton->FindNodeAndIndexByName("l_hand", m_lHandIndex);
            Node* lLoArmNode = m_jackSkeleton->FindNodeAndIndexByName("l_loArm", m_lLoArmIndex);
            Node* lLoLegNode = m_jackSkeleton->FindNodeAndIndexByName("l_loLeg", m_lLoLegIndex);
            Node* lAnkleNode = m_jackSkeleton->FindNodeAndIndexByName("l_ankle", m_lAnkleIndex);
            Node* rHandNode = m_jackSkeleton->FindNodeAndIndexByName("r_hand", m_rHandIndex);
            Node* rLoArmNode = m_jackSkeleton->FindNodeAndIndexByName("r_loArm", m_rLoArmIndex);
            Node* rLoLegNode = m_jackSkeleton->FindNodeAndIndexByName("r_loLeg", m_rLoLegIndex);
            Node* rAnkleNode = m_jackSkeleton->FindNodeAndIndexByName("r_ankle", m_rAnkleIndex);

            // Make sure all nodes exist.
            ASSERT_TRUE(rootNode && pelvisNode && lHandNode && lLoArmNode && lLoLegNode && lAnkleNode &&
                rHandNode && rLoArmNode && rLoLegNode && rAnkleNode) << "All nodes used should exist.";

            m_actor->SetMotionExtractionNodeIndex(m_jackRootIndex);
        }

        void SetupMirrorNodes()
        {
            m_actor->AllocateNodeMirrorInfos();
            m_actor->GetNodeMirrorInfo(m_lHandIndex).m_sourceNode = static_cast<uint16>(m_rHandIndex);
            m_actor->GetNodeMirrorInfo(m_rHandIndex).m_sourceNode = static_cast<uint16>(m_lHandIndex);
            m_actor->GetNodeMirrorInfo(m_lLoArmIndex).m_sourceNode = static_cast<uint16>(m_rLoArmIndex);
            m_actor->GetNodeMirrorInfo(m_rLoArmIndex).m_sourceNode = static_cast<uint16>(m_lLoArmIndex);
            m_actor->GetNodeMirrorInfo(m_lLoLegIndex).m_sourceNode = static_cast<uint16>(m_rLoLegIndex);
            m_actor->GetNodeMirrorInfo(m_rLoLegIndex).m_sourceNode = static_cast<uint16>(m_lLoLegIndex);
            m_actor->GetNodeMirrorInfo(m_lAnkleIndex).m_sourceNode = static_cast<uint16>(m_rAnkleIndex);
            m_actor->GetNodeMirrorInfo(m_rAnkleIndex).m_sourceNode = static_cast<uint16>(m_lAnkleIndex);
            m_actor->AutoDetectMirrorAxes();
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndZeroEffectOutputsCorrectMotionAndPose)
    {
        m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode);
        
        // Check position of root and pelvis to ensure actor's motion movement is correct.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            const float rootDifference = rootUpdatedPos.GetY() - rootCurrentPos.GetY();
            const float pelvisDifference = pelvisUpdatedPos.GetY() - pelvisCurrentPos.GetY();

            EXPECT_TRUE(rootUpdatedPos.GetY() > rootCurrentPos.GetY()) << "Y-axis position of root should increase.";
            EXPECT_TRUE(pelvisUpdatedPos.GetY() > pelvisCurrentPos.GetY()) << "Y-axis position of pelvis should increase.";
            EXPECT_TRUE(rootDifference == pelvisDifference) << "Movement of root and pelvis should be the same.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndLoopOutputsCorrectMotionAndPose)
    {
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->m_reload = true;
        m_motionNode->SetLoop(true);
        m_motionNode->InvalidateUniqueData(m_animGraphInstance);
        m_actorInstance->SetMotionExtractionEnabled(false);
        EXPECT_TRUE(m_motionNode->GetIsLooping()) << "Loop effect should be on.";
        GetEMotionFX().Update(0.0f); // Needed to trigger a refresh of motion node internals.

        // Update to half the motion's duration.
        AZ::Vector3 rootStartPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        AZ::Vector3 pelvisStartPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
        const float duration = m_motionNode->GetDuration(m_animGraphInstance);
        const float offset = duration * 0.5f;
        GetEMotionFX().Update(offset);
        EXPECT_FLOAT_EQ(uniqueData->GetCurrentPlayTime(), offset);
        AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
        EXPECT_TRUE(rootCurrentPos.GetY() > rootStartPos.GetY()) << "Y-axis position of root should increase.";
        EXPECT_TRUE(pelvisCurrentPos.GetY() > pelvisStartPos.GetY()) << "Y-axis position of pelvis should increase.";

        // Update so that we cause a loop till 10% in the motion playback time.
        rootStartPos = rootCurrentPos;
        pelvisStartPos = pelvisCurrentPos;
        GetEMotionFX().Update(duration * 0.6f);
        EXPECT_FLOAT_EQ(uniqueData->GetCurrentPlayTime(), duration * 0.1f);
        rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
        EXPECT_TRUE(rootCurrentPos.GetY() < rootStartPos.GetY()) << "Y-axis position of root should increase.";
        EXPECT_TRUE(pelvisCurrentPos.GetY() < pelvisStartPos.GetY()) << "Y-axis position of pelvis should increase.";
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndReverseOutputsCorrectMotionAndPose)
    {
        m_motionNode->SetReverse(true);
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->m_reload = true;
        GetEMotionFX().Update(1.1f);

        EXPECT_TRUE(m_motionNode->GetIsReversed()) << "Reverse effect should be on.";

        // Check position of root and pelvis to ensure actor's motion movement is reversed.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            const float rootDifference =  rootCurrentPos.GetY() - rootUpdatedPos.GetY();
            const float pelvisDifference = pelvisCurrentPos.GetY() - pelvisUpdatedPos.GetY();

            EXPECT_TRUE(rootUpdatedPos.GetY() < rootCurrentPos.GetY()) << "Y-axis position of root should decrease.";
            EXPECT_TRUE(pelvisUpdatedPos.GetY() < pelvisCurrentPos.GetY()) << "Y-axis position of pelvis should decrease.";
            EXPECT_TRUE(rootDifference == pelvisDifference) << "Movement of root and pelvis should be the same.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, DISABLED_NoInputAndMirrorMotionOutputsCorrectMotionAndPose)
    {
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->m_reload = true;
        GetEMotionFX().Update(1.0f);

        // Get positions before mirroring to compare with mirrored positions later.
        const AZ::Vector3 l_handCurrentPos = m_jackPose->GetModelSpaceTransform(m_lHandIndex).m_position;
        const AZ::Vector3 l_loArmCurrentPos = m_jackPose->GetModelSpaceTransform(m_lLoArmIndex).m_position;
        const AZ::Vector3 l_loLegCurrentPos = m_jackPose->GetModelSpaceTransform(m_lLoLegIndex).m_position;
        const AZ::Vector3 l_ankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_lAnkleIndex).m_position;
        const AZ::Vector3 r_handCurrentPos = m_jackPose->GetModelSpaceTransform(m_rHandIndex).m_position;
        const AZ::Vector3 r_loArmCurrentPos = m_jackPose->GetModelSpaceTransform(m_rLoArmIndex).m_position;
        const AZ::Vector3 r_loLegCurrentPos = m_jackPose->GetModelSpaceTransform(m_rLoLegIndex).m_position;
        const AZ::Vector3 r_ankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_rAnkleIndex).m_position;

        m_motionNode->SetMirrorMotion(true);
        uniqueData->m_reload = true;
        GetEMotionFX().Update(0.0001f);

        EXPECT_TRUE(m_motionNode->GetMirrorMotion()) << "Mirror motion effect should be on.";

        const AZ::Vector3 l_handMirroredPos = m_jackPose->GetModelSpaceTransform(m_lHandIndex).m_position;
        const AZ::Vector3 l_loArmMirroredPos = m_jackPose->GetModelSpaceTransform(m_lLoArmIndex).m_position;
        const AZ::Vector3 l_loLegMirroredPos = m_jackPose->GetModelSpaceTransform(m_lLoLegIndex).m_position;
        const AZ::Vector3 l_ankleMirroredPos = m_jackPose->GetModelSpaceTransform(m_lAnkleIndex).m_position;
        const AZ::Vector3 r_handMirroredPos = m_jackPose->GetModelSpaceTransform(m_rHandIndex).m_position;
        const AZ::Vector3 r_loArmMirroredPos = m_jackPose->GetModelSpaceTransform(m_rLoArmIndex).m_position;
        const AZ::Vector3 r_loLegMirroredPos = m_jackPose->GetModelSpaceTransform(m_rLoLegIndex).m_position;
        const AZ::Vector3 r_ankleMirroredPos = m_jackPose->GetModelSpaceTransform(m_rAnkleIndex).m_position;

        EXPECT_TRUE(PositionsAreMirrored(l_handCurrentPos, r_handMirroredPos, 0.001f)) << "Actor's left hand should be mirrored to right hand.";
        EXPECT_TRUE(PositionsAreMirrored(l_handMirroredPos, r_handCurrentPos, 0.001f)) << "Actor's right hand should be mirrored to left hand.";
        EXPECT_TRUE(PositionsAreMirrored(l_loArmCurrentPos, r_loArmMirroredPos, 0.001f)) << "Actor's left lower arm should be mirrored to right lower arm.";
        EXPECT_TRUE(PositionsAreMirrored(l_loArmMirroredPos, r_loArmCurrentPos, 0.001f)) << "Actor's right lower arm should be mirrored to left lower arm.";
        EXPECT_TRUE(PositionsAreMirrored(l_loLegCurrentPos, r_loLegMirroredPos, 0.001f)) << "Actor's left lower leg should be mirrored to right lower leg.";
        EXPECT_TRUE(PositionsAreMirrored(l_loLegMirroredPos, r_loLegCurrentPos, 0.001f)) << "Actor's right lower leg should be mirrored to left lower leg.";
        EXPECT_TRUE(PositionsAreMirrored(l_ankleCurrentPos, r_ankleMirroredPos, 0.001f)) << "Actor's left ankle should be mirrored to right ankle.";
        EXPECT_TRUE(PositionsAreMirrored(l_ankleMirroredPos, r_ankleCurrentPos, 0.001f)) << "Actor's right ankle should be mirrored to left ankle.";
    };

    TEST_F(AnimGraphMotionNodeFixture, InPlaceInputAndNoEffectOutputsCorrectMotionAndPose)
    {
        m_motionNode->AddConnection(m_paramNode, static_cast<uint16>(m_paramNode->FindOutputPortByName("InPlace")->m_portId), AnimGraphMotionNode::INPUTPORT_INPLACE);
        ParamSetValue<MCore::AttributeBool, bool>("InPlace", true);
        m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode);
        
        GetEMotionFX().Update(1.0f / 60.0f);

        EXPECT_TRUE(m_motionNode->GetIsInPlace(m_animGraphInstance)) << "In Place effect should be on.";

        // Check position of root and pelvis to ensure actor's motion movement is staying in place.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            const AZ::Vector3 lankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_lAnkleIndex).m_position;
            const AZ::Vector3 rankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_rAnkleIndex).m_position;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01PelvisIndex).m_position;
            const AZ::Vector3 lankleUpdatedPos = m_jackPose->GetModelSpaceTransform(m_lAnkleIndex).m_position;
            const AZ::Vector3 rankleUpdatedPos = m_jackPose->GetModelSpaceTransform(m_rAnkleIndex).m_position;
            EXPECT_TRUE(m_motionNode->GetIsInPlace(m_animGraphInstance)) << "InPlace flag of the motion node should be true.";
            EXPECT_TRUE(rootUpdatedPos.IsClose(rootCurrentPos, 0.0f)) << "Position of root should not change.";           
            EXPECT_TRUE(pelvisCurrentPos != pelvisUpdatedPos) << "Position of pelvis should change.";
            EXPECT_TRUE(lankleCurrentPos != lankleUpdatedPos) << "Position of left ankle should change.";
            EXPECT_TRUE(rankleCurrentPos != rankleUpdatedPos) << "Position of right ankle should change.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, PlaySpeedInputAndPlaySpeedEffectOutputsCorrectMotionAndPose)
    {
        // Connect motion node's PlaySpeed input port with a float constant node for control.
        m_fltConstNode->SetValue(1.0f);
        BlendTreeConnection* playSpeedConnection = m_motionNode->AddConnection(m_fltConstNode,
            BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, AnimGraphMotionNode::INPUTPORT_PLAYSPEED);

        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        GetEMotionFX().Update(1.0f / 60.0f);

        // Root node's initial position under the first speed factor.
        AZ::Vector3 rootInitialPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        uniqueData->m_reload = true;
        GetEMotionFX().Update(1.1f);
        
        // Root node's final position under the first speed factor.
        AZ::Vector3 rootFinalPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        std::vector<float> speedFactors = { 2.0f, 3.0f, 10.0f, 100.0f };
        std::vector<float> playTimes = { 0.6f, 0.4f, 0.11f, 0.011f };
        for (size_t i = 0; i < 4; i++)
        {
            m_motionNode->Rewind(m_animGraphInstance);
            m_fltConstNode->SetValue(speedFactors[i]);
            GetEMotionFX().Update(1.0f / 60.0f);

            uniqueData->m_reload = true;
            const AZ::Vector3 rootInitialPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;

            // Faster play speed requires less play time to reach its final pose.
            GetEMotionFX().Update(playTimes[i]);
            const AZ::Vector3 rootFinalPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;

            EXPECT_TRUE(rootInitialPosUnderSpeed1.IsClose(rootInitialPosUnderSpeed2, 0.0f)) << "Root initial position should be same in different motion speeds.";
            EXPECT_TRUE(rootFinalPosUnderSpeed1.IsClose(rootFinalPosUnderSpeed2, 0.0f)) << "Root final position should be same in different motion speeds.";

            // Update positions to the new speed for comparing with the next speed factor.
            rootInitialPosUnderSpeed1 = rootInitialPosUnderSpeed2;
            rootFinalPosUnderSpeed1 = rootFinalPosUnderSpeed2;
        }

        // Disconnect PlaySpeed port.
        // Check playspeed control through motion node's own method SetPlaySpeed().
        m_motionNode->RemoveConnection(playSpeedConnection);
        m_motionNode->Rewind(m_animGraphInstance);
        m_motionNode->SetMotionPlaySpeed(1.0f);
        GetEMotionFX().Update(1.0f / 60.0f);

        rootInitialPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;
        uniqueData->m_reload = true;
        GetEMotionFX().Update(1.1f);
        rootFinalPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;

        // Similar test to using the InPlace input port.
        for (size_t i = 0; i < 4; i++)
        {
            m_motionNode->Rewind(m_animGraphInstance);
            m_motionNode->SetMotionPlaySpeed(speedFactors[i]);
            GetEMotionFX().Update(1.0f / 60.0f);

            uniqueData->m_reload = true;
            const AZ::Vector3 rootInitialPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;

            GetEMotionFX().Update(playTimes[i]);
            const AZ::Vector3 rootFinalPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jackRootIndex).m_position;

            EXPECT_TRUE(rootInitialPosUnderSpeed1.IsClose(rootInitialPosUnderSpeed2, 0.0f));
            EXPECT_TRUE(rootFinalPosUnderSpeed1.IsClose(rootFinalPosUnderSpeed2, 0.0f));

            rootInitialPosUnderSpeed1 = rootInitialPosUnderSpeed2;
            rootFinalPosUnderSpeed1 = rootFinalPosUnderSpeed2;
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, TwoMotionsOutputsCorrectMotionAndPose)
    {
        // Add one more motion, Jack falling back and down.
        // Loop effect turned on to ensure motions are changing in loops.
        AddMotionData(TestMotionAssets::GetJackDie(), "jack_death_fall_back_zup");
        m_motionNode->AddMotionId("jack_death_fall_back_zup");
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->m_reload = true;
        m_motionNode->Reinit();

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_RANDOMIZE);
        m_motionNode->SetNextMotionAfterLoop(true);
        m_motionNode->SetLoop(true);
        GetEMotionFX().Update(1.0f / 60.0f);
        
        EXPECT_TRUE(m_motionNode->GetNumMotions() == 2) << "Motion node should have 2 motions after adding motion id.";
        EXPECT_TRUE(m_motionNode->GetIsLooping()) << "Motion node loop effect should be on.";

        // In randomized index mode, all motions should at least appear once over 10 loops.
        bool motion1Displayed = false;
        bool motion2Displayed = false;
        for (size_t i = 0; i < 20; i++)
        {
            // Run the test loop multiple times to make sure all the motion index is picked.
            uniqueData->m_reload = true;
            m_motionNode->Reinit();
            GetEMotionFX().Update(2.0f);

            const uint32 motionIndex = uniqueData->m_activeMotionIndex;
            if (motionIndex == 0)
            {
                motion1Displayed = true;
            }
            else if (motionIndex == 1)
            {
                motion2Displayed = true;
            }
            else
            {
                EXPECT_TRUE(false) << "Unexpected motion index.";
            }
            if (motion1Displayed && motion2Displayed)
            {
                break;
            }
        }
        EXPECT_TRUE(motion1Displayed && motion2Displayed) << "Motion 1 and motion 2 should both have been displayed.";

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_RANDOMIZE_NOREPEAT);
        uniqueData->Reset();
        m_motionNode->Reinit();
        uniqueData->Update();
        uint32 currentMotionIndex = uniqueData->m_activeMotionIndex;

        // In randomized no repeat index mode, motions should change in each loop.
        for (size_t i = 0; i < 10; i++)
        {
            uniqueData->m_reload = true;
            m_motionNode->Reinit();

            // As we keep and use the cached version of the unique data, we need to manually update it.
            uniqueData->Update();

            const AZ::u32 updatedMotionIndex = uniqueData->m_activeMotionIndex;
            EXPECT_TRUE(updatedMotionIndex != currentMotionIndex) << "Updated motion index should be different from its previous motion index.";
            currentMotionIndex = updatedMotionIndex;
        }

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_SEQUENTIAL);

        // In sequential index mode, motions should increase its index each time and wrap around. Basically iterating over the list of motions.
        for (size_t i = 0; i < 10; i++)
        {
            uniqueData->m_reload = true;
            m_motionNode->Reinit();
            uniqueData->Update();
            EXPECT_NE(currentMotionIndex, uniqueData->m_activeMotionIndex) << "Updated motion index should match the expected motion index.";
            currentMotionIndex = uniqueData->m_activeMotionIndex;
        }
    };
} // end namespace EMotionFX
