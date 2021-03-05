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

#include "LmbrCentral_precompiled.h"
#include "Source/Rendering/EditorLensFlareComponent.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <Mocks/ISystemMock.h>
#include <Mocks/IConsoleMock.h>

using ::testing::NiceMock;

namespace UnitTest
{
    // Provides public access to protected/private members to allow for testing.
    class Test_EditorLensFlareComponent :
        public LmbrCentral::EditorLensFlareComponent
    {
    public:
        AZ_COMPONENT(Test_EditorLensFlareComponent, "{2FB6C076-BB92-47A3-93C1-6ED7E622D7E0}", LmbrCentral::EditorLensFlareComponent);

        Test_EditorLensFlareComponent() : EditorLensFlareComponent()
        {

        }

        ~Test_EditorLensFlareComponent() override
        {

        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<Test_EditorLensFlareComponent, EditorLensFlareComponent>()->
                    Version(1);
            }
        }

        LmbrCentral::EditorLensFlareConfiguration Public_GetEditorLensFlareConfiguration() const
        {
            return GetEditorLensFlareConfiguration();
        }
    };

    class MockLensFlareHandlerAndCatalog
        : public AZ::Data::AssetHandler
        , public AZ::Data::AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(MockLensFlareHandlerAndCatalog, AZ::SystemAllocator, 0);

        AZ::Data::AssetPtr CreateAsset([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& /*type*/) override
        {
            return aznew LmbrCentral::LensFlareAsset();
        }

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> /*stream*/,
            const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/) override
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<LmbrCentral::LensFlareAsset>::Uuid());
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& /*type*/) override
        {
            AZ::Data::AssetStreamInfo info;
            // Set valid stream info. This ensures the asset load doesn't result in an
            // error on another thread that can occur during shutdown.
            info.m_streamName = AZStd::string::format("MockLensFlareHandlerAndCatalog%s", assetId.ToString<AZStd::string>().c_str());
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            return info;
        }
    };

    class EditorLensFlareComponentTests
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
        , public AZ::Data::AssetBus::MultiHandler
    {
    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::MultiHandler

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            EXPECT_EQ(asset.GetId(), m_waitForAssetIdLoad);
            m_waitForAssetIdLoad.SetInvalid();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void OnAssetCanceled(AZ::Data::AssetId assetId) override
        {
            // With the right timing, it's possible for the asset to get canceled if the test ends
            // before the load actually starts.  Make sure that if this case happens, we still clear out
            // the pending asset Id and disconnect from the bus.
            EXPECT_EQ(assetId, m_waitForAssetIdLoad);
            m_waitForAssetIdLoad.SetInvalid();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void OnAssetError([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            // No asset errors should happen during this test.
            EXPECT_TRUE(false);
        }

        //////////////////////////////////////////////////////////////////////////
        void SetUp() override
        {
            m_app.Start(m_descriptor);

            // Mock up the File I/O system
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // for FileIO, you must set the instance to null before changing it.
            // this is a way to tell the singleton system that you mean to replace a singleton and its
            // not a mistake.
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_fileIOMock);
            AZ::IO::MockFileIOBase::InstallDefaultReturns(m_fileIOMock);

            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_EditorLensFlareComponentDescriptor = LmbrCentral::EditorLensFlareComponent::CreateDescriptor();
            m_testEditorLensFlareComponentDescriptor = Test_EditorLensFlareComponent::CreateDescriptor();
            m_lensFlareComponentDescriptor = LmbrCentral::LensFlareComponent::CreateDescriptor();

            m_testEditorLensFlareComponentDescriptor->Reflect(m_app.GetSerializeContext());
            m_lensFlareComponentDescriptor->Reflect(m_app.GetSerializeContext());
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            AZ::Data::AssetManager::Instance().RegisterHandler(&m_lensFlareHandlerAndCatalog, AZ::AzTypeInfo<LmbrCentral::LensFlareAsset>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(&m_lensFlareHandlerAndCatalog, AZ::AzTypeInfo<LmbrCentral::LensFlareAsset>::Uuid());

            EXPECT_CALL(m_system, GetConfigSpec(::testing::_))
                .WillRepeatedly(::testing::Return(CONFIG_AUTO_SPEC));

            // Setup Mocks on a stub environment
            m_stubEnv.pConsole = &m_console;
            m_stubEnv.pSystem = &m_system;
            m_priorEnv = gEnv;
            gEnv = &m_stubEnv;
        }
        void TearDown() override
        {
            gEnv = m_priorEnv;

            if (m_waitForAssetIdLoad.IsValid())
            {
                const int assetLoadSleepMS = 20;
                int totalWaitTimeMS = 5000;
                while (m_waitForAssetIdLoad.IsValid() && totalWaitTimeMS > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(assetLoadSleepMS));
                    AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
                    if (m_waitForAssetIdLoad.IsValid())
                    {
                        totalWaitTimeMS -= assetLoadSleepMS;
                    }
                }
                EXPECT_GT(totalWaitTimeMS, 0);
                EXPECT_FALSE(m_waitForAssetIdLoad.IsValid());
                AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            }

            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_lensFlareHandlerAndCatalog);
            AZ::Data::AssetManager::Instance().UnregisterCatalog(&m_lensFlareHandlerAndCatalog);

            m_EditorLensFlareComponentDescriptor->ReleaseDescriptor();
            m_testEditorLensFlareComponentDescriptor->ReleaseDescriptor();
            m_lensFlareComponentDescriptor->ReleaseDescriptor();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            m_app.Stop();

            // Disconnect last, ToolsApplication::Stop() can result in messages that we are using 
            // TraceMessageBus to handle.
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void SetupLensFlareEntity(AZ::Entity& entity)
        {
            entity.CreateComponent<AzFramework::TransformComponent>();
            entity.CreateComponent<Test_EditorLensFlareComponent>();

            entity.Init();
            entity.Activate();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;

        MockLensFlareHandlerAndCatalog m_lensFlareHandlerAndCatalog;

        AZ::ComponentDescriptor* m_EditorLensFlareComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_testEditorLensFlareComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_lensFlareComponentDescriptor = nullptr;

        // If this is set to a valid asset ID, teardown won't finish until
        // this asset has been loaded. If this takes too long, teardown will error out.
        AZ::Data::AssetId m_waitForAssetIdLoad;

        SSystemGlobalEnvironment m_stubEnv;
        NiceMock<SystemMock> m_system;
        NiceMock<ConsoleMock> m_console;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        ::testing::NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;
    };

    TEST_F(EditorLensFlareComponentTests, EditorLensFlareComponent_AddEditorLensFlareComponent_ComponentExists)
    {
        AZ::Entity entity;
        SetupLensFlareEntity(entity);

        Test_EditorLensFlareComponent* lensFlareComponent =
            entity.FindComponent<Test_EditorLensFlareComponent>();
        EXPECT_TRUE(lensFlareComponent != nullptr);
    }

    TEST_F(EditorLensFlareComponentTests, EditorLensFlareComponent_SetAssetId_GetAssetIdMatchesSet)
    {
        AZ::Entity entity;
        SetupLensFlareEntity(entity);

        // Use an arbitrary, non-default asset ID to verify the set & get work.
        AZ::Data::AssetId assetIdToSet(AZ::Uuid("{377939BD-57BB-4476-B7B5-35A162B1335E}"), 5);
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetType = azrtti_typeid<LmbrCentral::LensFlareAsset>();
        assetInfo.m_assetId = assetIdToSet;
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, assetIdToSet, assetInfo);

        // m_waitForAssetIdLoad is cleared when the asset load finishes, so keep a local copy of the asset ID.
        m_waitForAssetIdLoad = assetIdToSet;
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_waitForAssetIdLoad);

        Test_EditorLensFlareComponent* lensFlareComponent =
            entity.FindComponent<Test_EditorLensFlareComponent>();
        lensFlareComponent->SetPrimaryAsset(assetIdToSet);

        LmbrCentral::EditorLensFlareConfiguration lensFlareConfiguration =
            lensFlareComponent->Public_GetEditorLensFlareConfiguration();
        EXPECT_EQ(lensFlareConfiguration.m_asset.GetId(), assetIdToSet);
    }

    TEST_F(EditorLensFlareComponentTests, EditorLensFlareComponent_SetAssetIdBuildGameEntity_GameEntityHasAssetId)
    {
        AZ::Entity entity;
        SetupLensFlareEntity(entity);

        // Use an arbitrary, non-default asset ID to verify the set & get work.
        AZ::Data::AssetId assetIdToSet(AZ::Uuid("{BC7C7A76-C5F2-44E2-8E32-3CE03C3C5ADE}"), 6);
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetType = azrtti_typeid<LmbrCentral::LensFlareAsset>();
        assetInfo.m_assetId = assetIdToSet;
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, assetIdToSet, assetInfo);
        // m_waitForAssetIdLoad is cleared when the asset load finishes, so keep a local copy of the asset ID.
        m_waitForAssetIdLoad = assetIdToSet;
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_waitForAssetIdLoad);

        Test_EditorLensFlareComponent* lensFlareComponent =
            entity.FindComponent<Test_EditorLensFlareComponent>();
        lensFlareComponent->SetPrimaryAsset(assetIdToSet);

        AZ::Entity gameEntity;
        lensFlareComponent->BuildGameEntity(&gameEntity);
        LmbrCentral::LensFlareComponent* gameComponent =
            gameEntity.FindComponent<LmbrCentral::LensFlareComponent>();
        EXPECT_TRUE(gameComponent != nullptr);
        EXPECT_EQ(gameComponent->GetLensFlareConfiguration().m_asset.GetId(), assetIdToSet);

    }
} // namespace UnitTest
