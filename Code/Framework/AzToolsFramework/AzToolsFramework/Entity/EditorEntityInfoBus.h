/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorEntitySortBus.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityStartStatus.h>

namespace AzToolsFramework
{
    /**
     * This bus can be used to query any Entity in the EditorEntityContext
     * It contains a hierarchy derived from the transform hierarchy but also provides entity sort information at each level
     * The results returned here are cached and very efficient compared to listening and deriving the data from existing events
     */
    class EditorEntityInfoRequests : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityInfoRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        // EntityId -> Parent Info
        virtual AZ::EntityId GetParent() const = 0;

        // EntityId -> Child Info
        virtual EntityIdList GetChildren() const = 0;
        virtual AZ::EntityId GetChild(AZStd::size_t index) const = 0;
        virtual AZStd::size_t GetChildCount() const = 0;
        virtual AZ::u64 GetChildIndex(AZ::EntityId childId) const = 0;

        virtual AZStd::string GetName() const = 0;

        // EntityId -> Slice Info
        virtual AZStd::string GetSliceAssetName() const = 0;
        virtual bool IsSliceEntity() const = 0;
        virtual bool IsSubsliceEntity() const = 0;
        virtual bool IsSliceRoot() const = 0;
        virtual bool IsSubsliceRoot() const = 0;
        virtual bool HasSliceEntityAnyChildrenAddedOrDeleted() const = 0;
        virtual bool HasSliceEntityPropertyOverridesInTopLevel() const = 0;
        virtual bool HasSliceEntityOverrides() const = 0;
        virtual bool HasSliceChildrenOverrides() const = 0;
        virtual bool HasSliceAnyOverrides() const = 0;
        virtual bool HasCyclicDependency() const = 0;
        virtual void AddToCyclicDependencyList(const AZ::EntityId& entityId) = 0;
        virtual void RemoveFromCyclicDependencyList(const AZ::EntityId& entityId) = 0;
        virtual AzToolsFramework::EntityIdList GetCyclicDependencyList() const = 0;

        // EntityId -> Order Info
        virtual AZ::u64 GetIndexForSorting() const = 0;

        virtual bool IsSelected() const = 0;
        virtual bool IsVisible() const = 0;
        virtual bool IsHidden() const = 0;
        virtual bool IsLocked() const = 0;
        virtual EditorEntityStartStatus GetStartStatus() const = 0;

        // Locked layers override the lock status of entities, but shouldn't change
        // that entity's lock state. This allows layers to have their lock toggled on and
        // off while preserving the state of the entities within.
        // This check allows the outliner to detect if this entity was specifically locked,
        // so it can render that state. Everywhere else should use the IsLocked check, which checks entities.
        virtual bool IsJustThisEntityLocked() const = 0;

        virtual bool IsComponentExpanded(AZ::ComponentId id) const = 0;
        virtual void SetComponentExpanded(AZ::ComponentId id, bool expanded) = 0;
    };
    using EditorEntityInfoRequestBus = AZ::EBus<EditorEntityInfoRequests>;

    /**
     * This bus will be used to notify handlers about changes that occur that will change the result of InfoRequest calls.
     * These will generally line up one to one with the above InfoRequest calls.
     * Calling the appropriate info bus method during or after the notification event will return the new result.
     */
    class EditorEntityInfoNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityInfoNotifications() = default;
        virtual void OnEntityInfoResetBegin() {}
        virtual void OnEntityInfoResetEnd() {}
        virtual void OnEntityInfoUpdatedAddChildBegin(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) {}
        virtual void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) {}
        virtual void OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) {}
        virtual void OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) {}
        virtual void OnEntityInfoUpdatedOrderBegin(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/, AZ::u64 /*index*/) {}
        virtual void OnEntityInfoUpdatedOrderEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/, AZ::u64 /*index*/) {}
        virtual void OnEntityInfoUpdatedSelection(AZ::EntityId /*entityId*/, bool /*selected*/){}
        virtual void OnEntityInfoUpdatedLocked(AZ::EntityId /*entityId*/, bool /*locked*/){}
        virtual void OnEntityInfoUpdatedVisibility(AZ::EntityId /*entityId*/, bool /*visible*/){}
        virtual void OnEntityInfoUpdatedName(AZ::EntityId /*entityId*/, const AZStd::string& /*name*/){}
        virtual void OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId /*entityId*/) {}
        virtual void OnEntityInfoUpdateSliceOwnership(AZ::EntityId /*entityId*/){}
    };
    using EditorEntityInfoNotificationBus = AZ::EBus<EditorEntityInfoNotifications>;
}

DECLARE_EBUS_EXTERN(AzToolsFramework::EditorEntityInfoRequests);
