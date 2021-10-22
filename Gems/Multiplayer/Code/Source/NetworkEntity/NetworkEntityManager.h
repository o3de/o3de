/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Source/NetworkEntity/NetworkSpawnableLibrary.h>
#include <Multiplayer/Components/MultiplayerComponentRegistry.h>
#include <Multiplayer/EntityDomains/IEntityDomain.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>

namespace Multiplayer
{
    //! Implementation of the networked entity manager interface.
    //! This class creates and manages all networked entities.
    class NetworkEntityManager final
        : public INetworkEntityManager
        , public AzFramework::RootSpawnableNotificationBus::Handler
    {
    public:
        NetworkEntityManager();
        ~NetworkEntityManager();

        //! INetworkEntityManager overrides.
        //! @{
        void Initialize(const HostId& hostId, AZStd::unique_ptr<IEntityDomain> entityDomain) override;
        bool IsInitialized() const override;
        IEntityDomain* GetEntityDomain() const override;
        NetworkEntityTracker* GetNetworkEntityTracker() override;
        NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker() override;
        MultiplayerComponentRegistry* GetMultiplayerComponentRegistry() override;
        const HostId& GetHostId() const override;
        ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const override;
        NetEntityId GetNetEntityIdById(const AZ::EntityId& entityId) const override;

        EntityList CreateEntitiesImmediate(const AzFramework::Spawnable& spawnable, NetEntityRole netEntityRole, AutoActivate autoActivate);
        EntityList CreateEntitiesImmediate
        (
            const PrefabEntityId& prefabEntryId,
            NetEntityRole netEntityRole,
            const AZ::Transform& transform,
            AutoActivate autoActivate = AutoActivate::Activate
        ) override;
        EntityList CreateEntitiesImmediate
        (
            const PrefabEntityId& prefabEntryId,
            NetEntityId netEntityId,
            NetEntityRole netEntityRole,
            AutoActivate autoActivate,
            const AZ::Transform& transform
        ) override;

        AZStd::unique_ptr<AzFramework::EntitySpawnTicket> RequestNetSpawnableInstantiation(
            const AZ::Data::Asset<AzFramework::Spawnable>& netSpawnable, const AZ::Transform& transform) override;
        void SetupNetEntity(AZ::Entity* netEntity, PrefabEntityId prefabEntityId, NetEntityRole netEntityRole) override;
        uint32_t GetEntityCount() const override;
        NetworkEntityHandle AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity) override;
        void MarkForRemoval(const ConstNetworkEntityHandle& entityHandle) override;
        bool IsMarkedForRemoval(const ConstNetworkEntityHandle& entityHandle) const override;
        void ClearEntityFromRemovalList(const ConstNetworkEntityHandle& entityHandle) override;
        void ClearAllEntities() override;
        void AddEntityMarkedDirtyHandler(AZ::Event<>::Handler& entityMarkedDirtyHandle) override;
        void AddEntityNotifyChangesHandler(AZ::Event<>::Handler& entityNotifyChangesHandle) override;
        void AddEntityExitDomainHandler(EntityExitDomainEvent::Handler& entityExitDomainHandler) override;
        void AddControllersActivatedHandler(ControllersActivatedEvent::Handler& controllersActivatedHandler) override;
        void AddControllersDeactivatedHandler(ControllersDeactivatedEvent::Handler& controllersDeactivatedHandler) override;
        void NotifyEntitiesDirtied() override;
        void NotifyEntitiesChanged() override;
        void NotifyControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) override;
        void NotifyControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) override;
        void HandleLocalRpcMessage(NetworkEntityRpcMessage& message) override;
        void DebugDraw() const override;
        //! @}

        void DispatchLocalDeferredRpcMessages();
        void UpdateEntityDomain();
        void OnEntityExitDomain(NetEntityId entityId);

        //! RootSpawnableNotificationBus
        //! @{
        void OnRootSpawnableAssigned(AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, uint32_t generation) override;
        void OnRootSpawnableReleased(uint32_t generation) override;
        //! @}

        //! Used to release all memory prior to shutdown.
        void Reset();

    private:
        void RemoveEntities();
        NetEntityId NextId();

        NetworkEntityTracker m_networkEntityTracker;
        NetworkEntityAuthorityTracker m_networkEntityAuthorityTracker;
        MultiplayerComponentRegistry m_multiplayerComponentRegistry;

        AZ::ScheduledEvent m_removeEntitiesEvent;
        AZStd::vector<NetEntityId> m_removeList;
        AZStd::unique_ptr<IEntityDomain> m_entityDomain;
        AZ::ScheduledEvent m_updateEntityDomainEvent;

        OwnedEntitySet m_ownedEntities;

        EntityExitDomainEvent m_entityExitDomainEvent;
        AZ::Event<> m_onEntityMarkedDirty;
        AZ::Event<> m_onEntityNotifyChanges;
        ControllersActivatedEvent m_controllersActivatedEvent;
        ControllersDeactivatedEvent m_controllersDeactivatedEvent;

        HostId m_hostId = InvalidHostId;
        NetEntityId m_nextEntityId = NetEntityId{ 0 };

        // Local RPCs are buffered and dispatched at the end of the frame rather than processed immediately
        // This is done to prevent local and network sent RPC's from having different dispatch behaviours
        typedef AZStd::deque<NetworkEntityRpcMessage> DeferredRpcMessages;
        DeferredRpcMessages m_localDeferredRpcMessages;

        NetworkSpawnableLibrary m_networkPrefabLibrary;
    };
}
