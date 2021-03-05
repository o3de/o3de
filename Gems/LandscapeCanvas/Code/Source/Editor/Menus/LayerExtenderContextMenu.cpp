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
