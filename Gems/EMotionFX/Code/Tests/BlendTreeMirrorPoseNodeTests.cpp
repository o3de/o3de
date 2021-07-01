/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeMirrorPoseNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Skeleton.h>
#include <Tests/JackGraphFixture.h>

namespace EMotionFX
{
    class BlendTreeMirrorPoseNodeFixture 
        : public JackGraphFixture
    {
    public:
        void SetupMirrorNodes(const Node* leftNode, const Node* rightNode)
        {
            m_actor->GetNodeMirrorInfo(leftNode->GetNodeIndex()).mSourceNode = static_cast<uint16>(rightNode->GetNodeIndex());
            m_actor->GetNodeMirrorInfo(rightNode->GetNodeIndex()).mSourceNode = static_cast<uint16>(leftNode->GetNodeIndex());
        }

        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();

            // MirrorSetup for essential nodes in tests
            m_actor->AllocateNodeMirrorInfos();
            m_jackSkeleton = m_actor->GetSkeleton();
            const Node* lUpArmNode = m_jackSkeleton->FindNodeByName("l_upArm");
            const Node* rUpArmNode = m_jackSkeleton->FindNodeByName("r_upArm");
            const Node* lShldrNode = m_jackSkeleton->FindNodeByName("l_shldr");
            const Node* rShldrNode = m_jackSkeleton->FindNodeByName("r_shldr");
            const Node* lLoArmNode = m_jackSkeleton->FindNodeByName("l_loArm");
            const Node* rLoArmNode = m_jackSkeleton->FindNodeByName("r_loArm");
            const Node* lHandNode = m_jackSkeleton->FindNodeByName("l_hand");
            const Node* rHandNode = m_jackSkeleton->FindNodeByName("r_hand");
            SetupMirrorNodes(lUpArmNode, rUpArmNode);
            SetupMirrorNodes(lShldrNode, rShldrNode);
            SetupMirrorNodes(lLoArmNode, rLoArmNode);
            SetupMirrorNodes(lHandNode, rHandNode);
            m_actor->AutoDetectMirrorAxes();
        
            // Import a motion of Jack centered in the axis, with right arm raised up
            const AZStd::string base64MotionData = "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";
            AZStd::vector<AZ::u8> skeletalMotiondata;
            AzFramework::StringFunc::Base64::Decode(skeletalMotiondata, base64MotionData.c_str(), base64MotionData.size());
            Motion* rightAramUpMotion = EMotionFX::GetImporter().LoadMotion(skeletalMotiondata.begin(), skeletalMotiondata.size());
            EMotionFX::MotionSet::MotionEntry* rightAramUpmotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            rightAramUpmotionEntry->SetMotion(rightAramUpMotion);
            m_motionSet->AddMotionEntry(rightAramUpmotionEntry);
            m_motionSet->SetMotionEntryId(rightAramUpmotionEntry, "jack_right_arm_up");

            /*
            m_blendtree:
            +--------------------+
            |m_motionNode        +---+
            |                    |   |
            +--------------------+   |   +--------------------+      +--------------------+
                                     +-->+m_mirrorPoseNode    +----->+m_finalNode         |
                                     +-->+                    |      |                    |
            +--------------------+   |   +--------------------+      +--------------------+
            |m_floatConstantNode |   |
            |                    +---+
            +--------------------+
            */
            AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
            motionNode->AddMotionId( "jack_right_arm_up" );

            m_floatConstantNode = aznew BlendTreeFloatConstantNode();
            m_mirrorPoseNode = aznew BlendTreeMirrorPoseNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(motionNode);
            m_blendTree->AddChildNode(m_floatConstantNode);
            m_blendTree->AddChildNode(m_mirrorPoseNode);
            m_blendTree->AddChildNode(finalNode);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);
            
