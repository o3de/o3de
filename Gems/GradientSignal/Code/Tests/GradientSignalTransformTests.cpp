/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Tests/GradientSignalTestMocks.h"

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <Source/Components/GradientTransformComponent.h>

namespace UnitTest
{
    struct GradientSignalTransformTestsFixture : public GradientSignalTest
    {
        // By default, we'll use a shape half extents of (5, 10, 20) for every test, and a world translation of (100, 200, 300).
        struct GradientTransformSetupData
        {
            GradientSignal::WrappingType m_wrappingType{ GradientSignal::WrappingType::None };
            AZ::Vector3 m_shapeHalfExtents{ 5.0f, 10.0f, 20.0f };
            AZ::Vector3 m_worldTranslation{ 100.0f, 200.0f, 300.0f };
            float m_frequencyZoom{ 1.0f };
        };

        struct GradientTransformTestData
        {
            AZ::Vector3 m_positionToTest;
            AZ::Vector3 m_expectedOutputUVW;
            bool m_expectedOutputRejectionResult;
        };

        static constexpr float UvEpsilon = GradientSignal::GradientTransform::UvEpsilon;

        void TestGradientTransform(const GradientTransformSetupData& setup, const GradientTransformTestData& test)
        {
            AZ::Aabb shapeBounds = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), setup.m_shapeHalfExtents);
            AZ::Matrix3x4 transform = AZ::Matrix3x4::CreateTranslation(setup.m_worldTranslation);
            float frequencyZoom = setup.m_frequencyZoom;
            GradientSignal::WrappingType wrappingType = setup.m_wrappingType;

            AZ::Vector3 outUVW;
            bool wasPointRejected;

            // Perform the query with a 3D gradient and verify that the results match expectations.
            GradientSignal::GradientTransform gradientTransform3d(shapeBounds, transform, true, frequencyZoom, wrappingType);
            gradientTransform3d.TransformPositionToUVW(test.m_positionToTest, outUVW, wasPointRejected);
            EXPECT_THAT(outUVW, IsClose(test.m_expectedOutputUVW));
            EXPECT_EQ(wasPointRejected, test.m_expectedOutputRejectionResult);

