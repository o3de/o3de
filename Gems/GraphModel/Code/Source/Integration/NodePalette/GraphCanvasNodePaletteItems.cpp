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

#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>

namespace GraphModelIntegration
{
    /// Add common utilities to a specific Node Palette tree.
    void AddCommonNodePaletteUtilities(GraphCanvas::GraphCanvasTreeItem* rootItem, const GraphCanvas::EditorId& editorId)
    {
        GraphCanvas::IconDecoratedNodePaletteTreeItem* utilitiesCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Utilities", editorId);
        utilitiesCategory->SetTitlePalette("UtilityNodeTitlePalette");
        utilitiesCategory->CreateChildNode<CommentNodePaletteTreeItem>("Comment", editorId);
        utilitiesCategory->CreateChildNode<NodeGroupNodePaletteTreeItem>("Node Group", editorId);
    }
}
