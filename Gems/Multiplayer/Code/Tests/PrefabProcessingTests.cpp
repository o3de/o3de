/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabCatchmentProcessor.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabDomTypes.h>
#include <Prefab/Spawnable/PrefabProcessorContext.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>
#include <Source/Pipeline/NetworkPrefabProcessor.h>

namespace UnitTest
{
    class TestPrefabProcessorContext
        : public AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext
    {
    public:
        AZ_CLASS_ALLOCATOR(TestPrefabProcessorContext, AZ::SystemAllocator);
        AZ_RTTI(TestPrefabProcessorContext, "{2FFFAA06-BA78-4CB3-AE0E-6532822A9B69}",
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext);

        explicit TestPrefabProcessorContext(const AZ::Uuid& sourceUuid)
            : PrefabProcessorContext(sourceUuid)
        {
        }

        const AZStd::vector<AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasStore>& GetEntityAliases() const
        {
            return m_entityAliases;
        }
    };

    class PrefabProcessingTestFixture : public ::testing::Test
    {

    public:
        void SetUp() override
        {
            using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

            // Create test entities: 1 networked and 1 static
            AZStd::vector<AZ::Entity*> entities;
            entities.emplace_back(CreateSourceEntity(m_staticEntityName.c_str(), false, AZ::Transform::CreateIdentity()));
            entities.emplace_back(CreateSourceEntity(m_netEntityName.c_str(), true, AZ::Transform::CreateIdentity()));

            // Convert the entities into prefab. Note: This will transfer the ownership of AZ::Entity* into Prefab
            ConvertEntitiesToPrefab(entities, m_networkPrefabDom, "test/path");

            // Create a non-networked prefab
            AZStd::vector<AZ::Entity*> nonNetworkEntities;
            nonNetworkEntities.emplace_back(CreateSourceEntity("NonNetEntity_1", false, AZ::Transform::CreateIdentity()));
            nonNetworkEntities.emplace_back(CreateSourceEntity("NonNetEntity_2", false, AZ::Transform::CreateIdentity()));
            ConvertEntitiesToPrefab(nonNetworkEntities, m_nonNetworkPrefabDom, "test/path_non_networked");
        }

        void TearDown() override
        {
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();
        }

        static void ConvertEntitiesToPrefab(const AZStd::vector<AZ::Entity*>& entities, AzToolsFramework::Prefab::PrefabDom& prefabDom, AZ::IO::PathView filePath)
        {
            auto* prefabSystem = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(prefabSystem->CreatePrefab(entities, {}, filePath));
            ASSERT_TRUE(sourceInstance);

            auto& prefabTemplateDom = prefabSystem->FindTemplateDom(sourceInstance->GetTemplateId());
            prefabDom.CopyFrom(prefabTemplateDom, prefabDom.GetAllocator());
        }

        static AZ::Entity* CreateSourceEntity(const char* name, bool networked, const AZ::Transform& tm, AZ::Entity* parent = nullptr)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            auto* transformComponent = entity->CreateComponent<AzFramework::TransformComponent>();

            if (parent)
            {
                transformComponent->SetParent(parent->GetId());
                transformComponent->SetLocalTM(tm);
            }
            else
            {
                transformComponent->SetWorldTM(tm);
            }

            if(networked)
            {
                entity->CreateComponent<Multiplayer::NetBindComponent>();
            }

            return entity;
        }

        static bool IsChildAfterParent(const AZStd::string& childName, const AZStd::string& parentName, const AzFramework::Spawnable::EntityList& entityList)
        {
            int parentIndex = -1;
            int childIndex = -1;
            for (int i = 0; i < entityList.size(); ++i)
            {
                if ((entityList[i]->GetName() == parentName) && (parentIndex == -1))
                {
                    parentIndex = i;
                }
                if ((entityList[i]->GetName() == childName) && (childIndex == -1))
                {
                    childIndex = i;
                }
            }

            EXPECT_NE(childIndex, -1);
            EXPECT_NE(parentIndex, -1);
            return childIndex > parentIndex;
        }

        static bool ContainsEntity(const AzFramework::Spawnable::EntityList& entityList, const AZStd::string& name)
        {
            for (const auto& entity : entityList)
            {
                if (entity->GetName() == name)
                {
                    return true;
                }
            }

            return false;
        }

        const AZStd::string m_staticEntityName = "static_floor";
        const AZStd::string m_netEntityName = "networked_entity";

