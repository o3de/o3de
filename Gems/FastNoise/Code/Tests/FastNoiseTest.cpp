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
#include <GradientSignalTestHelpers.h>
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
    auto noiseEntity = AZStd::make_unique<AZ::Entity>("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    AZStd::unique_ptr<AZ::Component> createdComponent {noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>()};

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_EQ(noiseComp, createdComponent.get());
}

TEST_F(FastNoiseTest, FastNoise_ComponentMatchesConfiguration)
{
    auto noiseEntity = AZStd::make_unique<AZ::Entity>("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);

    FastNoiseGem::FastNoiseGradientConfig cfg;
    FastNoiseGem::FastNoiseGradientConfig componentConfig;

    AZStd::unique_ptr<AZ::Component> transformComponent {noiseEntity->CreateComponent<AzFramework::TransformComponent>()};
    AZStd::unique_ptr<AZ::Component> boxShapeComponent {noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId)};
    AZStd::unique_ptr<AZ::Component> gradientTransformComponent {noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>()};
    AZStd::unique_ptr<AZ::Component> createdNoiseComponent {noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg)};

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_EQ(noiseComp, createdNoiseComponent.get());
    noiseComp->WriteOutConfig(&componentConfig);
    ASSERT_EQ(cfg, componentConfig);
}

TEST_F(FastNoiseTest, FastNoise_ComponentEbusWorksSuccessfully)
{
    auto noiseEntity = AZStd::make_unique<AZ::Entity>("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    AZStd::unique_ptr<AZ::Component> transformComponent {noiseEntity->CreateComponent<AzFramework::TransformComponent>()};
    AZStd::unique_ptr<AZ::Component> boxShapeComponent {noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId)};
    AZStd::unique_ptr<AZ::Component> gradientTransformComponent {noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>()};
    AZStd::unique_ptr<AZ::Component> createdNoiseComponent {noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>()};

    noiseEntity->Init();
    noiseEntity->Activate();

    GradientSignal::GradientSampleParams params;
    float sample = -1.0f;

    GradientSignal::GradientRequestBus::EventResult(sample, noiseEntity->GetId(),
        &GradientSignal::GradientRequestBus::Events::GetValue, params);
    ASSERT_TRUE(sample >= 0.0f);
    ASSERT_TRUE(sample <= 1.0f);

    noiseEntity->Deactivate();
}

TEST_F(FastNoiseTest, FastNoise_VerifyGetValueAndGetValuesMatch)
{
    const float shapeHalfBounds = 128.0f;

    auto noiseEntity = AZStd::make_unique<AZ::Entity>("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    AZStd::unique_ptr<AZ::Component> transformComponent {noiseEntity->CreateComponent<AzFramework::TransformComponent>()};
    AZStd::unique_ptr<AZ::Component> gradientTransformComponent {noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>()};

    // Create a Box Shape to map our gradient into
    LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(shapeHalfBounds * 2.0f));
    AZStd::unique_ptr<AZ::Component> boxComponent {noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId)};
    boxComponent->SetConfiguration(boxConfig);

    // Create a Fast Noise component with an adjusted frequency. (The defaults of Perlin noise with frequency=1.0 would cause us
    // to always get back the same noise value)
    FastNoiseGem::FastNoiseGradientConfig cfg;
    cfg.m_frequency = 0.01f;
    AZStd::unique_ptr<AZ::Component> createdNoiseComponent {noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg)};

    noiseEntity->Init();
    noiseEntity->Activate();

    // Create a gradient sampler and run through a series of points to see if they match expectations.
    UnitTest::GradientSignalTestHelpers::CompareGetValueAndGetValues(noiseEntity->GetId(), -shapeHalfBounds, shapeHalfBounds);

    noiseEntity->Deactivate();
}

// This uses custom test / benchmark hooks so that we can load LmbrCentral and GradientSignal Gems.
AZ_UNIT_TEST_HOOK(new UnitTest::FastNoiseTestEnvironment, UnitTest::FastNoiseBenchmarkEnvironment);

