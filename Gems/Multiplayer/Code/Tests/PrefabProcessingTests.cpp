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
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabDomTypes.h>
#include <Prefab/Spawnable/PrefabProcessorContext.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Source/Pipeline/NetworkPrefabProcessor.h>

namespace UnitTest
{
    class PrefabProcessingTestFixture : public ::testing::Test
    {
    public:
        static void ConvertEntitiesToPrefab(const AZStd::vector<AZ::Entity*>& entities, AzToolsFramework::Prefab::PrefabDom& prefabDom)
        {
            auto* prefabSystem = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(prefabSystem->CreatePrefab(entities, {}, "test/path"));
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
    };

    TEST_F(PrefabProcessingTestFixture, NetworkPrefabProcessor_ProcessPrefabTwoEntities_NetEntityGoesToNetSpawnable)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;

        AZStd::vector<AZ::Entity*> entities;

        // Create test entities: 1 networked and 1 static
        const AZStd::string staticEntityName = "static_floor";
        entities.emplace_back(CreateSourceEntity(staticEntityName.c_str(), false, AZ::Transform::CreateIdentity()));

        const AZStd::string netEntityName = "networked_entity";
        entities.emplace_back(CreateSourceEntity(netEntityName.c_str(), true, AZ::Transform::CreateIdentity()));

        // Convert the entities into prefab. Note: This will transfer the ownership of AZ::Entity* into Prefab
        AzToolsFramework::Prefab::PrefabDom prefabDom;
        ConvertEntitiesToPrefab(entities, prefabDom);

        // Add the prefab into the Prefab Processor Context
        const AZStd::string prefabName = "testPrefab";
        PrefabProcessorContext prefabProcessorContext{AZ::Uuid::CreateRandom()};
        prefabProcessorContext.AddPrefab(prefabName, AZStd::move(prefabDom));

        // Request NetworkPrefabProcessor to process the prefab
        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);

        // Validate results
        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        // Should be 1 networked spawnable
        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();
        EXPECT_EQ(processedObjects.size(), 1);

        // Verify the name and the type of the spawnable asset 
        const AZ::Data::AssetData& spawnableAsset = processedObjects[0].GetAsset();
        EXPECT_EQ(prefabName + ".network.spawnable", processedObjects[0].GetId());
        EXPECT_EQ(spawnableAsset.GetType(), azrtti_typeid<AzFramework::Spawnable>());

        // Verify we have only the networked entity in the network spawnable and not the static one
        const AzFramework::Spawnable* netSpawnable = azrtti_cast<const AzFramework::Spawnable*>(&spawnableAsset);
        const AzFramework::Spawnable::EntityList& entityList = netSpawnable->GetEntities();
        auto countEntityCallback = [](const auto& name)
        {
            return [name](const auto& entity)
            {
                return entity->GetName() == name;
            };
        };

        EXPECT_EQ(0, AZStd::count_if(entityList.begin(), entityList.end(), countEntityCallback(staticEntityName)));
        EXPECT_EQ(1, AZStd::count_if(entityList.begin(), entityList.end(), countEntityCallback(netEntityName)));
    }

} // namespace UnitTest
