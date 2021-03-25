/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "RenderToTexture_precompiled.h"

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
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
#include <AzCore/Module/Environment.h>

#include "CryCommon/CryLibrary.h"
#include "CrySystem/ExtensionSystem/CryFactoryRegistryImpl.h"

#include "../Source/RenderToTextureComponent.h"
#include "../Source/RenderToTextureModule.h"
#if RENDER_TO_TEXTURE_EDITOR
#include "../Source/EditorRenderToTextureComponent.h"
#endif

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

struct MockGlobalEnvironment
{
    MockGlobalEnvironment()
    {
        memset(&m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_stubEnv.pTimer = &m_stubTimer;
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
    testing::NiceMock<TimerMock> m_stubTimer;
    testing::NiceMock<CryPakMock> m_stubPak;
    testing::NiceMock<ConsoleMock> m_stubConsole;
    testing::NiceMock<SystemMock> m_stubSystem;
};

TEST(RenderToTextureTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    // create the mock global environment
    MockGlobalEnvironment mocks;

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    systemEntity->Init();
    systemEntity->Activate();

    AZ::Entity* renderToTextureEntity = aznew AZ::Entity("rendertotexture_entity");
    renderToTextureEntity->CreateComponent<RenderToTexture::RenderToTextureComponent>();
    app.AddEntity(renderToTextureEntity);

    app.Destroy();
}

class RenderToTextureTestApp
    : public ::testing::Test
{
public:
    RenderToTextureTestApp()
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
            modules.emplace_back(new RenderToTexture::RenderToTextureModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
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

// testing class to compare config and protected members of the RenderToTextureComponent
struct RenderToTextureComponentTester : public RenderToTexture::RenderToTextureComponent
{
    const RenderToTexture::RenderToTextureConfig& GetConfig() const { return m_config; }
    int GetRenderTargetHandle() const { return m_renderTargetHandle; }

    void ExpectTrue(const RenderToTexture::RenderToTextureConfig& cfg)
    {
        EXPECT_TRUE(m_config.m_camera == cfg.m_camera);
        EXPECT_TRUE(m_config.m_renderContextId == cfg.m_renderContextId);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_width == cfg.m_renderContextConfig.m_width);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_height == cfg.m_renderContextConfig.m_height);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_sRGBWrite == cfg.m_renderContextConfig.m_sRGBWrite);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_alphaMode == cfg.m_renderContextConfig.m_alphaMode);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_oceanEnabled == cfg.m_renderContextConfig.m_oceanEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_terrainEnabled == cfg.m_renderContextConfig.m_terrainEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_vegetationEnabled == cfg.m_renderContextConfig.m_vegetationEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_shadowsEnabled == cfg.m_renderContextConfig.m_shadowsEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_shadowsNumCascades == cfg.m_renderContextConfig.m_shadowsNumCascades);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_shadowsGSMRange == cfg.m_renderContextConfig.m_shadowsGSMRange);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_shadowsGSMRangeStep == cfg.m_renderContextConfig.m_shadowsGSMRangeStep);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_depthOfFieldEnabled == cfg.m_renderContextConfig.m_depthOfFieldEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_motionBlurEnabled == cfg.m_renderContextConfig.m_motionBlurEnabled);
        EXPECT_TRUE(m_config.m_renderContextConfig.m_aaMode == cfg.m_renderContextConfig.m_aaMode);
        EXPECT_TRUE(m_config.m_maxFPS == cfg.m_maxFPS);
        EXPECT_TRUE(m_config.m_displayDebugImage == cfg.m_displayDebugImage);
    }
};

TEST_F(RenderToTextureTestApp, RTT_RenderToTextureComponentDefaults)
{
    AZ::Entity* renderToTextureEntity = aznew AZ::Entity("rendertotexture_entity");
    ASSERT_TRUE(renderToTextureEntity != nullptr);
    renderToTextureEntity->CreateComponent<RenderToTexture::RenderToTextureComponent>();

    renderToTextureEntity->Init();
    EXPECT_EQ(renderToTextureEntity->GetState(), AZ::Entity::ES_INIT);

    renderToTextureEntity->Activate();
    EXPECT_EQ(renderToTextureEntity->GetState(), AZ::Entity::ES_ACTIVE);

    RenderToTexture::RenderToTextureComponent* renderToTextureComponent = renderToTextureEntity->FindComponent<RenderToTexture::RenderToTextureComponent>();
    ASSERT_TRUE(renderToTextureComponent != nullptr);

    // render context ID should be invalid because there is no system setup to create a render context
    const RenderToTexture::RenderToTextureConfig rttConfig = reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig();
    EXPECT_TRUE(rttConfig.m_renderContextId.IsNull());

    const AzRTT::RenderContextConfig contextConfig = rttConfig.m_renderContextConfig;
    EXPECT_GT(contextConfig.m_width, 0);
    EXPECT_GT(contextConfig.m_height, 0);
    EXPECT_EQ(contextConfig.m_alphaMode, AzRTT::AlphaMode::ALPHA_OPAQUE);
    EXPECT_FALSE(contextConfig.m_sRGBWrite);
    EXPECT_TRUE(contextConfig.m_oceanEnabled);
    EXPECT_TRUE(contextConfig.m_terrainEnabled);
    EXPECT_TRUE(contextConfig.m_vegetationEnabled);
    EXPECT_TRUE(contextConfig.m_shadowsEnabled);
    EXPECT_EQ(contextConfig.m_shadowsNumCascades, -1);
    EXPECT_EQ(contextConfig.m_shadowsGSMRange, -1.f);
    EXPECT_EQ(contextConfig.m_shadowsGSMRangeStep, -1.f);
    EXPECT_FALSE(contextConfig.m_depthOfFieldEnabled);
    EXPECT_FALSE(contextConfig.m_motionBlurEnabled);
    EXPECT_EQ(contextConfig.m_aaMode, 0);

    renderToTextureEntity->Deactivate();
    EXPECT_EQ(renderToTextureEntity->GetState(), AZ::Entity::ES_INIT);

    delete renderToTextureEntity;
}

