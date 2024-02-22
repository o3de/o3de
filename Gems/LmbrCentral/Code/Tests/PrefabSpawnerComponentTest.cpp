/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)

#include "LmbrCentralReflectionTest.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
// LmbrCentral/Source
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include "Scripting/PrefabSpawnerComponent.h"
#include "LmbrCentral.h"

#ifdef LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include "LmbrCentralEditor.h"
#include "Scripting/EditorPrefabSpawnerComponent.h"
#endif

// Tracks PrefabSpawnerComponentNotificationBus events
class PrefabSpawnWatcher
    : public LmbrCentral::PrefabSpawnerComponentNotificationBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(PrefabSpawnWatcher, AZ::SystemAllocator);

    PrefabSpawnWatcher(AZ::EntityId spawnerEntityId)
    {
        LmbrCentral::PrefabSpawnerComponentNotificationBus::Handler::BusConnect(spawnerEntityId);
    }

    void OnSpawnBegin(const AzFramework::EntitySpawnTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnBegin = true;
    }

    void OnSpawnEnd(const AzFramework::EntitySpawnTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnEnd = true;
    }

    void OnEntitySpawned(const AzFramework::EntitySpawnTicket& ticket, const AZ::EntityId& spawnedEntity) override
    {
        m_tickets[ticket].m_onEntitySpawned.emplace_back(spawnedEntity);
    }

    void OnEntitiesSpawned(const AzFramework::EntitySpawnTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        m_tickets[ticket].m_onEntitiesSpawned = spawnedEntities;
    }

    void OnSpawnedPrefabDestroyed(const AzFramework::EntitySpawnTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnedPrefabDestroyed = true;
    }

    // Which events have fired for a particular ticket
    struct TicketInfo
    {
        bool m_onSpawnBegin = false;
        bool m_onSpawnEnd = false;
        AZStd::vector<AZ::EntityId> m_onEntitySpawned;
        AZStd::vector<AZ::EntityId> m_onEntitiesSpawned;
        bool m_onSpawnedPrefabDestroyed = false;
    };

    AZStd::unordered_map<AzFramework::EntitySpawnTicket, TicketInfo> m_tickets;
};

// Simplified version of AzFramework::Application
class PrefabSpawnerApplication
    : public AzFramework::Application
{
    // override and only include system components required for spawner tests.
    AZ::ComponentTypeList GetRequiredSystemComponents() const override
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            //azrtti_typeid<AZ::PrefabSystemComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
        };
    }

    void RegisterCoreComponents() override
    {
        AzFramework::Application::RegisterCoreComponents();
        RegisterComponentDescriptor(LmbrCentral::PrefabSpawnerComponent::CreateDescriptor());
    }
};

