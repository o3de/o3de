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

#include <Source/NetworkEntity/NetworkEntityManager.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/NetBindComponent.h>
#include <Pipeline/NetworkSpawnableHolderComponent.h>

namespace Multiplayer
{
    AZ_CVAR(bool, net_DebugCheckNetworkEntityManager, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables extra debug checks inside the NetworkEntityManager");
    AZ_CVAR(AZ::TimeMs, net_EntityDomainUpdateMs, AZ::TimeMs{ 500 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Frequency for updating the entity domain in ms");

    NetworkEntityManager::NetworkEntityManager()
        : m_networkEntityAuthorityTracker(*this)
        , m_removeEntitiesEvent([this] { RemoveEntities(); }, AZ::Name("NetworkEntityManager remove entities event"))
        , m_updateEntityDomainEvent([this] { UpdateEntityDomain(); }, AZ::Name("NetworkEntityManager update entity domain event"))
        , m_onSpawnedHandler([this](AZ::Data::Asset<AzFramework::Spawnable> spawnable) { this->OnSpawned(spawnable); })
        , m_onDespawnedHandler([this](AZ::Data::Asset<AzFramework::Spawnable> spawnable) { this->OnDespawned(spawnable); })
    {
        AzFramework::RootSpawnableNotificationBus::Handler::BusConnect();

        AzFramework::SpawnableEntitiesInterface::Get()->AddOnSpawnedHandler(m_onSpawnedHandler);
        AzFramework::SpawnableEntitiesInterface::Get()->AddOnDespawnedHandler(m_onDespawnedHandler);
    }

    NetworkEntityManager::~NetworkEntityManager()
    {
        AzFramework::RootSpawnableNotificationBus::Handler::BusDisconnect();
    }

    void NetworkEntityManager::Initialize(HostId hostId, AZStd::unique_ptr<IEntityDomain> entityDomain)
    {
        m_hostId = hostId;
        m_entityDomain = AZStd::move(entityDomain);
        m_updateEntityDomainEvent.Enqueue(net_EntityDomainUpdateMs, true);
    }

    NetworkEntityTracker* NetworkEntityManager::GetNetworkEntityTracker()
    {
        return &m_networkEntityTracker;
    }

    NetworkEntityAuthorityTracker* NetworkEntityManager::GetNetworkEntityAuthorityTracker()
    {
        return &m_networkEntityAuthorityTracker;
    }

    MultiplayerComponentRegistry* NetworkEntityManager::GetMultiplayerComponentRegistry()
    {
        return &m_multiplayerComponentRegistry;
    }

    HostId NetworkEntityManager::GetHostId() const
    {
        return m_hostId;
    }

    ConstNetworkEntityHandle NetworkEntityManager::GetEntity(NetEntityId netEntityId) const
    {
        return m_networkEntityTracker.Get(netEntityId);
    }

    uint32_t NetworkEntityManager::GetEntityCount() const
    {
        return m_networkEntityTracker.size();
    }

    NetworkEntityHandle NetworkEntityManager::AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity)
    {
        m_networkEntityTracker.Add(netEntityId, entity);
        return NetworkEntityHandle(entity, netEntityId, &m_networkEntityTracker);
    }

    void NetworkEntityManager::MarkForRemoval(const ConstNetworkEntityHandle& entityHandle)
    {
        if (entityHandle.Exists())
        {
            if (net_DebugCheckNetworkEntityManager)
            {
                AZ_Assert(entityHandle.GetNetBindComponent(), "No NetBindComponent found on networked entity");
                [[maybe_unused]] const bool isClientOnlyEntity = false;// (ServerIdFromEntityId(it->first) == InvalidHostId);
                AZ_Assert(entityHandle.GetNetBindComponent()->IsAuthority() || isClientOnlyEntity, "Trying to delete a proxy entity, this will lead to issues deserializing entity updates");
            }
            m_removeList.push_back(entityHandle.GetNetEntityId());
            m_removeEntitiesEvent.Enqueue(AZ::TimeMs{ 0 });
        }
    }

    bool NetworkEntityManager::IsMarkedForRemoval(const ConstNetworkEntityHandle& entityHandle) const
    {
        for (auto removeEntId : m_removeList)
        {
            if (entityHandle.GetNetEntityId() == removeEntId)
            {
                return true;
            }
        }
        return false;
    }

    void NetworkEntityManager::ClearEntityFromRemovalList(const ConstNetworkEntityHandle& entityHandle)
    {
        for (auto iter = m_removeList.begin(); iter != m_removeList.end(); ++iter)
        {
            if (*iter == entityHandle.GetNetEntityId())
            {
                iter = m_removeList.erase(iter);
                break;
            }
        }
    }

    void NetworkEntityManager::ClearAllEntities()
    {
        // Note is looping through a hash map not a vector.  Could cause performance issues even on shutdown.
        for (NetworkEntityTracker::iterator it = m_networkEntityTracker.begin(); it != m_networkEntityTracker.end(); ++it)
        {
            m_removeList.push_back(it->first);
        }
        RemoveEntities();

        // Keystone has refactored these API's, rewrite required
        //AZ::SliceComponent* rootSlice = nullptr;
        //{
        //    AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        //    AzFramework::GameEntityContextRequestBus::BroadcastResult(gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
        //    AzFramework::EntityContextRequestBus::BroadcastResult(rootSlice, &AzFramework::EntityContextRequests::GetRootSlice);
        //    AZ_Assert(rootSlice != nullptr, "Root slice returned was nullptr");
        //}
        //
        //for (AZ::Entity* entity : m_nonNetworkedEntities)
        //{
        //    rootSlice->RemoveEntity(entity);
        //}
        m_networkEntityTracker.clear();
    }

    void NetworkEntityManager::AddEntityMarkedDirtyHandler(AZ::Event<>::Handler& entityMarkedDirtyHandler)
    {
        entityMarkedDirtyHandler.Connect(m_onEntityMarkedDirty);
    }

    void NetworkEntityManager::AddEntityNotifyChangesHandler(AZ::Event<>::Handler& entityNotifyChangesHandler)
    {
        entityNotifyChangesHandler.Connect(m_onEntityNotifyChanges);
    }

    void NetworkEntityManager::AddEntityExitDomainHandler(EntityExitDomainEvent::Handler& entityExitDomainHandler)
    {
        entityExitDomainHandler.Connect(m_entityExitDomainEvent);
    }

    void NetworkEntityManager::AddControllersActivatedHandler(ControllersActivatedEvent::Handler& controllersActivatedHandler)
    {
        controllersActivatedHandler.Connect(m_controllersActivatedEvent);
    }

    void NetworkEntityManager::AddControllersDeactivatedHandler(ControllersDeactivatedEvent::Handler& controllersDeactivatedHandler)
    {
        controllersDeactivatedHandler.Connect(m_controllersDeactivatedEvent);
    }

    void NetworkEntityManager::NotifyEntitiesDirtied()
    {
        m_onEntityMarkedDirty.Signal();
    }

    void NetworkEntityManager::NotifyEntitiesChanged()
    {
        m_onEntityNotifyChanges.Signal();
    }

    void NetworkEntityManager::NotifyControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating)
    {
        m_controllersActivatedEvent.Signal(entityHandle, entityIsMigrating);
    }

    void NetworkEntityManager::NotifyControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating)
    {
        m_controllersDeactivatedEvent.Signal(entityHandle, entityIsMigrating);
    }

