/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentralReflectionTest.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
// LmbrCentral/Source
#include "Scripting/SpawnerComponent.h"
#include "LmbrCentral.h"

#ifdef LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include "LmbrCentralEditor.h"
#include "Scripting/EditorSpawnerComponent.h"
#endif

// Tracks SpawnerComponentNotificationBus events
class SpawnWatcher
    : public LmbrCentral::SpawnerComponentNotificationBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(SpawnWatcher, AZ::SystemAllocator);

    SpawnWatcher(AZ::EntityId spawnerEntityId)
    {
        LmbrCentral::SpawnerComponentNotificationBus::Handler::BusConnect(spawnerEntityId);
    }

    void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnBegin = true;
    }

    void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnEnd = true;
    }

    void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZ::EntityId& spawnedEntity) override
    {
        m_tickets[ticket].m_onEntitySpawned.emplace_back(spawnedEntity);
    }

    void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        m_tickets[ticket].m_onEntitiesSpawned = spawnedEntities;
    }

    void OnSpawnedSliceDestroyed(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnedSliceDestroyed = true;
    }

    // Which events have fired for a particular ticket
    struct TicketInfo
    {
        bool m_onSpawnBegin = false;
        bool m_onSpawnEnd = false;
        AZStd::vector<AZ::EntityId> m_onEntitySpawned;
        AZStd::vector<AZ::EntityId> m_onEntitiesSpawned;
        bool m_onSpawnedSliceDestroyed = false;
    };

    AZStd::unordered_map<AzFramework::SliceInstantiationTicket, TicketInfo> m_tickets;
};

// Simplified version of AzFramework::Application
class SpawnerApplication
    : public AzFramework::Application
{
    // override and only include system components required for spawner tests.
    AZ::ComponentTypeList GetRequiredSystemComponents() const override
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
        };
    }

    void RegisterCoreComponents() override
    {
        AzFramework::Application::RegisterCoreComponents();
        RegisterComponentDescriptor(LmbrCentral::SpawnerComponent::CreateDescriptor());
    }
};

class SpawnerComponentTest
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        // start application
        AZ::ComponentApplication::Descriptor appDescriptor;
        appDescriptor.m_useExistingAllocator = true;

        m_application = aznew SpawnerApplication();

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_application->Start(appDescriptor, startupParameters);

        // create a dynamic slice in the asset system
        AZ::Entity* sliceAssetEntity = aznew AZ::Entity();
        AZ::SliceComponent* sliceAssetComponent = sliceAssetEntity->CreateComponent<AZ::SliceComponent>();
        sliceAssetComponent->SetSerializeContext(m_application->GetSerializeContext());
        sliceAssetEntity->Init();
        sliceAssetEntity->Activate();

        AZ::Entity* entityInSlice1 = aznew AZ::Entity("spawned entity 1");
        entityInSlice1->CreateComponent<AzFramework::TransformComponent>();
        sliceAssetComponent->AddEntity(entityInSlice1);

        AZ::Entity* entityInSlice2 = aznew AZ::Entity("spawned entity 2");
        entityInSlice2->CreateComponent<AzFramework::TransformComponent>();
        sliceAssetComponent->AddEntity(entityInSlice2);

        m_sliceAssetRef = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId("{E47E78B1-FF5E-4191-BE72-A06428D324F3}"), AZ::Data::AssetLoadBehavior::Default);
        m_sliceAssetRef.Get()->SetData(sliceAssetEntity, sliceAssetComponent);

        // create an entity with a spawner component
        AZ::Entity* spawnerEntity = aznew AZ::Entity("spawner");
        m_spawnerComponent = spawnerEntity->CreateComponent<LmbrCentral::SpawnerComponent>();
        spawnerEntity->Init();
        spawnerEntity->Activate();

        // create class that will watch for spawner component notifications
        m_spawnWatcher = aznew SpawnWatcher(m_spawnerComponent->GetEntityId());
    }

    void TearDown() override
    {
        delete m_spawnWatcher;
        m_spawnWatcher = nullptr;

        delete m_application->FindEntity(m_spawnerComponent->GetEntityId());
        m_spawnerComponent = nullptr;

        // reset game context (delete any spawned slices and their entities)
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::ResetGameContext);

        m_sliceAssetRef = AZ::Data::Asset<AZ::SliceAsset>();

        m_application->Stop();
        delete m_application;
        m_application = nullptr;
    }

    // Tick application until 'condition' function returns true.
    // If 'maxTicks' elapse without condition passing, return false.
    bool TickUntil(AZStd::function<bool()> condition, size_t maxTicks=100)
    {
        for (size_t tickI = 0; tickI < maxTicks; ++tickI)
        {
            if (condition())
            {
                return true;
            }

            m_application->Tick();
        }
        return false;
    }

    // Common test operation: Spawn m_sliceAssetRef and tick application until OnSpawnEnd fires.
    AzFramework::SliceInstantiationTicket SpawnDefaultSlice()
    {
        AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
        bool onSpawnEndFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnEnd; });
        EXPECT_TRUE(onSpawnEndFired); // sanity check
        return ticket;
    }

    // Common test operation: Spawn m_sliceAssetRef many times and tick application until OnSpawnEnd fires for each spawn.
    AZStd::vector<AzFramework::SliceInstantiationTicket> SpawnManyDefaultSlices()
    {
        AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
        for (int i = 0; i < 10; ++i)
        {
            tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
        }

        bool onSpawnEndFiredForAll = TickUntil(
            [this, &tickets]()
            {
                for (AzFramework::SliceInstantiationTicket& ticket : tickets)
                {
                    if (!m_spawnWatcher->m_tickets[ticket].m_onSpawnEnd)
                    {
                        return false;
                    }
                }
                return true;
            });
        EXPECT_TRUE(onSpawnEndFiredForAll); // sanity check

        return tickets;
    }


    SpawnerApplication*             m_application = nullptr;
    AZ::Data::Asset<AZ::SliceAsset> m_sliceAssetRef; // a slice asset to spawn
    LmbrCentral::SpawnerComponent*  m_spawnerComponent = nullptr; // a spawner component to test
    SpawnWatcher*                   m_spawnWatcher = nullptr; // tracks events from the spawner component
};

