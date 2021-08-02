/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>

namespace LandscapeCanvasEditor
{
    class NodeContextMenu
        : public GraphCanvas::NodeContextMenu
    {
    public:
        NodeContextMenu(const AZ::EntityId& sceneId, QWidget* parent = nullptr);

    protected:
        void OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId) override;
    };
}
