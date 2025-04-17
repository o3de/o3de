/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayerTools.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Pipeline/NetworkPrefabProcessor.h>

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>

namespace Multiplayer
{
    void NetworkPrefabProcessor::PostProcessSpawnable(const AZStd::string& prefabName, AzFramework::Spawnable& spawnable)
    {
        if (m_processedNetworkPrefabs.contains(prefabName))
        {
            AzFramework::Spawnable::EntityList& entityList = spawnable.GetEntities();
            uint32_t entityOffset = 0;
            for (AZStd::unique_ptr<AZ::Entity>& entity : entityList)
            {
                NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
                if (netBindComponent)
                {
                    PrefabEntityId prefabEntityId = netBindComponent->GetPrefabEntityId();
                    prefabEntityId.m_entityOffset = entityOffset;

                    netBindComponent->SetPrefabEntityId(prefabEntityId);
                }

                ++entityOffset;
            }
        }
    }

    NetworkPrefabProcessor::NetworkPrefabProcessor()
        : m_postProcessHandler([this](const AZStd::string& prefabName, AzFramework::Spawnable& spawnable){ this->PostProcessSpawnable(prefabName, spawnable); })
    {
    }

    void NetworkPrefabProcessor::Process(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context)
    {
        context.AddPrefabSpawnablePostProcessEventHandler(m_postProcessHandler);

        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument;

        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (mpTools)
        {
            mpTools->SetDidProcessNetworkPrefabs(false);
        }

        bool networkPrefabsAdded = false;
        context.ListPrefabs(
            [this, &networkPrefabsAdded, &context](PrefabDocument& prefab)
            {
                if (ProcessPrefab(context, prefab))
                {
                    networkPrefabsAdded = true;
                }
            });

        if (mpTools && networkPrefabsAdded)
        {
            mpTools->SetDidProcessNetworkPrefabs(true);
        }
    }

    void NetworkPrefabProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<NetworkPrefabProcessor, PrefabProcessor>()
                ->Version(6)
            ;
        }
    }

    static void PopulateNetworkInstance(
        AzToolsFramework::Prefab::Instance* sourceInstance,
        AzToolsFramework::Prefab::Instance* networkInstance,
        AzToolsFramework::Prefab::Instance* rootSourceInstance,
        AzToolsFramework::Prefab::Instance* rootNetworkInstance,
        const AZStd::string& prefabName,
        const AZStd::string& uniqueName,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context,
        const AZ::Data::AssetId& networkSpawnableAssetId,
        uint32_t& networkEntityCount)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;
        using namespace AzToolsFramework::Prefab;

        sourceInstance->GetEntities(
            [networkInstance,
             rootSourceInstance,
             rootNetworkInstance,
             prefabName,
             uniqueName,
             &context,
             networkSpawnableAssetId,
             &networkEntityCount](AZStd::unique_ptr<AZ::Entity>& sourceEntity)
        { 
            if (!sourceEntity->FindComponent<NetBindComponent>())
            {
                return true; // return true tells GetEntities to continue iterating over all the entities
            }

            ++networkEntityCount;
            const AZ::Entity* entity = sourceEntity.get();
            AZ::Entity* netEntity = SpawnableUtils::CreateEntityAlias(
                prefabName,
                *rootSourceInstance,
                uniqueName,
                *rootNetworkInstance,
                *networkInstance,
                entity->GetId(),
                PrefabConversionUtils::EntityAliasType::Replace,
                PrefabConversionUtils::EntityAliasSpawnableLoadBehavior::DependentLoad,
                NetworkEntityManager::NetworkEntityTag,
                context);

            AZ_Assert(
                netEntity, "Unable to create alias for network entity %s [%zu] from the source prefab instance %s", entity->GetName().c_str(),
                aznumeric_cast<AZ::u64>(entity->GetId()),
                prefabName.c_str());

            netEntity->InvalidateDependencies();
            netEntity->EvaluateDependencies();

            PrefabEntityId prefabEntityId;
            prefabEntityId.m_prefabName = sourceEntity->GetName();
            NetBindComponent* netBindComponent = netEntity->FindComponent<NetBindComponent>();
            netBindComponent->SetPrefabAssetId(networkSpawnableAssetId);
            netBindComponent->SetPrefabEntityId(prefabEntityId);
            
            return true;
        });

        sourceInstance->GetNestedInstances(
            [networkInstance,
             rootSourceInstance,
             rootNetworkInstance,
             prefabName,
             uniqueName,
             &context,
             networkSpawnableAssetId,
             &networkEntityCount](AZStd::unique_ptr<Instance>& sourceNestedInstance)
        {
            // Make a new nested instance for the network prefab instance
            AZStd::unique_ptr<Instance> networkNestedInstance =
                AZStd::make_unique<Instance>(
                    InstanceOptionalReference(*rootNetworkInstance),
                    sourceNestedInstance->GetInstanceAlias(),
                    EntityIdInstanceRelationship::OneToMany);
            Instance& targetNestedInstance = networkInstance->AddInstance(AZStd::move(networkNestedInstance));

            PopulateNetworkInstance(
                sourceNestedInstance.get(),
                &targetNestedInstance,
                rootSourceInstance,
                rootNetworkInstance,
                prefabName,
                uniqueName,
                context,
                networkSpawnableAssetId,
                networkEntityCount);
        });
    }

    bool NetworkPrefabProcessor::ProcessPrefab(
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument& prefab)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;
        using namespace AzToolsFramework::Prefab;

        AZStd::string uniqueName = prefab.GetName();
        AZStd::string prefabName = prefab.GetName();
        uniqueName += NetworkSpawnableFileExtension;
        prefabName += NetworkFileExtension;

        PrefabConversionUtils::PrefabDocument networkPrefab(prefabName, prefab.GetInstance().GetInstanceAlias());
        Instance& sourceInstance = prefab.GetInstance();
        Instance& networkInstance = networkPrefab.GetInstance();
        AZ::Data::AssetId networkSpawnableAssetId(context.GetSourceUuid(), AzFramework::SpawnableAssetHandler::BuildSubId(uniqueName));

        // Grab all net entities with their corresponding Instances to handle nested prefabs correctly
        uint32_t networkEntityCount = 0;
        PopulateNetworkInstance(
            &sourceInstance,
            &networkInstance,
            &sourceInstance,
            &networkInstance,
            prefab.GetName(),
            prefabName,
            context,
            networkSpawnableAssetId,
            networkEntityCount);

        if (networkEntityCount == 0)
        {
            return false;
        }

        context.AddPrefab(AZStd::move(networkPrefab));

        m_processedNetworkPrefabs.insert(prefabName);

        return true;
    }
}
