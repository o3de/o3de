/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma optimize( "", off )

#include <Multiplayer/IMultiplayerTools.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Pipeline/NetworkPrefabProcessor.h>

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Spawnable/SpawnableUtils.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>

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
                ->Version(5)
                ->Field("SerializationFormat", &NetworkPrefabProcessor::m_serializationFormat)
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
        AzFramework::Spawnable* networkSpawnable,
        uint32_t& offset)
    {
        using AzToolsFramework::Prefab::PrefabConversionUtils::ProcessedObjectStore;
        using namespace AzToolsFramework::Prefab;

        sourceInstance->GetEntities([networkInstance, rootSourceInstance, rootNetworkInstance, prefabName, uniqueName, &context, networkSpawnable, &offset](AZStd::unique_ptr<AZ::Entity>& sourceEntity)
        { 
            if (sourceEntity->FindComponent<NetBindComponent>())
            {
                AZ::Entity* entity = sourceEntity.get();
                AZ::Entity* netEntity = SpawnableUtils::CreateEntityAlias(
                    prefabName,
                    *rootSourceInstance,
                    uniqueName,
                    *rootNetworkInstance,
                    *networkInstance,
                    entity->GetId(),
                    AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType::Replace,
                    AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasSpawnableLoadBehavior::DependentLoad,
                    NetworkEntityManager::NetworkEntityTag,
                    context);

                AZ_Assert(
                    netEntity, "Unable to create alias for entity %s [%zu] from the source prefab instance", entity->GetName().c_str(),
                    aznumeric_cast<AZ::u64>(entity->GetId()));

                netEntity->InvalidateDependencies();
                netEntity->EvaluateDependencies();

                Multiplayer::PrefabEntityId prefabEntityId;
                prefabEntityId.m_prefabName = sourceEntity->GetName();
                prefabEntityId.m_entityOffset = offset;
                netEntity->FindComponent<NetBindComponent>()->SetPrefabAssetId(networkSpawnable->GetId());
                netEntity->FindComponent<NetBindComponent>()->SetPrefabEntityId(prefabEntityId);
                ++offset;
            }
            return true;
        });

        sourceInstance->GetNestedInstances([networkInstance, rootSourceInstance, rootNetworkInstance, prefabName, uniqueName, &context, networkSpawnable, &offset](AZStd::unique_ptr<AzToolsFramework::Prefab::Instance>& sourceNestedInstance)
        {
            // Make a new nested instance for the network prefab instance
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> networkNestedInstance = AZStd::make_unique<AzToolsFramework::Prefab::Instance>(AzToolsFramework::Prefab::InstanceOptionalReference(*rootNetworkInstance), sourceNestedInstance->GetInstanceAlias());
            AzToolsFramework::Prefab::Instance& targetNestedInstance = networkInstance->AddInstance(AZStd::move(networkNestedInstance));

            PopulateNetworkInstance(
                sourceNestedInstance.get(),
                &targetNestedInstance,
                rootSourceInstance,
                rootNetworkInstance,
                prefabName,
                uniqueName,
                context,
                networkSpawnable,
                offset);
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
        AZStd::string prefabName = prefab.GetName();
        uniqueName += NetworkSpawnableFileExtension;
        prefabName += NetworkFileExtension;

        PrefabConversionUtils::PrefabDocument networkPrefab(prefabName, prefab.GetInstance().GetInstanceAlias());

        auto serializer = [serializationFormat](AZStd::vector<uint8_t>& output, const ProcessedObjectStore& object) -> bool {
            AZ::IO::ByteContainerStream stream(&output);
            auto& asset = object.GetAsset();
            return AZ::Utils::SaveObjectToStream(stream, serializationFormat, &asset, asset.GetType());
        };

        auto&& [object, networkSpawnable] =
            ProcessedObjectStore::Create<AzFramework::Spawnable>(uniqueName, context.GetSourceUuid(), AZStd::move(serializer));

        Instance& sourceInstance = prefab.GetInstance();
        Instance& networkInstance = networkPrefab.GetInstance();
        // Grab all net entities with their corresponding Instances to handle nested prefabs correctly
        uint32_t offset = 0;
        PopulateNetworkInstance(
            &sourceInstance,
            &networkInstance,
            &sourceInstance,
            &networkInstance,
            prefab.GetName(),
            prefabName,
            context,
            networkSpawnable,
            offset);

        context.AddPrefab(AZStd::move(networkPrefab));
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

#pragma optimize( "", on )
