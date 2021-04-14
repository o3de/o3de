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

#pragma once

#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Source/NetworkEntity/INetworkEntityManager.h>
#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Source/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Source/EntityDomains/IEntityDomain.h>

namespace Multiplayer
{
    //! Implementation of the networked entity manager interface.
    //! This class creates and manages all networked entities.
    class NetworkEntityManager final
        : public INetworkEntityManager
    {
    public:
        NetworkEntityManager();
        ~NetworkEntityManager();

        //! Only invoked for authoritative hosts
        void Initialize(HostId hostId, AZStd::unique_ptr<IEntityDomain> entityDomain);

        //! INetworkEntityManager overrides.
        //! @{
        NetworkEntityTracker* GetNetworkEntityTracker() override;
        NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker() override;
        HostId GetHostId() const override;
        ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const override;
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
        //! @}

        void DispatchLocalDeferredRpcMessages();
        void UpdateEntityDomain();
        void OnEntityExitDomain(NetEntityId entityId);

    private:

        void OnEntityAdded(AZ::Entity* entity);
        void OnEntityRemoved(AZ::Entity* entity);
        void RemoveEntities();

        NetworkEntityTracker m_networkEntityTracker;
        NetworkEntityAuthorityTracker m_networkEntityAuthorityTracker;
        AZ::ScheduledEvent m_removeEntitiesEvent;
        AZStd::vector<NetEntityId> m_removeList;
        AZStd::vector<AZ::Entity*> m_nonNetworkedEntities; // Contains entities that we've instantiated, but are not networked entities

        AZStd::unique_ptr<IEntityDomain> m_entityDomain;
        AZ::ScheduledEvent m_updateEntityDomainEvent;

        IEntityDomain::EntitiesNotInDomain m_entitiesNotInDomain;
        OwnedEntitySet m_ownedEntities;

        EntityExitDomainEvent m_entityExitDomainEvent;
        AZ::Event<> m_onEntityMarkedDirty;
        AZ::Event<> m_onEntityNotifyChanges;
        ControllersActivatedEvent m_controllersActivatedEvent;
        ControllersDeactivatedEvent m_controllersDeactivatedEvent;
        AZ::EntityAddedEvent::Handler m_entityAddedEventHandler;
        AZ::EntityRemovedEvent::Handler m_entityRemovedEventHandler;

        HostId m_hostId = InvalidHostId;
        NetEntityId m_nextEntityId = NetEntityId{ 0 };

        // Local RPCs are buffered and dispatched at the end of the frame rather than processed immediately
        // This is done to prevent local and network sent RPC's from having different dispatch behaviours
        typedef AZStd::deque<NetworkEntityRpcMessage> DeferredRpcMessages;
        DeferredRpcMessages m_localDeferredRpcMessages;
    };
}
