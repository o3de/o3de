/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace LandscapeCanvasEditor
{
    class FindSelectedNodesAction
        : public GraphCanvas::ContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(FindSelectedNodesAction, AZ::SystemAllocator);

        FindSelectedNodesAction(QObject* parent);
        virtual ~FindSelectedNodesAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using GraphCanvas::ContextMenuAction::RefreshAction;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using GraphCanvas::ContextMenuAction::TriggerAction;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        void UpdateActionState();
    };
}
