/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/GradientSignalTestFixtures.h>

#include <AzTest/AzTest.h>

#include <AzFramework/Components/TransformComponent.h>

#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/InvertGradientComponent.h>

namespace UnitTest
{
    struct GradientSignalServicesTestsFixture
        : public GradientSignalTest
    {
    };

    TEST_F(GradientSignalServicesTestsFixture, ConstantGradientComponent_KnownValue)
    {
        // Given a constant value as input, verify that sampling a set of points all produces that same constant value.

        constexpr float dataSize = 8.0f;
        constexpr float expectedOutput = 0.123f;
        GradientSignal::ConstantGradientConfig config;
        config.m_value = expectedOutput;

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::ConstantGradientComponent>(config);
        ActivateEntity(entity.get());

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();
        for (float x = 0.0f; x < dataSize; x++)
        {
            for (float y = 0.0f; y < dataSize; y++)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);
                EXPECT_EQ(expectedOutput, gradientSampler.GetValue(params));
            }
        }
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_4x4At50Pct)
    {
        // With a 4x4 gradient filled with 8/16 (0.5), verify that the resulting dithered output 
        // is an expected checkerboard pattern with 8 of 16 pixels filled.

        constexpr int dataSize = 4;

        AZStd::vector<float> inputData(dataSize * dataSize, 8.0f / 16.0f);
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_4x4At50Pct_CrossingZero)
    {
        // With a 4x4 gradient filled with 8/16 (0.5), verify that the resulting dithered output
        // is an expected checkerboard pattern with 8 of 16 pixels filled. The pattern offset is
        // shifted -2 in the X direction so that the lookups go from [-2, 2) to verify that the
        // pattern remains consistent across negative and positive coordinates.

        constexpr int dataSize = 4;

        AZStd::vector<float> inputData(dataSize * dataSize, 8.0f / 16.0f);
        AZStd::vector<float> expectedOutput = {
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3(-2.0f, 0.0f, 0.0f);
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_4x4At50Pct_MorePointsPerUnit)
    {
        // With a 4x4 gradient filled with 8/16 (0.5), and 1/2 point per unit, if we query a 4x4 region,
        // we should get a checkerboard in 2x2 blocks of the same value because it takes 2 units before the value changes.

        constexpr int dataSize = 4;

        AZStd::vector<float> inputData(dataSize * dataSize, 8.0f / 16.0f);
        AZStd::vector<float> expectedOutput = {
            1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 0.5f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_4x4At50Pct_MorePointsAndCrossingZero)
    {
        // With a 4x4 gradient filled with 8/16 (0.5), and 2 points per unit, verify that querying
        // from -1 to 1 produces a constant checkerboard pattern of results as it crosses the 0 boundary.
        // Our expected results are a consistent checkerboard pattern, but with 2x2 blocks of the same value because we're
        // querying at 2x the point density (i.e. querying 4 points per unit) to ensure that fractional position lookups work too.

        float expectedValues[] = {
            1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        };

        // Create a 50% constant gradient.
        GradientSignal::ConstantGradientConfig constantConfig;
        constantConfig.m_value = 8.0f / 16.0f;
        auto constantGradientEntity = CreateEntity();
        constantGradientEntity->CreateComponent<GradientSignal::ConstantGradientComponent>(constantConfig);
        ActivateEntity(constantGradientEntity.get());

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 2.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = constantGradientEntity->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        // Run through [-1, 1) at 1/4 intervals and make sure we get our expected checkerboard. This is testing both that
        // we have a consistent pattern across the 0 boundary and that fractional position lookups work correctly
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();
        int expectedValueIndex = 0;
        for (float y = -1.0f; y < 1.0f; y += 0.25f)
        {
            for (float x = -1.0f; x < 1.0f; x += 0.25f)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);

                float actualValue = gradientSampler.GetValue(params);
                float expectedValue = expectedValues[expectedValueIndex++];

                EXPECT_NEAR(actualValue, expectedValue, 0.01f);
            }
        }
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_4x4At31Pct)
    {
        // With a 4x4 gradient filled with 5/16 (0.3125), verify that the resulting dithered output 
        // has the correct 5 of 16 pixels set

        constexpr int dataSize = 4;

        AZStd::vector<float> inputData(dataSize * dataSize, 5.0f / 16.0f);
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_8x8At50Pct)
    {
        // With an 8x8 gradient filled with 32/64 (0.5), verify that the resulting dithered output 
        // is an expected checkerboard pattern with 32 of 64 pixels filled.

        constexpr int dataSize = 8;

        AZStd::vector<float> inputData(dataSize * dataSize, 32.0f / 64.0f);
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_8x8;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, DitherGradientComponent_8x8At55Pct)
    {
        // With an 8x8 gradient filled with 35/64 (0.546875), verify that the resulting dithered output 
        // has the correct 35 of 64 pixels set

        constexpr int dataSize = 8;

        AZStd::vector<float> inputData(dataSize * dataSize, 35.0f / 64.0f);
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_8x8;
        config.m_gradientSampler.m_gradientId = entityMock->GetId();

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalServicesTestsFixture, InvertGradientComponent_InvertKnownPoints)
    {
        // Try inverting 0, 1, 0.5, and 0.2 (endpoints, middle, and arbitrary value) and verify
        // that we get back the expected inverted results.

        constexpr int dataSize = 2;
        AZStd::vector<float> inputData =
        { 
            0.0f, 1.0f,
            0.5f, 0.2f
        };
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.0f,
            0.5f, 0.8f
        };

        auto entityMock = CreateEntity();
        const AZ::EntityId id = entityMock->GetId();
        UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        // Create the entity with an arbitrarily-sized box.
        const float HalfBounds = 64.0f;
        auto entity = BuildTestInvertGradient(HalfBounds, entityMock->GetId());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }
}


