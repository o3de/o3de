/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Spawnable/Spawnable.h>

// Forward declarations
namespace AZ
{
    class Entity;
}

//! Simple spawn ticket ID for tracking UI spawn requests
using UiSpawnId = uint32_t;
static constexpr UiSpawnId InvalidUiSpawnId = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UI game entity context.
class UiGameEntityContextRequests
    : public AZ::EBusTraits
{
public:

    virtual ~UiGameEntityContextRequests() {}

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides. Accessed by EntityContextId
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef AzFramework::EntityContextId BusIdType;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //! Spawns entities from a spawnable asset asynchronously.
    //! \return a spawn ID identifying the spawn request.
    //!         Callers can immediately subscribe to the UiGameEntityContextSpawnResultsBus for this ID
    //!         to receive results for this specific request.
    virtual UiSpawnId SpawnSpawnable(
        const AZ::Data::Asset<AzFramework::Spawnable>& /*spawnableAsset*/,
        const AZ::Vector2& /*position*/,
        bool /*isViewportPosition*/,
        AZ::Entity* /*parent*/)
    { return InvalidUiSpawnId; }
};

using UiGameEntityContextBus = AZ::EBus<UiGameEntityContextRequests>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving notifications from the UI game entity context component.
class UiGameEntityContextNotifications
    : public AZ::EBusTraits
{
public:

    virtual ~UiGameEntityContextNotifications() = default;

    /// Fired when entities have been successfully spawned.
    virtual void OnSpawnCompleted(UiSpawnId /*spawnId*/,
        const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    /// Fired when a spawn request could not be completed.
    virtual void OnSpawnFailed(UiSpawnId /*spawnId*/) {}
};

using UiGameEntityContextNotificationBus = AZ::EBus<UiGameEntityContextNotifications>;


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving per-spawn notifications from the UI game entity context component. This bus is used
//! by the UiSpawnerComponent that depends on the UiGameEntityContext fixing entities up before
//! it sends out notifications to listeners on the UiSpawnerNotificationBus
class UiGameEntityContextSpawnResults
    : public AZ::EBusTraits
{
public:

    virtual ~UiGameEntityContextSpawnResults() = default;

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides. Addressed by UiSpawnId
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef UiSpawnId BusIdType;
    //////////////////////////////////////////////////////////////////////////

    //! Signals that entities were successfully spawned and are ready.
    virtual void OnEntityContextSpawnCompleted(const AZStd::vector<AZ::EntityId>& /*spawnedEntities*/) {}

    //! Signals that a spawn request failed.
    virtual void OnEntityContextSpawnFailed() {}
};

using UiGameEntityContextSpawnResultsBus = AZ::EBus<UiGameEntityContextSpawnResults>;
