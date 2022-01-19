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
#include <AzCore/Math/Random.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/clocks.h>
#include <FastNoiseSystemComponent.h>
#include <FastNoiseGradientComponent.h>
#include <FastNoiseModule.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformModifierRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

class MockGradientTransformComponent
    : public AZ::Component
    , private GradientSignal::GradientTransformRequestBus::Handler
    , private GradientSignal::GradientTransformModifierRequestBus::Handler
{
public:
    AZ_COMPONENT(MockGradientTransformComponent, "{464CF47B-7E10-4E1B-BD06-79BD2AC91399}");

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }
    static void Reflect([[maybe_unused]] AZ::ReflectContext* context) {}

    MockGradientTransformComponent() = default;
    ~MockGradientTransformComponent() = default;

    // AZ::Component interface
    void Activate() override {}
    void Deactivate() override {}

    ////////////////////////////////////////////////////////////////////////////
    //// GradientTransformRequestBus
    const GradientSignal::GradientTransform& GetGradientTransform() const override
    {
        return m_gradientTransform;
    }

    //////////////////////////////////////////////////////////////////////////
    // GradientTransformModifierRequestBus
    bool GetAllowReference() const override { return false; }
    void SetAllowReference([[maybe_unused]] bool value) override {}

    AZ::EntityId GetShapeReference() const override { return AZ::EntityId(); }
    void SetShapeReference([[maybe_unused]] AZ::EntityId shapeReference) override {}

    bool GetOverrideBounds() const override { return false; }
    void SetOverrideBounds([[maybe_unused]] bool value) override {}

    AZ::Vector3 GetBounds() const override { return AZ::Vector3(); }
    void SetBounds([[maybe_unused]] AZ::Vector3 bounds) override {}

    GradientSignal::TransformType GetTransformType() const override { return static_cast<GradientSignal::TransformType>(0); }
    void SetTransformType([[maybe_unused]] GradientSignal::TransformType type) override {}

    bool GetOverrideTranslate() const override { return false; }
    void SetOverrideTranslate([[maybe_unused]] bool value) override {}

    AZ::Vector3 GetTranslate() const override { return AZ::Vector3(); }
    void SetTranslate([[maybe_unused]] AZ::Vector3 translate) override {}

    bool GetOverrideRotate() const override { return false; }
    void SetOverrideRotate([[maybe_unused]] bool value) override {}

    AZ::Vector3 GetRotate() const override { return AZ::Vector3(); }
    void SetRotate([[maybe_unused]] AZ::Vector3 rotate) override {}

    bool GetOverrideScale() const override { return false; }
    void SetOverrideScale([[maybe_unused]] bool value) override {}

    AZ::Vector3 GetScale() const override { return AZ::Vector3(); }
    void SetScale([[maybe_unused]] AZ::Vector3 scale) override {}

    float GetFrequencyZoom() const override { return false; }
    void SetFrequencyZoom([[maybe_unused]] float frequencyZoom) override {}

    GradientSignal::WrappingType GetWrappingType() const override { return static_cast<GradientSignal::WrappingType>(0); } 
    void SetWrappingType([[maybe_unused]] GradientSignal::WrappingType type) override {}

    bool GetIs3D() const override { return false; }
    void SetIs3D([[maybe_unused]] bool value) override {}

    bool GetAdvancedMode() const override { return false; }
    void SetAdvancedMode([[maybe_unused]] bool value) override {}

    GradientSignal::GradientTransform m_gradientTransform;
};

TEST(FastNoiseTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    app.RegisterComponentDescriptor(FastNoiseGem::FastNoiseSystemComponent::CreateDescriptor());
    systemEntity->CreateComponent<FastNoiseGem::FastNoiseSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    AZ::Entity* noiseEntity = aznew AZ::Entity("fastnoise_entity");
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();
    app.AddEntity(noiseEntity);

    app.Destroy();
    ASSERT_TRUE(true);
}

class FastNoiseTestApp
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    FastNoiseTestApp()
        : m_application()
        , m_systemEntity(nullptr)
    {
    }

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new FastNoiseGem::FastNoiseModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_application.RegisterComponentDescriptor(MockGradientTransformComponent::CreateDescriptor());
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity;
};

TEST_F(FastNoiseTestApp, FastNoise_Component)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();
    m_application.AddEntity(noiseEntity);

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
}

TEST_F(FastNoiseTestApp, FastNoise_ComponentEbus)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);
    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();
    noiseEntity->CreateComponent<MockGradientTransformComponent>();

    noiseEntity->Init();
    noiseEntity->Activate();

    GradientSignal::GradientSampleParams params;
    float sample = -1.0f;

    GradientSignal::GradientRequestBus::EventResult(sample, noiseEntity->GetId(), &GradientSignal::GradientRequestBus::Events::GetValue, params);
    ASSERT_TRUE(sample >= 0.0f);
    ASSERT_TRUE(sample <= 1.0f);
}

TEST_F(FastNoiseTestApp, FastNoise_ComponentMatchesConfiguration)
{
    AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
    ASSERT_TRUE(noiseEntity != nullptr);

    AZ::SimpleLcgRandom rand(AZStd::GetTimeNowMicroSecond());

    FastNoiseGem::FastNoiseGradientConfig cfg;
    FastNoiseGem::FastNoiseGradientConfig componentConfig;

    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg);
    noiseEntity->CreateComponent<MockGradientTransformComponent>();

    m_application.AddEntity(noiseEntity);

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
    noiseComp->WriteOutConfig(&componentConfig);
    ASSERT_EQ(cfg, componentConfig);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

