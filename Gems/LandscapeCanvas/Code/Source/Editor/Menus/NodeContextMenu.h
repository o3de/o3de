/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuAction.h>

namespace LandscapeCanvasEditor
{
    bool CreateRerouteOnSelectedConnections(const GraphCanvas::GraphId& graphId);

    class CreateRerouteConnectionAction
        : public GraphCanvas::ContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateRerouteConnectionAction, AZ::SystemAllocator);

        explicit CreateRerouteConnectionAction(QObject* parent);

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using GraphCanvas::ContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::ContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(
            const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;
    };

    class DissolveRerouteNodeAction
        : public GraphCanvas::NodeContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DissolveRerouteNodeAction, AZ::SystemAllocator);

        explicit DissolveRerouteNodeAction(QObject* parent);

        using GraphCanvas::NodeContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::NodeContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(
            const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;
    };

    class ConnectionContextMenu
        : public GraphCanvas::ConnectionContextMenu
    {
    public:
        explicit ConnectionContextMenu(QWidget* parent = nullptr);
    };

    class NodeContextMenu
        : public GraphCanvas::NodeContextMenu
    {
    public:
        NodeContextMenu(const AZ::EntityId& sceneId, QWidget* parent = nullptr);

    protected:
        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId) override;
    };
}
