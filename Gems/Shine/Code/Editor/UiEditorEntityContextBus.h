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
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

// Forward declarations
namespace AZ
{
    class Entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for making requests to the UI Editor entity context.
//! There is one UiEntityContext per UI canvas, if it is loaded in the editor then its context
//! will be a UiEditorEntityContext.
class UiEditorEntityContextRequests
    : public AZ::EBusTraits
{
public:

    virtual ~UiEditorEntityContextRequests() {}

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides. Accessed by EntityContextId
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef AzFramework::EntityContextId BusIdType;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //! Retrieves the list of child entities for the UI entity context.
    virtual AZStd::vector<AZ::Entity*>& GetChildEntities() = 0;

    //! Use an undoable command to delete the given entities
    virtual void DeleteElements(AzToolsFramework::EntityIdList elements) = 0;

    //! Query whether there are pending asynchronous requests waiting on the asset system
    virtual bool HasPendingRequests() = 0;
};

using UiEditorEntityContextRequestBus = AZ::EBus<UiEditorEntityContextRequests>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus for receiving events/notifications from the UI editor entity context component.
class UiEditorEntityContextNotification
    : public AZ::EBusTraits
{
public:

    virtual ~UiEditorEntityContextNotification() {};

    /// Fired when the context is being reset.
    virtual void OnContextReset() {}

    /// Fired when entities have been loaded into the context (e.g. from a prefab file).
    virtual void OnEntitiesLoaded(const AZStd::vector<AZ::EntityId>& /*entityIds*/) {}
};

using UiEditorEntityContextNotificationBus = AZ::EBus<UiEditorEntityContextNotification>;
