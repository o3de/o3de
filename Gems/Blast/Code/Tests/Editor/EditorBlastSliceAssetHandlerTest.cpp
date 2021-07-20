/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StdAfx.h"

#include <Editor/EditorBlastSliceAssetHandler.h>
#include <Editor/EditorBlastMeshDataComponent.h>
#include <Asset/BlastSliceAsset.h>

#include <AzCore/UnitTest/MockComponentApplication.h>

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockComponentApplicationBusHandler final
        //: public MockComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockComponentApplicationBusHandler()
        {
            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        virtual ~MockComponentApplicationBusHandler()
        {
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(Destroy, void());
        MOCK_METHOD1(RegisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RemoveEntity, bool(AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool(const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string(const AZ::EntityId&));
        MOCK_METHOD1(AddEntity, bool(AZ::Entity*));
        MOCK_METHOD1(FindEntity, AZ::Entity*(const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void(const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_METHOD0(GetAppRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_METHOD0(GetDrillerManager, AZ::Debug::DrillerManager* ());
        MOCK_METHOD0(GetTickDeltaTime, float());
        MOCK_METHOD1(Tick, void(float));
        MOCK_METHOD0(TickSystem, void());
        MOCK_CONST_METHOD0(GetRequiredSystemComponents, AZ::ComponentTypeList());
        MOCK_METHOD1(ResolveModulePath, void(AZ::OSString&));
        MOCK_METHOD0(CreateSerializeContext, void());
        MOCK_METHOD0(DestroySerializeContext, void());
        MOCK_METHOD0(CreateBehaviorContext, void());
        MOCK_METHOD0(DestroyBehaviorContext, void());
        MOCK_METHOD0(RegisterCoreComponents, void());
        MOCK_METHOD1(AddSystemComponents, void(AZ::Entity*));
        MOCK_METHOD0(ReflectSerialize, void());
        MOCK_METHOD1(Reflect, void(AZ::ReflectContext*));
        MOCK_CONST_METHOD0(GetBinFolder, const char* ());
    };

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

    class EditorBlastSliceAssetHandlerTestFixture
        : public AllocatorsTestFixture
    {
    public:
        AZStd::unique_ptr<MockComponentApplicationBusHandler> m_mockComponentApplicationBusHandler;
        //AZStd::unique_ptr<UnitTest::MockComponentApplication> m_mockComponentApplicationBusHandler;
        AZStd::unique_ptr<MockAssetCatalogRequestBusHandler> m_mockAssetCatalogRequestBusHandler;
        AZStd::unique_ptr<MockAssetManager> m_mockAssetManager;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        const AZ::ComponentDescriptor* m_sliceComponentDescriptor = nullptr;

        void SetUpSliceComponents()
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            AZ::Entity::Reflect(m_serializeContext.get());
            Blast::BlastSliceAssetStorageComponent::Reflect(m_serializeContext.get());
            AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());

            m_sliceComponentDescriptor = AZ::SliceComponent::CreateDescriptor();
            m_sliceComponentDescriptor->Reflect(m_serializeContext.get());
        }

        void TearDownSliceComponents()
        {
            delete m_sliceComponentDescriptor;
            m_serializeContext.reset();
        }

        void SetUp() override final
        {
            AllocatorsTestFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_mockComponentApplicationBusHandler = AZStd::make_unique<MockComponentApplicationBusHandler>();
            m_mockAssetCatalogRequestBusHandler = AZStd::make_unique<MockAssetCatalogRequestBusHandler>();
            m_mockAssetManager = AZStd::make_unique<MockAssetManager>(AZ::Data::AssetManager::Descriptor{});

            AZ::Data::AssetManager::SetInstance(m_mockAssetManager.get());
        }

        void TearDown() override final
        {
            AZ::Data::AssetManager::SetInstance(nullptr);

            m_mockAssetManager.reset();
            m_mockAssetCatalogRequestBusHandler.reset();
            m_mockComponentApplicationBusHandler.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AllocatorsTestFixture::TearDown();
        }

        void SaveSliceAssetToStream(AZ::Entity* sliceAssetEntity, AZStd::vector<char>& buffer)
        {
            buffer.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&stream, *m_serializeContext.get(), AZ::ObjectStream::ST_XML);
            objStream->WriteClass(sliceAssetEntity);
            EXPECT_TRUE(objStream->Finalize());
        }
    };

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, EditorBlastSliceAssetHandler_AssetManager_Registered)
    {
        Blast::EditorBlastSliceAssetHandler handler;
        handler.Register();
        EXPECT_NE(nullptr, AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<Blast::BlastSliceAsset>()));
        handler.Unregister();
    }

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, BlastSliceAssetStorageComponent_Behavior_Registered)
    {
        AZ::BehaviorContext behaviorContext;
        Blast::BlastSliceAssetStorageComponent::Reflect(&behaviorContext);

        auto classEntry = behaviorContext.m_classes.find("BlastSliceAssetStorageComponent");
        EXPECT_NE(behaviorContext.m_classes.end(), classEntry);
        AZ::BehaviorClass* behaviorClass = classEntry->second;
        auto methodEntry = behaviorClass->m_methods.find("GenerateAssetInfo");
        EXPECT_NE(behaviorClass->m_methods.end(), methodEntry);
        AZ::BehaviorMethod* behaviorMethod = methodEntry->second;
        EXPECT_EQ(4, behaviorMethod->GetNumArguments());
        EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAssetStorageComponent>());
        EXPECT_EQ(behaviorMethod->GetArgument(1)->m_typeId, azrtti_typeid<AZStd::vector<AZStd::string>>());
        EXPECT_EQ(behaviorMethod->GetArgument(2)->m_typeId, azrtti_typeid<AZStd::string_view>());
        EXPECT_EQ(behaviorMethod->GetArgument(3)->m_typeId, azrtti_typeid<AZStd::string_view>());
    }

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, BlastSliceAsset_Behavior_Registered)
    {
        AZ::BehaviorContext behaviorContext;
        Blast::BlastSliceAsset::Reflect(&behaviorContext);

        auto classEntry = behaviorContext.m_classes.find("BlastSliceAsset");
        EXPECT_NE(behaviorContext.m_classes.end(), classEntry);
        AZ::BehaviorClass* behaviorClass = classEntry->second;

        auto setMeshIdListEntry = behaviorClass->m_methods.find("SetMeshIdList");
        EXPECT_NE(behaviorClass->m_methods.end(), setMeshIdListEntry);
        {
            AZ::BehaviorMethod* behaviorMethod = setMeshIdListEntry->second;
            EXPECT_EQ(2, behaviorMethod->GetNumArguments());
            EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAsset>());
            EXPECT_EQ(behaviorMethod->GetArgument(1)->m_typeId, azrtti_typeid<AZStd::vector<AZ::Data::AssetId>>());
        }

        auto getMeshIdListEntry = behaviorClass->m_methods.find("GetMeshIdList");
        EXPECT_NE(behaviorClass->m_methods.end(), getMeshIdListEntry);
        {
            AZ::BehaviorMethod* behaviorMethod = getMeshIdListEntry->second;
            EXPECT_EQ(1, behaviorMethod->GetNumArguments());
            EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAsset>());
            EXPECT_EQ(behaviorMethod->GetResult()->m_typeId, azrtti_typeid<AZStd::vector<AZ::Data::AssetId>>());
        }

        auto setMaterialIdEntry = behaviorClass->m_methods.find("SetMaterialId");
        EXPECT_NE(behaviorClass->m_methods.end(), setMaterialIdEntry);
        {
            AZ::BehaviorMethod* behaviorMethod = setMaterialIdEntry->second;
            EXPECT_EQ(2, behaviorMethod->GetNumArguments());
            EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAsset>());
            EXPECT_EQ(behaviorMethod->GetArgument(1)->m_typeId, azrtti_typeid<AZ::Data::AssetId>());
        }

        auto getMaterialIdEntry = behaviorClass->m_methods.find("GetMaterialId");
        EXPECT_NE(behaviorClass->m_methods.end(), getMaterialIdEntry);
        {
            AZ::BehaviorMethod* behaviorMethod = getMaterialIdEntry->second;
            EXPECT_EQ(1, behaviorMethod->GetNumArguments());
            EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAsset>());
            EXPECT_EQ(behaviorMethod->GetResult()->m_typeId, azrtti_typeid<AZ::Data::AssetId>());
        }

        auto getAssetTypeIdEntry = behaviorClass->m_methods.find("GetAssetTypeId");
        EXPECT_NE(behaviorClass->m_methods.end(), getAssetTypeIdEntry);
        {
            AZ::BehaviorMethod* behaviorMethod = getAssetTypeIdEntry->second;
            EXPECT_EQ(1, behaviorMethod->GetNumArguments());
            EXPECT_EQ(behaviorMethod->GetArgument(0)->m_typeId, azrtti_typeid<Blast::BlastSliceAsset>());
            EXPECT_EQ(behaviorMethod->GetResult()->m_typeId, azrtti_typeid<AZ::TypeId>());
        }
    }

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, EditorBlastSliceAssetHandler_AssetTypeInfoBus_Responds)
    {
        auto assetId = azrtti_typeid<Blast::BlastSliceAsset>();

        Blast::EditorBlastSliceAssetHandler handler;
        handler.Register();

        AZ::Data::AssetType assetType = AZ::Uuid::CreateNull();
        AZ::AssetTypeInfoBus::EventResult(assetType, assetId, &AZ::AssetTypeInfoBus::Events::GetAssetType);
        EXPECT_NE(AZ::Uuid::CreateNull(), assetType);

        const char* displayName = nullptr;
        AZ::AssetTypeInfoBus::EventResult(displayName, assetId, &AZ::AssetTypeInfoBus::Events::GetAssetTypeDisplayName);
        EXPECT_STREQ("Blast Slice Asset", displayName);

        const char* group = nullptr;
        AZ::AssetTypeInfoBus::EventResult(group, assetId, &AZ::AssetTypeInfoBus::Events::GetGroup);
        EXPECT_STREQ("Blast", group);

        const char* icon = nullptr;
        AZ::AssetTypeInfoBus::EventResult(icon, assetId, &AZ::AssetTypeInfoBus::Events::GetBrowserIcon);
        EXPECT_STREQ("Editor/Icons/Components/Box.png", icon);

        AZStd::vector<AZStd::string> extensions;
        AZ::AssetTypeInfoBus::Event(assetId, &AZ::AssetTypeInfoBus::Events::GetAssetTypeExtensions, extensions);
        ASSERT_EQ(1, extensions.size());
        ASSERT_EQ("blast_slice", extensions[0]);

        handler.Unregister();
    }

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, EditorBlastSliceAssetHandler_AssetHandler_Ready)
    {
        auto assetType = azrtti_typeid<Blast::BlastSliceAsset>();
        auto&& assetManager = AZ::Data::AssetManager::Instance();

        Blast::EditorBlastSliceAssetHandler handler;
        handler.Register();
        EXPECT_EQ(&handler, assetManager.GetHandler(assetType));

        // create and release an instance of the BlastSliceAsset asset type
        {
            using ::testing::Return;
            using ::testing::_;

            EXPECT_CALL(*m_mockAssetCatalogRequestBusHandler, GetAssetInfoById(_))
                .Times(1)
                .WillRepeatedly(Return(AZ::Data::AssetInfo{}));

            auto assetPtr = assetManager.CreateAsset<Blast::BlastSliceAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));
            EXPECT_NE(nullptr, assetPtr.Get());
            EXPECT_EQ(azrtti_typeid<Blast::BlastSliceAsset>(), assetPtr.GetType());
        }

        handler.Unregister();
    }

    TEST_F(EditorBlastSliceAssetHandlerTestFixture, EditorBlastSliceAssetHandler_AssetHandler_LoadsAssetData)
    {
        SetUpSliceComponents();

        AZStd::vector<AZStd::string> meshAssetPathList = { "/foo/path/thing.cgf", "/foo/path/that.cgf" };
        AZ::Entity* storageEntity = aznew AZ::Entity();
        auto* blastStorage = storageEntity->CreateComponent<Blast::BlastSliceAssetStorageComponent>();
        blastStorage->SetMeshPathList(meshAssetPathList);

        AZ::Entity sliceEntity;
        AZ::SliceComponent* slice = sliceEntity.CreateComponent<AZ::SliceComponent>();
        slice->AddEntity(storageEntity);

        AZStd::vector<char> buffer;
        SaveSliceAssetToStream(&sliceEntity, buffer);

        // Load a slice with the BlastSliceAssetStorageComponent
        Blast::EditorBlastSliceAssetHandler handler;
        handler.Register();
        {
            using ::testing::Return;
            using ::testing::_;

            EXPECT_CALL(*m_mockComponentApplicationBusHandler, GetSerializeContext)
                .Times(1)
                .WillOnce(Return(m_serializeContext.get()));

            EXPECT_CALL(*m_mockComponentApplicationBusHandler, FindEntity(_))
                .Times(1)
                .WillOnce(Return(&sliceEntity));

            EXPECT_CALL(*m_mockAssetCatalogRequestBusHandler, GetAssetIdByPath(_,_,_))
                .Times(2)
                .WillRepeatedly(Return(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0)));

            EXPECT_CALL(*m_mockAssetCatalogRequestBusHandler, GetAssetInfoById(_))
                .Times(2)
                .WillRepeatedly(Return(AZ::Data::AssetInfo{}));

            auto&& assetManager = AZ::Data::AssetManager::Instance();
            auto assetPtr = assetManager.CreateAsset<Blast::BlastSliceAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));

            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            const AZ::Data::AssetFilterCB assetLoadFilterCB{};
            bool loaded = handler.LoadAssetData(assetPtr, &stream, assetLoadFilterCB);
            EXPECT_TRUE(loaded);
        }
        handler.Unregister();

        TearDownSliceComponents();
    }
}