class PrefabSpawnerComponentTest
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        // start application
        AZ::ComponentApplication::Descriptor appDescriptor;
        appDescriptor.m_useExistingAllocator = true;

        m_application = aznew PrefabSpawnerApplication();

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_application->Start(appDescriptor, startupParameters);

        // create a dynamic prefab in the asset system
        //AZ::Entity* prefabAssetEntity = aznew AZ::Entity();
        //AzToolsFramework::Prefab::Instance* instance = aznew AzToolsFramework::Prefab::Instance();
        //instance->SetContainerEntity(*prefabAssetEntity);

        //AZ::Entity* prefabAssetEntity = aznew AZ::Entity();
        //AZ::Component* prefabAssetComponent = prefabAssetEntity->CreateComponent<AZ::Component>();
        //prefabAssetComponent->SetSerializeContext(m_application->GetSerializeContext());
        //prefabAssetEntity->Init();
        //prefabAssetEntity->Activate();

        AZ::Entity* entityInPrefab1 = aznew AZ::Entity("spawned entity 1");
        entityInPrefab1->CreateComponent<AzFramework::TransformComponent>();

        AZ::Entity* entityInPrefab2 = aznew AZ::Entity("spawned entity 2");
        entityInPrefab2->CreateComponent<AzFramework::TransformComponent>();

        m_prefabAssetRef = AZ::Data::AssetManager::Instance().CreateAsset<AzFramework::Spawnable>(
            AZ::Data::AssetId("{AAABBB11-AB12-24A1-BDE2-BDACE354BAA3}"), AZ::Data::AssetLoadBehavior::Default);
        AzFramework::Spawnable::EntityList& entities = m_prefabAssetRef.Get()->GetEntities();
        entities.emplace_back(entityInPrefab1);
        entities.emplace_back(entityInPrefab2);

        // create an entity with a spawner component
        AZ::Entity* spawnerEntity = aznew AZ::Entity("spawner");
        m_PrefabSpawnerComponent = spawnerEntity->CreateComponent<LmbrCentral::PrefabSpawnerComponent>();
        spawnerEntity->Init();
        spawnerEntity->Activate();

        // create class that will watch for spawner component notifications
        m_PrefabSpawnWatcher = aznew PrefabSpawnWatcher(m_PrefabSpawnerComponent->GetEntityId());
    }

    void TearDown() override
    {
        delete m_PrefabSpawnWatcher;
        m_PrefabSpawnWatcher = nullptr;

        delete m_application->FindEntity(m_PrefabSpawnerComponent->GetEntityId());
        m_PrefabSpawnerComponent = nullptr;

        // reset game context (delete any spawned prefabs and their entities)
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::ResetGameContext);

        m_prefabAssetRef = AZ::Data::Asset<AZ::PrefabAsset>();

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

    // Common test operation: Spawn m_prefabAssetRef and tick application until OnSpawnEnd fires.
    const AzFramework::EntitySpawnTicket SpawnDefaultPrefab()
    {
        const AzFramework::EntitySpawnTicket ticket = m_PrefabSpawnerComponent->SpawnPrefab(m_prefabAssetRef);
        bool onSpawnEndFired = TickUntil([this, ticket]() { return m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnEnd; });
        EXPECT_TRUE(onSpawnEndFired); // sanity check
        return ticket;
    }

    // Common test operation: Spawn m_prefabAssetRef many times and tick application until OnSpawnEnd fires for each spawn.
    AZStd::vector<AzFramework::EntitySpawnTicket> SpawnManyDefaultPrefabs()
    {
        AZStd::vector<AzFramework::EntitySpawnTicket> tickets;
        for (int i = 0; i < 10; ++i)
        {
            tickets.emplace_back(m_PrefabSpawnerComponent->SpawnPrefab(m_prefabAssetRef));
        }

        bool onSpawnEndFiredForAll = TickUntil(
            [this, &tickets]()
            {
                for (const AzFramework::EntitySpawnTicket& ticket : tickets)
                {
                    if (!m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnEnd)
                    {
                        return false;
                    }
                }
                return true;
            });
        EXPECT_TRUE(onSpawnEndFiredForAll); // sanity check

        return tickets;
    }


    PrefabSpawnerApplication*             m_application = nullptr;
    AZ::Data::Asset<AzFramework::Spawnable> m_prefabAssetRef; // a prefab asset to spawn
    LmbrCentral::PrefabSpawnerComponent*  m_PrefabSpawnerComponent = nullptr; // a spawner component to test
    PrefabSpawnWatcher*                   m_PrefabSpawnWatcher = nullptr; // tracks events from the spawner component
};

const size_t kEntitiesInPrefab = 2; // number of entities in asset we're testing with

TEST_F(PrefabSpawnerComponentTest, SanityCheck)
{
    // Tests that Setup/TearDown work as expected
}

TEST_F(PrefabSpawnerComponentTest, SpawnPrefab_OnSpawnEnd_Fires)
{
    // First test the helper function, which checks for OnSpawnEnd
    SpawnDefaultPrefab();
}

TEST_F(PrefabSpawnerComponentTest, SpawnPrefab_OnSpawnBegin_Fires)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();
    EXPECT_TRUE(m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnBegin);
}

TEST_F(PrefabSpawnerComponentTest, SpawnPrefab_OnEntitySpawned_FiresOncePerEntity)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    EXPECT_EQ(kEntitiesInPrefab, m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitySpawned.size());
}

TEST_F(PrefabSpawnerComponentTest, SpawnPrefab_OnEntitiesSpawned_FiresWithAllEntities)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    EXPECT_EQ(kEntitiesInPrefab, m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size());
}

TEST_F(PrefabSpawnerComponentTest, OnSpawnedPrefabDestroyed_FiresAfterEntitiesDeleted)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    for (AZ::EntityId spawnedEntityId : m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
    {
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity, spawnedEntityId);
    }

    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed; });
    EXPECT_TRUE(spawnDestructionFired);
}


