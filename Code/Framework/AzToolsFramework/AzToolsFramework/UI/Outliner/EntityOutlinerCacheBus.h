/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QModelIndex>

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{

    class EntityOutlinerCacheRequests
        : public AZ::EBusTraits
    {
    public:
        /// Request selection of the item at the given cache index
        virtual void SelectOutlinerCache(QModelIndex index) = 0;

        /// Request deselection of the item at the given cache index
        virtual void DeselectOutlinerCache(QModelIndex index) = 0;
    };

    /// \ref EditorVisibilityRequests
    using EntityOutlinerCacheRequestBus = AZ::EBus<EntityOutlinerCacheRequests>;

    /**
    * Messages dispatched when an entity has sustained changes that require
    * it be redrawn in the outliner.
    */
    class EntityOutlinerCacheNotifications
        : public AZ::EBusTraits
    {
    public:
        /// The entity has changed in such a way that its outliner representation has changed and should be redrawn
        /// invalidate now is true if the item needs to be redrawn immediately.
        virtual void EntityCacheChanged(const AZ::EntityId& /*entityId*/) {}

        /// The outliner cache item associated with the given entity has been selected
        /// and is requesting that a notification be sent to the tree view.
        /// These requests should be handled, considered, and either acted on or queued
        virtual void EntityCacheSelectionRequest(const AZ::EntityId&  /*entityId*/) {}

        /// The outliner cache item associated with the given entity has been deselected
        /// and is requesting that the a notification be sent to the tree view.
        /// These requests should be handled, considered, and either acted on or queued
        virtual void EntityCacheDeselectionRequest(const AZ::EntityId&  /*entityId*/) {}
    };

    /// \ref EditorVisibilityNotifications
    using EntityOutlinerCacheNotificationBus = AZ::EBus<EntityOutlinerCacheNotifications>;

    class EntityOutlinerModelNotifications
        : public AZ::EBusTraits
    {
    public:

        /// The outliner cache item associated with the given entity has been selected
        /// and is requesting that a notification be sent to the tree view.
        /// These requests should be handled, considered, and either acted on or queued
        virtual void ModelEntitySelectionChanged(const AZStd::unordered_set<AZ::EntityId>& /*selectedEntityIdList*/, const AZStd::unordered_set<AZ::EntityId>& /*deselectedEntityIdList*/) {}
        virtual void QueueScrollToNewContent(const AZ::EntityId& /*entityId*/) {}
    };

    /// \ref EditorVisibilityNotifications
    using EntityOutlinerModelNotificationBus = AZ::EBus<EntityOutlinerModelNotifications>;

}