TEST_F(RenderToTextureTestApp, RTT_RenderToTextureRequestBus)
{
    AZ::Entity* renderToTextureEntity = aznew AZ::Entity("rendertotexture_entity");
    ASSERT_TRUE(renderToTextureEntity != nullptr);
    renderToTextureEntity->CreateComponent<RenderToTexture::RenderToTextureComponent>();
    renderToTextureEntity->Init();
    renderToTextureEntity->Activate();

    RenderToTexture::RenderToTextureComponent* renderToTextureComponent = renderToTextureEntity->FindComponent<RenderToTexture::RenderToTextureComponent>();
    ASSERT_TRUE(renderToTextureComponent != nullptr);

    EXPECT_TRUE(static_cast<RenderToTexture::RenderToTextureRequestBus::Handler*>(renderToTextureComponent)->BusIsConnected());

    // alpha mode
    const AzRTT::AlphaMode mode = AzRTT::AlphaMode::ALPHA_DEPTH_BASED;
    RenderToTexture::RenderToTextureRequestBus::Event(renderToTextureEntity->GetId(), &RenderToTexture::RenderToTextureRequestBus::Events::SetAlphaMode, mode);
    EXPECT_EQ(reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig().m_renderContextConfig.m_alphaMode, mode);

    // camera
    const AZ::EntityId cameraEntityId = AZ::EntityId(0x12345);
    RenderToTexture::RenderToTextureRequestBus::Event(renderToTextureEntity->GetId(), &RenderToTexture::RenderToTextureRequestBus::Events::SetCamera, cameraEntityId);
    EXPECT_EQ(reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig().m_camera, cameraEntityId);

    // max fps
    const double maxFPS = 999.0;
    RenderToTexture::RenderToTextureRequestBus::Event(renderToTextureEntity->GetId(), &RenderToTexture::RenderToTextureRequestBus::Events::SetMaxFPS, maxFPS);
    EXPECT_EQ(reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig().m_maxFPS, maxFPS);

    // srgb
    const bool sRGBEnabled = !reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig().m_renderContextConfig.m_sRGBWrite;
    RenderToTexture::RenderToTextureRequestBus::Event(renderToTextureEntity->GetId(), &RenderToTexture::RenderToTextureRequestBus::Events::SetWriteSRGBEnabled, sRGBEnabled);
    EXPECT_EQ(reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetConfig().m_renderContextConfig.m_sRGBWrite, sRGBEnabled);

    // srgb
    int textureId = -9999;
    RenderToTexture::RenderToTextureRequestBus::EventResult(textureId, renderToTextureEntity->GetId(), &RenderToTexture::RenderToTextureRequestBus::Events::GetTextureResourceId);
    EXPECT_EQ(reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComponent)->GetRenderTargetHandle(), RenderToTexture::INVALID_RENDER_TARGET);

    renderToTextureEntity->Deactivate();
    delete renderToTextureEntity;
}

#if RENDER_TO_TEXTURE_EDITOR
#include "../Source/EditorRenderToTextureComponent.h"
TEST_F(RenderToTextureTestApp, RTT_EditorCreateGameEntity)
{
    AZ::Entity* renderToTextureEntity = aznew AZ::Entity("rendertotexture_editor_entity");
    ASSERT_TRUE(renderToTextureEntity != nullptr);

    RenderToTexture::EditorRenderToTextureComponent editor;
    auto* editorBase = static_cast<AzToolsFramework::Components::EditorComponentBase*>(&editor);
    editorBase->BuildGameEntity(renderToTextureEntity);

    // the new game entity's component should look like the default one
    RenderToTexture::RenderToTextureConfig config;

    RenderToTexture::RenderToTextureComponent* renderToTextureComp = renderToTextureEntity->FindComponent<RenderToTexture::RenderToTextureComponent>();
    ASSERT_TRUE(renderToTextureComp != nullptr);
    reinterpret_cast<RenderToTextureComponentTester*>(renderToTextureComp)->ExpectTrue(config);

    delete renderToTextureEntity;
}
#endif
