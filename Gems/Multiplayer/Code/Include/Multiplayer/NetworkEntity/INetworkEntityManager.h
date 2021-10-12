/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace Multiplayer
{
    class NetworkEntityTracker;
    class NetworkEntityAuthorityTracker;
    class NetworkEntityRpcMessage;
    class MultiplayerComponentRegistry;
    class IEntityDomain;

    using EntityExitDomainEvent = AZ::Event<const ConstNetworkEntityHandle&>;
    using ControllersActivatedEvent = AZ::Event<const ConstNetworkEntityHandle&, EntityIsMigrating>;
    using ControllersDeactivatedEvent = AZ::Event<const ConstNetworkEntityHandle&, EntityIsMigrating>;

    //! @class INetworkEntityManager
    //! @brief The interface for managing all networked entities.
    class INetworkEntityManager
    {
    public:
        AZ_RTTI(INetworkEntityManager, "{109759DE-9492-439C-A0B1-AE46E6FD029C}");

        using OwnedEntitySet = AZStd::unordered_set<ConstNetworkEntityHandle>;
        using EntityList = AZStd::vector<NetworkEntityHandle>;

        virtual ~INetworkEntityManager() = default;

        //! Configures the NetworkEntityManager to operate as an authoritative host.
        //! @param hostId       the hostId of this NetworkEntityManager
        //! @param entityDomain the entity domain used to determine which entities this manager has authority over
        virtual void Initialize(const HostId& hostId, AZStd::unique_ptr<IEntityDomain> entityDomain) = 0;

        //! Returns whether or not the network entity manager has been initialized to host.
        //! @return boolean true if this network entity manager has been intialized to host
        virtual bool IsInitialized() const = 0;

        //! Returns the entity domain associated with this network entity manager, this will be nullptr on clients.
        //! @return boolean the entity domain for this network entity manager
        virtual IEntityDomain* GetEntityDomain() const = 0;

        //! Returns the NetworkEntityTracker for this INetworkEntityManager instance.
        //! @return the NetworkEntityTracker for this INetworkEntityManager instance
        virtual NetworkEntityTracker* GetNetworkEntityTracker() = 0;

        //! Returns the NetworkEntityAuthorityTracker for this INetworkEntityManager instance.
        //! @return the NetworkEntityAuthorityTracker for this INetworkEntityManager instance
        virtual NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker() = 0;

        //! Returns the MultiplayerComponentRegistry for this INetworkEntityManager instance.
        //! @return the MultiplayerComponentRegistry for this INetworkEntityManager instance
        virtual MultiplayerComponentRegistry* GetMultiplayerComponentRegistry() = 0;

        //! Returns the HostId for this INetworkEntityManager instance.
        //! @return the HostId for this INetworkEntityManager instance
        virtual const HostId& GetHostId() const = 0;

        //! Creates new entities of the given archetype
        //! @param prefabEntryId the name of the spawnable to spawn
        virtual EntityList CreateEntitiesImmediate
        (
            const PrefabEntityId& prefabEntryId,
            NetEntityRole netEntityRole,
            const AZ::Transform& transform,
            AutoActivate autoActivate = AutoActivate::Activate
        ) = 0;

        //! Creates new entities of the given archetype
        //! This interface is internally used to spawn replicated entities
        //! @param prefabEntryId the name of the spawnable to spawn
        virtual EntityList CreateEntitiesImmediate
        (
            const PrefabEntityId& prefabEntryId,
            NetEntityId netEntityId,
            NetEntityRole netEntityRole,
            AutoActivate autoActivate,
            const AZ::Transform& transform
        ) = 0;

        //! Requests a network spawnable to instantiate at a given transform
        //! This is an async function. The instantiated entities are not available immediately but will be constructed by the spawnable system
        //! The spawnable ticket has to be kept for the whole lifetime of the entities
        //! @param netSpawnable the network spawnable to spawn
        //! @param transform the transform where the spawnable should be spawned
        //! @return the ticket for managing the spawned entities
        [[nodiscard]] virtual AZStd::unique_ptr<AzFramework::EntitySpawnTicket> RequestNetSpawnableInstantiation(
            const AZ::Data::Asset<AzFramework::Spawnable>& netSpawnable, const AZ::Transform& transform) = 0;

        //! Configures new networked entity
        //! @param netEntity the entity to setup
        //! @param prefabEntryId the name of the spawnable the entity originated from
        //! @param netEntityRole the net role the entity should be setup for
        virtual void SetupNetEntity(AZ::Entity* netEntity, PrefabEntityId prefabEntityId, NetEntityRole netEntityRole) = 0;

        //! Returns an ConstEntityPtr for the provided entityId.
        //! @param netEntityId the netEntityId to get an ConstEntityPtr for
        //! @return the requested ConstEntityPtr
        virtual ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const = 0;

        //! Returns the total number of entities tracked by this INetworkEntityManager instance.
        //! @return the total number of entities tracked by this INetworkEntityManager instance
        virtual uint32_t GetEntityCount() const = 0;

        //! Returns the Net Entity ID for a given AZ Entity ID.
        //! @param entityId the AZ Entity ID
        //! @return the Net Entity ID
        virtual NetEntityId GetNetEntityIdById(const AZ::EntityId& entityId) const = 0;

        //! Adds the provided entity to the internal entity map identified by the provided netEntityId.
        //! @param netEntityId the identifier to use for the added entity
        //! @param entity      the entity to add to the internal entity map
        //! @return a NetworkEntityHandle for the newly added entity
        virtual NetworkEntityHandle AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity) = 0;

        //! Marks the specified entity for removal and deletion.
        //! @param entityHandle the entity to remove and delete
        virtual void MarkForRemoval(const ConstNetworkEntityHandle& entityHandle) = 0;

        //! Returns true if the indicated entity is marked for removal.
        //! @param entityHandle the entity to test if marked for removal
        //! @return boolean true if the specified entity is marked for removal, false otherwise
        virtual bool IsMarkedForRemoval(const ConstNetworkEntityHandle& entityHandle) const = 0;

        //! Unmarks the specified entity so it will no longer be removed and deleted.
        //! @param entityHandle the entity to unmark for removal and deletion
        virtual void ClearEntityFromRemovalList(const ConstNetworkEntityHandle& entityHandle) = 0;

        //! Clears out and deletes all entities registered with the entity manager.
        virtual void ClearAllEntities() = 0;

        //! Adds an event handler to be invoked when we notify which entities have been marked dirty.
        //! @param entityMarkedDirtyHandle event handler for the dirtied entity
        virtual void AddEntityMarkedDirtyHandler(AZ::Event<>::Handler& entityMarkedDirtyHandle) = 0;

        //! Adds an event handler to be invoked when we notify entities to send their change notifications.
        //! @param entityNotifyChangesHandle event handler for the dirtied entity
        virtual void AddEntityNotifyChangesHandler(AZ::Event<>::Handler& entityNotifyChangesHandle) = 0;

        //! Adds an event handler to be invoked when we notify entities to send their change notifications.
        //! @param entityNotifyChangesHandle event handler for the dirtied entity
        virtual void AddEntityExitDomainHandler(EntityExitDomainEvent::Handler& entityExitDomainHandler) = 0;

        //! Adds an event handler to be invoked when an entities controllers have activated
        //! @param controllersActivatedHandler event handler for the entity
        virtual void AddControllersActivatedHandler(ControllersActivatedEvent::Handler& controllersActivatedHandler) = 0;

        //! Adds an event handler to be invoked when an entities controllers have been deactivated
        //! @param controllersDeactivatedHandler event handler for the entity
        virtual void AddControllersDeactivatedHandler(ControllersDeactivatedEvent::Handler& controllersDeactivatedHandler) = 0;

        //! Notifies entities that they should process their dirty state.
        virtual void NotifyEntitiesDirtied() = 0;

        //! Notifies entities that they should process change notifications.
        virtual void NotifyEntitiesChanged() = 0;

        //! Notifies that an entities controllers have activated.
        //! @param entityHandle handle to the entity whose controllers have activated
        //! @param entityIsMigrating true if the entity is activating after a migration
        virtual void NotifyControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) = 0;

        //! Notifies that an entities controllers have been deactivated.
        //! @param entityHandle handle to the entity whose controllers have been deactivated
        //! @param entityIsMigrating true if the entity is deactivating due to a migration
        virtual void NotifyControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) = 0;

        //! Handle a local rpc message.
        //! @param entityRpcMessage the local rpc message to handle
        virtual void HandleLocalRpcMessage(NetworkEntityRpcMessage& message) = 0;

        //! Visualization of network entity manager state.
        virtual void DebugDraw() const = 0;
    };
}
