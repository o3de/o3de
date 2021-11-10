/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Pipeline/NetworkSpawnableHolderComponent.h>

namespace Multiplayer
{
    AZ_CVAR(bool, net_DebugCheckNetworkEntityManager, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables extra debug checks inside the NetworkEntityManager");
    AZ_CVAR(AZ::TimeMs, net_EntityDomainUpdateMs, AZ::TimeMs{ 500 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Frequency for updating the entity domain in ms");

    NetworkEntityManager::NetworkEntityManager()
        : m_networkEntityAuthorityTracker(*this)
        , m_removeEntitiesEvent([this] { RemoveEntities(); }, AZ::Name("NetworkEntityManager remove entities event"))
        , m_updateEntityDomainEvent([this] { UpdateEntityDomain(); }, AZ::Name("NetworkEntityManager update entity domain event"))
    {
        AZ::Interface<INetworkEntityManager>::Register(this);
        AzFramework::RootSpawnableNotificationBus::Handler::BusConnect();
    }

    NetworkEntityManager::~NetworkEntityManager()
    {
        AzFramework::RootSpawnableNotificationBus::Handler::BusDisconnect();
        AZ::Interface<INetworkEntityManager>::Unregister(this);
    }

    void NetworkEntityManager::Initialize(const HostId& hostId, AZStd::unique_ptr<IEntityDomain> entityDomain)
    {
        m_hostId = hostId;

        // Configure our vended NetEntityIds so that no two hosts generate the same NetEntityId
        {
            // Needs more thought
            const uint64_t addrPortion = hostId.GetAddress(AzNetworking::ByteOrder::Host);
            const uint64_t portPortion = hostId.GetPort(AzNetworking::ByteOrder::Host);
            const uint64_t hostIdentifier = (portPortion << 32) | addrPortion;
            const AZ::HashValue32 hostHash = AZ::TypeHash32(hostIdentifier);

            NetEntityId hostEntityIdOffset = static_cast<NetEntityId>(hostHash) << 32;
            m_nextEntityId &= NetEntityId{ 0x0000000000000000FFFFFFFFFFFFFFFF };
            m_nextEntityId |= hostEntityIdOffset;
        }

        m_entityDomain = AZStd::move(entityDomain);
        m_updateEntityDomainEvent.Enqueue(net_EntityDomainUpdateMs, true);
        m_entityDomain->ActivateTracking(m_ownedEntities);
    }

    bool NetworkEntityManager::IsInitialized() const
    {
        return m_entityDomain != nullptr;
    }

    IEntityDomain* NetworkEntityManager::GetEntityDomain() const
    {
        return m_entityDomain.get();
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

    const HostId& NetworkEntityManager::GetHostId() const
    {
        return m_hostId;
    }

    ConstNetworkEntityHandle NetworkEntityManager::GetEntity(NetEntityId netEntityId) const
    {
        return m_networkEntityTracker.Get(netEntityId);
    }

    NetEntityId NetworkEntityManager::GetNetEntityIdById(const AZ::EntityId& entityId) const
    {
        return m_networkEntityTracker.Get(entityId);
    }

    uint32_t NetworkEntityManager::GetEntityCount() const
    {
        return static_cast<uint32_t>(m_networkEntityTracker.size());
    }

    NetworkEntityHandle NetworkEntityManager::AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity)
    {
        m_networkEntityTracker.Add(netEntityId, entity);
        return NetworkEntityHandle(entity, &m_networkEntityTracker);
    }

    void NetworkEntityManager::MarkForRemoval(const ConstNetworkEntityHandle& entityHandle)
    {
        if (entityHandle.Exists())
        {
            if (net_DebugCheckNetworkEntityManager)
            {
                AZ_Assert(entityHandle.GetNetBindComponent(), "No NetBindComponent found on networked entity");
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

    void NetworkEntityManager::DebugDraw() const
    {
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
        AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        for (NetworkEntityTracker::const_iterator it = m_networkEntityTracker.begin(); it != m_networkEntityTracker.end(); ++it)
        {
            AZ::Entity* entity = it->second;
            NetBindComponent* netBindComponent = m_networkEntityTracker.GetNetBindComponent(entity);
            AZ::Aabb entityBounds = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->GetEntityWorldBoundsUnion(entity->GetId());
            entityBounds.Expand(AZ::Vector3(0.01f));
            if (netBindComponent->GetNetEntityRole() == NetEntityRole::Authority)
            {
                debugDisplay->SetColor(AZ::Colors::Black);
                debugDisplay->SetAlpha(0.5f);
            }
            else
            {
                debugDisplay->SetColor(AZ::Colors::DeepSkyBlue);
                debugDisplay->SetAlpha(0.25f);
            }
            debugDisplay->DrawWireBox(entityBounds.GetMin(), entityBounds.GetMax());
        }

        if (m_entityDomain != nullptr)
        {
            m_entityDomain->DebugDraw();
        }
    }

    void NetworkEntityManager::DispatchLocalDeferredRpcMessages()
    {
        for (NetworkEntityRpcMessage& rpcMessage : m_localDeferredRpcMessages)
        {
            AZ::Entity* entity = m_networkEntityTracker.GetRaw(rpcMessage.GetEntityId());
            if (entity != nullptr)
            {
                NetBindComponent* netBindComponent = m_networkEntityTracker.GetNetBindComponent(entity);
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

        const IEntityDomain::EntitiesNotInDomain& entitiesNotInDomain = m_entityDomain->RetrieveEntitiesNotInDomain();
        for (NetEntityId exitingId : entitiesNotInDomain)
        {
            OnEntityExitDomain(exitingId);
        }
    }

    void NetworkEntityManager::OnEntityExitDomain(NetEntityId entityId)
    {
        bool safeToExit = true;
        NetworkEntityHandle entityHandle = m_networkEntityTracker.Get(entityId);

        // We also need special handling for the NetworkHierarchy as well, since related entities need to be migrated together
        NetworkHierarchyRootComponentController* hierarchyRootController = entityHandle.FindController<NetworkHierarchyRootComponentController>();
        NetworkHierarchyChildComponentController* hierarchyChildController = entityHandle.FindController<NetworkHierarchyChildComponentController>();

        // Find the root entity
        AZ::Entity* hierarchyRootEntity = nullptr;
        if (hierarchyRootController)
        {
            hierarchyRootEntity = hierarchyRootController->GetParent().GetHierarchicalRoot();
        }
        else if (hierarchyChildController)
        {
            hierarchyRootEntity = hierarchyChildController->GetParent().GetHierarchicalRoot();
        }

        if (hierarchyRootEntity)
        {
            NetEntityId rootNetId = GetNetEntityIdById(hierarchyRootEntity->GetId());
            ConstNetworkEntityHandle rootEntityHandle = GetEntity(rootNetId);

            // Check if the root entity is still tracked by this authority
            if (rootEntityHandle.Exists() && rootEntityHandle.GetNetBindComponent()->HasController())
            {
                safeToExit = false;
            }
        }

        // Validate that we aren't already planning to remove this entity
        if (safeToExit)
        {
            for (auto remoteEntityId : m_removeList)
            {
                if (remoteEntityId == remoteEntityId)
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

    void NetworkEntityManager::Reset()
    {
        m_multiplayerComponentRegistry.Reset();
        m_removeList.clear();
        m_entityDomain = nullptr;
        m_updateEntityDomainEvent.RemoveFromQueue();
        m_ownedEntities.clear();
        m_entityExitDomainEvent.DisconnectAllHandlers();
        m_onEntityMarkedDirty.DisconnectAllHandlers();
        m_onEntityNotifyChanges.DisconnectAllHandlers();
        m_controllersActivatedEvent.DisconnectAllHandlers();
        m_controllersDeactivatedEvent.DisconnectAllHandlers();
        m_localDeferredRpcMessages.clear();
    }

    void NetworkEntityManager::RemoveEntities()
    {
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

                // At the moment, we spawn one entity at a time and avoid Prefab API calls and never get a spawn ticket,
                // so this is the right way for now. Once we support prefabs we can use AzFramework::SpawnableEntitiesContainer
                // Additionally, prefabs spawning is async! Whereas we currently create entities immediately, see:
                // @NetworkEntityManager::CreateEntitiesImmediate
                AzFramework::GameEntityContextRequestBus::Broadcast(
                    &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity, netBindComponent->GetEntityId());
            }

            m_networkEntityTracker.erase(entityId);
        }
    }
    
    INetworkEntityManager::EntityList NetworkEntityManager::CreateEntitiesImmediate(
        const AzFramework::Spawnable& spawnable, NetEntityRole netEntityRole, AutoActivate autoActivate)
    {
        INetworkEntityManager::EntityList returnList;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AzFramework::Spawnable::EntityList& entities = spawnable.GetEntities();
        size_t entitiesSize = entities.size();

        using EntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;
        EntityIdMap originalToCloneIdMap;

        for (size_t i = 0; i < entitiesSize; ++i)
        {
            AZ::Entity* originalEntity = entities[i].get();
            AZ::Entity* clone = serializeContext->CloneObject(originalEntity);
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

            clone->SetId(AZ::Entity::MakeId());

            originalToCloneIdMap[originalEntity->GetId()] = clone->GetId();

            // Can't use NetworkEntityTracker to do the lookup since the entity has not activated yet
            NetBindComponent* netBindComponent = clone->FindComponent<NetBindComponent>();
            if (netBindComponent != nullptr)
            {
                // Update TransformComponent parent Id. It is guaranteed for the entities array to be sorted from parent->child here.
                auto* transformComponent = clone->FindComponent<AzFramework::TransformComponent>();
                AZ::EntityId parentId = transformComponent->GetParentId();
                if (parentId.IsValid())
                {
                    auto it = originalToCloneIdMap.find(parentId);
                    if (it != originalToCloneIdMap.end())
                    {
                        transformComponent->SetParentRelative(it->second);
                    }
                    else
                    {
                        AZ_Warning("NetworkEntityManager", false, "Entity %s doesn't have the parent entity %s present in network.spawnable",
                            clone->GetName().c_str(), parentId.ToString().data());
                    }
                }

                PrefabEntityId prefabEntityId;
                prefabEntityId.m_prefabName = m_networkPrefabLibrary.GetSpawnableNameFromAssetId(spawnable.GetId());
                prefabEntityId.m_entityOffset = aznumeric_cast<uint32_t>(i);

                const NetEntityId netEntityId = NextId();
                netBindComponent->PreInit(clone, prefabEntityId, netEntityId, netEntityRole);

                if (autoActivate == AutoActivate::DoNotActivate)
                {
                    clone->SetRuntimeActiveByDefault(false);
                }

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
        const AZ::Transform& transform,
        AutoActivate autoActivate
    )
    {
        return CreateEntitiesImmediate(prefabEntryId, NextId(), netEntityRole, autoActivate, transform);
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
        EntityList returnList;
        if (!AZ::Data::AssetManager::IsReady())
        {
            return returnList;
        }
        
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
            return CreateEntitiesImmediate(*netSpawnable, netEntityRole, autoActivate);
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

    AZStd::unique_ptr<AzFramework::EntitySpawnTicket> NetworkEntityManager::RequestNetSpawnableInstantiation(
        const AZ::Data::Asset<AzFramework::Spawnable>& netSpawnable, const AZ::Transform& transform)
    {
        // Prepare the parameters for the spawning process
        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_priority = AzFramework::SpawnablePriority_High;

        const AZ::Name netSpawnableName =
            AZ::Interface<INetworkSpawnableLibrary>::Get()->GetSpawnableNameFromAssetId(netSpawnable.GetId());

        if (netSpawnableName.IsEmpty())
        {
            AZ_Error("NetworkEntityManager", false,
                "RequestNetSpawnableInstantiation: Requested spawnable %s doesn't exist in the NetworkSpawnableLibrary. Please make sure it is a network spawnable",
                netSpawnable.GetHint().c_str());
            return nullptr;
        }

        // Pre-insertion callback allows us to do network-specific setup for the entities before they are added to the scene
        optionalArgs.m_preInsertionCallback = [netSpawnableName, rootTransform = transform]
            (AzFramework::EntitySpawnTicket::Id, AzFramework::SpawnableEntityContainerView entities)
        {
            bool shouldUpdateTransform = (rootTransform.IsClose(AZ::Transform::Identity()) == false);

            for (uint32_t netEntityIndex = 0, entitiesSize = aznumeric_cast<uint32_t>(entities.size());
                netEntityIndex < entitiesSize; ++netEntityIndex)
            {
                AZ::Entity* netEntity = *(entities.begin() + netEntityIndex);

                if (shouldUpdateTransform)
                {
                    AzFramework::TransformComponent* netEntityTransform =
                        netEntity->FindComponent<AzFramework::TransformComponent>();

                    AZ::Transform worldTm = netEntityTransform->GetWorldTM();
                    worldTm = rootTransform * worldTm;
                    netEntityTransform->SetWorldTM(worldTm);
                }

                PrefabEntityId prefabEntityId;
                prefabEntityId.m_prefabName = netSpawnableName;
                prefabEntityId.m_entityOffset = netEntityIndex;
                AZ::Interface<INetworkEntityManager>::Get()->SetupNetEntity(netEntity, prefabEntityId, NetEntityRole::Authority);
            }
        };

        // Spawn with the newly created ticket. This allows the calling code to manage the lifetime of the constructed entities
        auto ticket = AZStd::make_unique<AzFramework::EntitySpawnTicket>(netSpawnable);
        AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*ticket, AZStd::move(optionalArgs));
        return ticket;
    }

    void NetworkEntityManager::OnRootSpawnableAssigned(AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable,
        [[maybe_unused]] uint32_t generation)
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
        auto* multiplayer = GetMultiplayer();
        const auto agentType = multiplayer->GetAgentType();

        if (agentType == MultiplayerAgentType::Client)
        {
            multiplayer->SendReadyForEntityUpdates(false);
        }
    }

    void NetworkEntityManager::SetupNetEntity(AZ::Entity* netEntity, PrefabEntityId prefabEntityId, NetEntityRole netEntityRole)
    {
        auto* netBindComponent = netEntity->FindComponent<NetBindComponent>();

        if (netBindComponent)
        {
            const NetEntityId netEntityId = NextId();
            netBindComponent->PreInit(netEntity, prefabEntityId, netEntityId, netEntityRole);
        }
        else
        {
            AZ_Error("NetworkEntityManager", false, "SetupNetEntity called for an entity with no NetBindComponent. Entity: %s",
                netEntity->GetName().c_str());
        }
    }
}
