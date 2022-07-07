/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>
#include <Tests/GradientSignalTestHelpers.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    struct GradientSignalGetValuesTestsFixture
        : public GradientSignalTest
    {
        // Create an arbitrary size shape for comparing values within. It should be large enough that we detect any value anomalies
        // but small enough that the tests run quickly.
        const float TestShapeHalfBounds = 128.0f;
    };

    TEST_F(GradientSignalGetValuesTestsFixture, ImageGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestImageGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PerlinGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestPerlinGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, RandomGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestRandomGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ConstantGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestConstantGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ShapeAreaFalloffGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestShapeAreaFalloffGradient(TestShapeHalfBounds);

        // Use a query range larger than our shape to ensure that we're getting falloff values within our query bounds.
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), -TestShapeHalfBounds, TestShapeHalfBounds * 3.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, DitherGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);

        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, InvertGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestInvertGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, LevelsGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestLevelsGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, MixedGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto mixedEntity = BuildTestConstantGradient(TestShapeHalfBounds);
        auto entity = BuildTestMixedGradient(TestShapeHalfBounds, baseEntity->GetId(), mixedEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, PosterizeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestPosterizeGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ReferenceGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestReferenceGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SmoothStepGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestSmoothStepGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, ThresholdGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestThresholdGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceAltitudeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestSurfaceAltitudeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceMaskGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestSurfaceMaskGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }

    TEST_F(GradientSignalGetValuesTestsFixture, SurfaceSlopeGradientComponent_VerifyGetValueAndGetValuesMatch)
    {
        auto entity = BuildTestSurfaceSlopeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::CompareGetValueAndGetValues(entity->GetId(), 0.0f, TestShapeHalfBounds * 2.0f);
    }
}


