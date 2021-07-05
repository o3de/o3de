/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayerTools.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Pipeline/NetBindMarkerComponent.h>
#include <Pipeline/NetworkPrefabProcessor.h>
#include <Pipeline/NetworkSpawnableHolderComponent.h>

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>

namespace Multiplayer
{
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor;
    using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;

    void NetworkPrefabProcessor::Process(PrefabProcessorContext& context)
    {
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (mpTools)
        {
            mpTools->SetDidProcessNetworkPrefabs(false);
        }

        context.ListPrefabs([&context](AZStd::string_view prefabName, PrefabDom& prefab) {
            ProcessPrefab(context, prefabName, prefab);
        });

        if (mpTools && !context.GetProcessedObjects().empty())
        {
            mpTools->SetDidProcessNetworkPrefabs(true);
        }
    }

    void NetworkPrefabProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<NetworkPrefabProcessor, PrefabProcessor>()->Version(1);
        }
    }

    static AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> LoadInstanceFromPrefab(const PrefabDom& prefab)
    {
        using namespace AzToolsFramework::Prefab;

        // convert Prefab DOM into Prefab Instance.
        AZStd::unique_ptr<Instance> sourceInstance(aznew Instance());
        if (!PrefabDomUtils::LoadInstanceFromPrefabDom(*sourceInstance, prefab, PrefabDomUtils::LoadInstanceFlags::AssignRandomEntityId))
        {
            PrefabDomValueConstReference sourceReference = PrefabDomUtils::FindPrefabDomValue(prefab, PrefabDomUtils::SourceName);

            AZStd::string errorMessage("NetworkPrefabProcessor: Failed to Load Prefab Instance from given Prefab Dom.");
            if (sourceReference.has_value() && sourceReference->get().IsString() && sourceReference->get().GetStringLength() != 0)
            {
                AZStd::string_view source(sourceReference->get().GetString(), sourceReference->get().GetStringLength());
                errorMessage += AZStd::string::format("Prefab Source: %.*s", AZ_STRING_ARG(source));
            }
            AZ_Error("NetworkPrefabProcessor", false, errorMessage.c_str());
            return nullptr;
        }
        return sourceInstance;
    }

    static void GatherNetEntities(
        AzToolsFramework::Prefab::Instance* instance, 
        AZStd::vector<AZStd::pair<AZ::Entity*, AzToolsFramework::Prefab::Instance*>>& output)
    {
        instance->GetEntities([instance, &output](AZStd::unique_ptr<AZ::Entity>& prefabEntity) 
        { 
            if (prefabEntity->FindComponent<NetBindComponent>())
            {
                output.push_back(AZStd::make_pair(prefabEntity.get(), instance));
            }
            return true;
        });

        instance->GetNestedInstances([&output](AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>& nestedInstance) 
        { 
            GatherNetEntities(nestedInstance.get(), output);
        });
    }

    void NetworkPrefabProcessor::ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab)
    {
        using namespace AzToolsFramework::Prefab;

        // convert Prefab DOM into Prefab Instance.
        AZStd::unique_ptr<Instance> sourceInstance = LoadInstanceFromPrefab(prefab);
        if (!sourceInstance)
        {
            return;
        }

        AZStd::string uniqueName = prefabName;
        uniqueName += ".network.spawnable";

        auto serializer = [](AZStd::vector<uint8_t>& output, const ProcessedObjectStore& object) -> bool {
            AZ::IO::ByteContainerStream stream(&output);
            auto& asset = object.GetAsset();
            return AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_JSON, &asset, asset.GetType());
        };

        auto&& [object, networkSpawnable] =
            ProcessedObjectStore::Create<AzFramework::Spawnable>(uniqueName, context.GetSourceUuid(), AZStd::move(serializer));

        // Grab all net entities with their corresponding Instances to handle nested prefabs correctly
        AZStd::vector<AZStd::pair<AZ::Entity*, AzToolsFramework::Prefab::Instance*>> netEntities;
        GatherNetEntities(sourceInstance.get(), netEntities);

        if (netEntities.empty())
        {
            // No networked entities in the prefab, no need to do anything in this processor.
            return;
        }

        // Instance container for net entities
        AZStd::unique_ptr<Instance> networkInstance(aznew Instance());
        networkInstance->SetTemplateSourcePath(AZ::IO::PathView(uniqueName));

        // Create an asset for our future network spawnable: this allows us to put references to the asset in the components
        AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset;
        networkSpawnableAsset.Create(networkSpawnable->GetId());
        networkSpawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

        // Each spawnable has a root meta-data entity at position 0, so starting net indices from 1
        size_t netEntitiesIndexCounter = 1;

        for (auto& entityInstancePair : netEntities)
        {
            AZ::Entity* prefabEntity = entityInstancePair.first;
            Instance* instance = entityInstancePair.second;
            
            AZ::EntityId entityId = prefabEntity->GetId();
            AZ::Entity* netEntity = instance->DetachEntity(entityId).release();
            AZ_Assert(netEntity, "Unable to detach entity %s [%s] from the source prefab instance", 
                prefabEntity->GetName().c_str(), entityId.ToString().c_str());

            // Net entity will need a new ID to avoid IDs collision
            netEntity->SetId(AZ::Entity::MakeId());
            networkInstance->AddEntity(*netEntity);

            // Use the old ID for the breadcrumb entity to keep parent-child relationship in the original spawnable
            AZ::Entity* breadcrumbEntity = aznew AZ::Entity(entityId, netEntity->GetName());
            breadcrumbEntity->SetRuntimeActiveByDefault(netEntity->IsRuntimeActiveByDefault());

            // Marker component is responsible to spawning entities based on the index.
            NetBindMarkerComponent* netBindMarkerComponent = breadcrumbEntity->CreateComponent<NetBindMarkerComponent>();
            netBindMarkerComponent->SetNetEntityIndex(netEntitiesIndexCounter);
            netBindMarkerComponent->SetNetworkSpawnableAsset(networkSpawnableAsset);

            // Copy the transform component from the original entity to have the correct transform and parent-child relationship
            AzFramework::TransformComponent* transformComponent = netEntity->FindComponent<AzFramework::TransformComponent>();
            breadcrumbEntity->CreateComponent<AzFramework::TransformComponent>(*transformComponent);

            instance->AddEntity(*breadcrumbEntity);

            netEntitiesIndexCounter++;
        }

        // Add net spawnable asset holder to the prefab root
        {
            EntityOptionalReference containerEntityRef = sourceInstance->GetContainerEntity();
            if (containerEntityRef.has_value())
            {
                auto* networkSpawnableHolderComponent = containerEntityRef.value().get().CreateComponent<NetworkSpawnableHolderComponent>();
                networkSpawnableHolderComponent->SetNetworkSpawnableAsset(networkSpawnableAsset);
            }
            else
            {
                AZ::Entity* networkSpawnableHolderEntity = aznew AZ::Entity(uniqueName);
                auto* networkSpawnableHolderComponent = networkSpawnableHolderEntity->CreateComponent<NetworkSpawnableHolderComponent>();
                networkSpawnableHolderComponent->SetNetworkSpawnableAsset(networkSpawnableAsset);
                sourceInstance->AddEntity(*networkSpawnableHolderEntity);
            }
        }

        // save the final result in the target Prefab DOM.
        PrefabDom networkPrefab;
        if (!PrefabDomUtils::StoreInstanceInPrefabDom(*networkInstance, networkPrefab))
        {
            AZ_Error("NetworkPrefabProcessor", false, "Saving exported Prefab Instance within a Prefab Dom failed.");
            return;
        }

        if (!PrefabDomUtils::StoreInstanceInPrefabDom(*sourceInstance, prefab))
        {
            AZ_Error("NetworkPrefabProcessor", false, "Saving exported Prefab Instance within a Prefab Dom failed.");
            return;
        }

        bool result = SpawnableUtils::CreateSpawnable(*networkSpawnable, networkPrefab);
        if (result)
        {
            AzFramework::Spawnable::EntityList& entities = networkSpawnable->GetEntities();
            for (auto it = entities.begin(); it != entities.end(); ++it)
            {
                (*it)->InvalidateDependencies();
                (*it)->EvaluateDependencies();
            }

            SpawnableUtils::SortEntitiesByTransformHierarchy(*networkSpawnable);

            context.GetProcessedObjects().push_back(AZStd::move(object));
        }
        else
        {
            AZ_Error("Prefabs", false, "Failed to convert prefab '%.*s' to a spawnable.", AZ_STRING_ARG(prefabName));
            context.ErrorEncountered();
        }
    }
}
