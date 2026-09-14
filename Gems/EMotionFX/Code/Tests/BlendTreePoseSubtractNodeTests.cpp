/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2AdditiveNode.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreePoseSubtractNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/Matchers.h>

namespace EMotionFX
{
    class BlendTreePoseSubtractNodeFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            BlendTree* blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            AnimGraphMotionNode* basePoseNode = aznew AnimGraphMotionNode();
            basePoseNode->AddMotionId("basePose");
            blendTree->AddChildNode(basePoseNode);
            AnimGraphMotionNode* animatedPoseNode = aznew AnimGraphMotionNode();
            animatedPoseNode->AddMotionId("animatedPose");
            blendTree->AddChildNode(animatedPoseNode);
            BlendTreePoseSubtractNode* poseSubtractNode = aznew BlendTreePoseSubtractNode();
            blendTree->AddChildNode(poseSubtractNode);
            BlendTreeFloatConstantNode* weightNode = aznew BlendTreeFloatConstantNode();
            weightNode->SetValue(1.0f);
            blendTree->AddChildNode(weightNode);
            BlendTreeBlend2AdditiveNode* blendAdditiveNode = aznew BlendTreeBlend2AdditiveNode();
            blendTree->AddChildNode(blendAdditiveNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            blendTree->AddChildNode(finalNode);

            poseSubtractNode->AddConnection(animatedPoseNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreePoseSubtractNode::INPUTPORT_POSE_A);
            poseSubtractNode->AddConnection(basePoseNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreePoseSubtractNode::INPUTPORT_POSE_B);
            blendAdditiveNode->AddConnection(basePoseNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreeBlend2AdditiveNode::INPUTPORT_POSE_A);
            blendAdditiveNode->AddConnection(poseSubtractNode, BlendTreePoseSubtractNode::PORTID_OUTPUT_POSE, BlendTreeBlend2AdditiveNode::INPUTPORT_POSE_B);
            blendAdditiveNode->AddConnection(weightNode, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeBlend2AdditiveNode::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blendAdditiveNode, BlendTreeBlend2AdditiveNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            AddStaticMotion("basePose", m_basePose);
            AddStaticMotion("animatedPose", m_animatedPose);
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        void AddStaticMotion(const char* motionId, const Transform& rootJointTransform)
        {
            Motion* motion = aznew Motion(motionId);
            motion->SetMotionData(aznew NonUniformMotionData());
            motion->GetMotionData()->AddJoint("rootJoint", rootJointTransform, Transform::CreateIdentity());
            motion->GetMotionData()->SetDuration(1.0f);
            m_motionSet->AddMotionEntry(aznew MotionSet::MotionEntry(motionId, motionId, motion));
        }

        const Transform m_basePose{ AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion::CreateRotationZ(1.0f), AZ::Vector3(2.0f, 2.0f, 2.0f) };
        const Transform m_animatedPose{ AZ::Vector3(4.0f, -1.0f, 0.5f), AZ::Quaternion::CreateRotationX(0.5f), AZ::Vector3(1.5f, 0.5f, 1.0f) };
    };

    TEST_F(BlendTreePoseSubtractNodeFixture, AddingSubtractedPoseBackRestoresInputPose)
    {
        GetEMotionFX().Update(0.0f);

        const Pose* outputPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        EXPECT_THAT(outputPose->GetLocalSpaceTransform(0), IsClose(m_animatedPose));
    }
} // namespace EMotionFX
