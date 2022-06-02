/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#endif

namespace AtomToolsFramework
{
    // Settings for initializing graph canvas and node palettes
    struct GraphViewConfig
    {
        // Full path to the graph canvas style manager settings
        AZStd::string m_styleManagerPath;
        // Full path to translation settings
        AZStd::string m_translationPath;
        // Mime type identifying compatibility between nodes dragged from the node palette to the current graph view
        AZStd::string m_nodeMimeType;
        // String identifier used to save settings for graph canvas context menus
        AZStd::string m_nodeSaveIdentifier;
        // Callback function used to create node palette items
        AZStd::function<GraphCanvas::GraphCanvasTreeItem*(const AZ::Crc32&)> m_createNodeTreeItemsFn;
    };
} // namespace AtomToolsFramework
