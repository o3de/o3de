/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Editor/EditorBlastChunksAssetHandler.h>
#include <Editor/EditorBlastMeshDataComponent.h>
#include <Asset/BlastChunksAsset.h>

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <gmock/gmock.h>

#include <AzCore/UnitTest/MockComponentApplication.h>

namespace UnitTest
{
    MockComponentApplication::MockComponentApplication()
    {
        AZ::ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
    }

    MockComponentApplication::~MockComponentApplication()
    {
        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
    }

    class MockAssetCatalogRequestBusHandler final
        : public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        MockAssetCatalogRequestBusHandler()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        virtual ~MockAssetCatalogRequestBusHandler()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD3(GetAssetIdByPath, AZ::Data::AssetId(const char*, const AZ::Data::AssetType&, bool));
        MOCK_METHOD1(GetAssetInfoById, AZ::Data::AssetInfo(const AZ::Data::AssetId&));
        MOCK_METHOD1(AddAssetType, void(const AZ::Data::AssetType&));
        MOCK_METHOD1(AddDeltaCatalog, bool(AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(AddExtension, void(const char*));
        MOCK_METHOD0(ClearCatalog, void());
        MOCK_METHOD5(CreateBundleManifest, bool(const AZStd::string&, const AZStd::vector<AZStd::string>&, const AZStd::string&, int, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD2(CreateDeltaCatalog, bool(const AZStd::vector<AZStd::string>&, const AZStd::string&));
        MOCK_METHOD0(DisableCatalog, void());
        MOCK_METHOD1(EnableCatalogForAsset, void(const AZ::Data::AssetType&));
        MOCK_METHOD3(EnumerateAssets, void(BeginAssetEnumerationCB, AssetEnumerationCB, EndAssetEnumerationCB));
        MOCK_METHOD1(GenerateAssetIdTEMP, AZ::Data::AssetId(const char*));
        MOCK_METHOD1(GetAllProductDependencies, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string>(const AZ::Data::AssetId&));
        MOCK_METHOD3(GetAllProductDependenciesFilter, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string>(const AZ::Data::AssetId&, const AZStd::unordered_set<AZ::Data::AssetId>&, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD1(GetAssetPathById, AZStd::string(const AZ::Data::AssetId&));
        MOCK_METHOD1(GetDirectProductDependencies, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string>(const AZ::Data::AssetId&));
        MOCK_METHOD1(GetHandledAssetTypes, void(AZStd::vector<AZ::Data::AssetType>&));
        MOCK_METHOD0(GetRegisteredAssetPaths, AZStd::vector<AZStd::string>());
        MOCK_METHOD2(InsertDeltaCatalog, bool(AZStd::shared_ptr<AzFramework::AssetRegistry>, size_t));
        MOCK_METHOD2(InsertDeltaCatalogBefore, bool(AZStd::shared_ptr<AzFramework::AssetRegistry>, AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(LoadCatalog, bool(const char*));
        MOCK_METHOD2(RegisterAsset, void(const AZ::Data::AssetId&, AZ::Data::AssetInfo&));
        MOCK_METHOD1(RemoveDeltaCatalog, bool(AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(SaveCatalog, bool(const char*));
        MOCK_METHOD0(StartMonitoringAssets, void());
        MOCK_METHOD0(StopMonitoringAssets, void());
        MOCK_METHOD1(UnregisterAsset, void(const AZ::Data::AssetId&));
    };

    class MockAssetManager
        : public AZ::Data::AssetManager
    {
    public:
        MockAssetManager(const AZ::Data::AssetManager::Descriptor& desc) :
            AssetManager(desc)
        {
        }
    };

    class EditorBlastChunkAssetHandlerTestFixture
        : public AllocatorsTestFixture
    {
    public:
        AZStd::unique_ptr<UnitTest::MockComponentApplication> m_mockComponentApplicationBusHandler;
        AZStd::unique_ptr<MockAssetCatalogRequestBusHandler> m_mockAssetCatalogRequestBusHandler;
        AZStd::unique_ptr<MockAssetManager> m_mockAssetManager;

        void SetUp() override final
        {
            AllocatorsTestFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_mockComponentApplicationBusHandler = AZStd::make_unique<UnitTest::MockComponentApplication>();
            m_mockAssetCatalogRequestBusHandler = AZStd::make_unique<MockAssetCatalogRequestBusHandler>();
            m_mockAssetManager = AZStd::make_unique<MockAssetManager>(AZ::Data::AssetManager::Descriptor{});

            AZ::Data::AssetManager::SetInstance(m_mockAssetManager.get());
        }

        void TearDown() override final
        {
            m_mockAssetManager.release();
            AZ::Data::AssetManager::Destroy();

            m_mockAssetCatalogRequestBusHandler.reset();
            m_mockComponentApplicationBusHandler.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AllocatorsTestFixture::TearDown();
        }
    };

    TEST_F(EditorBlastChunkAssetHandlerTestFixture, EditorBlastChunkAssetHandler_AssetManager_Registered)
    {
        Blast::EditorBlastChunksAssetHandler handler;
        handler.Register();
        EXPECT_NE(nullptr, AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<Blast::BlastChunksAsset>()));
        handler.Unregister();
    }

    TEST_F(EditorBlastChunkAssetHandlerTestFixture, EditorBlastChunkAssetHandler_AssetTypeInfoBus_Responds)
    {
        auto assetId = azrtti_typeid<Blast::BlastChunksAsset>();

        Blast::EditorBlastChunksAssetHandler handler;
        handler.Register();

        AZ::Data::AssetType assetType = AZ::Uuid::CreateNull();
        AZ::AssetTypeInfoBus::EventResult(assetType, assetId, &AZ::AssetTypeInfoBus::Events::GetAssetType);
        EXPECT_NE(AZ::Uuid::CreateNull(), assetType);

        const char* displayName = nullptr;
        AZ::AssetTypeInfoBus::EventResult(displayName, assetId, &AZ::AssetTypeInfoBus::Events::GetAssetTypeDisplayName);
        EXPECT_STREQ("Blast Chunks Asset", displayName);

        const char* group = nullptr;
        AZ::AssetTypeInfoBus::EventResult(group, assetId, &AZ::AssetTypeInfoBus::Events::GetGroup);
        EXPECT_STREQ("Blast", group);

        const char* icon = nullptr;
        AZ::AssetTypeInfoBus::EventResult(icon, assetId, &AZ::AssetTypeInfoBus::Events::GetBrowserIcon);
        EXPECT_STREQ("Icons/Components/Box.png", icon);

        AZStd::vector<AZStd::string> extensions;
        AZ::AssetTypeInfoBus::Event(assetId, &AZ::AssetTypeInfoBus::Events::GetAssetTypeExtensions, extensions);
        ASSERT_EQ(1, extensions.size());
        ASSERT_EQ("blast_chunks", extensions[0]);

        handler.Unregister();
    }

    TEST_F(EditorBlastChunkAssetHandlerTestFixture, EditorBlastChunkAssetHandler_AssetHandler_Ready)
    {
        auto assetType = azrtti_typeid<Blast::BlastChunksAsset>();
        auto&& assetManager = AZ::Data::AssetManager::Instance();

        Blast::EditorBlastChunksAssetHandler handler;
        handler.Register();
        EXPECT_EQ(&handler, assetManager.GetHandler(assetType));

        // create and release an instance of the BlastChunkAsset asset type
        {
            using ::testing::Return;
            using ::testing::_;

            EXPECT_CALL(*m_mockAssetCatalogRequestBusHandler, GetAssetInfoById(_))
                .Times(2)
                .WillRepeatedly(Return(AZ::Data::AssetInfo{}));

            auto assetPtr = assetManager.CreateAsset<Blast::BlastChunksAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));
            EXPECT_NE(nullptr, assetPtr.Get());
            EXPECT_EQ(azrtti_typeid<Blast::BlastChunksAsset>(), assetPtr.GetType());
        }

        handler.Unregister();
    }

}