const size_t kEntitiesInSlice = 2; // number of entities in asset we're testing with

TEST_F(SpawnerComponentTest, SanityCheck)
{
    // Tests that Setup/TearDown work as expected
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnSpawnEnd_Fires)
{
    // First test the helper function, which checks for OnSpawnEnd
    SpawnDefaultSlice();
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnSpawnBegin_Fires)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();
    EXPECT_TRUE(m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin);
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnEntitySpawned_FiresOncePerEntity)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    EXPECT_EQ(kEntitiesInSlice, m_spawnWatcher->m_tickets[ticket].m_onEntitySpawned.size());
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnEntitiesSpawned_FiresWithAllEntities)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    EXPECT_EQ(kEntitiesInSlice, m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size());
}

TEST_F(SpawnerComponentTest, OnSpawnedSliceDestroyed_FiresAfterEntitiesDeleted)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    for (AZ::EntityId spawnedEntityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
    {
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity, spawnedEntityId);
    }

    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });
    EXPECT_TRUE(spawnDestructionFired);
}


TEST_F(SpawnerComponentTest, DISABLED_OnSpawnedSliceDestroyed_FiresWhenSpawningBadAssets) // disabled because AZ_TEST_START_TRACE_SUPPRESSION isn't currently suppressing the asserts
{
    // ID is made up and not registered with asset manager
    auto nonexistentAsset = AZ::Data::Asset<AZ::SliceAsset>(AZ::Data::AssetId("{9E3862CC-B6DF-485F-A9D8-5F4A966DE88B}"), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(nonexistentAsset);

    AZ_TEST_START_TRACE_SUPPRESSION;
    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    EXPECT_TRUE(spawnDestructionFired);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_EntitiesFromSpawn_AreDeleted)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    m_spawnerComponent->DestroySpawnedSlice(ticket);
    bool entitiesRemoved = TickUntil(
        [this, ticket]()
        {
            for (AZ::EntityId entityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
            {
                if (m_application->FindEntity(entityId))
                {
                    return false;
                }
            }
            return true;
        });

    EXPECT_TRUE(entitiesRemoved);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_OnSpawnedSliceDestroyed_Fires)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    m_spawnerComponent->DestroySpawnedSlice(ticket);
    bool onSpawnedSliceDestroyed = TickUntil([this, ticket](){ return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });

    EXPECT_TRUE(onSpawnedSliceDestroyed);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
    m_spawnerComponent->DestroySpawnedSlice(ticket);

    // wait a long time, just to be sure no queued entity instantiation takes place
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    EXPECT_FALSE(m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin);
    EXPECT_TRUE(m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed);
}