        AzToolsFramework::Prefab::PrefabDom m_networkPrefabDom;
        AzToolsFramework::Prefab::PrefabDom m_nonNetworkPrefabDom;
    };

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabTwoEntities_NetEntityGoesToNetSpawnable)
    {
        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        TestPrefabProcessorContext prefabProcessorContext{AZ::Uuid::CreateRandom()};
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument document(prefabName);
        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(m_networkPrefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

        // Request NetworkPrefabProcessor and PrefabCatchmentProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor prefabCatchmentProcessor;
        prefabCatchmentProcessor.Process(prefabProcessorContext);

        // Validate results
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Should be 1 spawnable and 1 networked spawnable
        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();
        EXPECT_EQ(processedObjects.size(), 2);

        // Verify the name and the type of the spawnable asset 
        const AZ::Data::AssetData& spawnableAsset = processedObjects[1].GetAsset();
        EXPECT_EQ(prefabName + Multiplayer::NetworkSpawnableFileExtension.data(), processedObjects[1].GetId());
        EXPECT_EQ(spawnableAsset.GetType(), azrtti_typeid<AzFramework::Spawnable>());

        // Verify we have only the networked entity in the network spawnable and not the static one
        const AzFramework::Spawnable* netSpawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
        const AzFramework::Spawnable::EntityList& networkSpawnableEntityList = netSpawnable->GetEntities();

        EXPECT_FALSE(ContainsEntity(networkSpawnableEntityList, m_staticEntityName));
        EXPECT_TRUE(ContainsEntity(networkSpawnableEntityList, m_netEntityName));
    }

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabTwoEntities_NonNetEntityDoesNotProduceNetSpawnable)
    {
        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        TestPrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument document(prefabName);

        ASSERT_TRUE(document.SetPrefabDom(AZStd::move(m_nonNetworkPrefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(document));

        // Request NetworkPrefabProcessor and PrefabCatchmentProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor prefabCatchmentProcessor;
        prefabCatchmentProcessor.Process(prefabProcessorContext);

        // Validate results
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Should be 1 spawnable and no networked spawnable
        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();
        EXPECT_EQ(processedObjects.size(), 1);

        // Verify the name and the type of the spawnable asset
        const AZ::Data::AssetData& spawnableAsset = processedObjects[0].GetAsset();
        EXPECT_EQ(prefabName + AzFramework::Spawnable::DotFileExtension, processedObjects[0].GetId());
        EXPECT_EQ(spawnableAsset.GetType(), azrtti_typeid<AzFramework::Spawnable>());
    }

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabTwoEntities_AliasesInsertedIntoContext)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasStore;
        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument;

        // Add the prefab into the Prefab Processor Context
        TestPrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        PrefabDocument prefabDocument("testPrefab");
        prefabDocument.SetPrefabDom(AZStd::move(m_networkPrefabDom));

        prefabProcessorContext.AddPrefab(AZStd::move(prefabDocument));

        // Request NetworkPrefabProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);

        const AZStd::vector<EntityAliasStore>& aliases = prefabProcessorContext.GetEntityAliases();

        // Only 1 networked entity, so should be 1 alias inserted.
        EXPECT_EQ(aliases.size(), 1);

        // Verify alias metadata
        const EntityAliasStore& alias = aliases[0];
        EXPECT_EQ(alias.m_aliasType, AzFramework::Spawnable::EntityAliasType::Replace);
        EXPECT_EQ(alias.m_tag, Multiplayer::NetworkEntityManager::NetworkEntityTag);
    }

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabEntityHierarchy_EntitiesSorted)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

        const AZStd::string parentName = "static_parent";
        const AZStd::string childName = "networked_child";
        const AZStd::string childOfChildName = "networked_childOfChild";

        // Create test entities with the following hierarchy:
        // static parent
        // + networked child
        //   + networked child
        AZStd::vector<AZ::Entity*> entities;
        AZ::Entity* parent = entities.emplace_back(CreateSourceEntity(parentName.c_str(), false, AZ::Transform::CreateIdentity()));
        AZ::Entity* child = entities.emplace_back(CreateSourceEntity(childName.c_str(), true, AZ::Transform::CreateIdentity(), parent));
        entities.emplace_back(CreateSourceEntity(childOfChildName.c_str(), true, AZ::Transform::CreateIdentity(), child));

        // Convert the entities into prefab. Note: This will transfer the ownership of AZ::Entity* into Prefab
        AzToolsFramework::Prefab::PrefabDom prefabDom;
        ConvertEntitiesToPrefab(entities, prefabDom, "test_entities_sorted/path");

        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        TestPrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument prefabDocument(prefabName);
        ASSERT_TRUE(prefabDocument.SetPrefabDom(AZStd::move(prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(prefabDocument));

        // Request NetworkPrefabProcessor and PrefabCatchmentProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor prefabCatchmentProcessor;
        prefabCatchmentProcessor.Process(prefabProcessorContext);
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Verify entities are ordered by parent/child hierarchy
        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();

        // Static spawnable
        {
            const AZ::Data::AssetData& spawnableAsset = processedObjects[0].GetAsset();
            const AzFramework::Spawnable* spawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
            const AzFramework::Spawnable::EntityList& entityList = spawnable->GetEntities();

            EXPECT_TRUE(IsChildAfterParent(childName, parentName, entityList));
            EXPECT_TRUE(IsChildAfterParent(childOfChildName, childName, entityList));
        }

        // Network spawnable
        {
            const AZ::Data::AssetData& spawnableAsset = processedObjects[1].GetAsset();
            const AzFramework::Spawnable* spawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
            const AzFramework::Spawnable::EntityList& entityList = spawnable->GetEntities();

            EXPECT_TRUE(IsChildAfterParent(childOfChildName, childName, entityList));
            EXPECT_FALSE(ContainsEntity(entityList, parentName));
        }
    }

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabEntityHierarchy_GrandchildNetworkEntityProducesNetworkSpawnable)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

        const AZStd::string parentName = "static_parent";
        const AZStd::string childName = "static_child";
        const AZStd::string childOfChildName = "networked_childOfChild";

        // Create test entities with the following hierarchy:
        // static parent
        // + static child
        //   + networked child
        AZStd::vector<AZ::Entity*> entities;
        AZ::Entity* parent = entities.emplace_back(CreateSourceEntity(parentName.c_str(), false, AZ::Transform::CreateIdentity()));
        AZ::Entity* child = entities.emplace_back(CreateSourceEntity(childName.c_str(), false, AZ::Transform::CreateIdentity(), parent));
        entities.emplace_back(CreateSourceEntity(childOfChildName.c_str(), true, AZ::Transform::CreateIdentity(), child));

        // Convert the entities into prefab. Note: This will transfer the ownership of AZ::Entity* into Prefab
        AzToolsFramework::Prefab::PrefabDom prefabDom;
        ConvertEntitiesToPrefab(entities, prefabDom, "test_entities_sorted/path");

        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        TestPrefabProcessorContext prefabProcessorContext{ AZ::Uuid::CreateRandom() };
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument prefabDocument(prefabName);
        ASSERT_TRUE(prefabDocument.SetPrefabDom(AZStd::move(prefabDom)));
        prefabProcessorContext.AddPrefab(AZStd::move(prefabDocument));

        // Request NetworkPrefabProcessor and PrefabCatchmentProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor prefabCatchmentProcessor;
        prefabCatchmentProcessor.Process(prefabProcessorContext);
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Verify entities are ordered by parent/child hierarchy
        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();

        // Verify that the nested network entity was discovered by the network prefab processor (NetworkPrefabProcessor::ProcessPrefab)
        // and 2 processed objects (spawnable and network.spawnable) were produced.
        EXPECT_EQ(processedObjects.size(), 2);

        // Static spawnable
        {
            const AZ::Data::AssetData& spawnableAsset = processedObjects[0].GetAsset();
            const AzFramework::Spawnable* spawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
            const AzFramework::Spawnable::EntityList& entityList = spawnable->GetEntities();

            EXPECT_TRUE(IsChildAfterParent(childName, parentName, entityList));
            EXPECT_TRUE(IsChildAfterParent(childOfChildName, childName, entityList));
        }

        // Network spawnable
        {
            const AZ::Data::AssetData& spawnableAsset = processedObjects[1].GetAsset();
            const AzFramework::Spawnable* spawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
            const AzFramework::Spawnable::EntityList& entityList = spawnable->GetEntities();

            EXPECT_FALSE(ContainsEntity(entityList, parentName));
            EXPECT_FALSE(ContainsEntity(entityList, childName));
            EXPECT_TRUE(ContainsEntity(entityList, childOfChildName));
        }
    }
} // namespace UnitTest
