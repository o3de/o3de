/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    // Basic frustum centered on the origin
    AZ::Frustum testFrustum1(
        AZ::ViewFrustumAttributes(AZ::Transform::CreateIdentity(), 1.0f, AZ::Constants::HalfPi, 1.0f, 100.0f));

    // Frustum that has slope 1/2 for the top/bottom/left/right clip planes
    AZ::Frustum testFrustum2(
        AZ::ViewFrustumAttributes(AZ::Transform::CreateIdentity(), 1.0f, 2.0f * atanf(0.5f), 10.0f, 90.0f));

    TEST(MATH_Frustum, TestFrustumSphereDisjointNear)
    {
        // Test a sphere in front of the near clip with a radius too small to intersect
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3::CreateZero(), 0.5f);
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 9.0f, 0.0f), 0.5f);
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereDisjointFar)
    {
        // Test a sphere beyond the far clip
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(0.0f, 102.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 92.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereIntersectNear)
    {
        // Test a sphere in front of the near clip with a radius large enough to intersect
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3::CreateZero(), 1.5f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 9.0f, 0.0f), 1.5f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereIntersectLeft)
    {
        // Test a sphere on the left clip plane
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(-50.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(-25.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereIntersectRight)
    {
        // Test a sphere on the right clip plane
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(50.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(25.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereIntersectTop)
    {
        // Test a sphere on the top clip plane
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(0.0f, 50.0f, 50.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 50.0f, 25.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereIntersectBottom)
    {
        // Test a sphere on the bottom clip plane
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(0.0f, 50.0f, -50.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 50.0f, -25.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumSphereContained)
    {
        // Test a sphere near the middle of the frustum
        {
            AZ::IntersectResult result = testFrustum1.IntersectSphere(AZ::Vector3(0.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Interior);
        }
        {
            AZ::IntersectResult result = testFrustum2.IntersectSphere(AZ::Vector3(0.0f, 50.0f, 0.0f), 1.0f);
            EXPECT_EQ(result, AZ::IntersectResult::Interior);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbDisjointNear)
    {
        // Test an AABB in front of the near clip with a size too small to intersect
        {
            AZ::Vector3 center = AZ::Vector3::CreateZero();
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(0.5f), center + AZ::Vector3(0.5f));
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 9.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(0.5f, 0.5f, 0.5f), center + AZ::Vector3(0.5f, 0.5f, 0.5f));
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbDisjointFar)
    {
        // Test an AABB beyond the far clip
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 102.0f, 0.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 92.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Exterior);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbIntersectNear)
    {
        // Test an AABB in front of the near clip with a size large enough to intersect
        {
            AZ::Vector3 center = AZ::Vector3::CreateZero();
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.5f), center + AZ::Vector3(1.5f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 9.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.5f, 1.5f, 1.5f), center + AZ::Vector3(1.5f, 1.5f, 1.5f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbIntersectLeft)
    {
        // Test an AABB on the left clip plane
        {
            AZ::Vector3 center = AZ::Vector3(-50.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::Vector3 center = AZ::Vector3(-25.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbIntersectRight)
    {
        // Test an AABB on the right clip plane
        {
            AZ::Vector3 center = AZ::Vector3(50.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::Vector3 center = AZ::Vector3(25.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbIntersectTop)
    {
        // Test an AABB on the top clip plane
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, 50.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, 25.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbIntersectBottom)
    {
        // Test an AABB on the bottom clip plane
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, -50.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, -25.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Overlaps);
        }
    }

    TEST(MATH_Frustum, TestFrustumAabbContained)
    {
        // Test an AABB near the middle of the frustum
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum1.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Interior);
        }
        {
            AZ::Vector3 center = AZ::Vector3(0.0f, 50.0f, 0.0f);
            AZ::IntersectResult result = testFrustum2.IntersectAabb(center - AZ::Vector3(1.0f), center + AZ::Vector3(1.0f));
            EXPECT_EQ(result, AZ::IntersectResult::Interior);
        }
    }

    TEST(MATH_Frustum, CalculateViewFrustumAttributesExample1)
    {
        const AZ::Vector3 translation(0.1f, 0.2f, 0.3f);
        const AZ::Quaternion quaternion(0.52f, 0.56f, 0.56f, 0.32f);
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(quaternion, translation);

        constexpr float aspectRatio = 1.3f;
        constexpr float fovRadians = 0.8f;
        constexpr float nearClip = 0.045f;
        constexpr float farClip = 10.3f;

        const AZ::Frustum frustum(AZ::ViewFrustumAttributes(transform, aspectRatio, fovRadians, nearClip, farClip));
        const AZ::ViewFrustumAttributes viewFrustumAttributes = frustum.CalculateViewFrustumAttributes();

        EXPECT_THAT(viewFrustumAttributes.m_worldTransform, IsClose(transform));
        EXPECT_NEAR(viewFrustumAttributes.m_aspectRatio, aspectRatio, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_verticalFovRadians, fovRadians, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_nearClip, nearClip, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_farClip, farClip, 1e-3f);
    }

    TEST(MATH_Frustum, CalculateViewFrustumAttributesExample2)
    {
        const auto transform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(5.0f)) *
            AZ::Transform::CreateRotationX(AZ::DegToRad(45.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(90.0f));

        constexpr float aspectRatio = 1024.0f/768.0f;
        constexpr float fovRadians = AZ::DegToRad(60.0f);
        constexpr float nearClip = 0.1f;
        constexpr float farClip = 100.0f;

        const AZ::Frustum frustum(AZ::ViewFrustumAttributes(transform, aspectRatio, fovRadians, nearClip, farClip));
        const AZ::ViewFrustumAttributes viewFrustumAttributes = frustum.CalculateViewFrustumAttributes();

        EXPECT_THAT(viewFrustumAttributes.m_worldTransform, IsClose(transform));
        EXPECT_NEAR(viewFrustumAttributes.m_aspectRatio, aspectRatio, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_verticalFovRadians, fovRadians, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_nearClip, nearClip, 1e-3f);
        EXPECT_NEAR(viewFrustumAttributes.m_farClip, farClip, 1e-3f);
    }

    TEST(MATH_Frustum, TestGetSetPlane)
    {
        // Assumes +x runs to the 'right', +y runs 'out' and +z points 'up'
        // A frustum is defined by 6 planes. In this case a box shape.
        AZ::Plane near_value = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -5.f, 0.f));
        AZ::Plane far_value = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 5.f, 0.f));
        AZ::Plane left = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-5.f, 0.f, 0.f));
        AZ::Plane right = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(5.f, 0.f, 0.f));
        AZ::Plane top = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 5.f));
        AZ::Plane bottom = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -5.f));

        AZ::Frustum frustum(near_value, far_value, left, right, top, bottom);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Near) == near_value);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Far) == far_value);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Left) == left);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Right) == right);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Top) == top);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Bottom) == bottom);

        AZ::Plane near1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -2.f, 0.f));
        AZ::Plane far1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 2.f, 0.f));
        AZ::Plane left1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-2.f, 0.f, 0.f));
        AZ::Plane right1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(2.f, 0.f, 0.f));
        AZ::Plane top1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 2.f));
        AZ::Plane bottom1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -2.f));
        AZ::Frustum frustum1(near1, far1, left1, right1, top1, bottom1);

        EXPECT_THAT(frustum, testing::Not(IsClose(frustum1)));
        frustum.Set(frustum1);
        EXPECT_THAT(frustum, IsClose(frustum1));

        frustum.SetPlane(AZ::Frustum::PlaneId::Near, near_value);
        frustum.SetPlane(AZ::Frustum::PlaneId::Far, far_value);
        frustum.SetPlane(AZ::Frustum::PlaneId::Left, left);
        frustum.SetPlane(AZ::Frustum::PlaneId::Right, right);
        frustum.SetPlane(AZ::Frustum::PlaneId::Top, top);
        frustum.SetPlane(AZ::Frustum::PlaneId::Bottom, bottom);

        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Near) == near_value);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Far) == far_value);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Left) == left);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Right) == right);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Top) == top);
        EXPECT_TRUE(frustum.GetPlane(AZ::Frustum::PlaneId::Bottom) == bottom);

        frustum = frustum1;
        EXPECT_THAT(frustum, IsClose(frustum1));
    }

    // TODO: Test frustum creation from View-Projection Matrices

    struct FrustumTestCase
    {
        AZ::Vector3 nearTopLeft;
        AZ::Vector3 nearTopRight;
        AZ::Vector3 nearBottomLeft;
        AZ::Vector3 nearBottomRight;
        AZ::Vector3 farTopLeft;
        AZ::Vector3 farTopRight;
        AZ::Vector3 farBottomLeft;
        AZ::Vector3 farBottomRight;
        // Used to set which plane is being used
        AZ::Frustum::PlaneId plane;
        // Used to set a test case name
        std::string testCaseName;
    };

    std::ostream& operator<< (std::ostream& stream, const UnitTest::FrustumTestCase& frustumTestCase)
    {
        stream << "NearTopLeft: " << frustumTestCase.nearTopLeft
            << std::endl << "NearTopRight: " << frustumTestCase.nearTopRight
            << std::endl << "NearBottomRight: " << frustumTestCase.nearBottomRight
            << std::endl << "NearBottomLeft: " << frustumTestCase.nearBottomLeft
            << std::endl << "FarTopLeft: " << frustumTestCase.farTopLeft
            << std::endl << "FarTopRight: " << frustumTestCase.farTopRight
            << std::endl << "FarBottomLeft: " << frustumTestCase.farBottomLeft
            << std::endl << "FarBottomRight: " << frustumTestCase.farBottomRight;
        return stream;
    }

    class Tests
        : public ::testing::WithParamInterface <FrustumTestCase>
        , public UnitTest::LeakDetectionFixture
    {

    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            // Build a frustum from 8 points
            // This allows us to generate test cases in each of the 9 regions in the 3x3 grid divided by the 6 planes,
            // as well as test cases that span across those regions
            m_testCase = GetParam();

            // Planes can be generated from triangles. Points must wind counter-clockwise (right-handed) for the normal to point in the correct direction.
            // The Frustum class assumes the plane normals point inwards
            m_planes[AZ::Frustum::PlaneId::Near] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.nearTopRight, m_testCase.nearBottomRight);
            m_planes[AZ::Frustum::PlaneId::Far] = AZ::Plane::CreateFromTriangle(m_testCase.farTopRight, m_testCase.farTopLeft, m_testCase.farBottomLeft);
            m_planes[AZ::Frustum::PlaneId::Left] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.nearBottomLeft, m_testCase.farTopLeft);
            m_planes[AZ::Frustum::PlaneId::Right] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopRight, m_testCase.farTopRight, m_testCase.farBottomRight);
            m_planes[AZ::Frustum::PlaneId::Top] = AZ::Plane::CreateFromTriangle(m_testCase.nearTopLeft, m_testCase.farTopLeft, m_testCase.farTopRight);
            m_planes[AZ::Frustum::PlaneId::Bottom] = AZ::Plane::CreateFromTriangle(m_testCase.nearBottomLeft, m_testCase.nearBottomRight, m_testCase.farBottomRight);

            // AZ::Plane::CreateFromTriangle uses Vector3::GetNormalized to create a normal, which is not perfectly normalized.
            // Since the distance value is set by float dist = -(normal.Dot(v0));, this means that an imperfect normal actually gives you a slightly different distance
            // and thus a slightly different plane. Correct that here.
            m_planes[AZ::Frustum::PlaneId::Near] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Near].GetNormal().GetNormalized(), m_testCase.nearTopLeft);
            m_planes[AZ::Frustum::PlaneId::Far] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Far].GetNormal().GetNormalized(), m_testCase.farTopRight);
            m_planes[AZ::Frustum::PlaneId::Left] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Left].GetNormal().GetNormalized(), m_testCase.nearTopLeft);
            m_planes[AZ::Frustum::PlaneId::Right] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Right].GetNormal().GetNormalized(), m_testCase.nearTopRight);
            m_planes[AZ::Frustum::PlaneId::Top] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Top].GetNormal().GetNormalized(), m_testCase.nearTopLeft);
            m_planes[AZ::Frustum::PlaneId::Bottom] = AZ::Plane::CreateFromNormalAndPoint(m_planes[AZ::Frustum::PlaneId::Bottom].GetNormal().GetNormalized(), m_testCase.nearBottomLeft);

            // Create the frustum itself
            for (AZ::Frustum::PlaneId planeId = AZ::Frustum::PlaneId::Near; planeId < AZ::Frustum::PlaneId::MAX; ++planeId)
            {
                m_frustum.SetPlane(planeId, m_planes[planeId]);
            }

            // Get the center points
            m_centerPoints[AZ::Frustum::PlaneId::Near] = m_testCase.nearTopLeft + 0.5f * (m_testCase.nearBottomRight - m_testCase.nearTopLeft);
            m_centerPoints[AZ::Frustum::PlaneId::Far] = m_testCase.farTopLeft + 0.5f * (m_testCase.farBottomRight - m_testCase.farTopLeft);
            m_centerPoints[AZ::Frustum::PlaneId::Left] = m_testCase.nearTopLeft + 0.5f * (m_testCase.farBottomLeft - m_testCase.nearTopLeft);
            m_centerPoints[AZ::Frustum::PlaneId::Right] = m_testCase.nearTopRight + 0.5f * (m_testCase.farBottomRight - m_testCase.nearTopRight);
            m_centerPoints[AZ::Frustum::PlaneId::Top] = m_testCase.nearTopLeft + 0.5f * (m_testCase.farTopRight - m_testCase.nearTopLeft);
            m_centerPoints[AZ::Frustum::PlaneId::Bottom] = m_testCase.nearBottomLeft + 0.5f * (m_testCase.farBottomRight - m_testCase.nearBottomLeft);

            // Get the shortest edge of the frustum
            AZStd::vector<AZ::Vector3> edges;
            // Near plane
            edges.push_back(m_testCase.nearTopLeft - m_testCase.nearTopRight);
            edges.push_back(m_testCase.nearTopRight - m_testCase.nearBottomRight);
            edges.push_back(m_testCase.nearBottomRight - m_testCase.nearBottomLeft);
            edges.push_back(m_testCase.nearBottomLeft - m_testCase.nearTopLeft);

            // Edges from near plane to far plane
            edges.push_back(m_testCase.nearTopLeft - m_testCase.farTopLeft);
            edges.push_back(m_testCase.nearTopRight - m_testCase.farTopRight);
            edges.push_back(m_testCase.nearBottomRight - m_testCase.farBottomRight);
            edges.push_back(m_testCase.nearBottomLeft - m_testCase.farBottomLeft);

            // Far plane
            edges.push_back(m_testCase.farTopLeft - m_testCase.farTopRight);
            edges.push_back(m_testCase.farTopRight - m_testCase.farBottomRight);
            edges.push_back(m_testCase.farBottomRight - m_testCase.farBottomLeft);
            edges.push_back(m_testCase.farBottomLeft - m_testCase.farTopLeft);

            m_minEdgeLength = std::numeric_limits<float>::max();
            for (const AZ::Vector3& edge : edges)
            {
                m_minEdgeLength = AZ::GetMin(m_minEdgeLength, static_cast<float>(edge.GetLength()));
            }
        }

        AZ::Sphere GenerateSphereOutsidePlane(const AZ::Plane& plane, const AZ::Vector3& planeCenter)
        {
            // Get a radius small enough for the entire sphere to fit outside the plane without extending past the other planes in the frustum
            float radius = 0.25f * m_minEdgeLength;

            // Get the outward pointing normal of the plane
            AZ::Vector3 normal = -1.0f * plane.GetNormal();

            // Create a sphere that is outside the plane
            AZ::Vector3 center = planeCenter + normal * (radius + m_marginOfErrorOffset);

            return AZ::Sphere(center, radius);
        }

        AZ::Sphere GenerateSphereInsidePlane(const AZ::Plane& plane, const AZ::Vector3& planeCenter)
        {
            // Get a radius small enough for the entire sphere to fit outside the plane without extending past the other planes in the frustum
            float radius = 0.25f * m_minEdgeLength;

            // Get the inward pointing normal of the plane
            AZ::Vector3 normal = plane.GetNormal();

            // Create a sphere that is inside the plane
            AZ::Vector3 center = planeCenter + normal * (radius + m_marginOfErrorOffset);

            return AZ::Sphere(center, radius);
        }

        // The points used to generate the test cases
        FrustumTestCase m_testCase;

        // The planes generated from the points.
        // Keep these around for access to the normals, which are used to generate shapes inside/outside of the frustum
        AZ::Plane m_planes[AZ::Frustum::PlaneId::MAX];

        // The center points on the frustum for each plane
        AZ::Vector3 m_centerPoints[AZ::Frustum::PlaneId::MAX];

        // The length of the shortest edge in the frustum
        float m_minEdgeLength = std::numeric_limits<float>::max();

        // Shapes generated inside/outside the frustum will be offset by this much as a margin of error
        // Through trial and error determined that, for the test cases below, the sphere needs to be offset from the frustum by at least 0.115f to be guaranteed to pass,
        // which gives a reasonable idea of how precise these intersection tests are.
        // For the box shaped frustum, the tests passed when offset by FLT_EPSILON, but the frustums further away from the origin were less precise.
        float m_marginOfErrorOffset = 0.115f;

        // The frustum under test
        AZ::Frustum m_frustum;
    };

    // Tests that a frustum does not contain a sphere that is outside the frustum
    TEST_P(Tests, FrustumContainsSphere_SphereOutsidePlane_False)
    {
        AZ::Sphere testSphere = GenerateSphereOutsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum contains sphere even though sphere is completely outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere outside the frustum does not overlap the frustum
    TEST_P(Tests, SphereOverlapsFrustum_SphereOutsidePlane_False)
    {
        AZ::Sphere testSphere = GenerateSphereOutsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere overlaps frustum even though sphere is completely outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a frustum contains a sphere that is inside the frustum
    TEST_P(Tests, FrustumContainsSphere_SphereInsidePlane_True)
    {
        AZ::Sphere testSphere = GenerateSphereInsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum does not contain sphere even though sphere is completely inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere inside the frustum overlaps the frustum
    TEST_P(Tests, SphereOverlapsFrustum_SphereInsidePlane_True)
    {
        AZ::Sphere testSphere = GenerateSphereInsidePlane(m_planes[m_testCase.plane], m_centerPoints[m_testCase.plane]);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere does not overlap frustum even though sphere is completely inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a frustum does not contain a sphere that is half inside half outside the frustum
    TEST_P(Tests, FrustumContainsSphere_SphereHalfInsideHalfOutsidePlane_False)
    {
        AZ::Sphere testSphere(m_centerPoints[m_testCase.plane], m_minEdgeLength * .25f);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(m_frustum, testSphere)) << "Frustum contains sphere even though sphere is partially outside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    // Tests that a sphere half inside half outside the frustum overlaps the frustum
    TEST_P(Tests, SphereOverlapsFrustum_SphereHalfInsideHalfOutsidePlane_True)
    {
        AZ::Sphere testSphere(m_centerPoints[m_testCase.plane], m_minEdgeLength * .25f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(testSphere, m_frustum)) << "Sphere does not overlap frustum even though sphere is partially inside the frustum." << std::endl << "Frustum:" << std::endl << m_testCase << std::endl << "Sphere:" << std::endl << testSphere << std::endl;
    }

    std::vector<FrustumTestCase> GenerateFrustumIntersectionTestCases()
    {
        std::vector<FrustumTestCase> testCases;

        std::vector<FrustumTestCase> frustums;
        // 2x2x2 box (Z-up coordinate system)
        FrustumTestCase box;
        box.nearTopLeft = AZ::Vector3(-1.0f, -1.0f, 1.0f);
        box.nearTopRight = AZ::Vector3(1.0f, -1.0f, 1.0f);
        box.nearBottomLeft = AZ::Vector3(-1.0f, -1.0f, -1.0f);
        box.nearBottomRight = AZ::Vector3(1.0f, -1.0f, -1.0f);
        box.farTopLeft = AZ::Vector3(-1.0f, 1.0f, 1.0f);
        box.farTopRight = AZ::Vector3(1.0f, 1.0f, 1.0f);
        box.farBottomLeft = AZ::Vector3(-1.0f, 1.0f, -1.0f);
        box.farBottomRight = AZ::Vector3(1.0f, 1.0f, -1.0f);
        box.testCaseName = "BoxShaped";
        frustums.push_back(box);

        FrustumTestCase defaultCameraFrustum;
        defaultCameraFrustum.nearTopLeft = AZ::Vector3(-0.204621f, 0.200000f, 0.153465f);
        defaultCameraFrustum.nearTopRight = AZ::Vector3(0.204621f, 0.200000f, 0.153465f);
        defaultCameraFrustum.nearBottomLeft = AZ::Vector3(-0.204621f, 0.200000f, -0.153465f);
        defaultCameraFrustum.nearBottomRight = AZ::Vector3(0.204621f, 0.200000f, -0.153465f);
        defaultCameraFrustum.farTopLeft = AZ::Vector3(-1047.656982f, 1024.000000f, 785.742737f);
        defaultCameraFrustum.farTopRight = AZ::Vector3(1047.656982f, 1024.000000f, 785.742737f);
        defaultCameraFrustum.farBottomLeft = AZ::Vector3(-1047.656982f, 1024.000000f, -785.742737f);
        defaultCameraFrustum.farBottomRight = AZ::Vector3(1047.656982f, 1024.000000f, -785.742737f);
        defaultCameraFrustum.testCaseName = "DefaultCamera";
        frustums.push_back(defaultCameraFrustum);

        // These frustums were generated from flying around StarterGame and dumping the frustum values for the viewport camera and shadow cascade frustums to a log file
        FrustumTestCase starterGame0;
        starterGame0.nearTopLeft = AZ::Vector3(41656.351563f, 794907.750000f, -604483.687500f);
        starterGame0.nearTopRight = AZ::Vector3(41662.343750f, 794907.375000f, -604483.687500f);
        starterGame0.nearBottomLeft = AZ::Vector3(41656.164063f, 794904.125000f, -604488.437500f);
        starterGame0.nearBottomRight = AZ::Vector3(41662.156250f, 794903.750000f, -604488.437500f);
        starterGame0.farTopLeft = AZ::Vector3(41677.691406f, 795314.937500f, -604793.375000f);
        starterGame0.farTopRight = AZ::Vector3(41683.683594f, 795314.562500f, -604793.375000f);
        starterGame0.farBottomLeft = AZ::Vector3(41677.503906f, 795311.312500f, -604798.125000f);
        starterGame0.farBottomRight = AZ::Vector3(41683.496094f, 795310.937500f, -604798.125000f);
        starterGame0.testCaseName = "StarterGame0";
        frustums.push_back(starterGame0);

        FrustumTestCase starterGame1;
        starterGame1.nearTopLeft = AZ::Vector3(0.166996f, 0.240156f, 0.117468f);
        starterGame1.nearTopRight = AZ::Vector3(0.226816f, -0.184736f, 0.117423f);
        starterGame1.nearBottomLeft = AZ::Vector3(0.169258f, 0.240499f, -0.113460f);
        starterGame1.nearBottomRight = AZ::Vector3(0.229078f, -0.184393f, -0.113506f);
        starterGame1.farTopLeft = AZ::Vector3(83.497986f, 120.077904f, 58.734165f);
        starterGame1.farTopRight = AZ::Vector3(113.408234f, -92.368172f, 58.711285f);
        starterGame1.farBottomLeft = AZ::Vector3(84.628860f, 120.249550f, -56.730221f);
        starterGame1.farBottomRight = AZ::Vector3(114.539108f, -92.196526f, -56.753101f);
        starterGame1.testCaseName = "StarterGame1";
        frustums.push_back(starterGame1);

        FrustumTestCase starterGame2;
        starterGame2.nearTopLeft = AZ::Vector3(-0.007508f, 0.091724f, -0.043323f);
        starterGame2.nearTopRight = AZ::Vector3(0.018772f, 0.090096f, -0.043323f);
        starterGame2.nearBottomLeft = AZ::Vector3(-0.008393f, 0.077435f, -0.065421f);
        starterGame2.nearBottomRight = AZ::Vector3(0.017887f, 0.075807f, -0.065421f);
        starterGame2.farTopLeft = AZ::Vector3(-11.637362f, 142.172897f, -67.151001f);
        starterGame2.farTopRight = AZ::Vector3(29.096817f, 139.649338f, -67.151001f);
        starterGame2.farBottomLeft = AZ::Vector3(-13.009484f, 120.024765f, -101.403290f);
        starterGame2.farBottomRight = AZ::Vector3(27.724697f, 117.501205f, -101.403290f);
        starterGame2.testCaseName = "StarterGame2";
        frustums.push_back(starterGame2);

        FrustumTestCase starterGame3;
        starterGame3.nearTopLeft = AZ::Vector3(0.211831f, -0.207308f, 0.107297f);
        starterGame3.nearTopRight = AZ::Vector3(-0.217216f, -0.201659f, 0.107297f);
        starterGame3.nearBottomLeft = AZ::Vector3(0.211954f, -0.197980f, -0.123455f);
        starterGame3.nearBottomRight = AZ::Vector3(-0.217093f, -0.192331f, -0.123455f);
        starterGame3.farTopLeft = AZ::Vector3(8473.246094f, -8292.310547f, 4291.892578f);
        starterGame3.farTopRight = AZ::Vector3(-8688.623047f, -8066.359863f, 4291.892578f);
        starterGame3.farBottomLeft = AZ::Vector3(8478.158203f, -7919.196777f, -4938.199219f);
        starterGame3.farBottomRight = AZ::Vector3(-8683.710938f, -7693.245605f, -4938.199219f);
        starterGame3.testCaseName = "StarterGame3";
        frustums.push_back(starterGame3);

        // For each test frustum, create test cases for every plane
        for (FrustumTestCase frustum : frustums)
        {
            for (int i = 0; i < AZ::Frustum::PlaneId::MAX; ++i)
            {
                frustum.plane = static_cast<AZ::Frustum::PlaneId>(i);
                testCases.push_back(frustum);
            }
        }

        return testCases;
    }

    std::string GenerateFrustumIntersectionTestCaseName(const ::testing::TestParamInfo<FrustumTestCase>& info)
    {
        std::string testCaseName = info.param.testCaseName;
        switch (info.param.plane)
        {
        case AZ::Frustum::PlaneId::Near:
            testCaseName += "_Near";
            break;
        case AZ::Frustum::PlaneId::Far:
            testCaseName += "_Far";
            break;
        case AZ::Frustum::PlaneId::Left:
            testCaseName += "_Left";
            break;
        case AZ::Frustum::PlaneId::Right:
            testCaseName += "_Right";
            break;
        case AZ::Frustum::PlaneId::Top:
            testCaseName += "_Top";
            break;
        case AZ::Frustum::PlaneId::Bottom:
            testCaseName += "_Bottom";
            break;
        }
        return testCaseName;
    }

    INSTANTIATE_TEST_CASE_P(
        MATH_Frustum, Tests, ::testing::ValuesIn(GenerateFrustumIntersectionTestCases()),
        GenerateFrustumIntersectionTestCaseName);

    TEST(MATH_Frustum, AabbInsideOrientatedFrustum)
    {
        // position the frustum slightly along the x-axis looking down the negative x-axis
        const AZ::Vector3 frustumOrigin = AZ::Vector3(5.0f, 0.0f, 0.0f);
        const AZ::Quaternion frustumOrientation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f));
        const AZ::Transform frustumTransform =
            AZ::Transform::CreateFromQuaternionAndTranslation(frustumOrientation, frustumOrigin);

        const AZ::Frustum viewFrustum = AZ::Frustum(
            AZ::ViewFrustumAttributes(frustumTransform, 1920.0f / 1080.0f, AZ::DegToRad(60.0f), 0.1f, 100.0f));

        // position the aabb at the origin (inside the frustum's view)
        const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));

        // the aabb is contained within the frustum
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(viewFrustum, aabb));
    }

    TEST(MATH_Frustum, FrustumCorners)
    {
        // position the frustum slightly along the x-axis looking down the negative x-axis
        const AZ::Vector3 frustumOrigin = AZ::Vector3(5.0f, 0.0f, 0.0f);
        const AZ::Quaternion frustumOrientation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f));
        const AZ::Transform frustumTransform =
            AZ::Transform::CreateFromQuaternionAndTranslation(frustumOrientation, frustumOrigin);

        const AZ::Frustum viewFrustum = AZ::Frustum(
            AZ::ViewFrustumAttributes(frustumTransform, 1920.0f / 1080.0f, AZ::DegToRad(60.0f), 0.1f, 100.0f));

        AZ::Frustum::CornerVertexArray corners;
        bool valid = viewFrustum.GetCorners(corners);

        EXPECT_TRUE(valid);

        // 8 unique positions all on 3 planes must be the 8 corners of the frustum.

        // The various corners should all be unique
        for (uint32_t i = 0; i < corners.size(); ++i)
        {
            for (uint32_t j = i + 1; j < corners.size(); ++j)
            {
                EXPECT_THAT(corners.at(i), testing::Not(IsClose(corners.at(j))));
            }
        }

        // The various corners should all lie on exactly 3 planes of the frustum
        for (auto corner : corners)
        {
            uint32_t onPlaneCount = 0;
            for (uint32_t planeId = 0; planeId < AZ::Frustum::PlaneId::MAX; ++planeId)
            {
                const float distanceToPlane = viewFrustum.GetPlane(AZ::Frustum::PlaneId(planeId)).GetPointDist(corner);
                if (AZStd::abs(distanceToPlane) < 0.001f)
                {
                    ++onPlaneCount;
                }
            }
            EXPECT_EQ(onPlaneCount, 3);
        }
    }
} // namespace UnitTest
