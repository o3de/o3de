/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzCore/Component/Entity.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>
#include <Tests/FileIOBaseTestTypes.h>

#include <Vegetation/DynamicSliceInstanceSpawner.h>
#include <Vegetation/EmptyInstanceSpawner.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>

namespace UnitTest
{
    // Mock VegetationSystemComponent is needed to reflect only the DynamicSliceInstanceSpawner.
    class MockDynamicSliceInstanceVegetationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockDynamicSliceInstanceVegetationSystemComponent, "{41BCCB16-1E27-4B8E-9053-762CC5034F18}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect)
        {
            Vegetation::InstanceSpawner::Reflect(reflect);
            Vegetation::DynamicSliceInstanceSpawner::Reflect(reflect);
            Vegetation::EmptyInstanceSpawner::Reflect(reflect);
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("VegetationSystemService", 0xa2322728));
        }
    };

    class DynamicSliceAssetCatalogAndHandler
        : public Vegetation::DescriptorNotificationBus::Handler
        , public AZ::Data::AssetCatalogRequestBus::Handler
        , public AZ::Data::AssetHandler
        , public AZ::Data::AssetCatalog
        , public AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler
    {
    public:
        DynamicSliceAssetCatalogAndHandler()
        {
            // Intercept messages for finding assets by name and creating/destroying slices.
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
            AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler::BusConnect();
        }

        ~DynamicSliceAssetCatalogAndHandler()
        {
            AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        // Helper methods:

        // Set up a mock asset with the given name and id and direct the instance spawner to use it.
        void CreateAndSetMockAsset(Vegetation::DynamicSliceInstanceSpawner& instanceSpawner, AZ::Data::AssetId assetId, AZStd::string assetPath)
        {
            // Save these off for use from our mock AssetCatalogRequestBus
            m_assetId = assetId;
            m_assetPath = assetPath;

            Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);

            // Tell the spawner to use this asset.  Note that this also triggers a LoadAssets() call
            // internally.
            instanceSpawner.SetSliceAssetPath(m_assetPath);

            // Our instance spawner should now have a valid asset reference.
            // It may or may not be loaded already by the time we get here,
            // depending on how quickly the Asset Processor job thread picks it up.
            EXPECT_FALSE(instanceSpawner.HasEmptyAssetReferences());

            // Since the asset load is going through the real AssetManager, there's a delay while a separate
            // job thread executes and actually loads our mock dynamic slice asset.
            // If our asset hasn't loaded successfully after 5 seconds, it's unlikely to succeed.
            // This choice of delay should be *reasonably* safe because it's all CPU-based processing,
            // no actual I/O occurs as a part of the test.
            constexpr int sleepMs = 10;
            constexpr int totalWaitTimeMs = 5000;
            int numRetries = totalWaitTimeMs / sleepMs;
            while ((m_numOnLoadedCalls < 1) && (numRetries >= 0))
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
                AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepMs));
                numRetries--;
            }

            ASSERT_TRUE(m_numOnLoadedCalls == 1);
            EXPECT_TRUE(instanceSpawner.IsLoaded());
            EXPECT_TRUE(instanceSpawner.IsSpawnable());

            Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
        }


        // AssetHandler
        // Minimalist mocks to look like a Dynamic Slice has been created/loaded/destroyed successfully
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            AZ::DynamicSliceAsset* sliceAsset = new AZ::DynamicSliceAsset(id);
            AZ::Entity* mockEntity = new AZ::Entity();
            mockEntity->Init();
            auto mockComponent = mockEntity->CreateComponent<AZ::SliceComponent>();
            mockEntity->Activate();
            sliceAsset->SetData(mockEntity, mockComponent);
            MockAssetData* temp = reinterpret_cast<MockAssetData*>(sliceAsset);
            temp->SetStatus(AZ::Data::AssetData::AssetStatus::NotLoaded);

            return sliceAsset;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override { delete ptr; }
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override { assetTypes.push_back(AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid()); }
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            MockAssetData* temp = reinterpret_cast<MockAssetData*>(asset.GetData());
            temp->SetStatus(AZ::Data::AssetData::AssetStatus::Ready);
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        // DescriptorNotificationBus
        // Keep track of whether or not the Spawner successfully loaded the asset and notified listeners
        void OnDescriptorAssetsLoaded() override { m_numOnLoadedCalls++; }

        // AssetCatalogRequestBus
        // Minimalist mocks to provide our desired asset path or asset id
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& /*id*/) override { return m_assetPath; }
        AZ::Data::AssetId GetAssetIdByPath(const char* /*path*/, const AZ::Data::AssetType& /*typeToRegister*/, bool /*autoRegisterIfNotFound*/) override { return m_assetId; }
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& /*id*/) override
        {
            AZ::Data::AssetInfo assetInfo;
            assetInfo.m_assetId = m_assetId;
            assetInfo.m_assetType = AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid();
            assetInfo.m_relativePath = m_assetPath;
            return assetInfo;
        }

        // SliceGameEntityOwnershipServiceBus
        // Minimalist mocks to mock out InstantiateDynamicSlice  
        void CancelDynamicSliceInstantiation(const AzFramework::SliceInstantiationTicket& /*ticket*/) override {}
        bool DestroyDynamicSliceByEntity(const AZ::EntityId& /*id*/) override { return true; }
        AzFramework::SliceInstantiationTicket InstantiateDynamicSlice(
            const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/,
            const AZ::Transform& /*worldTransform*/,
            const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& /*customIdMapper*/) override
        {
            return AzFramework::SliceInstantiationTicket(AzFramework::EntityContextId::Create(), 1);
        }

        // AssetCatalog
        // Minimalist mock to pretend like we've loaded a Dynamic Slice asset
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad([[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamName = m_assetPath;
            info.m_dataLen = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            return info;
        }

        AZStd::string m_assetPath;
        AZ::Data::AssetId m_assetId;
        int m_numOnLoadedCalls = 0;
    };

    // To test Dynamic Slice spawning, we need to mock up enough of the asset management system and the dynamic slice
    // asset handling to pretend like we're loading/unloading dynamic slices successfully.
    class DynamicSliceInstanceSpawnerTests
        : public VegetationComponentTests
    {
    public:
        DynamicSliceInstanceSpawnerTests()
            : m_restoreFileIO(m_fileIOMock)
        {
            AZ::IO::MockFileIOBase::InstallDefaultReturns(m_fileIOMock);
        }
        void SetUp() override
        {
            VegetationComponentTests::SetUp();

            // Create a real Asset Mananger, and point to ourselves as the handler for DynamicSliceAsset.
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            // Initialize the job manager with 1 thread for the AssetManager to use.
            AZ::JobManagerDesc jobDesc;
            AZ::JobManagerThreadDesc threadDesc;
            jobDesc.m_workerThreads.push_back(threadDesc);
            m_jobManager = aznew AZ::JobManager(jobDesc);
            m_jobContext = aznew AZ::JobContext(*m_jobManager);
            AZ::JobContext::SetGlobalContext(m_jobContext);

            AZ::Data::AssetManager::Descriptor descriptor;
            AZ::Data::AssetManager::Create(descriptor);
            m_testHandler = AZStd::make_unique<DynamicSliceAssetCatalogAndHandler>();
            AZ::Data::AssetManager::Instance().RegisterHandler(m_testHandler.get(), AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(m_testHandler.get(), AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid());

            m_app.RegisterComponentDescriptor(AZ::SliceComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            // Clear out the list of queued AssetBus Events before unregistering the AssetHandler
            // to make sure pending references to Asset<AssetData> instances are cleared
            AZ::Data::AssetManager::Instance().DispatchEvents();
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_testHandler.get());
            AZ::Data::AssetManager::Instance().UnregisterCatalog(m_testHandler.get());
            AZ::Data::AssetManager::Destroy();

            m_testHandler.reset();

            AZ::JobContext::SetGlobalContext(nullptr);
            delete m_jobContext;
            delete m_jobManager;

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            VegetationComponentTests::TearDown();
        }

        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockDynamicSliceInstanceVegetationSystemComponent::CreateDescriptor());
        }

    protected:
        AZStd::unique_ptr<DynamicSliceAssetCatalogAndHandler> m_testHandler;

    private:
        AZ::JobManager* m_jobManager{ nullptr };
        AZ::JobContext* m_jobContext{ nullptr };
        SetRestoreFileIOBaseRAII m_restoreFileIO;
        ::testing::NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;
    };

    TEST_F(DynamicSliceInstanceSpawnerTests, BasicInitializationTest)
    {
        // Basic test to make sure we can construct / destroy without errors.

        Vegetation::DynamicSliceInstanceSpawner instanceSpawner;
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, DefaultSpawnersAreEqual)
    {
        // Two different instances of the default DynamicSliceInstanceSpawner should
        // be considered data-equivalent.

        Vegetation::DynamicSliceInstanceSpawner instanceSpawner1;
        Vegetation::DynamicSliceInstanceSpawner instanceSpawner2;

        EXPECT_TRUE(instanceSpawner1 == instanceSpawner2);
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, DifferentSpawnersAreNotEqual)
    {
        // Two spawners with different data should *not* be data-equivalent.

        Vegetation::DynamicSliceInstanceSpawner instanceSpawner1;
        Vegetation::DynamicSliceInstanceSpawner instanceSpawner2;

        // Give the second instance spawner a non-default asset reference.
        m_testHandler->CreateAndSetMockAsset(instanceSpawner2, AZ::Uuid::CreateRandom(), "test");

        // The test is written this way because only the == operator is overloaded.
        EXPECT_TRUE(!(instanceSpawner1 == instanceSpawner2));
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, LoadAndUnloadAssets)
    {
        // The spawner should successfully load/unload assets without errors.

        Vegetation::DynamicSliceInstanceSpawner instanceSpawner;

        // Our instance spawner should be empty before we set the assets.
        EXPECT_TRUE(instanceSpawner.HasEmptyAssetReferences());

        // This will test the asset load.
        m_testHandler->CreateAndSetMockAsset(instanceSpawner, AZ::Uuid::CreateRandom(), "test");

        // Test the asset unload works too.
        m_testHandler->Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);
        instanceSpawner.UnloadAssets();
        EXPECT_FALSE(instanceSpawner.IsLoaded());
        EXPECT_FALSE(instanceSpawner.IsSpawnable());
        m_testHandler->Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, CreateAndDestroyInstance)
    {
        // The spawner should successfully create and destroy an instance without errors.

        Vegetation::DynamicSliceInstanceSpawner instanceSpawner;

        m_testHandler->CreateAndSetMockAsset(instanceSpawner, AZ::Uuid::CreateRandom(), "test");

        instanceSpawner.OnRegisterUniqueDescriptor();

        Vegetation::InstanceData instanceData;
        Vegetation::InstancePtr instance = instanceSpawner.CreateInstance(instanceData);
        EXPECT_TRUE(instance);
        instanceSpawner.DestroyInstance(0, instance);

        instanceSpawner.OnReleaseUniqueDescriptor();
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, SpawnerRegisteredWithDescriptor)
    {
        // Validate that the Descriptor successfully gets DynamicSliceInstanceSpawner registered with it,
        // as long as InstanceSpawner and DynamicSliceInstanceSpawner have been reflected.

        MockDynamicSliceInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        Vegetation::Descriptor descriptor;
        descriptor.RefreshSpawnerTypeList();
        auto spawnerTypes = descriptor.GetSpawnerTypeList();
        EXPECT_TRUE(spawnerTypes.size() > 0);
        EXPECT_TRUE(spawnerTypes[0].first == Vegetation::DynamicSliceInstanceSpawner::RTTI_Type());
    }

    TEST_F(DynamicSliceInstanceSpawnerTests, DescriptorCreatesCorrectSpawner)
    {
        // Validate that the Descriptor successfully creates a new DynamicSliceInstanceSpawner if we change
        // the spawner type on the Descriptor.

        MockDynamicSliceInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        // We expect the Descriptor to start off with a Legacy Vegetation spawner, but then should correctly get an
        // DynamicSliceInstanceSpawner after we change spawnerType.
        Vegetation::Descriptor descriptor;
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) == Vegetation::DynamicSliceInstanceSpawner::RTTI_Type());
        descriptor.m_spawnerType = Vegetation::EmptyInstanceSpawner::RTTI_Type();
        descriptor.RefreshSpawnerTypeList();
        descriptor.SpawnerTypeChanged();
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) != Vegetation::DynamicSliceInstanceSpawner::RTTI_Type());
    }
}