            // Perform the query with a 2D gradient and verify that the results match, but always returns a W value of 0.
            GradientSignal::GradientTransform gradientTransform2d(shapeBounds, transform, false, frequencyZoom, wrappingType);
            gradientTransform2d.TransformPositionToUVW(test.m_positionToTest, outUVW, wasPointRejected);
            EXPECT_THAT(outUVW, IsClose(AZ::Vector3(test.m_expectedOutputUVW.GetX(), test.m_expectedOutputUVW.GetY(), 0.0f)));
            EXPECT_EQ(wasPointRejected, test.m_expectedOutputRejectionResult);
        }
    };

    TEST_F(GradientSignalTransformTestsFixture, UnboundedWrappingReturnsTranslatedInput)
    {
        GradientTransformSetupData setup = { GradientSignal::WrappingType::None };
        GradientTransformTestData test = {
            // Input position to query
            { 0.0f, 0.0f, 0.0f },

            // Output: For no wrapping, the output is just the input position offset by the world translation.
            { -100.0f, -200.0f, -300.0f }, false
        };

        TestGradientTransform(setup, test);
    }

    TEST_F(GradientSignalTransformTestsFixture, ClampToEdgeReturnsValuesClampedToShapeBounds)
    {
        GradientTransformSetupData setup = { GradientSignal::WrappingType::ClampToEdge };
        GradientTransformTestData tests[] = {
            // Test:  Input point far below minimum shape bounds
            // Our input point is below the minimum of shape bounds, so the result should be the minimum corner of the shape.
            { { 0.0f, 0.0f, 0.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test:  Input point directly on minimum shape bounds
            // Our input point is directly on the minimum of shape bounds, so the result should be the minimum corner of the shape.
            { { 95.0f, 190.0f, 280.0f }, { -5.0f, -10.0f, -20.0f }, false },
            
            // Test:  Input point inside shape bounds
            // Our input point is inside the shape bounds, so the result is just input - translation.
            { { 101.0f, 202.0f, 303.0f }, { 1.0f, 2.0f, 3.0f }, false },

            // Test:  Input point directly on maximum shape bounds
            // On the maximum side, GradientTransform clamps to "max - epsilon" for consistency with other wrapping types, so our
            // expected results are the max shape corner - epsilon.
            { { 105.0f, 210.0f, 320.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, false },

            // Test:  Input point far above maximum shape bounds
            // On the maximum side, GradientTransform clamps to "max - epsilon" for consistency with other wrapping types, so our
            // expected results are the max shape corner - epsilon.
            { { 1000.0f, 1000.0f, 1000.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, false },
        };

        for (auto& test : tests)
        {
            TestGradientTransform(setup, test);
        }
    }

    TEST_F(GradientSignalTransformTestsFixture, MirrorReturnsValuesMirroredBasedOnShapeBounds)
    {
        /* Here's how the results are expected to work for various inputs when using Mirror wrapping.
         * This assumes shape half extents of (5, 10, 20), and a center translation of (100, 200, 300):
         *       Inputs:                                 Outputs:
         *       ...                                     ...
         *       (75, 150, 200) - (85, 170, 240)         (-5, -10, -20) to (5, 10, 20)       // forward mirror
         *       (85, 170, 240) - (95, 190, 280)         (5, 10, 20) to (-5, -10, -20)       // back mirror
         *       (95, 190, 280) - (105, 210, 320)        (-5, -10, -20) to (5, 10, 20)       // starting point
         *       (105, 210, 320) - (115, 230, 360)       (5, 10, 20) to (-5, -10, -20)       // back mirror
         *       (115, 230, 360) - (125, 250, 400)       (-5, -10, -20) to (5, 10, 20)       // forward mirror
         *       ...                                     ...
         * When below the starting point, both forward and back mirrors will be adjusted by UvEpsilon except for points that fall on the
         * shape minimums.
         * When above the starting point, only back mirrors will be adjusted by UvEpsilon.
         */

        GradientTransformSetupData setup = { GradientSignal::WrappingType::Mirror };
        GradientTransformTestData tests[] = {
            // Test: Input exactly 2x below minimum bounds
            // When landing exactly on the 2x boundary, we return the minumum shape bounds. There is no adjustment by epsilon
            // on the minimum side of the bounds, even when we're in a mirror below the shape bounds.
            { { 75.0f, 150.0f, 200.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test: Input within 2nd mirror repeat below minimum bounds
            // The second mirror repeat should go forward in values, but will be adjusted by UvEpsilon since we're below the
            // minimum bounds.
            { { 84.0f, 168.0f, 237.0f }, { 4.0f - UvEpsilon, 8.0f - UvEpsilon, 17.0f - UvEpsilon }, false },

            // Test: Input exactly 1x below minimum bounds.
            // When landing exactly on the 1x boundary, we return the maximum shape bounds minus epsilon.
            { { 85.0f, 170.0f, 240.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, false },

            // Test: Input within 1st mirror repeat below minimum bounds
            // The first mirror repeat should go backwards in values, but will be adjusted by UvEpsilon since we're below the
            // minimum bounds.
            { { 94.0f, 188.0f, 277.0f }, { -4.0f - UvEpsilon, -8.0f - UvEpsilon, -17.0f - UvEpsilon }, false },

            // Test: Input inside shape bounds
            // The translated input position is (1, 2, 3) is inside the shape bounds, so we should just get the translated
            // position back as output.
            { { 101.0f, 202.0f, 303.0f }, { 1.0f, 2.0f, 3.0f }, false },

            // Test: Input within 1st mirror repeat above maximum bounds
            // The first mirror repeat should go backwards in values. We're above the maximum bounds, so the expected result
            // is (4, 8, 17) minus an epsilon.
            { { 106.0f, 212.0f, 323.0f }, { 4.0f - UvEpsilon, 8.0f - UvEpsilon, 17.0f - UvEpsilon }, false },

            // Test: Input exactly 2x above minimum bounds.
            // When landing exactly on the 2x boundary, we return the exact minimum value again.
            { { 115.0f, 230.0f, 360.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test: Input within 2nd mirror repeat above maximum bounds
            // The second mirror repeat should go forwards in values. We're above the maximum bounds, so the expected result
            // is (-4, -8, -17) with no epsilon.
            { { 116.0f, 232.0f, 363.0f }, { -4.0f, -8.0f, -17.0f }, false },

            // Test: Input exactly 2x above maximum bounds
            // When landing exactly on the 2x boundary, we return the maximum adjusted by the epsilon again.
            { { 125.0f, 250.0f, 400.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, false }
        };

        for (auto& test : tests)
        {
            TestGradientTransform(setup, test);
        }
    }

    TEST_F(GradientSignalTransformTestsFixture, RepeatReturnsRepeatingValuesBasedOnShapeBounds)
    {
        /* Here's how the results are expected to work for various inputs when using Repeat wrapping.
         * This assumes shape half extents of (5, 10, 20), and a center translation of (100, 200, 300):
         *       Inputs:                                 Outputs:
         *       ...                                     ...
         *       (75, 150, 200) - (85, 170, 240)         (-5, -10, -20) to (5, 10, 20)
         *       (85, 170, 240) - (95, 190, 280)         (-5, -10, -20) to (5, 10, 20)
         *       (95, 190, 280) - (105, 210, 320)        (-5, -10, -20) to (5, 10, 20)       // starting point
         *       (105, 210, 320) - (115, 230, 360)       (-5, -10, -20) to (5, 10, 20)
         *       (115, 230, 360) - (125, 250, 400)       (-5, -10, -20) to (5, 10, 20)
         *       ...                                     ...
         * Every shape min/max boundary point below the starting point will have the max shape value.
         * Every shape min/max boundary point above the starting point with have the min shape value.
         */


        GradientTransformSetupData setup = { GradientSignal::WrappingType::Repeat };
        GradientTransformTestData tests[] = {
            // Test: 2x below minimum shape bounds
            // We're on a shape boundary below the minimum bounds, so it should return the maximum.
            { { 75.0f, 150.0f, 200.0f }, { 5.0f, 10.0f, 20.0f }, false },

            // Test: Input within 2nd repeat below minimum shape bounds
            // Every repeat should go forwards in values.
            { { 76.0f, 152.0f, 203.0f }, { -4.0f, -8.0f, -17.0f }, false },

            // Test: 1x below minimum shape bounds
            // We're on a shape boundary below the minimum bounds, so it should return the maximum.
            { { 85.0f, 170.0f, 240.0f }, { 5.0f, 10.0f, 20.0f }, false },

            // Test: Input within 1st repeat below minimum shape bounds
            // Every repeat should go forwards in values.
            { { 86.0f, 172.0f, 243.0f }, { -4.0f, -8.0f, -17.0f }, false },

            // Test: Input exactly on minimum shape bounds
            // This should return the actual minimum bounds.
            { { 95.0f, 190.0f, 280.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test: Input inside shape bounds
            // This should return the mapped value.
            { { 101.0f, 202.0f, 303.0f }, { 1.0f, 2.0f, 3.0f }, false },

            // Test: Input exactly on maximum shape bounds
            // We're on a shape boundary above the minimum bounds, so it should return the minimum.
            { { 105.0f, 210.0f, 320.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test: Input within 1st repeat above maximum shape bounds
            // Every repeat should go forwards in values.
            { { 106.0f, 212.0f, 323.0f }, { -4.0f, -8.0f, -17.0f }, false },

            // Test: 1x above maximum shape bounds
            // We're on a shape boundary above the minimum bounds, so it should return the minimum.
            { { 105.0f, 210.0f, 320.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test: Input within 2nd repeat above maximum shape bounds
            // Every repeat should go forwards in values.
            { { 106.0f, 212.0f, 323.0f }, { -4.0f, -8.0f, -17.0f }, false },
        };

        for (auto& test : tests)
        {
            TestGradientTransform(setup, test);
        }
    }

    TEST_F(GradientSignalTransformTestsFixture, ClampToZeroReturnsClampedValuesBasedOnShapeBounds)
    {
        GradientTransformSetupData setup = { GradientSignal::WrappingType::ClampToZero };
        GradientTransformTestData tests[] = {
            // Test:  Input point far below minimum shape bounds
            // Our input point is below the minimum of shape bounds, so the result should be the minimum corner of the shape.
            // Points outside the shape bounds should return "true" for rejected.
            { { 0.0f, 0.0f, 0.0f }, { -5.0f, -10.0f, -20.0f }, true },

            // Test:  Input point directly on minimum shape bounds
            // Our input point is directly on the minimum of shape bounds, so the result should be the minimum corner of the shape.
            { { 95.0f, 190.0f, 280.0f }, { -5.0f, -10.0f, -20.0f }, false },

            // Test:  Input point inside shape bounds
            // Our input point is inside the shape bounds, so the result is just input - translation.
            { { 101.0f, 202.0f, 303.0f }, { 1.0f, 2.0f, 3.0f }, false },

            // Test:  Input point directly on maximum shape bounds
            // On the maximum side, GradientTransform clamps to "max - epsilon" for consistency with other wrapping types, so our
            // expected results are the max shape corner - epsilon.
            // Points outside the shape bounds (which includes the maximum edge of the shape bounds) should return "true" for rejected.
            { { 105.0f, 210.0f, 320.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, true },

            // Test:  Input point far above maximum shape bounds
            // On the maximum side, GradientTransform clamps to "max - epsilon" for consistency with other wrapping types, so our
            // expected results are the max shape corner - epsilon.
            // Points outside the shape bounds should return "true" for rejected.
            { { 1000.0f, 1000.0f, 1000.0f }, { 5.0f - UvEpsilon, 10.0f - UvEpsilon, 20.0f - UvEpsilon }, true },
        };

        for (auto& test : tests)
        {
            TestGradientTransform(setup, test);
        }
    }
}


