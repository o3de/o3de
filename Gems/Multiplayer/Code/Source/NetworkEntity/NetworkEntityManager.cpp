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
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

AZ_DECLARE_BUDGET(MULTIPLAYER);
namespace Multiplayer
{
    AZ_CVAR(bool, net_DebugCheckNetworkEntityManager, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables extra debug checks inside the NetworkEntityManager");

    NetworkEntityManager::NetworkEntityManager()
        : m_networkEntityAuthorityTracker(*this)
        , m_removeEntitiesEvent([this] { RemoveEntities(); }, AZ::Name("NetworkEntityManager remove entities event"))
    {
        AZ::Interface<INetworkEntityManager>::Register(this);
        AzFramework::RootSpawnableNotificationBus::Handler::BusConnect();
        AzFramework::SpawnableAssetEventsBus::Handler::BusConnect();
    }

    NetworkEntityManager::~NetworkEntityManager()
    {
        AzFramework::SpawnableAssetEventsBus::Handler::BusDisconnect();
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

    void NetworkEntityManager::RemoveEntityFromEntityMap(NetEntityId netEntityId)
    {
        m_networkEntityTracker.erase(netEntityId);
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
            if (!m_removeEntitiesEvent.IsScheduled())
            {
                m_removeEntitiesEvent.Enqueue(AZ::Time::ZeroTimeMs);
            }
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
        AZ_PROFILE_SCOPE(MULTIPLAYER, "NetworkEntityManager: NotifyEntitiesDirtied");
        m_onEntityMarkedDirty.Signal();
    }

    void NetworkEntityManager::NotifyEntitiesChanged()
    {
        AZ_PROFILE_SCOPE(MULTIPLAYER, "NetworkEntityManager: NotifyEntitiesChanged");
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
        m_localDeferredRpcMessages.emplace_back(message);
    }

    void NetworkEntityManager::HandleEntitiesExitDomain(const NetEntityIdSet& entitiesNotInDomain)
    {
        for (NetEntityId exitingId : entitiesNotInDomain)
        {
            NetworkEntityHandle entityHandle = m_networkEntityTracker.Get(exitingId);

            bool safeToExit = IsHierarchySafeToExit(entityHandle, entitiesNotInDomain);

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
                // Tell all the attached replicators for this entity that it's exited the domain
                m_entityExitDomainEvent.Signal(entityHandle);
            }
        }
    }

    void NetworkEntityManager::ForceAssumeAuthority(const ConstNetworkEntityHandle& entityHandle)
    {
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            netBindComponent->ConstructControllers();
        }
    }

    void NetworkEntityManager::MarkAlwaysRelevantToClients(const ConstNetworkEntityHandle& entityHandle, bool alwaysRelevant)
    {
        if (alwaysRelevant)
        {
            AZ_Assert(entityHandle.GetNetBindComponent()->IsNetEntityRoleAuthority(), "Marking an entity always relevant can only be done on an authoritative entity");
            m_alwaysRelevantToClients.emplace(entityHandle);
        }
        else
        {
            m_alwaysRelevantToClients.erase(entityHandle);
        }
    }

    void NetworkEntityManager::MarkAlwaysRelevantToServers(const ConstNetworkEntityHandle& entityHandle, bool alwaysRelevant)
    {
        if (alwaysRelevant)
        {
            AZ_Assert(entityHandle.GetNetBindComponent()->IsNetEntityRoleAuthority(), "Marking an entity always relevant can only be done on an authoritative entity");
            m_alwaysRelevantToServers.emplace(entityHandle);
        }
        else
        {
            m_alwaysRelevantToServers.erase(entityHandle);
        }
    }

    const NetEntityHandleSet& NetworkEntityManager::GetAlwaysRelevantToClientsSet() const
    {
        return m_alwaysRelevantToClients;
    }

    const NetEntityHandleSet& NetworkEntityManager::GetAlwaysRelevantToServersSet() const
    {
        return m_alwaysRelevantToServers;
    }