TEST_F(PrefabSpawnerComponentTest, DISABLED_OnSpawnedPrefabDestroyed_FiresWhenSpawningBadAssets) // disabled because AZ_TEST_START_TRACE_SUPPRESSION isn't currently suppressing the asserts
{
    // ID is made up and not registered with asset manager
    auto nonexistentAsset = AZ::Data::Asset<AzFramework::Spawnable>(
        AZ::Data::AssetId("{9E3862CC-B6DF-485F-A9D8-5F4A966DE88B}"), AZ::AzTypeInfo<AzFramework::Spawnable>::Uuid());
    const AzFramework::EntitySpawnTicket ticket = m_PrefabSpawnerComponent->SpawnPrefab(nonexistentAsset);

    AZ_TEST_START_TRACE_SUPPRESSION;
    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed; });
    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    EXPECT_TRUE(spawnDestructionFired);
}

TEST_F(PrefabSpawnerComponentTest, DestroySpawnedPrefab_EntitiesFromSpawn_AreDeleted)
{
    AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    m_PrefabSpawnerComponent->DestroySpawnedPrefab(ticket);
    bool entitiesRemoved = TickUntil(
        [this, ticket]()
        {
            for (AZ::EntityId entityId : m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
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

TEST_F(PrefabSpawnerComponentTest, DestroySpawnedPrefab_OnSpawnedPrefabDestroyed_Fires)
{
    AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    m_PrefabSpawnerComponent->DestroySpawnedPrefab(ticket);
    bool onSpawnedPrefabDestroyed = TickUntil([this, ticket](){ return m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed; });

    EXPECT_TRUE(onSpawnedPrefabDestroyed);
}

TEST_F(PrefabSpawnerComponentTest, DestroySpawnedPrefab_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AzFramework::EntitySpawnTicket ticket = m_PrefabSpawnerComponent->SpawnPrefab(m_prefabAssetRef);
    m_PrefabSpawnerComponent->DestroySpawnedPrefab(ticket);

    // wait a long time, just to be sure no queued entity instantiation takes place
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    EXPECT_FALSE(m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnBegin);
    EXPECT_TRUE(m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed);
}

TEST_F(PrefabSpawnerComponentTest, DestroySpawnedPrefab_WhenManySpawnsInProgress_DoesntAffectOtherSpawns)
{
    AZStd::vector<AzFramework::EntitySpawnTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_PrefabSpawnerComponent->SpawnPrefab(m_prefabAssetRef));
    }

    m_PrefabSpawnerComponent->DestroySpawnedPrefab(tickets[0]);

    // check that other prefabs finish spawning
    bool entitiesSpawnedInOtherPrefabs = TickUntil(
        [this, &tickets]()
        {
            for (size_t i = 1; i < tickets.size(); ++i)
            {
                if (m_PrefabSpawnWatcher->m_tickets[tickets[i]].m_onEntitiesSpawned.size() > 0)
                {
                    return false;
                }
            }
            return true;
        });

    EXPECT_TRUE(entitiesSpawnedInOtherPrefabs);

    // check that one prefab destroyed
    bool prefabDestroyed = TickUntil([this, &tickets]()
    {
        return m_PrefabSpawnWatcher->m_tickets[tickets[0]].m_onSpawnedPrefabDestroyed;
    });
    EXPECT_TRUE(prefabDestroyed);

    // make sure no other prefab get destroyed
    bool anyOtherPrefabDestroyed = false;
    for (size_t i = 1; i < tickets.size(); ++i)
    {
        if (m_PrefabSpawnWatcher->m_tickets[tickets[i]].m_onSpawnedPrefabDestroyed)
        {
            anyOtherPrefabDestroyed = true;
        }
    }
    EXPECT_FALSE(anyOtherPrefabDestroyed);
}

TEST_F(PrefabSpawnerComponentTest, DestroyAllSpawnedPrefabs_AllSpawnedEntities_AreDestroyed)
{
    AZStd::vector<AzFramework::EntitySpawnTicket> tickets = SpawnManyDefaultPrefabs();

    m_PrefabSpawnerComponent->DestroyAllSpawnedPrefabs();

    bool allEntitiesDestroyed = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::EntitySpawnTicket& ticket : tickets)
            {
                for (AZ::EntityId spawnedEntityId : m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
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

TEST_F(PrefabSpawnerComponentTest, DestroyAllSpawnedPrefabs_OnSpawnedPrefabDestroyed_FiresForAll)
{
    AZStd::vector<AzFramework::EntitySpawnTicket> tickets = SpawnManyDefaultPrefabs();

    m_PrefabSpawnerComponent->DestroyAllSpawnedPrefabs();

    bool onSpawnedPrefabDestroyedFiresForAll = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::EntitySpawnTicket& ticket : tickets)
            {
                if (!m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed)
                {
                    return false;
                }
            }
            return true;
        });
    EXPECT_TRUE(onSpawnedPrefabDestroyedFiresForAll);
}

TEST_F(PrefabSpawnerComponentTest, DestroyAllSpawnedPrefabs_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AZStd::vector<AzFramework::EntitySpawnTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_PrefabSpawnerComponent->SpawnPrefab(m_prefabAssetRef));
    }

    m_PrefabSpawnerComponent->DestroyAllSpawnedPrefabs();

    // wait a long time, to ensure no queued activity results in an instantiation
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    bool anyOnSpawnBegan = false;
    bool allOnSpawnedPrefabDestroyed = true;
    for (const AzFramework::EntitySpawnTicket& ticket : tickets)
    {
        anyOnSpawnBegan |= m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnBegin;
        allOnSpawnedPrefabDestroyed &= m_PrefabSpawnWatcher->m_tickets[ticket].m_onSpawnedPrefabDestroyed;
    }

    EXPECT_FALSE(anyOnSpawnBegan);
    EXPECT_TRUE(allOnSpawnedPrefabDestroyed);
}