class GameEntityContextWatcher
    : public AzFramework::SliceGameEntityOwnershipServiceNotificationBus::Handler
{
public:
    GameEntityContextWatcher()
    {
        AzFramework::SliceGameEntityOwnershipServiceNotificationBus::Handler::BusConnect();
    }

    void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*instance*/, const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_onSliceInstantiatedTickets.emplace(ticket);
    }

    void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_onSliceInstantiationFailedTickets.emplace(ticket);
    }
    
    AZStd::unordered_set<AzFramework::SliceInstantiationTicket> m_onSliceInstantiatedTickets;
    AZStd::unordered_set<AzFramework::SliceInstantiationTicket> m_onSliceInstantiationFailedTickets;
};

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_BeforeOnSpawnBegin_ContextFiresOnSliceInstantiationFailed)
{
    // context should send out instantiation failure message, even if ticket is explicitly cancelled.
    // others might be listening to the context and not know about the cancellation.

    GameEntityContextWatcher contextWatcher;
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
    m_spawnerComponent->DestroySpawnedSlice(ticket);

    TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });

    bool onSliceInstantiationFailed = contextWatcher.m_onSliceInstantiationFailedTickets.count(ticket) > 0;
    EXPECT_TRUE(onSliceInstantiationFailed);

    bool onSliceInstantiated = contextWatcher.m_onSliceInstantiatedTickets.count(ticket) > 0;
    EXPECT_FALSE(onSliceInstantiated);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_WhenManySpawnsInProgress_DoesntAffectOtherSpawns)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
    }

    m_spawnerComponent->DestroySpawnedSlice(tickets[0]);

    // check that other slices finish spawning
    bool entitiesSpawnedInOtherSlices = TickUntil(
        [this, &tickets]()
        {
            for (size_t i = 1; i < tickets.size(); ++i)
            {
                if (m_spawnWatcher->m_tickets[tickets[i]].m_onEntitiesSpawned.size() > 0)
                {
                    return false;
                }
            }
            return true;
        });

    EXPECT_TRUE(entitiesSpawnedInOtherSlices);

    // check that one slice destroyed
    bool sliceDestroyed = TickUntil([this, &tickets]()
    {
        return m_spawnWatcher->m_tickets[tickets[0]].m_onSpawnedSliceDestroyed;
    });
    EXPECT_TRUE(sliceDestroyed);

    // make sure no other slice get destroyed
    bool anyOtherSliceDestroyed = false;
    for (size_t i = 1; i < tickets.size(); ++i)
    {
        if (m_spawnWatcher->m_tickets[tickets[i]].m_onSpawnedSliceDestroyed)
        {
            anyOtherSliceDestroyed = true;
        }
    }
    EXPECT_FALSE(anyOtherSliceDestroyed);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_AllSpawnedEntities_AreDestroyed)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    m_spawnerComponent->DestroyAllSpawnedSlices();

    bool allEntitiesDestroyed = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::SliceInstantiationTicket& ticket : tickets)
            {
                for (AZ::EntityId spawnedEntityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
                {
                    if (m_application->FindEntity(spawnedEntityId))
                    {
                        return false;
                    }
                }
            }
            return true;
        });
    EXPECT_TRUE(allEntitiesDestroyed);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_OnSpawnedSliceDestroyed_FiresForAll)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    m_spawnerComponent->DestroyAllSpawnedSlices();

    bool onSpawnedSliceDestroyedFiresForAll = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::SliceInstantiationTicket& ticket : tickets)
            {
                if (!m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed)
                {
                    return false;
                }
            }
            return true;
        });
    EXPECT_TRUE(onSpawnedSliceDestroyedFiresForAll);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
    }

    m_spawnerComponent->DestroyAllSpawnedSlices();

    // wait a long time, to ensure no queued activity results in an instantiation
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    bool anyOnSpawnBegan = false;
    bool allOnSpawnedSliceDestroyed = true;
    for (AzFramework::SliceInstantiationTicket& ticket : tickets)
    {
        anyOnSpawnBegan |= m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin;
        allOnSpawnedSliceDestroyed &= m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed;
    }

    EXPECT_FALSE(anyOnSpawnBegan);
    EXPECT_TRUE(allOnSpawnedSliceDestroyed);
}

