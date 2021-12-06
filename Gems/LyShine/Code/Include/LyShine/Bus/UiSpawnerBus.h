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
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceInstantiationTicket.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UiSpawnerComponent
class UiSpawnerInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiSpawnerInterface() {}

    //! Spawn the selected slice at the entity's location
    virtual AzFramework::SliceInstantiationTicket Spawn() = 0;

    //! Spawn the selected slice at the entity's location with the provided relative offset
    virtual AzFramework::SliceInstantiationTicket SpawnRelative(const AZ::Vector2& relative) = 0;

    //! Spawn the selected slice at the specified viewport position
    virtual AzFramework::SliceInstantiationTicket SpawnViewport(const AZ::Vector2& pos) { (void)pos; return AzFramework::SliceInstantiationTicket(); };

    //! Spawn the provided slice at the entity's location
    virtual AzFramework::SliceInstantiationTicket SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice) { (void)slice; return AzFramework::SliceInstantiationTicket(); };

    //! Spawn the provided slice at the entity's location with the provided relative offset
    virtual AzFramework::SliceInstantiationTicket SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& relative) { (void)slice; (void)relative; return AzFramework::SliceInstantiationTicket(); };

    //! Spawn the provided slice at the specified viewport position
    virtual AzFramework::SliceInstantiationTicket SpawnSliceViewport(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& pos) { (void)slice; (void)pos; return AzFramework::SliceInstantiationTicket(); };
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

    //! Notify that slice has been spawned, but entities have not yet been activated.
    //! OnEntitySpawned events are about to be dispatched.
    virtual void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    //! Notify that an entity has spawned, will be called once for each entity spawned in a slice.
    virtual void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& /*ticket*/, const AZ::EntityId& /*spawnedEntity*/) {}

    //! Single event notification for an entire slice spawn, providing a list of all resulting entity Ids.
    virtual void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& /*ticket*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    //! Single event notification for an entire slice spawn, providing a list of all resulting top-level entity Ids.
    //! Top-level entities are ones that are not the child of any other entity in the slice
    virtual void OnTopLevelEntitiesSpawned(const AzFramework::SliceInstantiationTicket& /*ticket*/, const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    //! Notify that a spawn has been completed. All spawn notifications for this ticket have been dispatched.
    virtual void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    //! Notify that slice has failed to be spawned.
    virtual void OnSpawnFailed(const AzFramework::SliceInstantiationTicket& /*ticket*/) {}
};

using UiSpawnerNotificationBus = AZ::EBus<UiSpawnerNotifications>;