    void NetworkEntityManager::SetMigrateTimeoutTimeMs(AZ::TimeMs timeoutTimeMs)
    {
        m_networkEntityAuthorityTracker.SetTimeoutTimeMs(timeoutTimeMs);
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
            if (!entityBounds.IsValid())
            {
                continue;
            }

            entityBounds.Expand(AZ::Vector3(0.01f));
            if ((netBindComponent != nullptr) && netBindComponent->GetNetEntityRole() == NetEntityRole::Authority)
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
        // Local messages may get queued up while we process other local messages,
        // so let @m_localDeferredRpcMessages accumulate,
        // while we work on the current messages.
        AZStd::deque<NetworkEntityRpcMessage> copy;
        copy.swap(m_localDeferredRpcMessages);

        for (NetworkEntityRpcMessage& rpcMessage : copy)
        {
            AZ::Entity* entity = m_networkEntityTracker.GetRaw(rpcMessage.GetEntityId());
            if (entity != nullptr)
            {
                NetBindComponent* netBindComponent = m_networkEntityTracker.GetNetBindComponent(entity);
                AZ_Assert(netBindComponent != nullptr, "Attempting to send an RPC to an entity with no NetBindComponent");
                switch(rpcMessage.GetRpcDeliveryType())
                {
                case RpcDeliveryType::AuthorityToClient:
                case RpcDeliveryType::AuthorityToAutonomous:
                    netBindComponent->HandleRpcMessage(nullptr, NetEntityRole::Authority, rpcMessage);
                    break;
                case RpcDeliveryType::AutonomousToAuthority:
                    netBindComponent->HandleRpcMessage(nullptr, NetEntityRole::Autonomous, rpcMessage);
                    break;
                case RpcDeliveryType::ServerToAuthority:
                    netBindComponent->HandleRpcMessage(nullptr, NetEntityRole::Server, rpcMessage);
                    break;
                case RpcDeliveryType::None:
                    break;
                }
            }
        }
    }

