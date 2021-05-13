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

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabDomTypes.h>
#include <Prefab/Spawnable/PrefabProcessorContext.h>
#include <Source/Components/NetBindComponent.h>
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

        const AZStd::string staticEntityName = "static_floor";
        entities.emplace_back(CreateSourceEntity(staticEntityName.c_str(), false, AZ::Transform::CreateIdentity()));

        const AZStd::string netEntityName = "networked_entity";
        entities.emplace_back(CreateSourceEntity(netEntityName.c_str(), true, AZ::Transform::CreateIdentity()));

        AzToolsFramework::Prefab::PrefabDom prefabDom;
        ConvertEntitiesToPrefab(entities, prefabDom);

        const AZStd::string prefabName = "testPrefab";
        PrefabProcessorContext prefabProcessorContext{AZ::Uuid::CreateRandom()};
        prefabProcessorContext.AddPrefab(prefabName, AZStd::move(prefabDom));

        Multiplayer::NetworkPrefabProcessor processor;
        processor.Process(prefabProcessorContext);

        EXPECT_TRUE(prefabProcessorContext.HasCompletedSuccessfully());

        const auto& processedObjects = prefabProcessorContext.GetProcessedObjects();
        EXPECT_EQ(processedObjects.size(), 1);

        const AZ::Data::AssetData& spawnableAsset = processedObjects[0].GetAsset();
        EXPECT_EQ(prefabName + ".network.spawnable", processedObjects[0].GetId());
        EXPECT_EQ(spawnableAsset.GetType(), azrtti_typeid<AzFramework::Spawnable>());

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
