/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBounds.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    const float g_epsilon = 1e-4f;

    using namespace AzToolsFramework::Picking;

    TEST(ManipulatorBounds, Sphere)
    {
        ManipulatorBoundSphere manipulatorBoundSphere(RegisteredBoundId{});
        manipulatorBoundSphere.m_center = AZ::Vector3::CreateZero();
        manipulatorBoundSphere.m_radius = 2.0f;

        float intersectionDistance;
        bool intersection = manipulatorBoundSphere.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 8.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Box)
    {
        ManipulatorBoundBox manipulatorBoundBox(RegisteredBoundId{});
        manipulatorBoundBox.m_center = AZ::Vector3::CreateZero();
        manipulatorBoundBox.m_halfExtents = AZ::Vector3(1.0f);
        manipulatorBoundBox.m_axis1 = AZ::Vector3::CreateAxisX();
        manipulatorBoundBox.m_axis2 = AZ::Vector3::CreateAxisY();
        manipulatorBoundBox.m_axis3 = AZ::Vector3::CreateAxisZ();

        float intersectionDistance;
        bool intersection = manipulatorBoundBox.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 9.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Cylinder)
    {
        ManipulatorBoundCylinder manipulatorCylinder(RegisteredBoundId{});
        manipulatorCylinder.m_base = AZ::Vector3::CreateAxisZ(-5.0f);
        manipulatorCylinder.m_axis = AZ::Vector3::CreateAxisZ();
        manipulatorCylinder.m_height = 10.0f;
        manipulatorCylinder.m_radius = 2.0f;

        float intersectionDistance;
        bool intersection = manipulatorCylinder.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 8.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Cone)
    {
        ManipulatorBoundCone manipulatorCone(RegisteredBoundId{});
        manipulatorCone.m_apexPosition = AZ::Vector3::CreateAxisZ(-5.0f);
        manipulatorCone.m_height = 10.0f;
        manipulatorCone.m_dir = AZ::Vector3::CreateAxisZ();
        manipulatorCone.m_radius = 4.0f;

        float intersectionDistance;
        bool intersection = manipulatorCone.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 8.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Quad)
    {
        ManipulatorBoundQuad manipulatorQuad(RegisteredBoundId{});
        manipulatorQuad.m_corner1 = AZ::Vector3(-1.0f, 0.0f, 1.0f);
        manipulatorQuad.m_corner2 = AZ::Vector3(1.0f, 0.0f, 1.0f);
        manipulatorQuad.m_corner3 = AZ::Vector3(1.0f, 0.0f, -1.0f);
        manipulatorQuad.m_corner4 = AZ::Vector3(-1.0f, 0.0f, -1.0f);

        float intersectionDistance;
        bool intersection = manipulatorQuad.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 10.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Torus)
    {
        ManipulatorBoundTorus manipulatorTorus(RegisteredBoundId{});
        manipulatorTorus.m_axis = AZ::Vector3::CreateAxisY();
        manipulatorTorus.m_center = AZ::Vector3::CreateZero();
        manipulatorTorus.m_majorRadius = 5.0f;
        manipulatorTorus.m_minorRadius = 0.5f;

        float _;
        bool intersectionCenter = manipulatorTorus.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), _);

        // miss - through center
        EXPECT_TRUE(!intersectionCenter);
       
        float intersectionDistance;
        bool intersectionOutside = manipulatorTorus.IntersectRay(
            AZ::Vector3(5.0f, 10.0f, 0.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 9.5f, g_epsilon);
        EXPECT_TRUE(intersectionOutside);
    }

    TEST(ManipulatorBounds, RayIntersectsTorusAtAcuteAngle)
    {
        // torus approximation is side-on to ray
        ManipulatorBoundTorus manipulatorTorus(RegisteredBoundId{});
        manipulatorTorus.m_axis = AZ::Vector3::CreateAxisX();
        manipulatorTorus.m_center = AZ::Vector3::CreateZero();
        manipulatorTorus.m_majorRadius = 5.0f;
        manipulatorTorus.m_minorRadius = 0.5f;

        // calculation used to orientate the ray to hit
        // the inside edge of the cylinder
        //
        // tan @ = opp / adj
        // tan @ = 0.5 / 5.0 = 0.1
        // @ = atan(0.1) = 5.71 degrees
        //
        // tan 5.71 = x / 15
        // x = 15 * tan 5.71 = ~1.5

        const AZ::Vector3 orientatedPickRay =
            AZ::Quaternion::CreateRotationZ(AZ::DegToRad(5.7f)).TransformVector(-AZ::Vector3::CreateAxisY());

        float _;
        bool intersection = manipulatorTorus.IntersectRay(
            AZ::Vector3(-1.5f, 10.0f, 0.0f), orientatedPickRay, _);

        // ensure we get a valid intersection (even if the first hit
        // might have happened in the 'hollow' part of the cylinder)
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Line)
    {
        ManipulatorBoundLineSegment manipulatorLine(RegisteredBoundId{});
        manipulatorLine.m_worldStart = AZ::Vector3(-5.0f, 0.0f, 0.0f);
        manipulatorLine.m_worldEnd = AZ::Vector3(5.0f, 0.0f, 0.0f);
        manipulatorLine.m_width = 0.2f;

        float intersectionDistance;
        bool intersection = manipulatorLine.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 10.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    TEST(ManipulatorBounds, Spline)
    {
        AZStd::shared_ptr<AZ::BezierSpline> bezierSpline = AZStd::make_shared<AZ::BezierSpline>();
        bezierSpline->m_vertexContainer.AddVertex(AZ::Vector3(-10.0f, 0.0f, 0.0f));
        bezierSpline->m_vertexContainer.AddVertex(AZ::Vector3(-5.0f, 0.0f, 0.0f));
        bezierSpline->m_vertexContainer.AddVertex(AZ::Vector3(5.0f, 0.0f, 0.0f));
        bezierSpline->m_vertexContainer.AddVertex(AZ::Vector3(10.0f, 0.0f, 0.0f));

        ManipulatorBoundSpline manipulatorSpline(RegisteredBoundId{});
        manipulatorSpline.m_spline = bezierSpline;
        manipulatorSpline.m_transform = AZ::Transform::CreateIdentity();
        manipulatorSpline.m_width = 0.2f;

        float intersectionDistance;
        bool intersection = manipulatorSpline.IntersectRay(
            AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(), intersectionDistance);

        EXPECT_NEAR(intersectionDistance, 10.0f, g_epsilon);
        EXPECT_TRUE(intersection);
    }

    // replicates a scenario in the Editor using a cone and a pick ray which should have failed but passed with Intersect::IntersectRayCone
    TEST(ManipulatorIntersectRayConeTest, RayConeEditorScenarioTest)
    {
        auto rayOrigin = AZ::Vector3(0.0f, -0.808944702f, 0.0f);
        auto rayDir = AZ::Vector3(0.301363617f, 0.939044654f, 0.165454566f);
        float t = 0.0f;
        bool hit = AzToolsFramework::Picking::IntersectRayCone(
            rayOrigin, rayDir, AZ::Vector3(0.0f, 0.0f, 0.161788940f), AZ::Vector3(0.0f, 0.0f, -1.0f), 0.0453009047, 0.0113252262, t);
        EXPECT_FALSE(hit);
    }

    // cone lying flat, ray going towards base of cone
    TEST(ManipulatorIntersectRayConeTest, RayIntersectsConeBase)
    {
        auto rayOrigin = AZ::Vector3::CreateZero();
        auto rayDir = AZ::Vector3::CreateAxisY();
        float t = 0.0f;
        bool hit = AzToolsFramework::Picking::IntersectRayCone(
            rayOrigin, rayDir, AZ::Vector3::CreateAxisY(10.0f), AZ::Vector3::CreateAxisY(-1.0f), 5.0f, 1.0f, t);
        EXPECT_TRUE(hit);
        EXPECT_THAT(t, ::testing::FloatNear(5.0f, 0.0001f));
    }

    // cone standing up, ray going towards mid side of cone
    TEST(ManipulatorIntersectRayConeTest, RayIntersectsConeSide)
    {
        auto rayOrigin = AZ::Vector3::CreateZero();
        auto rayDir = AZ::Vector3::CreateAxisY();
        float t = 0.0f;
        bool hit = AzToolsFramework::Picking::IntersectRayCone(
            rayOrigin, rayDir, AZ::Vector3(0.0f, 10.0f, 5.0f), AZ::Vector3::CreateAxisZ(-1.0f), 10.0f, 5.0f, t);
        EXPECT_TRUE(hit);
        EXPECT_THAT(t, ::testing::FloatNear(7.5f, 0.0001f));
    }

    // cone standing up, ray going towards mid side of cone
    TEST(ManipulatorIntersectRayConeTest, RayIntersectsConeApex)
    {
        auto rayOrigin = AZ::Vector3::CreateZero();
        auto rayDir = AZ::Vector3::CreateAxisY();
        float t = 0.0f;
        bool hit = AzToolsFramework::Picking::IntersectRayCone(
            rayOrigin, rayDir, AZ::Vector3::CreateAxisY(2.5f), AZ::Vector3::CreateAxisY(1.0f), 5.0f, 1.0f, t);
        EXPECT_TRUE(hit);
        EXPECT_THAT(t, ::testing::FloatNear(2.5f, 0.0001f));
    }
} // namespace UnitTest
