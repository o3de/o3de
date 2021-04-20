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
#include <Source/Components/NetBindComponent.h>
#include <Include/IMultiplayer.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Components/TransformComponent.h>

namespace Multiplayer
{
    AZ_CVAR(bool, net_DebugCheckNetworkEntityManager, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables extra debug checks inside the NetworkEntityManager");
    AZ_CVAR(AZ::TimeMs, net_EntityDomainUpdateMs, AZ::TimeMs{ 500 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Frequency for updating the entity domain in ms");

    NetworkEntityManager::NetworkEntityManager()
        : m_networkEntityAuthorityTracker(*this)
        , m_removeEntitiesEvent([this] { RemoveEntities(); }, AZ::Name("NetworkEntityManager remove entities event"))
        , m_updateEntityDomainEvent([this] { UpdateEntityDomain(); }, AZ::Name("NetworkEntityManager update entity domain event"))
        , m_entityAddedEventHandler([this](AZ::Entity* entity) { OnEntityAdded(entity); })
        , m_entityRemovedEventHandler([this](AZ::Entity* entity) { OnEntityRemoved(entity); })
    {
        AZ::Interface<INetworkEntityManager>::Register(this);
        if (AZ::Interface<AZ::ComponentApplicationRequests>::Get() != nullptr)
        {
            // Null guard needed for unit tests
            AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityAddedEventHandler(m_entityAddedEventHandler);
            AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RegisterEntityRemovedEventHandler(m_entityRemovedEventHandler);
        }
    }

    NetworkEntityManager::~NetworkEntityManager()
    {
        AZ::Interface<INetworkEntityManager>::Unregister(this);
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
        m_nonNetworkedEntities.clear();
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
                netBindComponent->HandleRpcMessage(NetEntityRole::Server, rpcMessage);
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

    void NetworkEntityManager::OnEntityAdded(AZ::Entity* entity)
    {
        NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
        if (netBindComponent != nullptr)
        {
            // @pereslav
            // Note that this is a total hack.. we should not be listening to this event on a client
            // Entities should instead be spawned by the prefabEntityId inside EntityReplicationManager::HandlePropertyChangeMessage()
            const bool isClient = AZ::Interface<IMultiplayer>::Get()->GetAgentType() == MultiplayerAgentType::Client;
            const NetEntityRole netEntityRole = isClient ? NetEntityRole::Client: NetEntityRole::Authority;
            const NetEntityId netEntityId = m_nextEntityId++;
            netBindComponent->PreInit(entity, PrefabEntityId(), netEntityId, netEntityRole);
        }
    }

    void NetworkEntityManager::OnEntityRemoved(AZ::Entity* entity)
    {
        NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
        if (netBindComponent != nullptr)
        {
            MarkForRemoval(netBindComponent->GetEntityHandle());
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
}
