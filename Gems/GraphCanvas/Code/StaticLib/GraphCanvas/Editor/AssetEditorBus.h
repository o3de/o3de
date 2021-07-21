/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Types/Types.h>

namespace AZ
{
    class Entity;
}

class QToolButton;

namespace GraphCanvas
{
    class ConstructTypePresetBucket;
    class EditorContextMenu;
    class EditorConstructPresets;    

    class AssetEditorSettingsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = EditorId;

        //! Get the snapping distance for connections around slots
        virtual double GetSnapDistance() const { return 10.0; }

        virtual bool IsGroupDoubleClickCollapseEnabled() const { return true; };

        virtual bool IsBookmarkViewportControlEnabled() const { return false; };

        // Various advance connection feature controls.
        virtual bool IsDragNodeCouplingEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDragCouplingTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsDragConnectionSpliceEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDragConnectionSpliceTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsDropConnectionSpliceEnabled() const { return true; }
        virtual AZStd::chrono::milliseconds GetDropConnectionSpliceTime() const { return AZStd::chrono::milliseconds(500); }

        virtual bool IsSplicedNodeNudgingEnabled() const
        {
            return false;
        }

        virtual bool IsNodeNudgingEnabled() const { return IsSplicedNodeNudgingEnabled(); }

        // Shake Configuration
        virtual bool IsShakeToDespliceEnabled() const { return false; }

        virtual int GetShakesToDesplice() const { return 3; };

        // REturns the minimum amount of distance and object must move in order for it to be considered
        // a shake.
        virtual float GetMinimumShakePercent() const { return 40.0f; }

        // Returns the minimum amount of distance the cursor must move before shake processing begins
        virtual float GetShakeDeadZonePercent() const { return 20.0f; }

        // Returns how 'straight' the given shakes must be in order to be classified.
        virtual float GetShakeStraightnessPercent() const { return 0.75f; }

        virtual AZStd::chrono::milliseconds GetMaximumShakeDuration() const { return AZStd::chrono::milliseconds(1000); }

        // Alignment
        virtual AZStd::chrono::milliseconds GetAlignmentTime() const { return AZStd::chrono::milliseconds(250); }

        // Zoom Configuration
        //
        // These are scale values, so they are specifying the largest and smallest scale to be applied
        // to the individual objects. MinZoom implies the smallest element size, so the maximum amount you can zoom out to.
        // While MaxZoom represents the largest element size, so the maximum amount you can zoom in to.
        virtual float GetMaxZoom() const { return 2.0f; }

        // Edge of Screen Pan Configurations
        virtual float GetEdgePanningPercentage() const { return 0.1f; }
        virtual float GetEdgePanningScrollSpeed() const { return 100.0f; }        

        // Construct Presets
        virtual EditorConstructPresets* GetConstructPresets() const { return nullptr; }
        virtual const ConstructTypePresetBucket* GetConstructTypePresetBucket(ConstructType) const { return nullptr; }

        // Styling
        virtual Styling::ConnectionCurveType GetConnectionCurveType() const { return Styling::ConnectionCurveType::Straight; }
        virtual Styling::ConnectionCurveType GetDataConnectionCurveType() const { return Styling::ConnectionCurveType::Straight; }

        // Enable Node Disabling
        virtual bool AllowNodeDisabling() const { return false; }

