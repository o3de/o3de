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

    //! Retrieves the root slice for the UI entity context.
    virtual AZ::SliceComponent* GetUiRootSlice() = 0;

    //! Clones an existing slice instance in this UI entity context. New instance is immediately returned.
    //! \return address of new slice instance. A null address will be returned if the source instance address is invalid.
    virtual AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance) = 0;

    //! Instantiates a UI slice.
    virtual AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, AZ::Vector2 viewportPosition) = 0;

    /// Instantiates a UI slice at the specified child index
    virtual AzFramework::SliceInstantiationTicket InstantiateEditorSliceAtChildIndex(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
                                                                                        AZ::Vector2 viewportPosition,
                                                                                        int childIndex) = 0;

    //! Restores an entity back to a slice instance for undo/redo *only*. A valid \ref EntityRestoreInfo must be provided,
    //! and is only extracted directly via \ref SliceReference::GetEntityRestoreInfo().
    virtual void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info) = 0;

    /// Editor functionality to replace a set of entities with a new instance of a new slice asset.
    /// This is a deferred operation since the asset may not yet have been processed (i.e. new asset).
    /// Once the asset has been created, it will be loaded and instantiated.
    /// \param targetPath path to the slice asset to be instanced in-place over the specified entities.
    /// \param selectedToAssetMap relates selected (live) entity Ids to Ids in the slice asset for post-replace Id reference patching.
    /// \param entitiesToReplace contains entity Ids to be replaced.
    virtual void QueueSliceReplacement(const char* targetPath, 
                                        const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                                        const AZStd::unordered_set<AZ::EntityId>& entitiesToReplace,
                                        AZ::Entity* commonParent, AZ::Entity* insertBefore) = 0;

    //! Use an undoable command to delete the given entities
    virtual void DeleteElements(AzToolsFramework::EntityIdList elements) = 0;

    //! Query whether there are pending asynchronous requests waiting on the asset system
    virtual bool HasPendingRequests() = 0;

    //! Query whether there are slices being instantiated asynchronously
    virtual bool IsInstantiatingSlices() = 0;

    //! Detaches entities from their current slice instance and adds them to root slice as loose entities.
    virtual void DetachSliceEntities(const AzToolsFramework::EntityIdList& entities) = 0;
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

    /// Fired when a slice has been successfully instantiated.
    virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

    /// Fired when a slice has failed to instantiate.
    virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}
};

using UiEditorEntityContextNotificationBus = AZ::EBus<UiEditorEntityContextNotification>;