    void NetworkEntityManager::HandleLocalRpcMessage(NetworkEntityRpcMessage& message)
    {
        AZ_Assert(message.GetRpcDeliveryType() == RpcDeliveryType::ServerToAuthority, "Only ServerToAuthority rpc messages can be locally deferred");
        m_localDeferredRpcMessages.emplace_back(AZStd::move(message));
    }

    void NetworkEntityManager::DispatchLocalDeferredRpcMessages()
    {
        for (NetworkEntityRpcMessage& rpcMessage : m_localDeferredRpcMessages)
        {
            AZ::Entity* entity = m_networkEntityTracker.GetRaw(rpcMessage.GetEntityId());
            if (entity != nullptr)
            {
                NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
                AZ_Assert(netBindComponent != nullptr, "Attempting to send an RPC to an entity with no NetBindComponent");
                netBindComponent->HandleRpcMessage(nullptr, NetEntityRole::Server, rpcMessage);
            }
        }
        m_localDeferredRpcMessages.clear();
    }

    void NetworkEntityManager::UpdateEntityDomain()
    {
        if (m_entityDomain == nullptr)
        {
            return;
        }

        m_entitiesNotInDomain.clear();
        m_entityDomain->RetrieveEntitiesNotInDomain(m_entitiesNotInDomain);
        for (NetEntityId exitingId : m_entitiesNotInDomain)
        {
            OnEntityExitDomain(exitingId);
        }
    }

