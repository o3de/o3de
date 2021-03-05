/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        AZ_CLASS_ALLOCATOR(FindSelectedNodesAction, AZ::SystemAllocator, 0);

        FindSelectedNodesAction(QObject* parent);
        virtual ~FindSelectedNodesAction() = default;

        GraphCanvas::ActionGroupId GetActionGroupId() const override;
        void RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;
        GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        void UpdateActionState();
    };
}
