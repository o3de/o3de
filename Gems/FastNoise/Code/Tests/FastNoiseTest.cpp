/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
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
    void TransformPositionToUVW([[maybe_unused]] const AZ::Vector3& inPosition, [[maybe_unused]] AZ::Vector3& outUVW, [[maybe_unused]] const bool shouldNormalizeOutput, [[maybe_unused]] bool& wasPointRejected) const override {}
    void GetGradientLocalBounds([[maybe_unused]] AZ::Aabb& bounds) const override {}
    void GetGradientEncompassingBounds([[maybe_unused]] AZ::Aabb& bounds) const override {}

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
};

struct MockGlobalEnvironment
{
    MockGlobalEnvironment()
    {
        m_stubEnv.pCryPak = &m_stubPak;
        m_stubEnv.pConsole = &m_stubConsole;
        m_stubEnv.pSystem = &m_stubSystem;
        gEnv = &m_stubEnv;
    }

    ~MockGlobalEnvironment()
    {
        gEnv = nullptr;
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    testing::NiceMock<CryPakMock> m_stubPak;
    testing::NiceMock<ConsoleMock> m_stubConsole;
    testing::NiceMock<SystemMock> m_stubSystem;
};

TEST(FastNoiseTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    MockGlobalEnvironment mocks;

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
    : public ::testing::Test
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
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
        appDesc.m_stackRecordLevels = 20;

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
    MockGlobalEnvironment m_mocks;
};

//////////////////////////////////////////////////////////////////////////
// testing class to inspect protected data members in the FastNoiseGradientComponent
struct FastNoiseGradientComponentTester : public FastNoiseGem::FastNoiseGradientComponent
{
    const FastNoiseGem::FastNoiseGradientConfig& GetConfig() const { return m_configuration; }

    void AssertTrue(const FastNoiseGem::FastNoiseGradientConfig& cfg)
    {
        ASSERT_TRUE(m_configuration.m_cellularDistanceFunction == cfg.m_cellularDistanceFunction);
        ASSERT_TRUE(m_configuration.m_cellularJitter == cfg.m_cellularJitter);
        ASSERT_TRUE(m_configuration.m_cellularReturnType == cfg.m_cellularReturnType);
        ASSERT_TRUE(m_configuration.m_fractalType == cfg.m_fractalType);
        ASSERT_TRUE(m_configuration.m_frequency == cfg.m_frequency);
        ASSERT_TRUE(m_configuration.m_gain == cfg.m_gain);
        ASSERT_TRUE(m_configuration.m_interp == cfg.m_interp);
        ASSERT_TRUE(m_configuration.m_lacunarity == cfg.m_lacunarity);
        ASSERT_TRUE(m_configuration.m_noiseType == cfg.m_noiseType);
        ASSERT_TRUE(m_configuration.m_octaves == cfg.m_octaves);
        ASSERT_TRUE(m_configuration.m_seed == cfg.m_seed);
    }
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

    noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg);
    noiseEntity->CreateComponent<MockGradientTransformComponent>();

    m_application.AddEntity(noiseEntity);

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
    reinterpret_cast<FastNoiseGradientComponentTester*>(noiseComp)->AssertTrue(cfg);
}

#if FASTNOISE_EDITOR
#include <EditorFastNoiseGradientComponent.h>

TEST_F(FastNoiseTestApp, FastNoise_EditorCreateGameEntity)
{
    AZStd::unique_ptr<AZ::Entity> noiseEntity(aznew AZ::Entity("editor_noise_entity"));
    ASSERT_TRUE(noiseEntity != nullptr);

    FastNoiseGem::EditorFastNoiseGradientComponent editor;
    auto* editorBase = static_cast<AzToolsFramework::Components::EditorComponentBase*>(&editor);
    editorBase->BuildGameEntity(noiseEntity.get());

    // the new game entity's ocean component should look like the default one
    FastNoiseGem::FastNoiseGradientConfig cfg;

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);
    reinterpret_cast<FastNoiseGradientComponentTester*>(noiseComp)->AssertTrue(cfg);
}

#endif

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