    void NetworkEntityManager::Reset()
    {
        m_multiplayerComponentRegistry.Reset();
        m_removeList.clear();
        m_entityDomain = nullptr;
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
                // If we've spawned entities through @NetworkEntityManager::CreateEntitiesImmediate
                // then we destroy those entities here by processing the removal list.
                // Note that if we've spawned entities through @NetworkPrefabSpawnerComponent::SpawnPrefab
                // we should instead use the SpawnableEntitiesManager to destroy them.
                AzFramework::GameEntityContextRequestBus::Broadcast(
                    &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity, removeEntity.GetEntity()->GetId());

                m_networkEntityTracker.erase(entityId);
            }
        }
    }

    INetworkEntityManager::EntityList NetworkEntityManager::CreateEntitiesImmediate(
        const AzFramework::Spawnable& spawnable, NetEntityRole netEntityRole, const AZ::Transform& transform, AutoActivate autoActivate)
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

            // Can't use NetworkEntityTracker to do the lookup since the entity has not activated yet
            if (!originalEntity->FindComponent<NetBindComponent>())
            {
                continue;
            }

            AZ::Entity* clone = serializeContext->CloneObject(originalEntity);
            AZ_Assert(clone != nullptr, "Failed to clone spawnable entity.");

            clone->SetId(AZ::Entity::MakeId());

            originalToCloneIdMap[originalEntity->GetId()] = clone->GetId();

            // Update TransformComponent parent Id. It is guaranteed for the entities array to be sorted from parent->child here.
            auto cloneNetBindComponent = clone->FindComponent<NetBindComponent>();
            auto cloneTransformComponent = clone->FindComponent<AzFramework::TransformComponent>();
            AZ::EntityId parentId = cloneTransformComponent->GetParentId();
            bool removeParent = false;
            if (parentId.IsValid())
            {
                auto it = originalToCloneIdMap.find(parentId);
                if (it != originalToCloneIdMap.end())
                {
                    // Note: The need to remove and readd the transform component parent will go away once this method replaces serializeContext->CloneObject
                    //    with the standard AzFramework::SpawnableEntitiesInterface::SpawnEntities
                    // This stops SetParentRelative() from printing distracting warnings, due to the cloned component m_entity being null.
                    // AddComponent properly sets the component's m_entity. 
                    clone->RemoveComponent(cloneTransformComponent);
                    clone->AddComponent(cloneTransformComponent);
                    cloneTransformComponent->SetParentRelative(it->second);
                }
                else
                {
                    // This network entity is referencing a non-network parent
                    // We need to clear the parent.
                    // Note: The need to clear the parent will go away once this method replaces serializeContext->CloneObject
                    //    with the standard AzFramework::SpawnableEntitiesInterface::SpawnEntities
                    removeParent = true;
                }
            }

            PrefabEntityId prefabEntityId;
            prefabEntityId.m_prefabName = m_networkPrefabLibrary.GetSpawnableNameFromAssetId(spawnable.GetId());
            prefabEntityId.m_entityOffset = aznumeric_cast<uint32_t>(i);

            const NetEntityId netEntityId = NextId();
            cloneNetBindComponent->PreInit(clone, prefabEntityId, netEntityId, netEntityRole);

            // Set the transform if we're a root entity (have no parent); otherwise, keep the local transform
            if (!parentId.IsValid() || removeParent)
            {
                cloneTransformComponent->SetWorldTM(transform);
            }

            if (autoActivate == AutoActivate::DoNotActivate)
            {
                clone->SetRuntimeActiveByDefault(false);
            }

            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::AddGameEntity, clone);

            if (removeParent)
            {
                cloneTransformComponent->SetParent(AZ::EntityId());
            }
            
            returnList.push_back(cloneNetBindComponent->GetEntityHandle());
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
            return CreateEntitiesImmediate(*netSpawnable, netEntityRole, transform, autoActivate);
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
            const bool shouldUpdateTransform = !rootTransform.IsClose(AZ::Transform::Identity());

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

    void NetworkEntityManager::OnResolveAliases(
        AzFramework::Spawnable::EntityAliasVisitor& aliases,
        [[maybe_unused]] const AzFramework::SpawnableMetaData& metadata,
        [[maybe_unused]] const AzFramework::Spawnable::EntityList& entities)
    {
        auto* multiplayer = GetMultiplayer();
        if (!multiplayer->GetShouldSpawnNetworkEntities())
        {
            aliases.UpdateAliases(NetworkEntityTag, [](
                AzFramework::Spawnable::EntityAliasType& aliasType,
                [[maybe_unused]] bool& queueLoad,
                [[maybe_unused]] const AZ::Data::Asset<AzFramework::Spawnable>& aliasedSpawnable,
                [[maybe_unused]] const AZ::Crc32 tag,
                [[maybe_unused]] const uint32_t sourceIndex,
                [[maybe_unused]] const uint32_t targetIndex)
            {
                aliasType = AzFramework::Spawnable::EntityAliasType::Disable;
            });
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

    bool NetworkEntityManager::IsHierarchySafeToExit(NetworkEntityHandle& entityHandle, const NetEntityIdSet& entitiesNotInDomain)
    {
        bool safeToExit = true;

        // We also need special handling for the NetworkHierarchy as well, since related entities need to be migrated together
        NetworkHierarchyRootComponentController* hierarchyRootController = entityHandle.FindController<NetworkHierarchyRootComponentController>();
        NetworkHierarchyChildComponentController* hierarchyChildController = entityHandle.FindController<NetworkHierarchyChildComponentController>();

        AZStd::vector<AZ::Entity*> hierarchicalEntities;

        // Get the entities in this hierarchy
        if (hierarchyRootController)
        {
            hierarchicalEntities = hierarchyRootController->GetParent().GetHierarchicalEntities();
        }
        else if (hierarchyChildController)
        {
            hierarchicalEntities = hierarchyChildController->GetParent().GetHierarchicalEntities();
        }

        // Check if *all* entities in the hierarchy are ready to migrate.
        // If any are still "in domain", keep the whole hierarchy within the current authority for now
        for (AZ::Entity* entity : hierarchicalEntities)
        {
            NetEntityId netEntityId = GetNetEntityIdById(entity->GetId());
            if (netEntityId != InvalidNetEntityId && !entitiesNotInDomain.contains(netEntityId))
            {
                safeToExit = false;
                break;
            }
        }

        return safeToExit;
    }

} // namespace Multiplayer
