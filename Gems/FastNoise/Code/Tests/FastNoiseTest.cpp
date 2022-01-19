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
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

class FastNoiseTestApp : public ::testing::Test
{
};

TEST_F(FastNoiseTestApp, FastNoise_Component)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
}

TEST_F(FastNoiseTestApp, FastNoise_ComponentMatchesConfiguration)
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

TEST_F(FastNoiseTestApp, FastNoise_ComponentEbus)
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

// This uses custom test / benchmark hooks so that we can load LmbrCentral and GradientSignal Gems.
AZ_UNIT_TEST_HOOK(new UnitTest::FastNoiseTestEnvironment, UnitTest::FastNoiseBenchmarkEnvironment);