TEST_F(SpawnerComponentTest, GetCurrentEntitiesFromSpawnedSlice_ReturnsEntities)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();
    AZStd::vector<AZ::EntityId> entities = m_spawnerComponent->GetCurrentEntitiesFromSpawnedSlice(ticket);

    EXPECT_EQ(m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size(), entities.size());
}

TEST_F(SpawnerComponentTest, GetCurrentEntitiesFromSpawnedSlice_WithEntityDeleted_DoesNotReturnDeletedEntity)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    AZStd::vector<AZ::EntityId>& entitiesBeforeDelete = m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned;

    AZ::EntityId entityToDelete = entitiesBeforeDelete[0];
    delete m_application->FindEntity(entityToDelete);

    AZStd::vector<AZ::EntityId> entitiesAfterDelete = m_spawnerComponent->GetCurrentEntitiesFromSpawnedSlice(ticket);

    EXPECT_EQ(entitiesBeforeDelete.size() - 1, entitiesAfterDelete.size());

    bool deletedEntityPresent = AZStd::find(entitiesAfterDelete.begin(), entitiesAfterDelete.end(), entityToDelete) != entitiesAfterDelete.end();
    EXPECT_FALSE(deletedEntityPresent);
}

TEST_F(SpawnerComponentTest, GetAllCurrentlySpawnedEntities_ReturnsEntities)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    AZStd::vector<AZ::EntityId> entities = m_spawnerComponent->GetAllCurrentlySpawnedEntities();

    bool allEntitiesFound = true;
    size_t numEntities = 0;

    // compare against entities from OnEntitiesSpawned event
    for (auto& ticketInfoPair : m_spawnWatcher->m_tickets)
    {
        for (AZ::EntityId spawnedEntity : ticketInfoPair.second.m_onEntitiesSpawned)
        {
            ++numEntities;
            allEntitiesFound &= AZStd::find(entities.begin(), entities.end(), spawnedEntity) != entities.end();
        }
    }

    EXPECT_EQ(numEntities, entities.size());
    EXPECT_TRUE(allEntitiesFound);
}

// Legacy SpawnerComponent from game data
// Should get converted into modern SpawnerComponent
const char kWrappedGameSpawnerComponent[] =
R"DELIMITER(<ObjectStream version="3">
    <Class name="SpawnerComponent" field="element" version="1" type="{8022A627-FA7D-4516-A155-657A0927A3CA}">
        <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
            <Class name="AZ::u64" field="Id" value="8317941343245109563" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        </Class>
        <Class name="Asset" field="Slice" value="id={6F11134F-84C9-559F-AABA-3D1778656707}:2,type={78802ABF-9595-463A-8D2B-D022F906F9B1},hint={slices/particle_electrical_damage.dynamicslice}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        <Class name="bool" field="SpawnOnActivate" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        <Class name="bool" field="DestroyOnDeactivate" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
    </Class>
</ObjectStream>)DELIMITER";

class LoadSpawnerComponentFromLegacyGameData
    : public LoadReflectedObjectTest<AzFramework::Application, LmbrCentral::LmbrCentralModule, LmbrCentral::SpawnerComponent>
{
public:
    const char* GetSourceDataBuffer() const { return kWrappedGameSpawnerComponent; }

    void SetUp()
    {
        LoadReflectedObjectTest::SetUp();

        if (m_object)
        {
            m_readConfigSuccess = m_object->GetConfiguration(m_spawnerConfig);
        }
    }

    LmbrCentral::SpawnerConfig m_spawnerConfig;
    bool m_readConfigSuccess = false;
};

TEST_F(LoadSpawnerComponentFromLegacyGameData, Fixture_SanityCheck)
{
    EXPECT_NE(nullptr, GetApplication());
}

