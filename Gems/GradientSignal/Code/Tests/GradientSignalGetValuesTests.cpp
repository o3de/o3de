/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    struct GradientSignalGetValuesTestsFixture
        : public GradientSignalTest
    {
        // Create an arbitrary size shape for comparing values within. It should be large enough that we detect any value anomalies
        // but small enough that the tests run quickly.
        const float TestShapeHalfBounds = 128.0f;

        void CompareGetValueAndGetValues(AZ::EntityId gradientEntityId)
        {
            // Create a gradient sampler and run through a series of points to see if they match expectations.

            const AZ::Aabb queryRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds));
            const AZ::Vector2 stepSize(1.0f, 1.0f);

            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = gradientEntityId;

            const size_t numSamplesX = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetX() / stepSize.GetX()));
            const size_t numSamplesY = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetY() / stepSize.GetY()));

            // Build up the list of positions to query.
            AZStd::vector<AZ::Vector3> positions(numSamplesX * numSamplesY);
            size_t index = 0;
            for (size_t yIndex = 0; yIndex < numSamplesY; yIndex++)
            {
                float y = queryRegion.GetMin().GetY() + (stepSize.GetY() * yIndex);
                for (size_t xIndex = 0; xIndex < numSamplesX; xIndex++)
                {
                    float x = queryRegion.GetMin().GetX() + (stepSize.GetX() * xIndex);
                    positions[index++] = AZ::Vector3(x, y, 0.0f);
                }
            }

            // Get the results from GetValues
            AZStd::vector<float> results(numSamplesX * numSamplesY);
            gradientSampler.GetValues(positions, results);

            // For each position, call GetValue and verify that the values match.
            for (size_t positionIndex = 0; positionIndex < positions.size(); positionIndex++)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = positions[positionIndex];
                float value = gradientSampler.GetValue(params);

                // We use ASSERT_EQ instead of EXPECT_EQ because if one value doesn't match, they probably all won't, so there's no reason
                // to keep running and printing failures for every value.
                ASSERT_EQ(value, results[positionIndex]);
            }
        }
    };

    TEST_F(GradientSignalGetValuesTestsFixture, ImageGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestImageGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PerlinGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestPerlinGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, RandomGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestRandomGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ConstantGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestConstantGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ShapeAreaFalloffGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestShapeAreaFalloffGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, DitherGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);

        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, InvertGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestInvertGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, LevelsGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestLevelsGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, MixedGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto mixedEntity = BuildTestConstantGradient(TestShapeHalfBounds);
        auto entity = BuildTestMixedGradient(TestShapeHalfBounds, baseEntity->GetId(), mixedEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PosterizeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestPosterizeGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ReferenceGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestReferenceGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SmoothStepGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestSmoothStepGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ThresholdGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestThresholdGradient(TestShapeHalfBounds, baseEntity->GetId());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceAltitudeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceAltitudeGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceMaskGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceMaskGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceSlopeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceSlopeGradient(TestShapeHalfBounds);
        CompareGetValueAndGetValues(entity->GetId());
    }
}


