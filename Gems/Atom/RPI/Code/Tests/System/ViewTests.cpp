/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/View.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <Common/RPITestFixture.h>

namespace UnitTest
{
    /*
    * Helper function to create a view with a given vertical field of view and aspect ratio
    */
    AZ::RPI::ViewPtr CreateView(float fovY, float aspectRatio)
    {
        using namespace AZ;
        using namespace RPI;

        ViewPtr view =  View::CreateView(AZ::Name("TestView"), RPI::View::UsageCamera);

        AZ::Matrix4x4 worldToView = AZ::Matrix4x4::CreateIdentity();

        const float nearDist = 0.1f;
        const float farDist = 100.0f;
        AZ::Matrix4x4 viewToClip = AZ::Matrix4x4::CreateProjection(fovY, aspectRatio, nearDist, farDist);

        view->SetWorldToViewMatrix(worldToView);
        view->SetViewToClipMatrix(viewToClip);

        return view;
    }

    /*
    * Helper function to do a sanity check on CalculateSphereAreaInClipSpace
    * Given a fovY and aspect ratio it creates a view.
    * It then computes how far away the sphere of the given radius would have to be such that the edge of
    * the sphere horizon would touch the top and bottom edges of the view
    * It then uses CalculateSphereAreaInClipSpace with this distance and checks the answer agrees
    */
    void TestCalculateSphereAreaInClipSpaceWithSphereFillingYDimension(float fovY, float aspectRatio, float sphereRadius)
    {
        using namespace AZ;
        using namespace RPI;

        ViewPtr view = CreateView(fovY, aspectRatio);

        // Sphere distance from camera is radius/sin(fov/2)
        // At this distance the horizon of the sphere should be touching the edge of the viewport vertically
        // So the visible area of the sphere would be a circle the diameter of the viewport height
        // So the coverage percentage would be 0.5f * 0.5f * pi (radius will be half screen height)
        float sinHalfFovY = sin(fovY * 0.5f);
        float dist = sphereRadius / sinHalfFovY;
        AZ::Vector3 center(0.0f, 0.0f, -dist);

        float coverage = view->CalculateSphereAreaInClipSpace(center, sphereRadius);

        const float expectedCoverage = 0.5f * 0.5f * AZ::Constants::Pi;
        const float allowedEpsilon = 0.001f;
        float diff = fabsf(coverage - expectedCoverage);

        EXPECT_TRUE(diff < allowedEpsilon);
    }

    /*
    * Helper function to do a sanity check on CalculateSphereAreaInClipSpace
    * This uses a calculation that computes the projected radius of a sphere when the camera is looking directly at the sphere
    */
    void TestCalculateSphereAreaInClipSpaceVsProjectedRadius(float fovY, float aspectRatio, const AZ::Vector3& sphereCenter, float sphereRadius)
    {
        using namespace AZ;
        using namespace RPI;

        ViewPtr view = CreateView(fovY, aspectRatio);

        float coverage = view->CalculateSphereAreaInClipSpace(sphereCenter, sphereRadius);

        // This computes the projected radius of a sphere using the calculation described here:
        // https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
        float radiusSq = sphereRadius * sphereRadius;
        AZ::Vector3 distanceVector = sphereCenter;
        float distanceSq = distanceVector.GetLengthSq();
        float tanHalfFovY = tan(fovY * 0.5f);
        float cotHalfFovY = 1.0f / tanHalfFovY; // this is actually just the same as view->GetViewToClipMatrix().GetElement(1, 1)
        float sqrtDistanceSqMinusRadiusSq = sqrt(distanceSq - radiusSq);
        float projectedRadius = cotHalfFovY * sphereRadius / sqrtDistanceSqMinusRadiusSq;

        // projectedRadius is a percentage of half of the view height. To get as percentage of view height we halve it
        float prAsAPercentOfViewHeight = projectedRadius * 0.5f;

        float prSq = prAsAPercentOfViewHeight * prAsAPercentOfViewHeight;
        float expectedArea = prSq * AZ::Constants::Pi;

        const float allowedEpsilon = 0.0001f;
        float diff = fabsf(coverage - expectedArea);

        EXPECT_TRUE(diff < allowedEpsilon);
    }

    class ViewTests
        : public RPITestFixture
    {
    };

