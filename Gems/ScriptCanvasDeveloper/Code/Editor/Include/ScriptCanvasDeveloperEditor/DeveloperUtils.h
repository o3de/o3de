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

#include "precompiled.h"

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvasDeveloperEditor
{
    class AutomationInterface
    {
    public:
        virtual void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& activeScriptCanvasId) = 0;
    };
    
    class ProcessNodePaletteInterface
        : public AutomationInterface
    {
    public:
        virtual bool ShouldProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) const = 0;
        virtual void ProcessItem(const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) = 0;
    };
    
    class ProcessVariablePaletteInterface
        : public AutomationInterface
    {
    public:
        virtual bool ShouldProcessVariableType(const ScriptCanvas::Data::Type& dataType) const = 0;
        virtual void ProcessVariableType(const ScriptCanvas::Data::Type& dataType) = 0;
    };
    
    class DeveloperUtils
    {
    public:
        static ScriptCanvasEditor::NodeIdPair HandleMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, GraphCanvas::GraphId graphCanvasGraphId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing);

        static void UpdateViewportPositionOffsetForNode(GraphCanvas::NodeId nodeId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing);

        static void ProcessNodePalette(ProcessNodePaletteInterface& processNodePaletteInterface);
        static void ProcessVariablePalette(ProcessVariablePaletteInterface& processVariablePaletteInterface);
    };
}