    void NetworkEntityManager::OnEntityExitDomain(NetEntityId entityId)
    {
        bool safeToExit = true;
        NetworkEntityHandle entityHandle = m_networkEntityTracker.Get(entityId);

        // ClientAutonomous entities need special handling here. When we migrate a player's entity the player's client must tell the new server which
        //  entity they were controlling. If we tell them to migrate before they know which entity they control it results in them requesting a new entity
        //  from the new server, resulting in an orphaned PlayerChar. PlayerControllerComponentServerAuthority::PlayerClientHasControlledEntity()
        //  will tell us whether the client sent an RPC acknowledging that they now know which entity is theirs.
        if (AZ::Entity* entity = entityHandle.GetEntity())
        {
            //if (PlayerComponent::Authority* playerController = FindController<PlayerComponent::Authority>(nonConstExitingEntityPtr))
            //{
            //    safeToExit = playerController->PlayerClientHasControlledEntity();
            //}
        }

        // We also need special handling for the EntityHierarchyComponent as well, since related entities need to be migrated together
        //auto* hierarchyController = FindController<EntityHierarchyComponent::Authority>(nonConstExitingEntityPtr);
        //if (hierarchyController)
        //{
        //    if (hierarchyController->GetParentRelatedEntity())
        //    {
        //        safeToExit = false;
        //    }
        //}

        // Validate that we aren't already planning to remove this entity
        if (safeToExit)
        {
            for (auto entityId : m_removeList)
            {
                if (entityId == entityId)
                {
                    safeToExit = false;
                }
            }
        }

        if (safeToExit)
        {
            m_entityExitDomainEvent.Signal(entityHandle);
        }
    }

    void NetworkEntityManager::RemoveEntities()
    {
        //RewindableObjectState::ClearRewoundEntities();

        // Keystone has refactored these API's, rewrite required
        //AZ::SliceComponent* rootSlice = nullptr;
        //{
        //    AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        //    AzFramework::GameEntityContextRequestBus::BroadcastResult(gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
        //    AzFramework::EntityContextRequestBus::BroadcastResult(rootSlice, &AzFramework::EntityContextRequests::GetRootSlice);
        //    AZ_Assert(rootSlice != nullptr, "Root slice returned was NULL");
        //}

        AZStd::vector<NetEntityId> removeList;
        removeList.swap(m_removeList);
        for (NetEntityId entityId : removeList)
        {
            NetworkEntityHandle removeEntity = m_networkEntityTracker.Get(entityId);

            if (removeEntity != nullptr)
            {
                // We need to notify out that our entity is about to deactivate so that other entities can read state before we clean up
                NetBindComponent* netBindComponent = removeEntity.GetNetBindComponent();
                AZ_Assert(netBindComponent != nullptr, "NetBindComponent not found on networked entity");
                netBindComponent->StopEntity();

                // Delete Entity, method depends on how it was loaded
                // Try slice removal first, then force delete
                //AZ::Entity* rawEntity = removeEntity.GetEntity();
                //if (!rootSlice->RemoveEntity(rawEntity))
                //{
                //    delete rawEntity;
                //}
            }

            m_networkEntityTracker.erase(entityId);
        }
    }
    
