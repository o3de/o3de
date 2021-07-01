/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectPoint.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>
#include <Tests/BlendSpaceFixture.h>

namespace EMotionFX
{
    class BlendSpaceParamEvaluatorFixture
        : public BlendSpaceFixture
    {
    public:
        void TearDown() override
        {
            m_motionInstance->Destroy();
            m_evaluator->Destroy();
            BlendSpaceFixture::TearDown();
        }

        void PrepareTest(Motion* motion, BlendSpaceParamEvaluator* evaluator)
        {
            ASSERT_NE(motion, nullptr) << "Expected a valid motion.";
            m_motionInstance = MotionInstance::Create(motion, m_actorInstance);

            m_evaluator = evaluator;
            m_value = m_evaluator->ComputeParamValue(*m_motionInstance);
        }

    public:
        MotionInstance* m_motionInstance = nullptr;
        BlendSpaceParamEvaluator* m_evaluator = nullptr;
        float m_value = -1.0f;
    };

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceParamEvaluatorNoneTest)
    {
        PrepareTest(m_idleMotion, aznew BlendSpaceParamEvaluatorNone());
        EXPECT_FLOAT_EQ(m_value, 0.0f) << "Expected 0.0 from the none evaluator.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceMoveSpeedParamEvaluatorTest)
    {
        PrepareTest(m_forwardMotion, aznew BlendSpaceMoveSpeedParamEvaluator());
        EXPECT_FLOAT_EQ(m_value, 1.0f) << "Expected a move speed of 1.0 unit per second.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceTurnSpeedParamEvaluatorTest)
    {
        PrepareTest(m_rotateLeftMotion, aznew BlendSpaceTurnSpeedParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, -0.5f, 0.001f))
            << "Expected a turn speed of -0.5 radians per second. Negative because we prefer the convention of clockwise being positive turn speed.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceTravelDirectionParamEvaluatorTest)
    {
        PrepareTest(m_forwardStrafe45, aznew BlendSpaceTravelDirectionParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, AZ::DegToRad(45.0f), 0.001f))
            << "Expected a travel direction of 45 degrees.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceTravelSlopeParamEvaluatorTest)
    {
        PrepareTest(m_forwardSlope45, aznew BlendSpaceTravelSlopeParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, AZ::DegToRad(45.0f), 0.001f))
            << "";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceTurnAngleParamEvaluatorTest)
    {
        PrepareTest(m_rotateLeftMotion, aznew BlendSpaceTurnAngleParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, -0.5f, 0.001f))
            << "Expected a turn angle of -0.5 radians over the full motion.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceTravelDistanceParamEvaluatorTest)
    {
        PrepareTest(m_forwardStrafe45, aznew BlendSpaceTravelDistanceParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, AZ::Vector2(1.0f, 1.0f).GetLength(), 0.001f))
            << "Expected travel distance of Vec3(1.0, 1.0).Length().";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceLeftRightVelocityParamEvaluatorTest)
    {
        PrepareTest(m_strafeMotion, aznew BlendSpaceLeftRightVelocityParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, 1.0f, 0.001f))
            << "Expected strafe velocity 1.0 units per second.";
    }

    TEST_F(BlendSpaceParamEvaluatorFixture, BlendSpaceFrontBackVelocityParamEvaluatorTest)
    {
        PrepareTest(m_forwardMotion, aznew BlendSpaceFrontBackVelocityParamEvaluator());
        EXPECT_TRUE(AZ::IsClose(m_value, 1.0f, 0.001f))
            << "Expected forward velocity 1.0 units per second.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    TEST_F(BlendSpace1DFixture, MotionCoordinatesTest)
    {
        const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions = m_blendSpace1DNode->GetMotions();
        BlendSpace1DNode::UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(m_blendSpace1DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        EXPECT_EQ(motions.size(), 3);
        EXPECT_EQ(uniqueData->m_motionCoordinates.size(), 3);
        EXPECT_EQ(uniqueData->m_motionCoordinates[0], 0.0f);
        EXPECT_EQ(uniqueData->m_motionCoordinates[1], 1.0f);
        EXPECT_EQ(uniqueData->m_motionCoordinates[2], 2.0f);
    }

    TEST_F(BlendSpace1DFixture, EvaluationOutOfBounds)
    {
        m_floatNodeX->SetValue(-1.0f);
        GetEMotionFX().Update(0.1f);

        BlendSpace1DNode::UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(m_blendSpace1DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        EXPECT_EQ(uniqueData->m_currentPosition, -1.0f);
        EXPECT_EQ(uniqueData->m_blendInfos.size(), 1) << "Expected to only have one fully active motion.";
        EXPECT_EQ(uniqueData->m_currentSegment.m_segmentIndex, InvalidIndex32);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_motionIndex, 0);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_weight, 1.0f);
    }

    TEST_F(BlendSpace1DFixture, EvaluationOnMotionPoint)
    {
        m_floatNodeX->SetValue(0.0f);
        GetEMotionFX().Update(0.1f);

        BlendSpace1DNode::UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(m_blendSpace1DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        EXPECT_EQ(uniqueData->m_currentPosition, 0.0f);
        EXPECT_EQ(uniqueData->m_currentSegment.m_segmentIndex, 0);
        EXPECT_EQ(uniqueData->m_blendInfos.size(), 2);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_motionIndex, 0);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_weight, 1.0f);
        EXPECT_EQ(uniqueData->m_blendInfos[1].m_motionIndex, 1);
        EXPECT_EQ(uniqueData->m_blendInfos[1].m_weight, 0.0f);
    }

    TEST_F(BlendSpace1DFixture, EvaluationInsideSegment)
    {
        m_floatNodeX->SetValue(0.5f);
        GetEMotionFX().Update(0.1f);

        BlendSpace1DNode::UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(m_blendSpace1DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        EXPECT_EQ(uniqueData->m_currentPosition, 0.5f);
        EXPECT_EQ(uniqueData->m_blendInfos.size(), 2);
        EXPECT_EQ(uniqueData->m_currentSegment.m_segmentIndex, 0);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_motionIndex, 0);
        EXPECT_EQ(uniqueData->m_blendInfos[0].m_weight, 0.5f);
        EXPECT_EQ(uniqueData->m_blendInfos[1].m_motionIndex, 1);
        EXPECT_EQ(uniqueData->m_blendInfos[1].m_weight, 0.5f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_OnMotionPoint_MostActive)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_OnMotionPoint_AllActive)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_OnMotionPoint_None)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_NONE);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(0);

        m_floatNodeX->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_InsideSegment_MostActive)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.5f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_InsideSegment_AllActive)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(2);

        m_floatNodeX->SetValue(0.5f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, EventsTest_InsideSegment_None)
    {
        m_blendSpace1DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_NONE);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(0);

        m_floatNodeX->SetValue(0.5f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace1DFixture, ComputeMotionCoordinates_Nullptr_Asserts_FT)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;

        AZ::Vector2 testVec(0.0f, 0.0f);

        m_blendSpace1DNode->ComputeMotionCoordinates("", nullptr, testVec);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    TEST_F(BlendSpace2DFixture, MotionCoordinatesTest)
    {
        const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions = m_blendSpace2DNode->GetMotions();
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));

        EXPECT_EQ(motions.size(), 4);
        EXPECT_EQ(uniqueData->m_motionCoordinates.size(), 4);

        EXPECT_EQ(uniqueData->m_motionCoordinates[0], AZ::Vector2(0.0f, 0.0f));
        EXPECT_EQ(uniqueData->m_motionCoordinates[1], AZ::Vector2(1.0f, 0.0f));
        EXPECT_EQ(uniqueData->m_motionCoordinates[2], AZ::Vector2(2.0f, 0.0f));
        EXPECT_EQ(uniqueData->m_motionCoordinates[3], AZ::Vector2(1.0f, 1.0f));

        EXPECT_EQ(uniqueData->m_normMotionPositions[0], AZ::Vector2(-1.0f, -1.0f));
        EXPECT_EQ(uniqueData->m_normMotionPositions[1], AZ::Vector2(0.0f, -1.0f));
        EXPECT_EQ(uniqueData->m_normMotionPositions[2], AZ::Vector2(1.0f, -1.0f));
        EXPECT_EQ(uniqueData->m_normMotionPositions[3], AZ::Vector2(0.0f, 1.0f));

        EXPECT_EQ(uniqueData->m_rangeMin, AZ::Vector2(0.0f, 0.0f));
        EXPECT_EQ(uniqueData->m_rangeMax, AZ::Vector2(2.0f, 1.0f));
        EXPECT_EQ(uniqueData->m_rangeCenter, AZ::Vector2(1.0f, 0.5f));
        EXPECT_EQ(uniqueData->m_normalizationScale, AZ::Vector2(1.0f, 2.0f));
    }

    TEST_F(BlendSpace2DFixture, TriangulationTest)
    {
        const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions = m_blendSpace2DNode->GetMotions();
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        EXPECT_EQ(motions.size(), 4);
        EXPECT_EQ(uniqueData->m_triangles.size(), 2);

        // run      2 *
        //            |\
        //            | \
        //            |  \
        //            |   \
        //            |    \
        // forward  1 *     * 3 Strafe
        //            |    /
        //            |   /
        //            |  /
        //            | /
        //            |/
        // idle     0 *
        EXPECT_EQ(uniqueData->m_triangles[0], BlendSpace2DNode::Triangle(1, 0, 3));
        EXPECT_EQ(uniqueData->m_triangles[1], BlendSpace2DNode::Triangle(2, 1, 3));
    }

    TEST_F(BlendSpace2DFixture, EvaluationAtMotionPoint)
    {
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));

        // Idle corner point
        m_floatNodeX->SetValue(0.0f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.1f);

        EXPECT_EQ(uniqueData->m_currentTriangle.m_triangleIndex, 0);
        EXPECT_EQ(uniqueData->m_currentEdge.m_edgeIndex, InvalidIndex32);
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[0], 0.0f);
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[1], 1.0f); // Idle is the second point
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[2], 0.0f);
        EXPECT_EQ(uniqueData->m_currentPosition, AZ::Vector2(0.0f, 0.0f));
        EXPECT_EQ(uniqueData->m_normCurrentPosition, AZ::Vector2(-1.0f, -1.0f));
    }

    TEST_F(BlendSpace2DFixture, EvaluationOnTriangleEdge)
    {
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));

        // Edge between idle and forward
        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.1f);

        EXPECT_EQ(uniqueData->m_currentTriangle.m_triangleIndex, 0);
        EXPECT_EQ(uniqueData->m_currentEdge.m_edgeIndex, InvalidIndex32);
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[0], 0.5f); // Forward
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[1], 0.5f); // Idle
        EXPECT_EQ(uniqueData->m_currentTriangle.m_weights[2], 0.0f);
        EXPECT_EQ(uniqueData->m_currentPosition, AZ::Vector2(0.5f, 0.0f));
        EXPECT_EQ(uniqueData->m_normCurrentPosition, AZ::Vector2(-0.5f, -1.0f));
    }

    TEST_F(BlendSpace2DFixture, EvaluationOutsideOfEdge)
    {
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(-1.0f);
        GetEMotionFX().Update(0.1f);

        EXPECT_EQ(uniqueData->m_currentTriangle.m_triangleIndex, InvalidIndex32);
        EXPECT_EQ(uniqueData->m_currentEdge.m_edgeIndex, 3);
        // In case the evaluation point is on an edge, m_u will be used as interpolation weight.
        EXPECT_EQ(uniqueData->m_currentEdge.m_u, 0.5f); // Mid between forward and idle edge
        EXPECT_EQ(uniqueData->m_currentPosition, AZ::Vector2(0.5f, -1.0f));
        EXPECT_EQ(uniqueData->m_normCurrentPosition, AZ::Vector2(-0.5f, -3.0f));
    }

    TEST_F(BlendSpace2DFixture, EvaluationInsideTriangle)
    {
        BlendSpace2DNode::UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(m_blendSpace2DNode->FindOrCreateUniqueNodeData(m_animGraphInstance));

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.25f);
        GetEMotionFX().Update(0.1f);

        EXPECT_EQ(uniqueData->m_currentTriangle.m_triangleIndex, 0);
        EXPECT_EQ(uniqueData->m_currentEdge.m_edgeIndex, InvalidIndex32);
        EXPECT_EQ(uniqueData->m_currentPosition, AZ::Vector2(0.5f, 0.25f));
        EXPECT_EQ(uniqueData->m_normCurrentPosition, AZ::Vector2(-0.5f, -0.5f));

        const uint16_t* indices = uniqueData->m_triangles[0].m_vertIndices;
        const AZ::Vector2& a = uniqueData->m_normMotionPositions[indices[0]];
        const AZ::Vector2& b = uniqueData->m_normMotionPositions[indices[1]];
        const AZ::Vector2& c = uniqueData->m_normMotionPositions[indices[2]];
        const AZ::Vector2& p = uniqueData->m_normCurrentPosition;

        const AZ::Vector3 barycentricCoordinates = AZ::Intersect::Barycentric(
            AZ::Vector3(a.GetX(), a.GetY(), 0.0f),
            AZ::Vector3(b.GetX(), b.GetY(), 0.0f),
            AZ::Vector3(c.GetX(), c.GetY(), 0.0f),
            AZ::Vector3(p.GetX(), p.GetY(), 0.0f));

        EXPECT_TRUE(AZ::IsClose(barycentricCoordinates.GetX(), 0.25f, 0.001f));
        EXPECT_TRUE(AZ::IsClose(barycentricCoordinates.GetY(), 0.5f, 0.001f));
        EXPECT_TRUE(AZ::IsClose(barycentricCoordinates.GetZ(), 0.25f, 0.001f));
        EXPECT_TRUE(AZ::IsClose(uniqueData->m_currentTriangle.m_weights[0], barycentricCoordinates.GetX(), 0.001f)); // Idle
        EXPECT_TRUE(AZ::IsClose(uniqueData->m_currentTriangle.m_weights[1], barycentricCoordinates.GetY(), 0.001f)); // Forward
        EXPECT_TRUE(AZ::IsClose(uniqueData->m_currentTriangle.m_weights[2], barycentricCoordinates.GetZ(), 0.001f)); // Strafe
    }

    TEST_F(BlendSpace2DFixture, EventsTest_InsideTriangle_MostActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.25f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_InsideTriangle_AllActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(3);

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.25f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_InsideTriangle_None)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_NONE);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(0);

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.25f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_OnTriangleEdge_MostActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_OnTriangleEdge_AllActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(2);

        m_floatNodeX->SetValue(0.5f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_AtMotionPoint_MostActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.0f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, EventsTest_AtMotionPoint_AllActive)
    {
        m_blendSpace2DNode->SetEventFilterMode(BlendSpaceNode::EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS);
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_)).Times(1);

        m_floatNodeX->SetValue(0.0f);
        m_floatNodeY->SetValue(0.0f);
        GetEMotionFX().Update(0.2f);
    }

    TEST_F(BlendSpace2DFixture, MotionCoordinates_Nullptr_Asserts_FT)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;

        AZ::Vector2 testVec(0.0f, 0.0f);

        m_blendSpace2DNode->ComputeMotionCoordinates("", nullptr, testVec);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

} // namespace EMotionFX
