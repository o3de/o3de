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

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// GraphCanvas
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

// GraphModel
#include <GraphModel/Integration/EditorMainWindow.h>
#include <GraphModel/Model/Graph.h>

namespace GraphModelIntegration
{
    EditorMainWindow::EditorMainWindow(GraphCanvas::AssetEditorWindowConfig* config, QWidget* parent)
        : GraphCanvas::AssetEditorMainWindow(config, parent)
    {
    }

    EditorMainWindow::~EditorMainWindow()
    {
        GraphControllerNotificationBus::MultiHandler::BusDisconnect();
    }

    GraphModel::GraphPtr EditorMainWindow::GetGraphById(GraphCanvas::GraphId graphId) const
    {
        auto it = m_graphs.find(graphId);
        if (it != m_graphs.end())
        {
            return it->second;
        }

        return nullptr;
    }

    GraphCanvas::GraphId EditorMainWindow::GetGraphId(GraphModel::GraphPtr graph) const
    {
        auto it = AZStd::find_if(m_graphs.begin(), m_graphs.end(),
            [graph](decltype(m_graphs)::const_reference pair)
        {
            return graph == pair.second;
        });
        if (it != m_graphs.end())
        {
            return it->first;
        }

        return GraphCanvas::GraphId();
    }

    void EditorMainWindow::OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget)
    {
        GraphCanvas::AssetEditorMainWindow::OnEditorOpened(dockWidget);

        GraphCanvas::GraphId graphId = dockWidget->GetGraphId();

        // Create the new graph.
        GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(GetGraphContext());
        m_graphs[graphId] = graph;

        // Create the controller for the new graph.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(&GraphModelIntegration::GraphManagerRequests::CreateGraphController, graphId, graph);

        // Listen for GraphController notifications on the new graph.
        GraphModelIntegration::GraphControllerNotificationBus::MultiHandler::BusConnect(graphId);
    }

    void EditorMainWindow::OnEditorClosing(GraphCanvas::EditorDockWidget* dockWidget)
    {
        GraphCanvas::AssetEditorMainWindow::OnEditorClosing(dockWidget);

        GraphCanvas::GraphId graphId = dockWidget->GetGraphId();

        // Stop listening for GraphController notifications for this graph.
        GraphModelIntegration::GraphControllerNotificationBus::MultiHandler::BusDisconnect(graphId);

        // Remove the controller for this graph.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(&GraphModelIntegration::GraphManagerRequests::DeleteGraphController, graphId);

        // Delete the graph that was created.
        m_graphs.erase(graphId);
    }

    void EditorMainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        // Find the GraphModel::NodePtr whose action widget was clicked.
        GraphCanvas::GraphId graphId = GetActiveGraphCanvasGraphId();
        GraphModel::NodePtr node;
        GraphControllerRequestBus::EventResult(node, graphId, &GraphControllerRequests::GetNodeById, wrapperNode);
        AZ_Assert(node, "Unable to find NodePtr for the given NodeId");

        // Invoke the handler so that the client can handle this event if necessary.
        HandleWrapperNodeActionWidgetClicked(node, actionWidgetBoundingRect, scenePoint, screenPoint);
    }
}
