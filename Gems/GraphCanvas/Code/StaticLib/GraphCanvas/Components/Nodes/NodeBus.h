/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace AZ
{
    class any;
}

namespace GraphCanvas
{
    class Scene;

    //! NodeRequests
    //! Requests that get or set the properties of a node.
    class NodeRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the tooltip for the node, which will display when the mouse is over the node but not a child item.
        virtual void SetTooltip(const AZStd::string&) = 0;

        //! Get the tooltip that is currently set for the node.
        virtual const AZStd::string GetTooltip() const = 0;

        //! Sets whether or not the given node will display in the outliner.
        virtual void SetShowInOutliner(bool showInOutliner) = 0;

        //! Get whether to show this node in the outliner or not
        virtual bool ShowInOutliner() const = 0;

        //! Add a slot entity to the node.
        //! The node will manage the slot entity and its entity life-cycle will be linked to that of the node. If the
        //! slot must outlive the node, then it will need to be removed before the node is destroyed.
        virtual void AddSlot(const AZ::EntityId&) = 0;
        //! Remove a slot from the node.
        virtual void RemoveSlot(const AZ::EntityId&) = 0;

        //! Obtain a collection of the entity IDs of the slots owned by a node.
        virtual AZStd::vector<AZ::EntityId> GetSlotIds() const = 0;

        virtual AZStd::vector< SlotId > GetVisibleSlotIds() const = 0;

        virtual AZStd::vector<SlotId> FindVisibleSlotIdsByType(const ConnectionType& connectionType, const SlotType& slotType) const = 0;

        virtual bool HasConnections() const = 0;

        //! Get user data from this node
        virtual AZStd::any* GetUserData() = 0;

        //! Returns whether or not the Node is currently wrapped.
        virtual bool IsWrapped() const = 0;

        virtual void SetWrappingNode(const AZ::EntityId& wrappingNode) = 0;

        virtual AZ::EntityId GetWrappingNode() const = 0;

        // Signals that this node is involved in a batched connection manipulation action
        // This is usually splicing of some sort(deleting and adding a connection).
        //
        // Mainly there to postpone any updates that might occur while editing connections
        virtual void SignalBatchedConnectionManipulationBegin() = 0;
        virtual void SignalBatchedConnectionManipulationEnd() = 0;

        // Used to signal the node that a connection that belongs to it is beginning to be manipulated.
        virtual void SignalConnectionMoveBegin(const ConnectionId& connectionId) = 0;

        // Will attempt to update the partially disabled state based on the connection Execution connections
        virtual RootGraphicsItemEnabledState UpdateEnabledState() = 0;

        virtual bool HasHideableSlots() const = 0;
        virtual bool IsHidingUnusedSlots() const = 0;
        virtual void ShowAllSlots() = 0;
        virtual void HideUnusedSlots() = 0;

        virtual void SignalNodeAboutToBeDeleted() = 0;
    };

    using NodeRequestBus = AZ::EBus<NodeRequests>;

    //! NodeNotifications
    //! Notifications about changes to the state of nodes
    class NodeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Emitted when the node is added to a scene
        virtual void OnAddedToScene(const AZ::EntityId& /*sceneId*/) {}

        //! Emitted when a node is about to be deleted
        virtual void OnNodeAboutToBeDeleted() {}

        //! Emitted when the name of a node changes.
        virtual void OnNameChanged(const AZStd::string&) {}
        //! Emitted when the description of the node is changed.
        virtual void OnDescriptionChanged(const AZStd::string&) {}
        //! When the tooltip of the node is changed, this is emitted.
        virtual void OnTooltipChanged(const AZStd::string&) {}

        //! The addition of a slot to the node causes the emission of this event.
        //! # Parameters
        //! 1. The entity ID of the slot that was added.
        virtual void OnSlotAddedToNode(const AZ::EntityId&) {}

        //! The removal of a slot to the node causes the emission of this event.
        //! # Parameters
        //! 1. The entity ID of the slot that was removed.
        virtual void OnSlotRemovedFromNode(const AZ::EntityId&) {}

        virtual void OnNodeActivated() {}

        virtual void OnNodeWrapped(const AZ::EntityId& /*wrappingNode*/) {}
        virtual void OnNodeUnwrapped(const AZ::EntityId& /*wrappingNode*/) {}

        //! Signals that some batched connection manipulation operation is going on involving this node
        virtual void OnBatchedConnectionManipulationBegin() {};
        virtual void OnBatchedConnectionManipulationEnd() {};
    };

    using NodeNotificationBus = AZ::EBus<NodeNotifications>;

    class NodeSaveData
        : public ComponentSaveData
    {
    public:
        AZ_RTTI(NodeSaveData, "{24CB38BB-1705-4EC5-8F63-B574571B4DCD}", ComponentSaveData);
        AZ_CLASS_ALLOCATOR(NodeSaveData, AZ::SystemAllocator, 0);

        NodeSaveData() = default;
        ~NodeSaveData() = default;

        bool m_hideUnusedSlots = false;
    };
}
