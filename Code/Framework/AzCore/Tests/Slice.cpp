/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileIOBaseTestTypes.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

namespace UnitTest
{
    class MyTestComponent1
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MyTestComponent1, "{5D3B5B45-64DF-4F88-8003-271646B6CA27}");

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<MyTestComponent1, AZ::Component>()->
                    Field("float", &MyTestComponent1::m_float)->
                    Field("int", &MyTestComponent1::m_int);
            }
        }

        float m_float = 0.f;
        int m_int = 0;
    };

    class MyTestComponent2
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MyTestComponent2, "{ACD7B78D-0C30-46E8-A52E-61D4B33D5528}");

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<MyTestComponent2, AZ::Component>()->
                    Field("entityId", &MyTestComponent2::m_entityId);
            }
        }

        AZ::EntityId m_entityId;
    };

    class SliceTest_MockCatalog
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    private:
        AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();
        AZStd::vector<AZ::Data::AssetId> m_mockAssetIds;

    public:
        AZ_CLASS_ALLOCATOR(SliceTest_MockCatalog, AZ::SystemAllocator);

        SliceTest_MockCatalog()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~SliceTest_MockCatalog() override
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        AZ::Data::AssetId GenerateMockAssetId()
        {
            AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            m_mockAssetIds.push_back(assetId);
            return assetId;
        }
        
        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetCatalogRequestBus
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            result.m_assetType = AZ::AzTypeInfo<AZ::SliceAsset>::Uuid();
            for (const auto& assetId : m_mockAssetIds)
            {
                if (assetId == id)
                {
                    result.m_assetId = id;
                    break;
                }
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            for (int i = 0; i < m_mockAssetIds.size(); ++i)
            {
                if (m_mockAssetIds[i] == id)
                {
                    info.m_streamName = AZStd::string::format("MockSliceAssetName%d", i);
                }
            }

            if (!info.m_streamName.empty())
            {
                // this ensures tha parallel running unit tests do not overlap their files that they use.
                AZ::IO::Path fullName = GetTestFolderPath() / AZStd::string::format("%s-%s", randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = AZStd::move(fullName.Native());
                info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            AZ::Data::AssetStreamInfo info;
            info = GetStreamInfoForLoad(id, type);
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            return info;
        }

        bool SaveAsset(AZ::Data::Asset<AZ::SliceAsset>& asset)
        {
            volatile bool isDone = false;
            volatile bool succeeded = false;
            AZ::Data::AssetBusCallbacks callbacks;
            callbacks.SetCallbacks(nullptr, nullptr, nullptr,
                [&isDone, &succeeded](const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, bool isSuccessful, AZ::Data::AssetBusCallbacks& /*callbacks*/)
            {
                isDone = true;
                succeeded = isSuccessful;
            }, nullptr, nullptr, nullptr);

            callbacks.BusConnect(asset.GetId());
            asset.Save();

            while (!isDone)
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }
            return succeeded;
        }
    };

    class SliceTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = aznew AZ::SerializeContext(true, true);
            m_sliceDescriptor = AZ::SliceComponent::CreateDescriptor();

            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
            m_streamer = aznew AZ::IO::Streamer(AZStd::thread_desc{}, AZ::StreamerComponent::CreateStreamerStack());
            AZ::Interface<AZ::IO::IStreamer>::Register(m_streamer);

            m_sliceDescriptor->Reflect(m_serializeContext);
            MyTestComponent1::Reflect(m_serializeContext);
            MyTestComponent2::Reflect(m_serializeContext);
            AZ::SliceMetadataInfoComponent::Reflect(m_serializeContext);
            AZ::Entity::Reflect(m_serializeContext);
            AZ::DataPatch::Reflect(m_serializeContext);

            // Create database
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);
            AZ::Data::AssetManager::Instance().RegisterHandler(aznew AZ::SliceAssetHandler(m_serializeContext), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
            m_catalog.reset(aznew SliceTest_MockCatalog());
            AZ::Data::AssetManager::Instance().RegisterCatalog(m_catalog.get(), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
        }

        void TearDown() override
        {
            m_catalog->DisableCatalog();
            m_catalog.reset();
            AZ::Data::AssetManager::Destroy();
            delete m_sliceDescriptor;
            delete m_serializeContext;

            AZ::Interface<AZ::IO::IStreamer>::Unregister(m_streamer);
            delete m_streamer;
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);

            LeakDetectionFixture::TearDown();
        }

        AZ::IO::FileIOBase* m_prevFileIO{ nullptr };
        AZ::IO::Streamer* m_streamer{ nullptr };
        TestFileIOBase m_fileIO;
        AZStd::unique_ptr<SliceTest_MockCatalog> m_catalog;
        AZ::SerializeContext* m_serializeContext;
        AZ::ComponentDescriptor* m_sliceDescriptor;
    };

    TEST_F(SliceTest, UberTest)
    {
        AZ::SerializeContext& serializeContext = *m_serializeContext;
        AZStd::vector<char> binaryBuffer;

        AZ::Entity* sliceEntity = nullptr;
        AZ::SliceComponent* sliceComponent = nullptr;

        AZ::Entity* entity1 = nullptr;

        MyTestComponent1* component1 = nullptr;


        /////////////////////////////////////////////////////////////////////////////
        // Test prefab without base prefab - aka root prefab
        /////////////////////////////////////////////////////////////////////////////
        sliceEntity = aznew AZ::Entity();
        sliceComponent = aznew AZ::SliceComponent();
        sliceComponent->SetSerializeContext(&serializeContext);
        sliceEntity->AddComponent(sliceComponent);
        sliceEntity->Init();
        sliceEntity->Activate();

        // Create an entity with a component to be part of the prefab
        entity1 = aznew AZ::Entity();
        component1 = aznew MyTestComponent1();
        component1->m_float = 2.0f;
        component1->m_int = 11;
        entity1->AddComponent(component1);

        sliceComponent->AddEntity(entity1);

        // store to stream
        AZ::IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBuffer);
        AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create(&binaryStream, serializeContext, AZ::ObjectStream::ST_BINARY);
        binaryObjStream->WriteClass(sliceEntity);
        AZ_TEST_ASSERT(binaryObjStream->Finalize());

        // load from stream and check values
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ::Entity* sliceEntity2 = AZ::Utils::LoadObjectFromStream<AZ::Entity>(binaryStream, &serializeContext);
        AZ_TEST_ASSERT(sliceEntity2);
        AZ_TEST_ASSERT(sliceEntity2->FindComponent<AZ::SliceComponent>() != nullptr);
        AZ_TEST_ASSERT(sliceEntity2->FindComponent<AZ::SliceComponent>()->GetSlices().empty());

        const AZ::SliceComponent::EntityList& entityContainer = sliceEntity2->FindComponent<AZ::SliceComponent>()->GetNewEntities();
        AZ_TEST_ASSERT(entityContainer.size() == 1);
        AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>());
        AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>()->m_float == 2.0f);
        AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>()->m_int == 11);

        delete sliceEntity2;
        sliceEntity2 = nullptr;
        /////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////
        // Test slice component with a single base prefab

        // Create "fake" asset - so we don't have to deal with the asset database, etc.
        AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(m_catalog->GenerateMockAssetId(), AZ::Data::AssetLoadBehavior::Default);
        AZ::SliceAsset* sliceAsset1 = sliceAssetHolder.Get();
        sliceAsset1->SetData(sliceEntity, sliceComponent);
        // ASSERT_TRUE(m_catalog->SaveAsset(sliceAssetHolder));

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, sliceAssetHolder.GetId());
        ASSERT_TRUE(assetInfo.m_assetId.IsValid());

        sliceEntity2 = aznew AZ::Entity;
        AZ::SliceComponent* sliceComponent2 = sliceEntity2->CreateComponent<AZ::SliceComponent>();
        sliceComponent2->SetSerializeContext(&serializeContext);
        sliceComponent2->AddSlice(sliceAssetHolder);
        sliceEntity2->Init();
        sliceEntity2->Activate();
        AZ::SliceComponent::EntityList entities;
        sliceComponent2->GetEntities(entities); // get all entities (Instantiated if needed)
        AZ_TEST_ASSERT(entities.size() == 1);
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 2.0f);
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);

        // Modify Component
        entities.front()->FindComponent<MyTestComponent1>()->m_float = 5.0f;

        // Add a new entity
        AZ::Entity* entity2 = aznew AZ::Entity();
        entity2->CreateComponent<MyTestComponent2>();
        sliceComponent2->AddEntity(entity2);

        // FindSlice test
        auto prefabFindResult = sliceComponent2->FindSlice(entity2);
        AZ_TEST_ASSERT(prefabFindResult.GetReference() == nullptr); // prefab reference
        AZ_TEST_ASSERT(prefabFindResult.GetInstance() == nullptr); // prefab instance
        prefabFindResult = sliceComponent2->FindSlice(entities.front());
        AZ_TEST_ASSERT(prefabFindResult.GetReference() != nullptr); // prefab reference
        AZ_TEST_ASSERT(prefabFindResult.GetInstance() != nullptr); // prefab instance


        // store to stream
        binaryBuffer.clear();
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        binaryObjStream = AZ::ObjectStream::Create(&binaryStream, serializeContext, AZ::ObjectStream::ST_XML);
        binaryObjStream->WriteClass(sliceEntity2);
        AZ_TEST_ASSERT(binaryObjStream->Finalize());

        // load from stream and validate
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ::Entity* sliceEntity2Clone = AZ::Utils::LoadObjectFromStream<AZ::Entity>(binaryStream, &serializeContext);
        AZ_TEST_ASSERT(sliceEntity2Clone);
        AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<AZ::SliceComponent>() != nullptr);
        AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<AZ::SliceComponent>()->GetSlices().size() == 1);
        AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<AZ::SliceComponent>()->GetNewEntities().size() == 1);
        // instantiate the loaded component
        sliceEntity2Clone->FindComponent<AZ::SliceComponent>()->SetSerializeContext(&serializeContext);
        entities.clear();
        sliceEntity2Clone->FindComponent<AZ::SliceComponent>()->GetEntities(entities);
        AZ_TEST_ASSERT(entities.size() == 2); // original entity (prefab) plus the newly added one
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 5.0f); // test modified field
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);
        AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent2>());

        ////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////
        // Test slice component 3 levels of customization
        AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder1 = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(m_catalog->GenerateMockAssetId(), AZ::Data::AssetLoadBehavior::Default);
        AZ::SliceAsset* sliceAsset2 = sliceAssetHolder1.Get();
        sliceAsset2->SetData(sliceEntity2, sliceComponent2);

        AZ::Entity* sliceEntity3 = aznew AZ::Entity();
        AZ::SliceComponent* sliceComponent3 = sliceEntity3->CreateComponent<AZ::SliceComponent>();
        sliceComponent3->SetSerializeContext(&serializeContext);
        sliceEntity3->Init();
        sliceEntity3->Activate();
        sliceComponent3->AddSlice(sliceAssetHolder1);
        entities.clear();
        sliceComponent3->GetEntities(entities);
        AZ_TEST_ASSERT(entities.size() == 2);
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 5.0f);
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);
        AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent2>());

        // Modify Component
        entities.front()->FindComponent<MyTestComponent1>()->m_int = 22;

        // Remove a entity
        sliceComponent3->RemoveEntity(entities.back());

        // store to stream
        binaryBuffer.clear();
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        binaryObjStream = AZ::ObjectStream::Create(&binaryStream, serializeContext, AZ::ObjectStream::ST_XML);
        binaryObjStream->WriteClass(sliceEntity3);
        AZ_TEST_ASSERT(binaryObjStream->Finalize());

        // modify root asset (to make sure that changes propgate properly), we have not overridden MyTestComponent1::m_float
        entities.clear();
        sliceComponent2->GetEntities(entities);
        entities.front()->FindComponent<MyTestComponent1>()->m_float = 15.0f;

        // load from stream and validate
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ::Entity* sliceEntity3Clone = AZ::Utils::LoadObjectFromStream<AZ::Entity>(binaryStream, &serializeContext);
        // instantiate the loaded component
        sliceEntity3Clone->FindComponent<AZ::SliceComponent>()->SetSerializeContext(&serializeContext);
        sliceEntity3Clone->Init();
        sliceEntity3Clone->Activate();
        entities.clear();
        sliceEntity3Clone->FindComponent<AZ::SliceComponent>()->GetEntities(entities);
        AZ_TEST_ASSERT(entities.size() == 1);
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 15.0f); // this value was modified in the base prefab
        AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 22);

        /////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////
        // Testing patching instance-owned entities in an existing prefab.
        {
            AZ::Entity rootEntity;
            AZ::SliceComponent* rootSlice = rootEntity.CreateComponent<AZ::SliceComponent>();
            rootSlice->SetSerializeContext(&serializeContext);
            rootEntity.Init();
            rootEntity.Activate();

            AZ::Data::Asset<AZ::SliceAsset> testAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(m_catalog->GenerateMockAssetId(), AZ::Data::AssetLoadBehavior::Default);
            AZ::Entity* testAssetEntity = aznew AZ::Entity;
            AZ::SliceComponent* testAssetSlice = testAssetEntity->CreateComponent<AZ::SliceComponent>();
            testAssetSlice->SetSerializeContext(&serializeContext);
                
            // Add a couple test entities to the prefab asset.
            AZ::Entity* testEntity1 = aznew AZ::Entity();
            testEntity1->CreateComponent<MyTestComponent1>()->m_float = 15.f;
            testAssetSlice->AddEntity(testEntity1);
            AZ::Entity* testEntity2 = aznew AZ::Entity();
            testEntity2->CreateComponent<MyTestComponent1>()->m_float = 5.f;
            testAssetSlice->AddEntity(testEntity2);

            testAsset.Get()->SetData(testAssetEntity, testAssetSlice);

            // Test removing an entity from an existing prefab instance, cloning it, and re-adding it.
            // This emulates what's required for fast undo/redo entity restore for slice-owned entities.
            {
                rootSlice->AddSlice(testAsset);
                entities.clear();
                rootSlice->GetEntities(entities);

                // Grab one of the entities to remove, clone, and re-add.
                AZ::Entity* entity = entities.back();
                AZ_TEST_ASSERT(entity);
                AZ::SliceComponent::SliceInstanceAddress addr = rootSlice->FindSlice(entity);
                AZ_TEST_ASSERT(addr.IsValid());
                const AZ::SliceComponent::SliceInstanceId instanceId = addr.GetInstance()->GetId();
                AZ::SliceComponent::EntityAncestorList ancestors;
                addr.GetReference()->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
                AZ_TEST_ASSERT(ancestors.size() == 1);
                AZ::SliceComponent::EntityRestoreInfo restoreInfo;
                AZ_TEST_ASSERT(rootSlice->GetEntityRestoreInfo(entity->GetId(), restoreInfo));

                // Duplicate the entity and make a data change we can validate later.
                AZ::Entity* clone = serializeContext.CloneObject(entity);
                clone->FindComponent<MyTestComponent1>()->m_float = 10.f;
                // Remove the original. We have two entities in the instance, so this will not wipe the instance.
                rootSlice->RemoveEntity(entity);
                // Patch it back into the prefab.
                rootSlice->RestoreEntity(clone, restoreInfo);
                // Re-retrieve the entity. We should find it, and it should match the data of the clone.
                addr = rootSlice->FindSlice(clone);
                ASSERT_TRUE(addr.IsValid());
                EXPECT_EQ(instanceId, addr.GetInstance()->GetId());
                entities.clear();
                rootSlice->GetEntities(entities);
                ASSERT_EQ(clone, entities.back());
                EXPECT_EQ(10.0f, entities.back()->FindComponent<MyTestComponent1>()->m_float);
                ancestors.clear();
                addr.GetReference()->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
                EXPECT_EQ(1, ancestors.size());
            }

            // We also support re-adding when the reference no longer exists, so run the above
            // test, but such that the entity we remove is the last one, in turn destroying
            // the whole instance & reference.
            {
                rootSlice->RemoveSlice(testAsset);
                rootSlice->AddSlice(testAsset);
                entities.clear();
                rootSlice->GetEntities(entities);

                // Remove one of the entities.
                rootSlice->RemoveEntity(entities.front());

                AZ::Entity* entity = entities.back();
                AZ_TEST_ASSERT(entity);
                AZ::SliceComponent::SliceInstanceAddress addr = rootSlice->FindSlice(entity);
                AZ_TEST_ASSERT(addr.IsValid());
                const AZ::SliceComponent::SliceInstanceId instanceId = addr.GetInstance()->GetId();
                AZ::SliceComponent::EntityAncestorList ancestors;
                addr.GetReference()->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
                AZ_TEST_ASSERT(ancestors.size() == 1);
                AZ::SliceComponent::EntityRestoreInfo restoreInfo;
                AZ_TEST_ASSERT(rootSlice->GetEntityRestoreInfo(entity->GetId(), restoreInfo));

                // Duplicate the entity and make a data change we can validate later.
                AZ::Entity* clone = serializeContext.CloneObject(entity);
                clone->FindComponent<MyTestComponent1>()->m_float = 10.f;
                // Remove the original. We have two entities in the instance, so this will not wipe the instance.
                rootSlice->RemoveEntity(entity);
                // Patch it back into the prefab.
                rootSlice->RestoreEntity(clone, restoreInfo);
                // Re-retrieve the entity. We should find it, and it should match the data of the clone.
                addr = rootSlice->FindSlice(clone);
                AZ_TEST_ASSERT(addr.IsValid());
                AZ_TEST_ASSERT(addr.GetInstance()->GetId() == instanceId);
                entities.clear();
                rootSlice->GetEntities(entities);
                AZ_TEST_ASSERT(entities.back() == clone);
                AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent1>()->m_float == 10.0f);
                ancestors.clear();
                addr.GetReference()->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
                AZ_TEST_ASSERT(ancestors.size() == 1);
            }
        }
        /////////////////////////////////////////////////////////////////////////////

        // just reset the asset, don't delete the owned entity (we delete it later)
        sliceAsset1->SetData(nullptr, nullptr, false);
        sliceAsset2->SetData(nullptr, nullptr, false);

        delete sliceEntity;
        delete sliceEntity2;
        delete sliceEntity2Clone;
        delete sliceEntity3;
        delete sliceEntity3Clone;

        sliceAssetHolder.Reset(); // release asset
        sliceAssetHolder1.Reset();

        AZ::Data::AssetManager::Destroy();
    }

    /// a mock asset catalog which only contains two fixed assets.
    class SliceTest_RecursionDetection_Catalog
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceTest_RecursionDetection_Catalog, AZ::SystemAllocator);

        AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();

        AZ::Data::AssetId m_assetIdSlice1 = AZ::Data::AssetId("{9DE6E611-CE12-4D78-90A5-53D106A50042}", 0);
        AZ::Data::AssetId m_assetIdSlice2 = AZ::Data::AssetId("{884C3DEE-3C86-4941-85E9-89103BAE314B}", 0);

        SliceTest_RecursionDetection_Catalog()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~SliceTest_RecursionDetection_Catalog() override
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetCatalogRequestBus
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            result.m_assetType = azrtti_typeid<AZ::SliceAsset>();
            if (id == m_assetIdSlice1)
            {
                result.m_assetId = id;
            }
            else if (id == m_assetIdSlice2)
            {
                result.m_assetId = id;
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            if (m_assetIdSlice1 == id)
            {
                info.m_streamName = "MySliceAsset1.txt";
            }
            else if (m_assetIdSlice2 == id)
            {
                info.m_streamName = "MySliceAsset2.txt";
            }

            if (!info.m_streamName.empty())
            {
                // this ensures the parallel running unit tests do not overlap their files that they use.
                AZ::IO::Path fullName = GetTestFolderPath() / AZStd::string::format("%s-%s", randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = AZStd::move(fullName.Native());
                info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            AZ::Data::AssetStreamInfo info;
            info = GetStreamInfoForLoad(id, type);
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            return info;
        }
    };

    class DataFlags_CleanupTest
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            auto allEntitiesValidFunction = [](AZ::EntityId) { return true; };
            m_dataFlags = AZStd::make_unique<AZ::SliceComponent::DataFlagsPerEntity>(allEntitiesValidFunction);
            m_addressOfSetFlag.push_back(AZ_CRC_CE("Components"));
            m_valueOfSetFlag = AZ::DataPatch::Flag::ForceOverrideSet;
            m_entityIdToGoMissing = AZ::Entity::MakeId();
            m_entityIdToRemain = AZ::Entity::MakeId();
            m_remainingEntity = AZStd::make_unique<AZ::Entity>(m_entityIdToRemain);
            m_remainingEntityList.push_back(m_remainingEntity.get());

            m_dataFlags->SetEntityDataFlagsAtAddress(m_entityIdToGoMissing, m_addressOfSetFlag, m_valueOfSetFlag);
            m_dataFlags->SetEntityDataFlagsAtAddress(m_entityIdToRemain, m_addressOfSetFlag, m_valueOfSetFlag);

            m_dataFlags->Cleanup(m_remainingEntityList);
        }

        void TearDown() override
        {
            m_remainingEntity.reset();
            m_dataFlags.reset();
        }

        AZStd::unique_ptr<AZ::SliceComponent::DataFlagsPerEntity> m_dataFlags;
        AZ::DataPatch::AddressType m_addressOfSetFlag;
        AZ::DataPatch::Flags m_valueOfSetFlag;
        AZ::EntityId m_entityIdToGoMissing;
        AZ::EntityId m_entityIdToRemain;
        AZStd::unique_ptr<AZ::Entity> m_remainingEntity;
        AZ::SliceComponent::EntityList m_remainingEntityList;
    };

    TEST_F(DataFlags_CleanupTest, MissingEntitiesPruned)
    {
        EXPECT_TRUE(m_dataFlags->GetEntityDataFlags(m_entityIdToGoMissing).empty());
    }

    TEST_F(DataFlags_CleanupTest, ValidEntitiesRemain)
    {
        EXPECT_EQ(m_dataFlags->GetEntityDataFlagsAtAddress(m_entityIdToRemain, m_addressOfSetFlag), m_valueOfSetFlag);
    }

    TEST_F(SliceTest, SliceMetadataInfoComponentV1ToV2Converter)
    {
        AZStd::string_view sliceAssociatedEntityIds = R"(<ObjectStream version="3">
                <Class name="SliceMetadataInfoComponent" field="element" version="1" type="{25EE4D75-8A17-4449-81F4-E561005BAABD}">
                    <Class name="AZStd::unordered_set" field="AssociatedIds" type="{6C8F8E52-AB4A-5C1F-8E56-9AC390290B94}">
                        <Class name="EntityId" field="element" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="421626392978" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                    </Class>
                </Class>
            </ObjectStream>)";

        AZ::Entity* entity = aznew AZ::Entity("Slice");
        AZ::SliceMetadataInfoComponent* sliceMetadataInfoComponent = entity->CreateComponent<AZ::SliceMetadataInfoComponent>();
        AZ::IO::MemoryStream xmlStream(sliceAssociatedEntityIds.data(), sliceAssociatedEntityIds.size());
        AZ::Utils::LoadObjectFromStreamInPlace(xmlStream, *sliceMetadataInfoComponent, m_serializeContext);
        sliceMetadataInfoComponent->Activate();
        AZStd::set<AZ::EntityId> associatedEntities;
        AZ::SliceMetadataInfoRequestBus::Event(sliceMetadataInfoComponent->GetEntityId(), &AZ::SliceMetadataInfoRequestBus::Events::GetAssociatedEntities, associatedEntities);
        ASSERT_TRUE(associatedEntities.size() == 1);
        delete sliceMetadataInfoComponent;
        delete entity;
    }

    TEST_F(SliceTest, PreventOverrideOfPropertyinEntityFromSlice_InstancedSlicesCantOverrideProperty)
    {
        ////////////////////////////////////////////////////////////////////////
        // Create a root slice and make it an asset
        AZ::Entity* rootSliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* rootSliceComponent = rootSliceEntity->CreateComponent<AZ::SliceComponent>();
        AZ::EntityId entityId1InRootSlice;
        AZ::ComponentId componentId1InRootSlice;
        AZ::Data::Asset<AZ::SliceAsset> rootSliceAssetRef;
        {
            rootSliceComponent->SetSerializeContext(m_serializeContext);
            rootSliceEntity->Init();
            rootSliceEntity->Activate();

            // put 1 entity in the root slice
            AZ::Entity* entity1InRootSlice = aznew AZ::Entity();
            entityId1InRootSlice = entity1InRootSlice->GetId();
            MyTestComponent1* component1InRootSlice = entity1InRootSlice->CreateComponent<MyTestComponent1>();
            componentId1InRootSlice = component1InRootSlice->GetId();

            rootSliceComponent->AddEntity(entity1InRootSlice);

            // create a "fake" slice asset
            rootSliceAssetRef = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(m_catalog->GenerateMockAssetId(), AZ::Data::AssetLoadBehavior::Default);
            rootSliceAssetRef.Get()->SetData(rootSliceEntity, rootSliceComponent);
        }

        ////////////////////////////////////////////////////////////////////////
        // Create slice2, which contains an instance of the first slice
        // store slice2 in stream
        AZStd::vector<AZ::u8> slice2Buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> slice2Stream(&slice2Buffer);
        {
            AZ::Entity* slice2Entity = aznew AZ::Entity();
            AZ::SliceComponent* slice2Component = slice2Entity->CreateComponent<AZ::SliceComponent>();
            slice2Component->SetSerializeContext(m_serializeContext);
            slice2Component->AddSlice(rootSliceAssetRef);

            AZ::SliceComponent::EntityList entitiesInSlice2;
            slice2Component->GetEntities(entitiesInSlice2); // this instantiates slice2 and the rootSlice

            // change a property
            entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int = 43;

            auto slice2ObjectStream = AZ::ObjectStream::Create(&slice2Stream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
            EXPECT_TRUE(slice2ObjectStream->WriteClass(slice2Entity));
            EXPECT_TRUE(slice2ObjectStream->Finalize());

            delete slice2Entity;
        }

        ////////////////////////////////////////////////////////////////////////
        // Re-spawn slice2, the property should still be overridden
        {
            slice2Stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            auto slice2Entity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(slice2Stream, m_serializeContext);
            AZ::SliceComponent::EntityList entitiesInSlice2;
            slice2Entity->FindComponent<AZ::SliceComponent>()->GetEntities(entitiesInSlice2);

            EXPECT_EQ(43, entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int);
            delete slice2Entity;
        }

        ////////////////////////////////////////////////////////////////////////
        // Now modify the root slice to prevent override of the component's m_int value.
        {
            AZ::DataPatch::AddressType address;
            address.push_back(AZ_CRC_CE("Components"));
            address.push_back(componentId1InRootSlice);
            address.push_back(AZ_CRC_CE("int"));

            rootSliceComponent->SetEntityDataFlagsAtAddress(entityId1InRootSlice, address, AZ::DataPatch::Flag::PreventOverrideSet);

            // There are expectations that slice assets don't change after they're loaded,
            // so fake an "asset reload" by writing out the slice and reading it back in.
            AZStd::vector<char> rootSliceBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> rootSliceStream(&rootSliceBuffer);
            auto rootSliceObjectStream = AZ::ObjectStream::Create(&rootSliceStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            EXPECT_TRUE(rootSliceObjectStream->WriteClass(rootSliceEntity));
            EXPECT_TRUE(rootSliceObjectStream->Finalize());

            rootSliceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            rootSliceEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(rootSliceStream, m_serializeContext);
            rootSliceComponent = rootSliceEntity->FindComponent<AZ::SliceComponent>();
            rootSliceComponent->SetSerializeContext(m_serializeContext);
            rootSliceEntity->Init();
            rootSliceEntity->Activate();

            rootSliceAssetRef.Get()->SetData(rootSliceEntity, rootSliceComponent, true);
        }

        ////////////////////////////////////////////////////////////////////////
        // Re-spawn slice2, the property should NOT be overridden
        {
            slice2Stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            auto slice2Entity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(slice2Stream, m_serializeContext);
            AZ::SliceComponent::EntityList entitiesInSlice2;
            slice2Entity->FindComponent<AZ::SliceComponent>()->GetEntities(entitiesInSlice2);

            EXPECT_NE(43, entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int);
            delete slice2Entity;
        }
    }
}

#ifdef HAVE_BENCHMARK
namespace Benchmark
{
    static void BM_Slice_GenerateNewIdsAndFixRefs(benchmark::State& state)
    {
        AZ::ComponentApplication componentApp;

        AZ::ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        componentApp.Create(desc, startupParameters);

        UnitTest::MyTestComponent1::Reflect(componentApp.GetSerializeContext());
        UnitTest::MyTestComponent2::Reflect(componentApp.GetSerializeContext());

        // we use some randomness to set up this scenario,
        // seed the generator so we get the same results each time.
        AZ::u32 randSeed[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        AZ::Sfmt::GetInstance().Seed(randSeed, AZ_ARRAY_SIZE(randSeed));

        // setup a container with N entities
        AZ::SliceComponent::InstantiatedContainer container;
        for (int64_t entityI = 0; entityI < state.range(0); ++entityI)
        {
            auto entity = aznew AZ::Entity();

            // add components with nothing to remap, just to bulk up the volume of processed data
            entity->CreateComponent<UnitTest::MyTestComponent1>();
            entity->CreateComponent<UnitTest::MyTestComponent1>();
            entity->CreateComponent<UnitTest::MyTestComponent1>();

            // add component which references another EntityId
            auto component2 = entity->CreateComponent<UnitTest::MyTestComponent2>();
            if (entityI != 0)
            {
                component2->m_entityId = container.m_entities[AZ::Sfmt::GetInstance().Rand64() % container.m_entities.size()]->GetId();
            }

            container.m_entities.push_back(entity);
        }

        // shuffle entities so that some ID references are earlier in the data and some are later
        for (size_t i = container.m_entities.size() - 1; i > 0; --i)
        {
            AZStd::swap(container.m_entities[i], container.m_entities[AZ::Sfmt::GetInstance().Rand64() % (i+1)]);
        }

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remappedIds;
        while (state.KeepRunning())
        {
            // Setup
            state.PauseTiming();
            AZ::SliceComponent::InstantiatedContainer* clonedContainer = componentApp.GetSerializeContext()->CloneObject(&container);
            state.ResumeTiming();

            // Timed Test
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(clonedContainer, remappedIds, componentApp.GetSerializeContext());

            state.PauseTiming();
            delete clonedContainer;
            remappedIds.clear();
            state.ResumeTiming();
        }
    }

    BENCHMARK(BM_Slice_GenerateNewIdsAndFixRefs)->Arg(10)->Arg(1000);

} // namespace Benchmark
#endif // HAVE_BENCHMARK