    INetworkEntityManager::EntityList NetworkEntityManager::CreateEntitiesImmediate(
        const AzFramework::Spawnable& spawnable, NetEntityRole netEntityRole)
    {
        INetworkEntityManager::EntityList returnList;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AzFramework::Spawnable::EntityList& entities = spawnable.GetEntities();
        size_t entitiesSize = entities.size();

        for (size_t i = 0; i < entitiesSize; ++i)
        {
            AZ::Entity* clone = serializeContext->CloneObject(entities[i].get());
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
            clone->SetId(AZ::Entity::MakeId());

            NetBindComponent* netBindComponent = clone->FindComponent<NetBindComponent>();
            if (netBindComponent != nullptr)
            {
                PrefabEntityId prefabEntityId;
                prefabEntityId.m_prefabName = m_networkPrefabLibrary.GetPrefabNameFromAssetId(spawnable.GetId());
                prefabEntityId.m_entityOffset = aznumeric_cast<uint32_t>(i);

                const NetEntityId netEntityId = NextId();
                netBindComponent->PreInit(clone, prefabEntityId, netEntityId, netEntityRole);

                AzFramework::GameEntityContextRequestBus::Broadcast(
                    &AzFramework::GameEntityContextRequestBus::Events::AddGameEntity, clone);

                returnList.push_back(netBindComponent->GetEntityHandle());

            }
            else
            {
                delete clone;
            }
        }

        return returnList;
    }

    INetworkEntityManager::EntityList NetworkEntityManager::CreateEntitiesImmediate
    (
        const PrefabEntityId& prefabEntryId,
        NetEntityRole netEntityRole,
        const AZ::Transform& transform
    )
    {
        return CreateEntitiesImmediate(prefabEntryId, NextId(), netEntityRole, AutoActivate::Activate, transform);
    }