    TEST_F(ViewTests, SphereCoverageSpecialCases)
    {
        using namespace AZ;
        using namespace RPI;

        // Square view, 90 field of view
        {
            const float fovY = Constants::Pi * 0.5f;    // 90 degrees
            const float aspectRatio = 1.0f;
            ViewPtr view = CreateView(fovY, aspectRatio);

            // Sphere in front of camera but touching camera origin
            // This is treated as a special case and should return 1.0
            {
                AZ::Vector3 center(0.0f, 0.0f, -1.0f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 1.0f);
            }

            // Sphere at camera origin
            // This is treated as a special case and should return 1.0
            {
                AZ::Vector3 center(0.0f, 0.0f, 0.0f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 1.0f);
            }

            // Sphere fully behind the camera origin
            // This is treated as a special case and should return 0.0
            {
                AZ::Vector3 center(0.0f, 0.0f, 1.1f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 0.0f);
            }

            // Sphere behind camera but touching camera origin
            // This is treated as a special case and should return 1.0
            {
                AZ::Vector3 center(0.0f, 0.0f, 1.0f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 1.0f);
            }

            // Camera inside sphere, sphere center in front of camera
            // This is treated as a special case and should return 1.0
            {
                AZ::Vector3 center(0.0f, 0.0f, -0.75f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 1.0f);
            }

            // Camera inside sphere, sphere center behind camera
            // This is treated as a special case and should return 1.0
            {
                AZ::Vector3 center(0.0f, 0.0f, 0.5f);
                float radius = 1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 1.0f);
            }

            // Sphere radius zero
            // This is treated as a special case and should return 0.0
            {
                AZ::Vector3 center(0.0f, 0.0f, -10.0f);
                float radius = 0.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 0.0f);
            }

            // Sphere radius less than zero
            // This is treated as a special case and should return 0.0
            {
                AZ::Vector3 center(0.0f, 0.0f, -10.0f);
                float radius = -1.0f;

                float coverage = view->CalculateSphereAreaInClipSpace(center, radius);
                EXPECT_EQ(coverage, 0.0f);
            }
        }
    }

    TEST_F(ViewTests, SphereCoverageFillY)
    {
        using namespace AZ;
        using namespace RPI;

        // Square view, 90 field of view, radius 1
        {
            const float fovY = AZ::DegToRad(90.0f);
            const float aspectRatio = 1.0f;
            const float sphereRadius = 1.0f;

            TestCalculateSphereAreaInClipSpaceWithSphereFillingYDimension(fovY, aspectRatio, sphereRadius);
        }

        // Rectangular view, 60 field of view, radius 5
        {
            const float fovY = AZ::DegToRad(60.0f);
            const float aspectRatio = 1.5f;
            const float sphereRadius = 5.0f;

            TestCalculateSphereAreaInClipSpaceWithSphereFillingYDimension(fovY, aspectRatio, sphereRadius);
        }
    }

    TEST_F(ViewTests, SphereCoverageVsProjectedRadius)
    {
        using namespace AZ;
        using namespace RPI;

        // Square view, 90 field of view, radius 1, distance 3
        {
            const float fovY = AZ::DegToRad(90.0f);
            const float aspectRatio = 1.0f;
            AZ::Vector3 sphereCenter(0.0f, 0.0f, -3.0f);
            float sphereRadius = 1.0f;

            TestCalculateSphereAreaInClipSpaceVsProjectedRadius(fovY, aspectRatio, sphereCenter, sphereRadius);
        }

        // Rectangular view, 60 field of view, radius 4, distance 20
        {
            const float fovY = AZ::DegToRad(60.0f);
            const float aspectRatio = 1.5f;
            AZ::Vector3 sphereCenter(0.0f, 0.0f, -20.0f);
            float sphereRadius = 4.0f;

            TestCalculateSphereAreaInClipSpaceVsProjectedRadius(fovY, aspectRatio, sphereCenter, sphereRadius);
        }

        // Rectangular view, 70 field of view, radius 1, distance 30
        {
            const float fovY = AZ::DegToRad(70.0f);
            const float aspectRatio = 1.5f;
            AZ::Vector3 sphereCenter(0.0f, 0.0f, -30.0f);
            float sphereRadius = 0.05f;

            TestCalculateSphereAreaInClipSpaceVsProjectedRadius(fovY, aspectRatio, sphereCenter, sphereRadius);
        }
    }

}
