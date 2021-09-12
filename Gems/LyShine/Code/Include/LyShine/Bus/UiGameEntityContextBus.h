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
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>

// Forward declarations
namespace AZ
{
    class Entity;
}

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

    //! Instantiates a dynamic slice asynchronously.
    //! \return a ticket identifying the spawn request.
    //!         Callers can immediately subscribe to the SliceInstantiationResultBus for this ticket
    //!         to receive result for this specific request.
    virtual AzFramework::SliceInstantiationTicket InstantiateDynamicSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/,
        const AZ::Vector2& /*position*/,
        bool /*isViewportPosition*/,
        AZ::Entity* /*parent*/,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& /*customIdMapper*/)
    { return AzFramework::SliceInstantiationTicket(); }
};

using UiGameEntityContextBus = AZ::EBus<UiGameEntityContextRequests>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving notifications from the UI game entity context component.
class UiGameEntityContextNotifications
    : public AZ::EBusTraits
{
public:

    virtual ~UiGameEntityContextNotifications() = default;

    /// Fired when a slice has been successfully instantiated.
    virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/,
        const AZ::SliceComponent::SliceInstanceAddress& /*instance*/,
        const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    /// Fired when a slice asset could not be instantiated.
    virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/,
        const AzFramework::SliceInstantiationTicket& /*ticket*/) {}
};

using UiGameEntityContextNotificationBus = AZ::EBus<UiGameEntityContextNotifications>;


////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving notifications from the UI game entity context component. This bus is used
//! by the UiSpawnerComponent that depends on the UiGameEntityContext fixing entities up before
//! it sends out notifications to listeners on the UiSpawnerNotificationBus
class UiGameEntityContextSliceInstantiationResults
    : public AZ::EBusTraits
{
public:

    virtual ~UiGameEntityContextSliceInstantiationResults() = default;

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides. Addressed by SliceInstantiationTicket
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef AzFramework::SliceInstantiationTicket BusIdType;
    //////////////////////////////////////////////////////////////////////////

    //! Signals that a slice was successfully instantiated prior to entity registration.
    virtual void OnEntityContextSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

    //! Signals that a slice was successfully instantiated after entity registration.
    virtual void OnEntityContextSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

    //! Signals that a slice could not be instantiated.
    virtual void OnEntityContextSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/) {}
};

using UiGameEntityContextSliceInstantiationResultsBus = AZ::EBus<UiGameEntityContextSliceInstantiationResults>;
