/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ ...
#include <AzCore/std/containers/unordered_map.h>

// GraphCanvas ...
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>

// GraphModel ...
#include <GraphModel/GraphModelBus.h>

namespace GraphModelIntegration
{
    /**
    *   This class extends the base GraphCanvas windowing framework to integrate
    *   GraphModel functionality into the generic windowing framework.
    */
    class EditorMainWindow
        : public GraphCanvas::AssetEditorMainWindow
        , protected GraphControllerNotificationBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorMainWindow, AZ::SystemAllocator)
        explicit EditorMainWindow(GraphCanvas::AssetEditorWindowConfig* config, QWidget* parent = nullptr);
        ~EditorMainWindow() override;

    protected:
        /// Subclasses must implement this method so that this class can
        /// create graphs on their behalf.
        virtual GraphModel::GraphContextPtr GetGraphContext() const = 0;

        /// Helper method for retrieving the graph associated with a graphId.
        GraphModel::GraphPtr GetGraphById(GraphCanvas::GraphId graphId) const;

        /// Helper method for retrieving the graphId associated with a graph.
        GraphCanvas::GraphId GetGraphId(GraphModel::GraphPtr graph) const;

        // GraphCanvas::AssetEditorMainWindow overrides ...
        void OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget) override;
        void OnEditorClosing(GraphCanvas::EditorDockWidget* dockWidget) override;
        void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;

        /// Client can override this to handle click events on a wrapper node's action widget
        /// using a GraphModel::NodePtr instead of the lower-level GraphCanvas::NodeId
        virtual void HandleWrapperNodeActionWidgetClicked(GraphModel::NodePtr wrapperNode, [[maybe_unused]] const QRect& actionWidgetBoundingRect, [[maybe_unused]] const QPointF& scenePoint, [[maybe_unused]] const QPoint& screenPoint) {}

        /// Keep track of the graphs we create on behalf of the client when
        /// new editor dock widgets are created.
        AZStd::unordered_map<GraphCanvas::GraphId, GraphModel::GraphPtr> m_graphs;
    };
}
