/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UiSpawnerComponent
class UiSpawnerInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiSpawnerInterface() {}

    //! Spawn the selected spawnable at the entity's location
    virtual AzFramework::EntitySpawnTicket::Id Spawn() = 0;

    //! Spawn the selected spawnable at the entity's location with the provided relative offset
    virtual AzFramework::EntitySpawnTicket::Id SpawnRelative(const AZ::Vector2& relative) = 0;

    //! Spawn the selected spawnable at the specified viewport position
    virtual AzFramework::EntitySpawnTicket::Id SpawnViewport(const AZ::Vector2& pos) = 0;

    //! Spawn the provided spawnable at the entity's location
    virtual AzFramework::EntitySpawnTicket::Id SpawnSpawnable(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable) = 0;

    //! Spawn the provided spawnable at the entity's location with the provided relative offset
    virtual AzFramework::EntitySpawnTicket::Id SpawnSpawnableRelative(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& relative) = 0;

    //! Spawn the provided spawnable at the specified viewport position
    virtual AzFramework::EntitySpawnTicket::Id SpawnSpawnableViewport(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& pos) = 0;
};

using UiSpawnerBus = AZ::EBus<UiSpawnerInterface>;


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for notifications dispatched by the UiSpawnerComponent
//! Whenever one of the "Spawn" calls in UiSpawnerInterface is called then a listener connected
//! on this bus with the spawner entity ID will either get:
//! 1. This sequence of notifications:
//!    OnSpawnBegin
//!    N x OnEntitySpawned
//!    OnEntitiesSpawned
//!    OnTopLevelEntitiesSpawned
//!    OnSpawnEnd
//! 2. In the case of a spawn error just this notification:
//!    OnSpawnFailed
class UiSpawnerNotifications
    : public AZ::ComponentBus
{
public:
    virtual ~UiSpawnerNotifications() {}

    //! Notify that spawnable has been spawned, but entities have not yet been activated.
    //! OnEntitySpawned events are about to be dispatched.
    virtual void OnSpawnBegin(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/) {}

    //! Notify that an entity has spawned, will be called once for each entity spawned.
    virtual void OnEntitySpawned(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/, const AZ::EntityId& /*spawnedEntity*/) {}

    //! Single event notification for an entire spawn, providing a list of all resulting entity Ids.
    virtual void OnEntitiesSpawned(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    //! Single event notification for an entire spawn, providing a list of all resulting top-level entity Ids.
    //! Top-level entities are ones that are not the child of any other entity in the spawnable
    virtual void OnTopLevelEntitiesSpawned(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    //! Notify that a spawn has been completed. All spawn notifications for this ticket have been dispatched.
    virtual void OnSpawnEnd(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/) {}

    //! Notify that spawn has failed.
    virtual void OnSpawnFailed(const AzFramework::EntitySpawnTicket::Id& /*ticketId*/) {}
};

using UiSpawnerNotificationBus = AZ::EBus<UiSpawnerNotifications>;
