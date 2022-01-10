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
        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestImageGradient(entity.get());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PerlinGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestPerlinGradient(entity.get());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, RandomGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(entity.get());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ConstantGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestConstantGradient(entity.get());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, DitherGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(baseEntity.get());
        ActivateEntity(baseEntity.get());

        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestDitherGradient(entity.get(), baseEntity->GetId());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, InvertGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(baseEntity.get());
        ActivateEntity(baseEntity.get());

        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestInvertGradient(entity.get(), baseEntity->GetId());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, LevelsGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(baseEntity.get());
        ActivateEntity(baseEntity.get());

        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestLevelsGradient(entity.get(), baseEntity->GetId());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PosterizeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(baseEntity.get());
        ActivateEntity(baseEntity.get());

        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestPosterizeGradient(entity.get(), baseEntity->GetId());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ReferenceGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestRandomGradient(baseEntity.get());
        ActivateEntity(baseEntity.get());

        auto entity = CreateTestEntity(TestShapeHalfBounds);
        CreateTestReferenceGradient(entity.get(), baseEntity->GetId());
        ActivateEntity(entity.get());
        CompareGetValueAndGetValues(entity->GetId());
    }
}


