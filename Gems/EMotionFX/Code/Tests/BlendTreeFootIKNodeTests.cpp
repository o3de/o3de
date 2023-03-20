
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "JackGraphFixture.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFootIKNode.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Skeleton.h>
#include <MCore/Source/AzCoreConversions.h>
#include <Integration/AnimationBus.h>
#include <AzCore/Math/Plane.h>

namespace EMotionFX
{
    class BlendTreeFootIKNodeTests
        : public JackGraphFixture
        , private EMotionFX::Integration::IRaycastRequests
    {
    public:
        IRaycastRequests::RaycastResult Raycast(
            [[maybe_unused]] AZ::EntityId entityId, const IRaycastRequests::RaycastRequest& rayRequest) override
        {
            IRaycastRequests::RaycastResult result;

            //
            // z
            //
            // a  xxxxxxxx (z = m_rightSideHeight)
            // x  xxxxxxxx
            // i  xxxxxxxx
            // s  --------|---------- (z = m_leftSideHeight)
            //            0           (x-axis)
            //
            // The right side (as seen from the character's eyes), has a given height, as well as the left side.
            // The split is at x coordinate 0. So everything with a value smaller than 0 will have 'm_rightSideHeight' and everything
            // with with a value >= 0.0f will have a height of 'm_leftSideHeight'.
            const AZ::Vector3 planeNormal = (m_sceneTransform.TransformPoint(AZ::Vector3(0.0f, 0.0f, 1.0f))).GetNormalizedSafe();
            const AZ::Vector3 pointOnUpperPlane = m_sceneTransform.TransformPoint(AZ::Vector3(0.0f, 0.0f, m_rightSideHeight));
            const AZ::Vector3 pointOnLowerPlane = m_sceneTransform.TransformPoint(AZ::Vector3(0.0f, 0.0f, m_leftSideHeight));
            const AZ::Plane upperPlane = AZ::Plane::CreateFromNormalAndPoint(planeNormal, pointOnUpperPlane);
            const AZ::Plane lowerPlane = AZ::Plane::CreateFromNormalAndPoint(planeNormal, pointOnLowerPlane);
            AZ::Vector3 intersectionUpper = AZ::Vector3::CreateZero();
            AZ::Vector3 intersectionLower = AZ::Vector3::CreateZero();
            const bool intersectedUpper = upperPlane.IntersectSegment(rayRequest.m_start, rayRequest.m_start + rayRequest.m_direction * rayRequest.m_distance, intersectionUpper);
            const bool intersectedLower = lowerPlane.IntersectSegment(rayRequest.m_start, rayRequest.m_start + rayRequest.m_direction * rayRequest.m_distance, intersectionLower);

            if (intersectedUpper && intersectionUpper.GetX() < 0)
            {
                result.m_intersected = true;
                result.m_position = intersectionUpper;
            }
            else if (intersectedLower)
            {
                result.m_intersected = true;
                result.m_position = intersectionLower;
            }

            result.m_normal = planeNormal;
            return result;
        }

        void TearDown() override
        {
            JackGraphFixture::TearDown();
            AZ::Interface<IRaycastRequests>::Unregister(this);
        }

        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();

            // Create a weight parameter.
            m_weightParameter = static_cast<FloatSliderParameter*>(ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            m_weightParameter->SetName("IK Weight");
            m_weightParameter->SetDefaultValue(1.0f);
            m_animGraph->AddParameter(m_weightParameter);

            // Create the blend tree.
            BlendTree* blendTree = aznew BlendTree();
            m_animGraph->GetRootStateMachine()->AddChildNode(blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(blendTree);

            // Add a final node.
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            blendTree->AddChildNode(finalNode);

            // Add a foot IK node and connect it to the final node.
            m_ikNode = aznew BlendTreeFootIKNode();
            m_ikNode->SetForceUseRaycastBus(true);
            m_ikNode->SetLeftFootJointName(s_leftFootJointName);
            m_ikNode->SetRightFootJointName(s_rightFootJointName);
            m_ikNode->SetLeftToeJointName(s_leftToeJointName);
            m_ikNode->SetRightToeJointName(s_rightToeJointName);
            m_ikNode->SetHipJointName(s_hipJointName);
            blendTree->AddChildNode(m_ikNode);
            finalNode->AddConnection(m_ikNode, BlendTreeFootIKNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);

            // Create the parameter node.
            m_parameterNode = aznew BlendTreeParameterNode();
            blendTree->AddChildNode(m_parameterNode);

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            blendTree->AddChildNode(bindPoseNode);
            m_ikNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::OUTPUTPORT_RESULT, BlendTreeFootIKNode::INPUTPORT_POSE);

            // Connect the weight parameter to the weight of the IK node.
            m_ikNode->AddUnitializedConnection(m_parameterNode, 0 /* Weight parameter */, BlendTreeFootIKNode::INPUTPORT_WEIGHT);
        }

        void SetUp() override
        {
            JackGraphFixture::SetUp();
            
            // Disable raycasts in other handlers, and take over control (muahhahaha *evil laugh*).
            AZ::Interface<Integration::IRaycastRequests>::Get()->DisableRayRequests();
            AZ::Interface<Integration::IRaycastRequests>::Register(this);
        }

        void ValidateFootHeight(BlendTreeFootIKNode::LegId legId, const char* jointName, float height, float tolerance)
        {
            // Check the left foot height.
            size_t footIndex = InvalidIndex;
            Skeleton* skeleton = m_actor->GetSkeleton();
            skeleton->FindNodeAndIndexByName(jointName, footIndex);
            ASSERT_NE(footIndex, InvalidIndex);
            EMotionFX::Transform transform = m_actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(footIndex);
            const BlendTreeFootIKNode::UniqueData* uniqueData = static_cast<const BlendTreeFootIKNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_ikNode));
            const float correction = (m_actorInstance->GetWorldSpaceTransform().m_rotation.TransformVector(AZ::Vector3(0.0f, 0.0f, uniqueData->m_legs[legId].m_footHeight))).GetZ();
            const float pos = transform.m_position.GetZ() - correction;
            EXPECT_NEAR(pos, height, tolerance);
        }

        void ValidateFeetHeights(float leftFootHeight, float rightFootHeight, float tolerance=0.02f)
        {
            ValidateFootHeight(BlendTreeFootIKNode::LegId::Left, BlendTreeFootIKNodeTests::s_leftFootJointName, leftFootHeight, tolerance);
            ValidateFootHeight(BlendTreeFootIKNode::LegId::Right, BlendTreeFootIKNodeTests::s_rightFootJointName, rightFootHeight, tolerance);
        }

        void ResetActorInstanceTransform()
        {
            EMotionFX::Transform transform;
            transform.Identity();
            m_actorInstance->SetLocalSpaceTransform(transform);
        }

        void SetSceneHeights(float leftSideHeight, float rightSideHeight)
        {
            m_leftSideHeight = leftSideHeight;
            m_rightSideHeight = rightSideHeight;
        }

        void SimulateFrames(size_t numFrames=60)
        {
            for (size_t i = 0; i < numFrames; ++i)
            {
                Evaluate(1.0f / 60.0f);
            }
        }

        void SetSceneTransform(const AZ::Transform& transform)
        {
            m_sceneTransform = transform;
        }

    public:
        static const char*      s_leftFootJointName;
        static const char*      s_leftToeJointName;
        static const char*      s_rightFootJointName;
        static const char*      s_rightToeJointName;
        static const char*      s_hipJointName;

        BlendTreeFootIKNode*    m_ikNode            = nullptr;
        BlendTreeParameterNode* m_parameterNode     = nullptr;
        FloatSliderParameter*   m_weightParameter   = nullptr;
        AZ::Transform           m_sceneTransform    = AZ::Transform::CreateIdentity();
        float                   m_leftSideHeight    = 0.0f;
        float                   m_rightSideHeight   = 0.3f;
    };

    const char* BlendTreeFootIKNodeTests::s_leftFootJointName   = "l_ankle";
    const char* BlendTreeFootIKNodeTests::s_leftToeJointName    = "l_ball";
    const char* BlendTreeFootIKNodeTests::s_rightFootJointName  = "r_ankle";
    const char* BlendTreeFootIKNodeTests::s_rightToeJointName   = "r_ball";
    const char* BlendTreeFootIKNodeTests::s_hipJointName        = "Bip01__pelvis";
        
    TEST_F(BlendTreeFootIKNodeTests, Integrity)
    {
        // Process a bunch of frames, so that we are sure the IK fully blended in.
        SimulateFrames();

        // Do some integrity checks.
        BlendTreeFootIKNode::UniqueData* uniqueData = static_cast<BlendTreeFootIKNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_ikNode));
        ASSERT_TRUE(uniqueData != nullptr);
        ASSERT_TRUE(!uniqueData->GetHasError());
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_jointIndices[BlendTreeFootIKNode::LegJointId::UpperLeg], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_jointIndices[BlendTreeFootIKNode::LegJointId::Knee], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_jointIndices[BlendTreeFootIKNode::LegJointId::Foot], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_jointIndices[BlendTreeFootIKNode::LegJointId::Toe], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_jointIndices[BlendTreeFootIKNode::LegJointId::UpperLeg], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_jointIndices[BlendTreeFootIKNode::LegJointId::Knee], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_jointIndices[BlendTreeFootIKNode::LegJointId::Foot], InvalidIndex);
        ASSERT_NE(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_jointIndices[BlendTreeFootIKNode::LegJointId::Toe], InvalidIndex);
        ASSERT_NE(uniqueData->m_hipJointIndex, InvalidIndex);

        // Make sure the weights are fully active.
        ASSERT_FLOAT_EQ(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_weight, 1.0f);
        ASSERT_FLOAT_EQ(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_weight, 1.0f);

        // Make sure the leg length is about correct.
        EXPECT_NEAR(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_legLength, 0.898f, 0.003f);
        EXPECT_NEAR(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_legLength, 0.898f, 0.003f);

        // Check the foot height offset.
        EXPECT_NEAR(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Left].m_footHeight, 0.093f, 0.002f);
        EXPECT_NEAR(uniqueData->m_legs[BlendTreeFootIKNode::LegId::Right].m_footHeight, 0.093f, 0.002f);
    }

    TEST_F(BlendTreeFootIKNodeTests, LegIK)
    {
        // Disable hip adjustment.
        m_ikNode->SetAdjustHip(false);

        // Tests where the leg can reach the target position just fine.
        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.3f);

        SetSceneHeights(0.0f, 0.1f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.1f);

        SetSceneHeights(0.0f, 0.4f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.4f);

        SetSceneHeights(0.0f, 0.0f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);

        // Unreachable left leg, would need hip adjustment.
        SetSceneHeights(-0.3f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.3f);

        // Right leg is unreachable.
        SetSceneHeights(0.0f, 3.0f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);
    }

    TEST_F(BlendTreeFootIKNodeTests, HipAdjustment)
    {
        // Enable hip adjustment.
        m_ikNode->SetAdjustHip(true);

        EMotionFX::Transform transform;
        transform.Identity();
        m_actorInstance->SetLocalSpaceTransform(transform);

        // Tests where the leg can reach the target position just fine, make sure the hip adjustment doesn't break it.
        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.3f);

        SetSceneHeights(0.0f, 0.1f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.1f);

        SetSceneHeights(0.0f, 0.4f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.4f);

        SetSceneHeights(0.0f, 0.0f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);

        // Normally unreachable left leg, would need hip adjustment.
        // Hip adjustment is now enabled, so we should be able to reach it.
        SetSceneHeights(-0.3f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(-0.3f, 0.3f);

        SetSceneHeights(-0.3f, 0.0f);
        SimulateFrames();
        ValidateFeetHeights(-0.3f, 0.0f);
      
        SetSceneHeights(-m_ikNode->GetMaxHipAdjustment(), 0.0f);
        SimulateFrames();
        ValidateFeetHeights(-m_ikNode->GetMaxHipAdjustment(), 0.0f);

        // Right leg is unreachable. Even moving the hips, as it is just too far.
        SetSceneHeights(-2.0f, 0.0f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);
    }

    TEST_F(BlendTreeFootIKNodeTests, UpVectorSupport)
    {
        // Disable hip adjustment.
        m_ikNode->SetAdjustHip(false);

        // Rotate our scene.
        // Flip it 180 degrees over the x axis. Please keep in mind our tests only work with 180 degrees flipped or not.
        // A more robust method would be needed to really test it correctly otherwise.
        AZ::Transform sceneTransform = AZ::Transform::CreateRotationX(MCore::Math::pi);
        SetSceneTransform(sceneTransform);

        // Rotate the actor instance 180 degrees over the X axis as well.
        EMotionFX::Transform transform;
        transform.Identity();
        transform.m_rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::pi);
        m_actorInstance->SetLocalSpaceTransform(transform);

        // Tests where the leg can reach the target position just fine, make sure the hip adjustment doesn't break it.
        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, -0.3f);

        SetSceneHeights(0.0f, 0.2f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, -0.2f);

        SetSceneHeights(0.0f, 0.4f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, -0.4f);

        // Now check with hips included.
        // Enable hip adjustment.
        m_ikNode->SetAdjustHip(true);
        SetSceneHeights(-0.2f, 0.4f);
        SimulateFrames();
        ValidateFeetHeights(0.2f, -0.4f);

        SetSceneHeights(-0.4f, 0.0f);
        SimulateFrames();
        ValidateFeetHeights(0.4f, 0.0f);
    }

    TEST_F(BlendTreeFootIKNodeTests, InputWeight)
    {
        MCore::AttributeFloat* weightParam = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);
        weightParam->SetValue(1.0f);

        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.3f);

        // This should disable the IK.
        weightParam->SetValue(0.0f);

        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);
    }

    TEST_F(BlendTreeFootIKNodeTests, DisabledState)
    {
        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.3f);

        // Disable the node. It should now act as passthrough.
        m_ikNode->SetIsEnabled(false);

        SetSceneHeights(0.0f, 0.3f);
        SimulateFrames();
        ValidateFeetHeights(0.0f, 0.0f);
    }

} // namespace EMotionFX