    INetworkEntityManager::EntityList NetworkEntityManager::CreateEntitiesImmediate
    (
        const PrefabEntityId& prefabEntryId,
        NetEntityId netEntityId,
        NetEntityRole netEntityRole,
        AutoActivate autoActivate,
        const AZ::Transform& transform
    )
    {
        INetworkEntityManager::EntityList returnList;

        auto spawnableAssetId = m_networkPrefabLibrary.GetAssetIdByName(prefabEntryId.m_prefabName);
        // Required for sync-instantiation. Todo: keep the reference in NetworkSpawnableLibrary
        auto netSpawnableAsset = AZ::Data::AssetManager::Instance().GetAsset<AzFramework::Spawnable>(spawnableAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        AZ::Data::AssetManager::Instance().BlockUntilLoadComplete(netSpawnableAsset);
       
        AzFramework::Spawnable* netSpawnable = netSpawnableAsset.GetAs<AzFramework::Spawnable>();
        if (!netSpawnable)
        {
            return returnList;
        }

        const uint32_t entityIndex = prefabEntryId.m_entityOffset;

        if (entityIndex == PrefabEntityId::AllIndices)
        {
            return CreateEntitiesImmediate(*netSpawnable, netEntityRole);
        }

        const AzFramework::Spawnable::EntityList& entities = netSpawnable->GetEntities();
        size_t entitiesSize = entities.size();
        if (entityIndex >= entitiesSize)
        {
            return returnList;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Entity* clone = serializeContext->CloneObject(entities[entityIndex].get());
        AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");
        clone->SetId(AZ::Entity::MakeId());

        NetBindComponent* netBindComponent = clone->FindComponent<NetBindComponent>();
        if (netBindComponent)
        {
            netBindComponent->PreInit(clone, prefabEntryId, netEntityId, netEntityRole);

            auto* transformComponent = clone->FindComponent<AzFramework::TransformComponent>();
            if (transformComponent)
            {
                transformComponent->SetWorldTM(transform);
            }

            if (autoActivate == AutoActivate::DoNotActivate)
            {
                clone->SetRuntimeActiveByDefault(false);
            }

            AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::AddGameEntity, clone);

            returnList.push_back(netBindComponent->GetEntityHandle());
        }

        return returnList;
    }

    Multiplayer::NetEntityId NetworkEntityManager::NextId()
    {
        const NetEntityId netEntityId = m_nextEntityId++;
        return netEntityId;
    }

    void NetworkEntityManager::OnRootSpawnableAssigned(
        [[maybe_unused]] AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation)
    {
        auto* multiplayer = GetMultiplayer();
        const auto agentType = multiplayer->GetAgentType();

        if (agentType == MultiplayerAgentType::Client)
        {
            multiplayer->SendReadyForEntityUpdates(true);
        }
    }

    void NetworkEntityManager::OnRootSpawnableReleased([[maybe_unused]] uint32_t generation)
    {
        // TODO: Do we need to clear all entities here?
        auto* multiplayer = GetMultiplayer();
        const auto agentType = multiplayer->GetAgentType();

        if (agentType == MultiplayerAgentType::Client)
        {
            multiplayer->SendReadyForEntityUpdates(false);
        }
    }

    void NetworkEntityManager::OnSpawned(AZ::Data::Asset<AzFramework::Spawnable> spawnable)
    {
        AzFramework::Spawnable* spawnableData = spawnable.GetAs<AzFramework::Spawnable>();
        const auto& entityList = spawnableData->GetEntities();
        if (entityList.size() == 0)
        {
            AZ_Error("NetworkEntityManager", false, "OnSpawned: Spawnable %s doesn't have any entities.",
                spawnable.GetHint().c_str());
            return;
        }

        const auto& rootEntity = entityList[0];
        auto* spawnableHolder = rootEntity->FindComponent<NetworkSpawnableHolderComponent>();
        if (!spawnableHolder)
        {
            // Root entity doesn't have NetworkSpawnableHolderComponent. It means there's no corresponding network spawnable.
            return;
        }

        AZ::Data::Asset<AzFramework::Spawnable> netSpawnableAsset = spawnableHolder->GetNetworkSpawnableAsset();
        AzFramework::Spawnable* netSpawnable = netSpawnableAsset.GetAs<AzFramework::Spawnable>();
        if (!netSpawnable)
        {
            // TODO: Temp sync load until JsonSerialization of loadBehavior is fixed.
            netSpawnableAsset = AZ::Data::AssetManager::Instance().GetAsset<AzFramework::Spawnable>(
                netSpawnableAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
            AZ::Data::AssetManager::Instance().BlockUntilLoadComplete(netSpawnableAsset);

            netSpawnable = netSpawnableAsset.GetAs<AzFramework::Spawnable>();
        }

        if (!netSpawnable)
        {
            AZ_Error("NetworkEntityManager", false, "OnRootSpawnableAssigned: Net spawnable doesn't have any data.");
            return;
        }

        auto* multiplayer = GetMultiplayer();

        const auto agentType = multiplayer->GetAgentType();
        const bool spawnImmediately =
            (agentType == MultiplayerAgentType::ClientServer || agentType == MultiplayerAgentType::DedicatedServer);

        if (spawnImmediately)
        {
            CreateEntitiesImmediate(*netSpawnable, NetEntityRole::Authority);
        }
    }

    void NetworkEntityManager::OnDespawned([[maybe_unused]]AZ::Data::Asset<AzFramework::Spawnable> spawnable)
    {
        // TODO: Remove entities instantiated from the spawnable
    }
}