            m_mirrorPoseNode->AddConnection(motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeMirrorPoseNode::INPUTPORT_POSE);
            m_mirrorPoseNode->AddConnection(m_floatConstantNode, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendTreeMirrorPoseNode::INPUTPORT_ENABLED);
            finalNode->AddConnection(m_mirrorPoseNode, BlendTreeMirrorPoseNode::OUTPUTPORT_RESULT, BlendTreeFinalNode::INPUTPORT_POSE);
        };

    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatConstantNode = nullptr;
        BlendTreeMirrorPoseNode* m_mirrorPoseNode = nullptr;
        Skeleton* m_jackSkeleton = nullptr;
    };

    TEST_F(BlendTreeMirrorPoseNodeFixture, OutputsCorrectPose)
    {
        GetEMotionFX().Update(1.0f / 60.0f);
        AZ::u32 l_upArmIndex;
        AZ::u32 r_upArmIndex;
        AZ::u32 l_loArmIndex;
        AZ::u32 r_loArmIndex;
        AZ::u32 l_handIndex;
        AZ::u32 r_handIndex;
        m_jackSkeleton->FindNodeAndIndexByName("l_upArm", l_upArmIndex);
        m_jackSkeleton->FindNodeAndIndexByName("r_upArm", r_upArmIndex);
        m_jackSkeleton->FindNodeAndIndexByName("l_loArm", l_loArmIndex);
        m_jackSkeleton->FindNodeAndIndexByName("r_loArm", r_loArmIndex);
        m_jackSkeleton->FindNodeAndIndexByName("l_hand", l_handIndex);
        m_jackSkeleton->FindNodeAndIndexByName("r_hand", r_handIndex);

        // Mirror Pose Node not enabled
        m_floatConstantNode->SetValue(0.0f);

        // Remember the original position for comparison later
        Pose * jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();
        const AZ::Vector3 l_upArmOriginalPos = jackPose->GetModelSpaceTransform(l_upArmIndex).mPosition;
        const AZ::Vector3 r_upArmOriginalPos = jackPose->GetModelSpaceTransform(r_upArmIndex).mPosition;
        GetEMotionFX().Update(1.0f / 60.0f);

        // Remember mirrored position
        const AZ::Vector3 l_upArmMirroredPos = jackPose->GetModelSpaceTransform(l_upArmIndex).mPosition;
        const AZ::Vector3 r_upArmMirroredPos = jackPose->GetModelSpaceTransform(r_upArmIndex).mPosition;
        
        // Expect poses to be at the same position because mirror pose node is off
        EXPECT_FALSE(m_mirrorPoseNode->GetIsMirroringEnabled(m_animGraphInstance));
        EXPECT_TRUE(l_upArmOriginalPos == l_upArmMirroredPos);
        EXPECT_TRUE(r_upArmOriginalPos == r_upArmMirroredPos);

        // Mirror Pose Node enabled
        m_floatConstantNode->SetValue(1.0f);
        const AZ::Vector3 l_upArmPos = jackPose->GetModelSpaceTransform(l_upArmIndex).mPosition;
        const AZ::Vector3 r_upArmPos = jackPose->GetModelSpaceTransform(r_upArmIndex).mPosition;
        const AZ::Vector3 l_loArmPos = jackPose->GetModelSpaceTransform(l_loArmIndex).mPosition;
        const AZ::Vector3 r_loArmPos = jackPose->GetModelSpaceTransform(r_loArmIndex).mPosition;
        const AZ::Vector3 l_handPos = jackPose->GetModelSpaceTransform(l_handIndex).mPosition;
        const AZ::Vector3 r_handPos = jackPose->GetModelSpaceTransform(r_handIndex).mPosition;
        GetEMotionFX().Update(1.0f / 60.0f);
        const AZ::Vector3 mirroredl_upArmPos = jackPose->GetModelSpaceTransform(l_upArmIndex).mPosition;
        const AZ::Vector3 mirroredr_upArmPos = jackPose->GetModelSpaceTransform(r_upArmIndex).mPosition;
        const AZ::Vector3 mirroredl_loArmPos = jackPose->GetModelSpaceTransform(l_loArmIndex).mPosition;
        const AZ::Vector3 mirroredr_loArmPos = jackPose->GetModelSpaceTransform(r_loArmIndex).mPosition;
        const AZ::Vector3 mirroredl_handPos = jackPose->GetModelSpaceTransform(l_handIndex).mPosition;
        const AZ::Vector3 mirroredr_handPos = jackPose->GetModelSpaceTransform(r_handIndex).mPosition;

        EXPECT_TRUE(m_mirrorPoseNode->GetIsMirroringEnabled(m_animGraphInstance));

        // If mirrored positions correctly changed, then mirror pose node is properly functioning.
        // Original position: Vector3(-x,y,z), mirrored across x-axis
        // Mirrored position: Vector3(x,y,z)
        EXPECT_NEAR(mirroredl_upArmPos.GetX(), -r_upArmPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_upArmPos.GetY(), r_upArmPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_upArmPos.GetZ(), r_upArmPos.GetZ(), AZ::Constants::Tolerance);

        EXPECT_NEAR(mirroredr_upArmPos.GetX(), -l_upArmPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_upArmPos.GetY(), l_upArmPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_upArmPos.GetZ(), l_upArmPos.GetZ(), AZ::Constants::Tolerance);

        EXPECT_NEAR(mirroredl_loArmPos.GetX(), -r_loArmPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_loArmPos.GetY(), r_loArmPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_loArmPos.GetZ(), r_loArmPos.GetZ(), AZ::Constants::Tolerance);

        EXPECT_NEAR(mirroredr_loArmPos.GetX(), -l_loArmPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_loArmPos.GetY(), l_loArmPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_loArmPos.GetZ(), l_loArmPos.GetZ(), AZ::Constants::Tolerance);

        EXPECT_NEAR(mirroredl_handPos.GetX(), -r_handPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_handPos.GetY(), r_handPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredl_handPos.GetZ(), r_handPos.GetZ(), AZ::Constants::Tolerance);

        EXPECT_NEAR(mirroredr_handPos.GetX(), -l_handPos.GetX(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_handPos.GetY(), l_handPos.GetY(), AZ::Constants::Tolerance);
        EXPECT_NEAR(mirroredr_handPos.GetZ(), l_handPos.GetZ(), AZ::Constants::Tolerance);
    };
}// end namespace EMotionFX