TEST_F(PrefabSpawnerComponentTest, GetCurrentEntitiesFromSpawnedPrefab_ReturnsEntities)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();
    AZStd::vector<AZ::EntityId> entities = m_PrefabSpawnerComponent->GetCurrentEntitiesFromSpawnedPrefab(ticket);

    EXPECT_EQ(m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size(), entities.size());
}

TEST_F(PrefabSpawnerComponentTest, GetCurrentEntitiesFromSpawnedPrefab_WithEntityDeleted_DoesNotReturnDeletedEntity)
{
    const AzFramework::EntitySpawnTicket ticket = SpawnDefaultPrefab();

    AZStd::vector<AZ::EntityId>& entitiesBeforeDelete = m_PrefabSpawnWatcher->m_tickets[ticket].m_onEntitiesSpawned;

    AZ::EntityId entityToDelete = entitiesBeforeDelete[0];
    delete m_application->FindEntity(entityToDelete);

    AZStd::vector<AZ::EntityId> entitiesAfterDelete = m_PrefabSpawnerComponent->GetCurrentEntitiesFromSpawnedPrefab(ticket);

    EXPECT_EQ(entitiesBeforeDelete.size() - 1, entitiesAfterDelete.size());

    bool deletedEntityPresent = AZStd::find(entitiesAfterDelete.begin(), entitiesAfterDelete.end(), entityToDelete) != entitiesAfterDelete.end();
    EXPECT_FALSE(deletedEntityPresent);
}

TEST_F(PrefabSpawnerComponentTest, GetAllCurrentlySpawnedEntities_ReturnsEntities)
{
    AZStd::vector<AzFramework::EntitySpawnTicket> tickets = SpawnManyDefaultPrefabs();

    AZStd::vector<AZ::EntityId> entities = m_PrefabSpawnerComponent->GetAllCurrentlySpawnedEntities();

    bool allEntitiesFound = true;
    size_t numEntities = 0;

    // compare against entities from OnEntitiesSpawned event
    for (auto& ticketInfoPair : m_PrefabSpawnWatcher->m_tickets)
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

// Legacy PrefabSpawnerComponent from game data
// Should get converted into modern PrefabSpawnerComponent
const char kWrappedGamePrefabSpawnerComponent[] =
R"DELIMITER("EditorPrefabSpawnerComponent": {
                    "$type": "EditorPrefabSpawnerComponent",
                    "Id": 1164794098161216105,
                    "Prefab": {
                        "assetId": {
                            "guid": "{753CF94D-1A6B-53B5-ADF7-BF8BB222230D}",
                            "subId": 1263229191
                        },
                        "loadBehavior": "QueueLoad",
                        "assetHint": "prefabs/ai_walker.spawnable",
                        "SpawnOnActivate": true,
                        "DestroyOnDeactivate": true
                    }
                }
)DELIMITER";

