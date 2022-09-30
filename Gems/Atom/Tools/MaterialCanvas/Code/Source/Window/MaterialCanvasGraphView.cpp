/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Window/MaterialCanvasGraphView.h>

namespace MaterialCanvas
{
    MaterialCanvasGraphView::MaterialCanvasGraphView(
        const AZ::Crc32& toolId,
        const AZ::Uuid& documentId,
        const AtomToolsFramework::GraphViewConfig& graphViewConfig,
        QWidget* parent)
        : AtomToolsFramework::GraphView(toolId, GraphCanvas::GraphId(), graphViewConfig, parent)
        , m_documentId(documentId)
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        OnDocumentOpened(m_documentId);
        m_openedBefore = false;
    }

    MaterialCanvasGraphView::~MaterialCanvasGraphView()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasGraphView::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            GraphCanvas::GraphId activeGraphId = GraphCanvas::GraphId();
            MaterialCanvasDocumentRequestBus::EventResult(
                activeGraphId, m_documentId, &MaterialCanvasDocumentRequestBus::Events::GetGraphId);
            SetActiveGraphId(activeGraphId, true);

            // Show the entire graph and center the view the first time a graph is opened
            if (!m_openedBefore && activeGraphId.IsValid())
            {
                GraphCanvas::ViewId viewId;
                GraphCanvas::SceneRequestBus::EventResult(viewId, activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
                GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
                m_openedBefore = true;
            }
            return;
        }

        SetActiveGraphId(GraphCanvas::GraphId(), false);
    }

    void MaterialCanvasGraphView::OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
    }

    void MaterialCanvasGraphView::OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasGraphView.cpp>
