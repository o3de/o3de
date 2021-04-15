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

#include <Source/Pipeline/NetworkPrefabProcessor.h>

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>
#include <Source/Components/NetBindComponent.h>
#include <Source/Pipeline/NetBindMarkerComponent.h>
#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>

namespace Multiplayer
{
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor;
    using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;

    void NetworkPrefabProcessor::Process(PrefabProcessorContext& context)
    {
        context.ListPrefabs([&context](AZStd::string_view prefabName, PrefabDom& prefab) {
            ProcessPrefab(context, prefabName, prefab);
        });
    }

    void NetworkPrefabProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<NetworkPrefabProcessor, PrefabProcessor>()->Version(1);
        }
    }

    static AZStd::vector<AZ::Entity*> GetEntitiesFromInstance(AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>& instance)
    {
        AZStd::vector<AZ::Entity*> result;

        instance->GetNestedEntities([&result](const AZStd::unique_ptr<AZ::Entity>& entity) {
            result.emplace_back(entity.get());
            return true;
        });

        if (instance->HasContainerEntity())
        {
            auto containerEntityReference = instance->GetContainerEntity();
            result.emplace_back(&containerEntityReference->get());
        }

        return result;
    }
    void NetworkPrefabProcessor::ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab)
    {
        using namespace AzToolsFramework::Prefab;

        // convert Prefab DOM into Prefab Instance.
        AZStd::unique_ptr<Instance> sourceInstance(aznew Instance());
        if (!PrefabDomUtils::LoadInstanceFromPrefabDom(*sourceInstance, prefab,
            PrefabDomUtils::LoadInstanceFlags::AssignRandomEntityId))
        {
            PrefabDomValueReference sourceReference = PrefabDomUtils::FindPrefabDomValue(prefab, PrefabDomUtils::SourceName);

            AZStd::string errorMessage("NetworkPrefabProcessor: Failed to Load Prefab Instance from given Prefab Dom.");
            if (sourceReference.has_value() && sourceReference->get().IsString() && sourceReference->get().GetStringLength() != 0)
            {
                AZStd::string_view source(sourceReference->get().GetString(), sourceReference->get().GetStringLength());
                errorMessage += AZStd::string::format("Prefab Source: %.*s", AZ_STRING_ARG(source));
            }
            AZ_Error("NetworkPrefabProcessor", false, errorMessage.c_str());
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

        // grab all nested entities from the Instance as source entities.
        AZStd::vector<AZ::Entity*> sourceEntities = GetEntitiesFromInstance(sourceInstance);
        AZStd::vector<AZ::EntityId> networkedEntityIds;
        networkedEntityIds.reserve(sourceEntities.size());

        for (auto* sourceEntity : sourceEntities)
        {
            if (sourceEntity->FindComponent<NetBindComponent>())
            {
                networkedEntityIds.push_back(sourceEntity->GetId());
            }
        }
        if (!PrefabDomUtils::StoreInstanceInPrefabDom(*sourceInstance, prefab))
        {
            AZ_Error("NetworkPrefabProcessor", false, "Saving exported Prefab Instance within a Prefab Dom failed.");
            return;
        }

        AZStd::unique_ptr<Instance> networkInstance(aznew Instance());

        for (auto entityId : networkedEntityIds)
        {
            AZ::Entity* netEntity = sourceInstance->DetachEntity(entityId).release();

            networkInstance->AddEntity(*netEntity);

            AZ::Entity* breadcrumbEntity = aznew AZ::Entity(netEntity->GetName());
            breadcrumbEntity->SetRuntimeActiveByDefault(netEntity->IsRuntimeActiveByDefault());
            breadcrumbEntity->CreateComponent<NetBindMarkerComponent>();
            AzFramework::TransformComponent* transformComponent = netEntity->FindComponent<AzFramework::TransformComponent>();
            breadcrumbEntity->CreateComponent<AzFramework::TransformComponent>(*transformComponent);
            
            // TODO: Add NetBindMarkerComponent here referring to the net entity
            sourceInstance->AddEntity(*breadcrumbEntity);
        }

        // Add net spawnable asset holder
        {
            AZ::Data::AssetId assetId = networkSpawnable->GetId();
            AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset;
            networkSpawnableAsset.Create(assetId);

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
            context.GetProcessedObjects().push_back(AZStd::move(object));
        }
        else
        {
            AZ_Error("Prefabs", false, "Failed to convert prefab '%.*s' to a spawnable.", AZ_STRING_ARG(prefabName));
            context.ErrorEncountered();
        }
    }
}
