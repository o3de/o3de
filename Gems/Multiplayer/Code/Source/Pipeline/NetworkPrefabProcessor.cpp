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
#include <Pipeline/NetworkSpawnableHolderComponent.h>

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>

namespace Multiplayer
{
    void NetworkPrefabProcessor::Process(AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument;

        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (mpTools)
        {
            mpTools->SetDidProcessNetworkPrefabs(false);
        }

        AZ::DataStream::StreamType serializationFormat = GetAzSerializationFormat();

        bool networkPrefabsAdded = false;
        context.ListPrefabs(
            [&networkPrefabsAdded, &context, serializationFormat](PrefabDocument& prefab)
            {
                if (ProcessPrefab(context, prefab, serializationFormat))
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
            serializeContext->Enum<SerializationFormats>()
                ->Value("Binary", SerializationFormats::Binary)
                ->Value("Text", SerializationFormats::Text)
            ;

            serializeContext->Class<NetworkPrefabProcessor, PrefabProcessor>()
                ->Version(4)
                ->Field("SerializationFormat", &NetworkPrefabProcessor::m_serializationFormat)
            ;
        }
    }

    static void GatherNetEntities(
        AzToolsFramework::Prefab::Instance* instance,
        AZStd::unordered_map<AZ::Entity*, AzToolsFramework::Prefab::Instance*>& entityToInstanceMap,
        AZStd::vector<AZ::Entity*>& netEntities)
    {
        instance->GetEntities([instance, &entityToInstanceMap, &netEntities](AZStd::unique_ptr<AZ::Entity>& prefabEntity)
        { 
            if (prefabEntity->FindComponent<NetBindComponent>())
            {
                AZ::Entity* entity = prefabEntity.get();
                entityToInstanceMap[entity] = instance;
                netEntities.push_back(entity);
            }
            return true;
        });

        instance->GetNestedInstances([&entityToInstanceMap, &netEntities](AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>& nestedInstance)
        { 
            GatherNetEntities(nestedInstance.get(), entityToInstanceMap, netEntities);
        });
    }

    bool NetworkPrefabProcessor::ProcessPrefab(
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument& prefab,
        AZ::DataStream::StreamType serializationFormat)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;
        using namespace AzToolsFramework::Prefab;

        AZStd::string uniqueName = prefab.GetName();
        uniqueName += NetworkSpawnableFileExtension;

        auto serializer = [serializationFormat](AZStd::vector<uint8_t>& output, const ProcessedObjectStore& object) -> bool {
            AZ::IO::ByteContainerStream stream(&output);
            auto& asset = object.GetAsset();
            return AZ::Utils::SaveObjectToStream(stream, serializationFormat, &asset, asset.GetType());
        };

        auto&& [object, networkSpawnable] =
            ProcessedObjectStore::Create<AzFramework::Spawnable>(uniqueName, context.GetSourceUuid(), AZStd::move(serializer));
        auto& netSpawnableEntities = networkSpawnable->GetEntities();

        Instance& sourceInstance = prefab.GetInstance();
        // Grab all net entities with their corresponding Instances to handle nested prefabs correctly
        AZStd::unordered_map<AZ::Entity*, AzToolsFramework::Prefab::Instance*> netEntityToInstanceMap;
        AZStd::vector<AZ::Entity*> prefabNetEntities;
        GatherNetEntities(&sourceInstance, netEntityToInstanceMap, prefabNetEntities);

        if (prefabNetEntities.empty())
        {
            // No networked entities in the prefab, no need to do anything in this processor.
            return false;
        }

        // Sort the entities prior to processing. The entities will end up in the net spawnable in this order.
        SpawnableUtils::SortEntitiesByTransformHierarchy(prefabNetEntities);

        // Create an asset for our future network spawnable: this allows us to put references to the asset in the components
        AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset;
        networkSpawnableAsset.Create(networkSpawnable->GetId());
        networkSpawnableAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

        AZStd::unordered_set<AZ::EntityId> prefabNetEntityIds;

        for (auto* prefabEntity : prefabNetEntities)
        {
            Instance* instance = netEntityToInstanceMap[prefabEntity];
            
            AZ::EntityId entityId = prefabEntity->GetId();
            AZ::Entity* netEntity = instance->DetachEntity(entityId).release();
            AZ_Assert(netEntity, "Unable to detach entity %s [%s] from the source prefab instance", 
                prefabEntity->GetName().c_str(), entityId.ToString().c_str());

            netEntity->InvalidateDependencies();
            netEntity->EvaluateDependencies();

            auto* transformComponent = netEntity->FindComponent<AzFramework::TransformComponent>();
            if (transformComponent)
            {
                AZ::EntityId parentId = transformComponent->GetParentId();
                if (parentId.IsValid() && !prefabNetEntityIds.contains(parentId))
                {
                    // Clear parent ID for net entities parented to a non-net entity.
                    // To be addressed by the spawnable aliases system where non-net entities
                    // will be spawned together with the networked ones in which case we'll keep
                    // the cross-spawnable references.
                    transformComponent->SetParent(AZ::EntityId());
                }
            }

            prefabNetEntityIds.insert(netEntity->GetId());

            // Insert the entity into the target net spawnable
            netSpawnableEntities.emplace_back(netEntity);
        }

        // Add net spawnable asset holder to the prefab root
        {
            EntityOptionalReference containerEntityRef = sourceInstance.GetContainerEntity();
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
                sourceInstance.AddEntity(*networkSpawnableHolderEntity);
            }
        }

        context.GetProcessedObjects().push_back(AZStd::move(object));
        return true;
    }

    AZ::DataStream::StreamType NetworkPrefabProcessor::GetAzSerializationFormat() const
    {
        if (m_serializationFormat == SerializationFormats::Text)
        {
            return AZ::DataStream::StreamType::ST_JSON;
        }

        return AZ::DataStream::StreamType::ST_BINARY;
    }
}
