/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendTreeTwoLinkIKNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/BlendTreeVector3ComposeNode.h>
#include <Tests/JackGraphFixture.h>

namespace EMotionFX
{
    struct BlendTreeTwoLinkIKNodeTestsData
    {
        AZStd::string testJointName;
        std::vector<AZStd::string> linkedJointNames;
        std::vector<std::vector<float>> reachablePositions;
        std::vector<std::vector<float>> unreachablePositions;
        std::vector<std::vector<float>> rotations;
        std::vector<float> bendDirPosition;
        std::vector<AZStd::string> alignToNodeNames;
    };

    class BlendTreeTwoLinkIKNodeFixture
        : public JackGraphFixture
        , public ::testing::WithParamInterface<testing::tuple<bool, BlendTreeTwoLinkIKNodeTestsData>>
    {
    public:
        using NodeAlignmentData = AZStd::pair<AZStd::string, int>;
        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            m_param = testing::get<1>(GetParam());
            m_jackSkeleton = m_actor->GetSkeleton();

            /*
              Blend tree in animgraph:
              +------------+
              |bindPoseNode|---+
              +------------+   |    +-------------+
                               +--->|             |    +---------+
                                    |twoLinkIKNode|--->|finalNode|
               +-----------+   +--->|             |    +---------+
               |m_paramNode|---+    +-------------+
               +-----------+       
            */
            AddParameter<FloatSliderParameter>("WeightParam", 0.0f);
            AddParameter<Vector3Parameter>("GoalPosParam", AZ::Vector3(0.0f, 0.0f, 0.0f));
            AddParameter<RotationParameter>("RotationParam", AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
            AddParameter<Vector3Parameter>("BendDirParam", AZ::Vector3(0.0f, 0.0f, 0.0f));

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_paramNode = aznew BlendTreeParameterNode();
            m_twoLinkIKNode = aznew BlendTreeTwoLinkIKNode();

            m_twoLinkIKNode->SetEndNodeName(m_param.testJointName);

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(m_paramNode);
            m_blendTree->AddChildNode(m_twoLinkIKNode);
            m_blendTree->AddChildNode(finalNode);
            
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            m_twoLinkIKNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::OUTPUTPORT_RESULT, BlendTreeTwoLinkIKNode::INPUTPORT_POSE);
            finalNode->AddConnection(m_twoLinkIKNode, BlendTreeTwoLinkIKNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        };

        bool PosePositionCompareClose(const AZ::Vector3& posA, const AZ::Vector3& posB, float tolerance)
        {
            if (!posA.IsClose(posB, tolerance))
            {
                return false;
            }
            return true;
        };

        bool PositionVectorsPointInSameDirection(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& targetPos)
        {
            const AZ::Vector3& vecCtoB = (posB - posC).GetNormalized();
            const AZ::Vector3& vecBtoA = (posA - posB).GetNormalized();
            const AZ::Vector3& vecCtoTarget = (targetPos - posC).GetNormalized();

            const float dot1 = vecCtoB.Dot(vecBtoA);
            const float dot2 = vecCtoB.Dot(vecCtoTarget);
            return AZ::IsClose(dot1, 1.0f, 0.001f) && 
                   AZ::IsClose(dot2, 1.0f, 0.001f);
        };

        template <class paramType, class inputType>
        void ParamSetValue(const AZStd::string& paramName, const inputType& value)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
            paramType* typeParam = static_cast<paramType*>(param);
            typeParam->SetValue(value);
        }

    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeParameterNode* m_paramNode = nullptr;
        BlendTreeTwoLinkIKNode* m_twoLinkIKNode = nullptr;
        BlendTreeTwoLinkIKNodeTestsData m_param;
        
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
    };

    TEST_P(BlendTreeTwoLinkIKNodeFixture, ReachablePositionsOutputCorrectPose)
    {
        // Set values for vector3 and twoLinkIKNode weight parameter
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("GoalPosParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALPOS);

        GetEMotionFX().Update(1.0f / 60.0f);
        const float weight = testing::get<0>(GetParam());
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);

        // Remeber specific joint's original position to compare with its new position later
        const Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testJointIndex;
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);
        const AZ::Vector3& testJointPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;

        for (std::vector<float> goalPosXYZ : m_param.reachablePositions)
        {
            const float goalX = goalPosXYZ[0];
            const float goalY = goalPosXYZ[1];
            const float goalZ = goalPosXYZ[2];
            ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("GoalPosParam", AZ::Vector3(goalX, goalY, goalZ));

            GetEMotionFX().Update(5.0f / 60.0f);
            const AZ::Vector3& testJointNewPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;

            // Based on weight, check if position of node changes to reachable goal position
            if (weight)
            {
                const AZ::Vector3 expectedPosition(goalX, goalY, goalZ);
                const AZ::Vector3 dist = (expectedPosition - testJointNewPos).GetAbs();
                const float length = dist.GetLength();
                EXPECT_TRUE(PosePositionCompareClose(testJointNewPos, expectedPosition, 0.0001f))
                    << "Joint position should be similar to expected position.";
            }
            else
            {
                EXPECT_TRUE(testJointNewPos == testJointPos) << "Joint position should not change when weight is 0.";
            }
        }
    };

    TEST_P(BlendTreeTwoLinkIKNodeFixture, ReachableAlignToNodeOutputCorrectPose)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID,
            BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);

        GetEMotionFX().Update(1.0f / 60.0f);
        const float weight = testing::get<0>(GetParam());
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);
        
        const Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testJointIndex;
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);
        const AZ::Vector3& testJointPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;

        for (AZStd::string& nodeName : m_param.alignToNodeNames)
        {
            NodeAlignmentData alignToNode; 
            alignToNode.first = nodeName;
            alignToNode.second = 0;
            m_twoLinkIKNode->SetAlignToNode(alignToNode);
            
            // Update will set uniqueData->mMustUpdate to false for efficiency purposes
            // Unique data only updates once unless reset mMustUpdate to true again
            BlendTreeTwoLinkIKNode::UniqueData* uniqueData = static_cast<BlendTreeTwoLinkIKNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_twoLinkIKNode));
            uniqueData->Invalidate();
            AZ::u32 alignToNodeIndex;
            m_jackSkeleton->FindNodeAndIndexByName(nodeName, alignToNodeIndex);
            
            GetEMotionFX().Update(1.0f / 60.0f);
            const AZ::Vector3& alignToNodePos = jackPose->GetModelSpaceTransform(alignToNodeIndex).mPosition;
            const AZ::Vector3& testJointNewPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;

            // Based on weight, check if position of node changes to alignToNode position
            if (weight)
            {
                EXPECT_TRUE(PosePositionCompareClose(alignToNodePos, testJointNewPos, 0.0001f)) 
                    << "Test joint position should be similar to align-to joint position.";
            }
            else
            {
                EXPECT_TRUE(testJointPos == testJointNewPos) << "Joint position should not change when weight is 0.";
            }
        }
    };
    
    TEST_P(BlendTreeTwoLinkIKNodeFixture, UnreachablePositionsOutputCorrectPose)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID,
            BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("GoalPosParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALPOS);

        GetEMotionFX().Update(1.0f / 60.0f);
        const float weight = testing::get<0>(GetParam());
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);

        const Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testJointIndex;
        AZ::u32 linkedJoint0Index;
        AZ::u32 linkedJoint1Index;
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);
        m_jackSkeleton->FindNodeAndIndexByName(m_param.linkedJointNames[0], linkedJoint0Index);
        m_jackSkeleton->FindNodeAndIndexByName(m_param.linkedJointNames[1], linkedJoint1Index);
        const AZ::Vector3& testJointPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;
        
        for (std::vector<float> goalPosXYZ : m_param.unreachablePositions)
        {
            const float goalX = goalPosXYZ[0];
            const float goalY = goalPosXYZ[1];
            const float goalZ = goalPosXYZ[2];
            ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("GoalPosParam", AZ::Vector3(goalX, goalY, goalZ));

            GetEMotionFX().Update(1.0f / 60.0f);
            const AZ::Vector3& testJointNewPos = jackPose->GetModelSpaceTransform(testJointIndex).mPosition;
            const AZ::Vector3& linkedJoint0Pos = jackPose->GetModelSpaceTransform(linkedJoint0Index).mPosition;
            const AZ::Vector3& linkedJoint1Pos = jackPose->GetModelSpaceTransform(linkedJoint1Index).mPosition;

            // Based on weight, check if position of the test node 
            // And its linked nodes are pointing towards the unreachable position
            if (weight)
            {
                const AZ::Vector3 goalVec(goalX, goalY, goalZ);
                EXPECT_TRUE(PositionVectorsPointInSameDirection(testJointNewPos, linkedJoint0Pos, linkedJoint1Pos, goalVec)) 
                    << "Test joint and its linked joints should point to the expected vector.";
            }
            else
            {
                EXPECT_TRUE(testJointNewPos == testJointPos) << "Joint position should not change when weight is 0.";
            }
        }

    };

    TEST_P(BlendTreeTwoLinkIKNodeFixture, RotatedPositionsOutputCorrectPose)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID,
            BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("GoalPosParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALPOS);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("RotationParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALROT);
        m_twoLinkIKNode->SetRotationEnabled(true);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Set up test joint position and weight
        const float weight = testing::get<0>(GetParam());
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("GoalPosParam", AZ::Vector3(0.0f, 1.0f, 1.0f));

        const Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testJointIndex;
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);
        const AZ::Quaternion testJointRotation = jackPose->GetModelSpaceTransform(testJointIndex).mRotation;

        for (std::vector<float> rotateXYZ : m_param.rotations)
        {
            const float rotateX = rotateXYZ[0];
            const float rotateY = rotateXYZ[1];
            const float rotateZ = rotateXYZ[2];
            ParamSetValue<MCore::AttributeQuaternion, AZ::Quaternion>("RotationParam", AZ::Quaternion(rotateX, rotateY, rotateZ, 1.0f));
            
            GetEMotionFX().Update(1.0f / 60.0f);
            const AZ::Quaternion testJointNewRotation = jackPose->GetModelSpaceTransform(testJointIndex).mRotation;

            if (weight)
            {
                const AZ::Quaternion goalRotation(rotateX, rotateY, rotateZ, 1.0f);
                EXPECT_EQ(testJointNewRotation, goalRotation.GetNormalized()) << "Rotation of the test joint should be the same as the expected rotation.";
            }
            else
            {
                EXPECT_EQ(testJointNewRotation, testJointRotation) << "Rotation should not change when weight is 0.";
            }
        }
    };

    TEST_P(BlendTreeTwoLinkIKNodeFixture, BendDirectionOutputCorrectPose)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID,
            BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("GoalPosParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALPOS);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("BendDirParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_BENDDIR);
        m_twoLinkIKNode->SetRelativeBendDir(true);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Set up Jack's arm to specific position for testing
        const float weight = testing::get<0>(GetParam());
        const float x = m_param.bendDirPosition[0];
        const float y = m_param.bendDirPosition[1];
        const float z = m_param.bendDirPosition[2];
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("GoalPosParam", AZ::Vector3(x, y, z));
        GetEMotionFX().Update(1.0f / 60.0f);
        
        Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testBendJointIndex;
        AZ::u32 testJointIndex;
        AZStd::string& bendLoArm = m_param.linkedJointNames[0];
        m_jackSkeleton->FindNodeAndIndexByName(bendLoArm, testBendJointIndex);
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);

        const AZ::Vector3 testJointBendPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        // Bend the test joint to opposite positions and check positions are opposite
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(1.0f, 0.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendRightPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(-1.0f, 0.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendLeftPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;
        
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(0.0f, 1.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendDownPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(0.0f, -1.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendUpPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;
        
        if (weight)
        {
            // Z-axis(Height) of tested bend joint should behave correctly
            EXPECT_TRUE(testJointBendDownPos.GetZ() < testJointBendUpPos.GetZ()) << "Height of bent down joint should be lower than bent up joint.";
            EXPECT_TRUE(testJointBendPos.GetZ() < testJointBendUpPos.GetZ()) << "Height of original joint should be lower than bent up joint.";
            EXPECT_TRUE(testJointBendDownPos.GetZ() < testJointBendPos.GetZ()) << "Height of bent down joint should be lower than original joint.";
            
            // X-axis(Horizontal) of tested bend joint should behave correctly
            EXPECT_TRUE(testJointBendLeftPos.GetX() < testJointBendRightPos.GetX()) << "Bent left joint should be on the left of bent right joint.";
            EXPECT_TRUE(testJointBendPos.GetX() < testJointBendRightPos.GetX()) << "Original joint should be on the left of bent right joint.";
            EXPECT_TRUE(testJointBendLeftPos.GetX() < testJointBendPos.GetX()) << "Bent left joint should be on the left of original joint.";
        }
        else
        {
            // Position should not change if weight is 0
            EXPECT_EQ(testJointBendRightPos, testJointBendLeftPos) << "Joint position should not change.";
            EXPECT_EQ(testJointBendUpPos, testJointBendDownPos) << "Joint position should not change.";
        }
    };

    TEST_P(BlendTreeTwoLinkIKNodeFixture, CombinedFunctionsOutputCorrectPose)
    {
        // Two Link IK Node should not break when using all of its functions at the same time
        m_twoLinkIKNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("WeightParam")->mPortID,
            BlendTreeTwoLinkIKNode::INPUTPORT_WEIGHT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("GoalPosParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALPOS);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("RotationParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_GOALROT);
        m_twoLinkIKNode->AddConnection(m_paramNode,
            m_paramNode->FindOutputPortByName("BendDirParam")->mPortID, BlendTreeTwoLinkIKNode::INPUTPORT_BENDDIR);
        m_twoLinkIKNode->SetRotationEnabled(true);
        m_twoLinkIKNode->SetRelativeBendDir(true);
        GetEMotionFX().Update(1.0f / 60.0f);

        const Pose* jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        AZ::u32 testJointIndex;
        AZ::u32 testBendJointIndex;
        AZStd::string& bendLoArm = m_param.linkedJointNames[0];
        m_jackSkeleton->FindNodeAndIndexByName(m_param.testJointName, testJointIndex);
        m_jackSkeleton->FindNodeAndIndexByName(bendLoArm, testBendJointIndex);

        // Adding weight and goal position
        const float weight = testing::get<0>(GetParam());
        const float posX = m_param.bendDirPosition[0];
        const float posY = m_param.bendDirPosition[1];
        const float posZ = m_param.bendDirPosition[2];
        ParamSetValue<MCore::AttributeFloat, float>("WeightParam", weight);
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("GoalPosParam", AZ::Vector3(posX, posY, posZ));
        GetEMotionFX().Update(1.0f / 60.0f);

        // Add bend direction
        const AZ::Vector3 testJointBendPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(1.0f, 0.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendRightPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(-1.0f, 0.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendLeftPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(0.0f, 1.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendDownPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("BendDirParam", AZ::Vector3(0.0f, -1.0f, 0.0f));
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 testJointBendUpPos = jackPose->GetModelSpaceTransform(testBendJointIndex).mPosition;

        // Rotations with bent joint
        const AZ::Quaternion testJointOriginalRotation = jackPose->GetModelSpaceTransform(testJointIndex).mRotation;
        for (std::vector<float> rotateXYZ : m_param.rotations)
        {
            const float rotateX = rotateXYZ[0];
            const float rotateY = rotateXYZ[1];
            const float rotateZ = rotateXYZ[2];
            ParamSetValue<MCore::AttributeQuaternion, AZ::Quaternion>("RotationParam", AZ::Quaternion(rotateX, rotateY, rotateZ, 1.0f));
            GetEMotionFX().Update(1.0f / 60.0f);
            const AZ::Quaternion testJointNewRotation = jackPose->GetModelSpaceTransform(testJointIndex).mRotation;

            if (weight)
            {
                const AZ::Quaternion goalRotation(rotateX, rotateY, rotateZ, 1.0f);
                EXPECT_EQ(testJointNewRotation, goalRotation.GetNormalized()) << "Rotation of the test joint should be the same as the expected rotation.";
            }
            else
            {
                EXPECT_EQ(testJointNewRotation, testJointOriginalRotation) << "Rotation should not change when weight is 0.";
            }
        }

        if (weight)
        {
            // Z-axis(Height) of tested bend joint should behave correctly
            EXPECT_TRUE(testJointBendDownPos.GetZ() < testJointBendUpPos.GetZ()) << "Height of bent down joint should be lower than bent up joint.";
            EXPECT_TRUE(testJointBendPos.GetZ() < testJointBendUpPos.GetZ()) << "Height of original joint should be lower than bent up joint.";
            EXPECT_TRUE(testJointBendDownPos.GetZ() < testJointBendPos.GetZ()) << "Height of bent down joint should be lower than original joint.";

            // X-axis(Horizontal) of tested bend joint should behave correctly
            EXPECT_TRUE(testJointBendLeftPos.GetX() < testJointBendRightPos.GetX()) << "Bent left joint should be on the left of bent right joint.";
            EXPECT_TRUE(testJointBendPos.GetX() < testJointBendRightPos.GetX()) << "Original joint should be on the left of bent right joint.";
            EXPECT_TRUE(testJointBendLeftPos.GetX() < testJointBendPos.GetX()) << "Bent left joint should be on the left of original joint.";
        }
        else
        {
            // Position should not change if weight is 0
            EXPECT_EQ(testJointBendRightPos, testJointBendLeftPos) << "Joint position should not change.";
            EXPECT_EQ(testJointBendUpPos, testJointBendDownPos) << "Joint position should not change.";
        }
    };

    std::vector< BlendTreeTwoLinkIKNodeTestsData> testData
    {
        {
            "l_hand" /* Test joint */, 
            {"l_loArm","l_upArm"} /* Linked joints */,
            {
                // Reachable positions for the joint
                {0.0f, -0.02f, 1.09f},
                {-0.08f, 0.03f, 1.50f},
                {0.08f, 0.03f, 1.50f},
                {0.0f, 0.02f, 1.67f}
            },
            {
                // Unreachable positions for the joint
                {0.0f, 0.0f, 0.0f},
                {0.0f, 10.0f, 0.0f},
                {0.0f, 0.0f, 10.0f},
                {0.0f, 10.0f, 10.0f},
                {10.0f, 0.0f, 0.0f},
                {10.0f, 10.0f, 0.0f},
                {10.0f, 10.0f, 10.0f}
            },
            {
                // Rotations of the joint
                {-90.0f, 180.0f, 0.0f},
                {-540.0f, 0.0f, 1020.0f},
                {0.0f, 0.0f, 0.0f},
                {0.0f, 480.0f, -60.8f},
                {0.0f, -500.0f, 1000.0f},
                {1000.0f, -500.0f, 360.0f},
                {10.0f, 0.0f, -5.0f}
            },
            {0.2f, 0.4f, 1.5f} /* Fixed position of the tested joint for bend direction test */,
            {"spine1", "r_shldr", "l_shldr", "head"} /* Names of align-to node */
        }
        ,
        {
            "r_hand", 
            {"r_loArm","r_upArm"},
            {
                {0.0f, -0.02f, 1.09f},
                {-0.08f, 0.03f, 1.50f},
                {0.08f, 0.03f, 1.50f},
                {0.0f, 0.02f, 1.67f}
            },
            {
                {0.0f, 0.0f, 0.0f},
                {0.0f, 10.0f, 0.0f},
                {0.0f, 0.0f, 10.0f},
                {0.0f, 10.0f, 10.0f},
                {10.0f, 0.0f, 0.0f},
                {10.0f, 10.0f, 0.0f},
                {10.0f, 10.0f, 10.0f}
            },
            {
                {-0.05f, .10f, 0.0f},
                {-.05f, 0.0f, .10f},
                {0.0f, 0.0f, 0.0f},
                {0.0f, .10f, -.05f},
                {0.0f, -.05f, .10f},
                {0.10f, -.05f, 0.0f},
                {.10f, 0.0f, -.05f}
            },
            {-0.2f, 0.4f, 1.5f},
            {"spine1", "r_shldr", "l_shldr", "head"}
        }
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeTwoLinkIKNode_OutputTests,
        BlendTreeTwoLinkIKNodeFixture,
        ::testing::Combine(
            ::testing::Bool(),
            ::testing::ValuesIn(testData)
        )
    );
} // end namespace EMotionFX
