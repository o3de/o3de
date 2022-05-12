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
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>

// Forward declarations
namespace AZ
{
    class Entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UI entity context.
//! There is one UiEntityContext per UI canvas.
class UiEntityContextRequests
    : public AZ::EBusTraits
{
public:

    virtual ~UiEntityContextRequests() {}

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides. Accessed by EntityContextId
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef AzFramework::EntityContextId BusIdType;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //! Creates an entity in a UI context.
    //! \return a new entity
    virtual AZ::Entity* CreateUiEntity(const char* name) = 0;

    //! Registers an existing entity with a UI context.
    virtual void AddUiEntity(AZ::Entity* entity) = 0;

    //! Registers an existing set of entities with a UI context.
    virtual void AddUiEntities(const AzFramework::EntityList& entities) = 0;

    //! Destroys an entity in a UI context.
    //! \return whether or not the entity was destroyed. A false return value signifies the entity did not belong to the UI context.
    virtual bool DestroyUiEntity(AZ::EntityId entityId) = 0;

    //! Clones a set of entities.
    //! \param sourceEntities - the source set of entities to clone
    //! \param resultEntities - the set of entities cloned from the source
    virtual bool CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities) = 0;

};

using UiEntityContextRequestBus = AZ::EBus<UiEntityContextRequests>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving events/notifications from the UI entity context
class UiEntityContextNotification
    : public AZ::EBusTraits
{
public:

    virtual ~UiEntityContextNotification() {};

    //! Fired when the context is being reset.
    virtual void OnContextReset() {}

    //! Fired when a slice has been successfully instantiated.
    virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    //! Fired when a slice has failed to instantiate.
    virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    //! Fired when the entity stream has been successfully loaded.
    virtual void OnEntityStreamLoadSuccess() {}

    //! Fired when the entity stream load has failed
    virtual void OnEntityStreamLoadFailed() {}
};

using UiEntityContextNotificationBus = AZ::EBus<UiEntityContextNotification>;