        // Enable Reference Slots
        virtual bool AllowDataReferenceSlots() const { return false; }
    };

    using AssetEditorSettingsRequestBus = AZ::EBus<AssetEditorSettingsRequests>;

    class AssetEditorSettingsNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;        
        using BusIdType = EditorId;

        virtual void OnSettingsChanged() {}
    };

    using AssetEditorSettingsNotificationBus = AZ::EBus<AssetEditorSettingsNotifications>;

    // These are used to signal out to the editor on the whole, and general involve more singular elements rather then
    // per graph elements(so things like keeping track of which graph is active).
    class AssetEditorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        // Signal to the editor that a lot of selection events are going to be occurring
        // And certain actions can wait until these are complete before triggering the next state.
        virtual void OnSelectionManipulationBegin() {};
        virtual void OnSelectionManipulationEnd() {};

        //! Request to create a new Graph. Returns the GraphId that represents the newly created Graph.
        virtual GraphId CreateNewGraph() = 0;

        //! Returns whether or not this Asset Editor has an opened graph with the specified GraphId.
        virtual bool ContainsGraph(const GraphId& graphId) const = 0;

        //! Close a specified graph. Returns if the graph was closed.
        virtual bool CloseGraph(const GraphId& graphId) = 0;

        virtual void CustomizeConnectionEntity(AZ::Entity* connectionEntity)
        {
            AZ_UNUSED(connectionEntity);
        }

        virtual void ShowAssetPresetsMenu(ConstructType constructType)
        {
            AZ_UNUSED(constructType);
        }

        virtual ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
        {
            AZ_UNUSED(screenPoint);
            AZ_UNUSED(scenePoint);

            return ContextMenuAction::SceneReaction::Nothing;
        }

        virtual ContextMenuAction::SceneReaction ShowSceneContextMenuWithGroup(const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget)
        {
            AZ_UNUSED(groupTarget);

            return ShowSceneContextMenu(screenPoint, scenePoint);
        }

        virtual ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowCommentContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;
        virtual ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        virtual ContextMenuAction::SceneReaction ShowConnectionContextMenu(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
        {
            AZ_UNUSED(connectionId);
            AZ_UNUSED(screenPoint);
            AZ_UNUSED(scenePoint);

            return ContextMenuAction::SceneReaction::Nothing;
        }

        virtual ContextMenuAction::SceneReaction ShowConnectionContextMenuWithGroup(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget)
        {
            AZ_UNUSED(groupTarget);

            return ShowConnectionContextMenu(connectionId, screenPoint, scenePoint);
        }

        virtual ContextMenuAction::SceneReaction ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) = 0;

        //! This is sent when a Connection has no target.
        //! Returns the EntityId of the node create, if any.
        virtual Endpoint CreateNodeForProposal(const AZ::EntityId& connectionId, const Endpoint& endpoint, const QPointF& scenePosition, const QPoint& screenPosition)
        {
            AZ_UNUSED(connectionId);
            AZ_UNUSED(endpoint);
            AZ_UNUSED(scenePosition);
            AZ_UNUSED(screenPosition);

            return Endpoint();
        }

        virtual Endpoint CreateNodeForProposalWithGroup(const AZ::EntityId& connectionId, const Endpoint& endpoint, const QPointF& scenePosition, const QPoint& screenPosition, AZ::EntityId groupTarget)
        {
            AZ_UNUSED(groupTarget);

            return CreateNodeForProposal(connectionId, endpoint, scenePosition, screenPosition);
        }

        //! Callback for the Wrapper node action widgets
        virtual void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePosition, const QPoint& screenPosition) = 0;
    };

    using AssetEditorRequestBus = AZ::EBus< AssetEditorRequests >;

    class AssetEditorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual void OnGraphLoaded(const GraphId& /*graphId*/) {}
        virtual void OnGraphRefreshed(const GraphId& /*oldGraphId*/, const GraphId& /*newGraphId*/) {}
        virtual void OnGraphUnloaded(const GraphId& /*graphId*/) {}

        virtual void PreOnActiveGraphChanged() {}
        virtual void OnActiveGraphChanged(const AZ::EntityId& /*graphId*/) {}
        virtual void PostOnActiveGraphChanged() {}
    };

    using AssetEditorNotificationBus = AZ::EBus<AssetEditorNotifications>;

    // This one will use the same id'ing pattern but will be controlled by the EditorConstructPresets object.
    // For the creation through context menu.
    //
    // One off and Editor driven creations can be signalled when the changes are finalized in that Dialog.
    class AssetEditorPresetNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual void OnPresetsChanged() {};
        virtual void OnConstructPresetsChanged(ConstructType) {}
    };

    using AssetEditorPresetNotificationBus = AZ::EBus<AssetEditorPresetNotifications>;

    class AssetEditorAutomationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual bool RegisterObject(AZ::Crc32 elementId, QObject* object) = 0;
        virtual bool UnregisterObject(AZ::Crc32 elementId) = 0;

        virtual QObject* FindObject(AZ::Crc32 elementId) = 0;

        virtual QObject* FindElementByName(QString elementName) = 0;
    };

    using AssetEditorAutomationRequestBus = AZ::EBus<AssetEditorAutomationRequests>;
}
