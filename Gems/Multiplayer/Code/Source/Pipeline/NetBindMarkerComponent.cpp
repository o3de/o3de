/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Pipeline/NetBindMarkerComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>

namespace Multiplayer
{
    void NetBindMarkerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetBindMarkerComponent, AZ::Component>()
                ->Version(1)
                ->Field("NetEntityIndex", &NetBindMarkerComponent::m_netEntityIndex)
                ->Field("NetSpawnableAsset", &NetBindMarkerComponent::m_networkSpawnableAsset);
        }
    }

    AzFramework::Spawnable* GetSpawnableFromAsset(AZ::Data::Asset<AzFramework::Spawnable>& asset)
    {
        AzFramework::Spawnable* spawnable = asset.GetAs<AzFramework::Spawnable>();
        if (!spawnable)
        {
            asset =
                AZ::Data::AssetManager::Instance().GetAsset<AzFramework::Spawnable>(asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
            AZ::Data::AssetManager::Instance().BlockUntilLoadComplete(asset);

            spawnable = asset.GetAs<AzFramework::Spawnable>();
        }

        return spawnable;
    }


    void NetBindMarkerComponent::Activate()
    {
        const auto agentType = AZ::Interface<IMultiplayer>::Get()->GetAgentType();
        const bool spawnImmediately =
            (agentType == MultiplayerAgentType::ClientServer || agentType == MultiplayerAgentType::DedicatedServer);

        if (spawnImmediately && m_networkSpawnableAsset.GetId().IsValid())
        {
            AZ::Transform worldTm = GetEntity()->FindComponent<AzFramework::TransformComponent>()->GetWorldTM();
            auto preInsertionCallback =
                [worldTm = AZStd::move(worldTm), netEntityIndex = m_netEntityIndex, spawnableAssetId = m_networkSpawnableAsset.GetId()]
                (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableEntityContainerView entities)
            {
                if (entities.size() == 1)
                {
                    AZ::Entity* netEntity = *entities.begin();

                    auto* transformComponent = netEntity->FindComponent<AzFramework::TransformComponent>();
                    transformComponent->SetWorldTM(worldTm);

                    AZ::Name spawnableName = AZ::Interface<INetworkSpawnableLibrary>::Get()->GetSpawnableNameFromAssetId(spawnableAssetId);
                    PrefabEntityId prefabEntityId;
                    prefabEntityId.m_prefabName = spawnableName;
                    prefabEntityId.m_entityOffset = netEntityIndex;
                    AZ::Interface<INetworkEntityManager>::Get()->SetupNetEntity(netEntity, prefabEntityId, NetEntityRole::Authority);
                }
                else
                {
                    AZ_Error("NetBindMarkerComponent", false, "Requested to spawn 1 entity, but received %d", entities.size());
                }
            };

            m_netSpawnTicket = AzFramework::EntitySpawnTicket(m_networkSpawnableAsset);
            AzFramework::SpawnEntitiesOptionalArgs optionalArgs;
            optionalArgs.m_preInsertionCallback = AZStd::move(preInsertionCallback);
            AzFramework::SpawnableEntitiesInterface::Get()->SpawnEntities(
                m_netSpawnTicket, { m_netEntityIndex }, AZStd::move(optionalArgs));
        }
    }

    void NetBindMarkerComponent::Deactivate()
    {
        if(m_netSpawnTicket.IsValid())
        {
            AzFramework::SpawnableEntitiesInterface::Get()->DespawnAllEntities(m_netSpawnTicket);
        }
    }

    size_t NetBindMarkerComponent::GetNetEntityIndex() const
    {
        return m_netEntityIndex;
    }

    void NetBindMarkerComponent::SetNetEntityIndex(size_t netEntityIndex)
    {
        m_netEntityIndex = netEntityIndex;
    }

    void NetBindMarkerComponent::SetNetworkSpawnableAsset(AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset)
    {
        m_networkSpawnableAsset = networkSpawnableAsset;
    }

    AZ::Data::Asset<AzFramework::Spawnable> NetBindMarkerComponent::GetNetworkSpawnableAsset() const
    {
        return m_networkSpawnableAsset;
    }

}