class LoadPrefabSpawnerComponentFromLegacyGameData
    : public LoadReflectedObjectTest<AzFramework::Application, LmbrCentral::LmbrCentralModule, LmbrCentral::PrefabSpawnerComponent>
{
public:
    const char* GetSourceDataBuffer() const { return kWrappedGamePrefabSpawnerComponent; }

    void SetUp()
    {
        LoadReflectedObjectTest::SetUp();

        if (m_object)
        {
            m_readConfigSuccess = m_object->GetConfiguration(m_spawnerConfig);
        }
    }

    LmbrCentral::PrefabSpawnerConfig m_spawnerConfig;
    bool m_readConfigSuccess = false;
};

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, Fixture_SanityCheck)
{
    EXPECT_NE(nullptr, GetApplication());
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, PrefabSpawnerComponent_LoadsFromData)
{
    EXPECT_NE(nullptr, m_object.get());
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, ComponentId_ValuePreserved)
{
    EXPECT_EQ(AZ::ComponentId(8317941343245109563ULL), m_object->GetId());
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, PrefabAsset_ValuePreserved)
{
    EXPECT_EQ(AZ::Uuid("{753CF94D-1A6B-53B5-ADF7-BF8BB222230D}"), m_spawnerConfig.m_prefabAsset.GetId().m_guid);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, SpawnOnActivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_spawnOnActivate);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyGameData, DestroyOnDeactivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_destroyOnDeactivate);
}

#ifdef LMBR_CENTRAL_EDITOR
// Legacy PrefabSpawnerComponent from editor data (wrapped inside a GenericComponentWrapper)
// Should get converted into EditorPrefabSpawnerComponent
const char kWrappedLegacyPrefabSpawnerComponent[] =
R"DELIMITER(EditorPrefabSpawnerComponent": {
                    "$type": "EditorPrefabSpawnerComponent",
                    "Id": 1164794098161216105,
                    "Prefab": {
                        "assetId": {
                            "guid": "{753CF94D-1A6B-53B5-ADF7-BF8BB222230D}",
                            "subId": 1263229191
                        },
                        "loadBehavior": "QueueLoad",
                        "assetHint": "prefabs/ai_walker.spawnable",
                        "SpawnOnActivate": true,
                        "DestroyOnDeactivate": true
                    }
})DELIMITER";

class LoadPrefabSpawnerComponentFromLegacyEditorData
    : public LoadReflectedObjectTest<AzToolsFramework::ToolsApplication, LmbrCentral::LmbrCentralEditorModule, AzToolsFramework::Components::GenericComponentWrapper>
{
public:
    const char* GetSourceDataBuffer() const override { return kWrappedLegacyPrefabSpawnerComponent; }

    void SetUp() override
    {
        LoadReflectedObjectTest::SetUp();

        // reset values from previous run
        m_editorPrefabSpawnerComponent = nullptr;
        m_readConfigSuccess = false;

        if (m_object)
        {
            m_editorPrefabSpawnerComponent = azrtti_cast<LmbrCentral::EditorPrefabSpawnerComponent*>(m_object->GetTemplate());
            if (m_editorPrefabSpawnerComponent)
            {
                m_readConfigSuccess = m_editorPrefabSpawnerComponent->GetConfiguration(m_spawnerConfig);
            }
        }
    }

    LmbrCentral::EditorPrefabSpawnerComponent* m_editorPrefabSpawnerComponent;
    LmbrCentral::PrefabSpawnerConfig m_spawnerConfig;
    bool m_readConfigSuccess;

};

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, Fixture_SanityCheck)
{
    EXPECT_NE(nullptr, GetApplication());
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, ObjectStream_LoadsComponents)
{
    EXPECT_NE(nullptr, m_object.get());
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, LegacyPrefabSpawnerComponent_TurnedIntoEditorPrefabSpawnerComponent)
{
    EXPECT_NE(nullptr, m_editorPrefabSpawnerComponent);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, SpawnerConfig_SuccessfullyRead)
{
    EXPECT_TRUE(m_readConfigSuccess);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, PrefabAsset_ValuePreserved)
{
    EXPECT_EQ(AZ::Uuid("{753CF94D-1A6B-53B5-ADF7-BF8BB222230D}"), m_spawnerConfig.m_prefabAsset.GetId().m_guid);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, SpawnOnActivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_spawnOnActivate);
}

TEST_F(LoadPrefabSpawnerComponentFromLegacyEditorData, DestroyOnDeactivate_ValuePreserved)
{
    EXPECT_TRUE(m_spawnerConfig.m_destroyOnDeactivate);
}

#endif // LMBR_CENTRAL_EDITOR

#endif
