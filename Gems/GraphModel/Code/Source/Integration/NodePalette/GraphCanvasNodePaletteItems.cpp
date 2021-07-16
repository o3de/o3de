/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
