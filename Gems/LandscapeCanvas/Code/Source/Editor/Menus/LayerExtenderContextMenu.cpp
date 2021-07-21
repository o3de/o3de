/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Core/Core.h>
#include <Editor/Menus/LayerExtenderContextMenu.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>

namespace LandscapeCanvasEditor
{
    LayerExtenderContextMenu::LayerExtenderContextMenu(const GraphCanvas::NodePaletteConfig& nodePaletteConfig, QWidget* parent)
        : GraphCanvas::EditorContextMenu(LandscapeCanvas::LANDSCAPE_CANVAS_EDITOR_ID, parent)
    {
        AddNodePaletteMenuAction(nodePaletteConfig);
    }

    void LayerExtenderContextMenu::SetupDisplay()
    {
        GraphCanvas::EditorContextMenu::SetupDisplay();

        // Expand our node palette by default since it is the only thing showing and we don't have very many nodes to show
        m_nodePalette->GetTreeView()->expandAll();
    }

#include <Source/Editor/Menus/moc_LayerExtenderContextMenu.cpp>
}
