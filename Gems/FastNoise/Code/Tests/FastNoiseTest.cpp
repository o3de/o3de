/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <FastNoiseGradientComponent.h>
#include <FastNoiseTest.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformModifierRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

class FastNoiseTest : public ::testing::Test
{
};

TEST_F(FastNoiseTest, FastNoise_ComponentCreatesSuccessfully)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
}

TEST_F(FastNoiseTest, FastNoise_ComponentMatchesConfiguration)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);

    FastNoiseGem::FastNoiseGradientConfig cfg;
    FastNoiseGem::FastNoiseGradientConfig componentConfig;

    noiseEntity->CreateComponent<AzFramework::TransformComponent>();
    noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
    noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>();
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg);

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
    noiseComp->WriteOutConfig(&componentConfig);
    ASSERT_EQ(cfg, componentConfig);
}

TEST_F(FastNoiseTest, FastNoise_ComponentEbusWorksSuccessfully)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<AzFramework::TransformComponent>();
    noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
    noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>();
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();

    noiseEntity->Init();
    noiseEntity->Activate();

    GradientSignal::GradientSampleParams params;
    float sample = -1.0f;

    GradientSignal::GradientRequestBus::EventResult(sample, noiseEntity->GetId(),
        &GradientSignal::GradientRequestBus::Events::GetValue, params);
    ASSERT_TRUE(sample >= 0.0f);
    ASSERT_TRUE(sample <= 1.0f);
}

TEST_F(FastNoiseTest, FastNoise_VerifyGetValueAndGetValuesMatch)
{
    const float shapeHalfBounds = 128.0f;

    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<AzFramework::TransformComponent>();
    noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>();

    // Create a Box Shape to map our gradient into
    LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(shapeHalfBounds * 2.0f));
    auto boxComponent = noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
    boxComponent->SetConfiguration(boxConfig);

    // Create a Fast Noise component with an adjusted frequency. (The defaults of Perlin noise with frequency=1.0 would cause us
    // to always get back the same noise value)
    FastNoiseGem::FastNoiseGradientConfig cfg;
    cfg.m_frequency = 0.01f;
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg);

    noiseEntity->Init();
    noiseEntity->Activate();

    // Create a gradient sampler and run through a series of points to see if they match expectations.

    const AZ::Aabb queryRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-shapeHalfBounds), AZ::Vector3(shapeHalfBounds));
    const AZ::Vector2 stepSize(1.0f, 1.0f);

    GradientSignal::GradientSampler gradientSampler;
    gradientSampler.m_gradientId = noiseEntity->GetId();

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

        // We use ASSERT_NEAR instead of EXPECT_NEAR because if one value doesn't match, they probably all won't, so there's no
        // reason to keep running and printing failures for every value.
        ASSERT_NEAR(value, results[positionIndex], 0.000001f);
    }
}

// This uses custom test / benchmark hooks so that we can load LmbrCentral and GradientSignal Gems.
AZ_UNIT_TEST_HOOK(new UnitTest::FastNoiseTestEnvironment, UnitTest::FastNoiseBenchmarkEnvironment);