TEST_F(LoadSpawnerComponentFromLegacyGameData, SpawnerComponent_LoadsFromData)
{
    EXPECT_NE(nullptr, m_object.get());
}

TEST_F(LoadSpawnerComponentFromLegacyGameData, ComponentId_ValuePreserved)
{
    EXPECT_EQ(AZ::ComponentId(8317941343245109563ULL), m_object->GetId());
}

TEST_F(LoadSpawnerComponentFromLegacyGameData, SliceAsset_ValuePreserved)
{
    EXPECT_EQ(AZ::Uuid("{6F11134F-84C9-559F-AABA-3D1778656707}"), m_spawnerConfig.m_sliceAsset.GetId().m_guid);
}

TEST_F(LoadSpawnerComponentFromLegacyGameData, SpawnOnActivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_spawnOnActivate);
}

TEST_F(LoadSpawnerComponentFromLegacyGameData, DestroyOnDeactivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_destroyOnDeactivate);
}

#ifdef LMBR_CENTRAL_EDITOR
// Legacy SpawnerComponent from editor data (wrapped inside a GenericComponentWrapper)
// Should get converted into EditorSpawnerComponent
const char kWrappedLegacySpawnerComponent[] =
R"DELIMITER(<ObjectStream version="3">
    <Class name="GenericComponentWrapper" field="element" type="{68D358CA-89B9-4730-8BA6-E181DEA28FDE}">
        <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                <Class name="AZ::u64" field="Id" value="6866719809809621109" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
            </Class>
        </Class>
        <Class name="SpawnerComponent" field="m_template" version="1" type="{8022A627-FA7D-4516-A155-657A0927A3CA}">
            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                <Class name="AZ::u64" field="Id" value="0" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
            </Class>
            <Class name="Asset" field="Slice" value="id={3987FC80-0CF5-5A22-BE55-1EEDF382909E}:2,type={78802ABF-9595-463A-8D2B-D022F906F9B1},hint={slices/ai_walker.dynamicslice}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
            <Class name="bool" field="SpawnOnActivate" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
            <Class name="bool" field="DestroyOnDeactivate" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        </Class>
    </Class>
</ObjectStream>)DELIMITER";

class LoadSpawnerComponentFromLegacyEditorData
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentral::LmbrCentralEditorModule, AzToolsFramework::Components::GenericComponentWrapper>
{
public:
    const char* GetSourceDataBuffer() const override { return kWrappedLegacySpawnerComponent; }

    void SetUp() override
    {
        LoadReflectedObjectTest::SetUp();

        // reset values from previous run
        m_editorSpawnerComponent = nullptr;
        m_readConfigSuccess = false;

        if (m_object)
        {
            m_editorSpawnerComponent = azrtti_cast<LmbrCentral::EditorSpawnerComponent*>(m_object->GetTemplate());
            if (m_editorSpawnerComponent)
            {
                m_readConfigSuccess = m_editorSpawnerComponent->GetConfiguration(m_spawnerConfig);
            }
        }
    }

    LmbrCentral::EditorSpawnerComponent* m_editorSpawnerComponent;
    LmbrCentral::SpawnerConfig m_spawnerConfig;
    bool m_readConfigSuccess;

};

TEST_F(LoadSpawnerComponentFromLegacyEditorData, Fixture_SanityCheck)
{
    EXPECT_NE(nullptr, GetApplication());
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, ObjectStream_LoadsComponents)
{
    EXPECT_NE(nullptr, m_object.get());
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, LegacySpawnerComponent_TurnedIntoEditorSpawnerComponent)
{
    EXPECT_NE(nullptr, m_editorSpawnerComponent);
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, SpawnerConfig_SuccessfullyRead)
{
    EXPECT_TRUE(m_readConfigSuccess);
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, SliceAsset_ValuePreserved)
{
    EXPECT_EQ(AZ::Uuid("{3987FC80-0CF5-5A22-BE55-1EEDF382909E}"), m_spawnerConfig.m_sliceAsset.GetId().m_guid);
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, SpawnOnActivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_spawnOnActivate);
}

TEST_F(LoadSpawnerComponentFromLegacyEditorData, DestroyOnDeactivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_destroyOnDeactivate);
}

#endif // LMBR_CENTRAL_EDITOR
