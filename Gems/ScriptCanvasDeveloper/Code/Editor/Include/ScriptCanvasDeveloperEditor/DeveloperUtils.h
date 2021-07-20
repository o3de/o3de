/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvasDeveloperEditor
{
    class AutomationInterface
    {
    public:
        virtual void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& activeScriptCanvasId) = 0;
        virtual void OnProcessingComplete() {};
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
        enum ConnectionStyle
        {
            NoConnections,
            SingleExecutionConnection
        };

        struct CreateConnectedChainConfig
        {
            bool m_skipHandlers = false;
            ConnectionStyle m_connectionStyle = ConnectionStyle::NoConnections;

            ScriptCanvasEditor::NodeIdPair m_fallbackNode;
            GraphCanvas::Endpoint m_previousEndpoint;            
        };

        static ScriptCanvasEditor::NodeIdPair HandleMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, GraphCanvas::GraphId graphCanvasGraphId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing);

        static void UpdateViewportPositionOffsetForNode(GraphCanvas::NodeId nodeId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing);
        static bool CreateConnectedChain(const ScriptCanvasEditor::NodeIdPair& nodeIdPair, CreateConnectedChainConfig& connectionConfig);

        static void ProcessNodePalette(ProcessNodePaletteInterface& processNodePaletteInterface);
        static void ProcessVariablePalette(ProcessVariablePaletteInterface& processVariablePaletteInterface);
    };
}
