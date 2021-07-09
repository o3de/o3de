/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Interface/Interface.h>
#include <Core/Core.h>
#include <PrefabDependencyViewerWidget.h>
#include <QBoxLayout>
#include <QLabel>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

namespace PrefabDependencyViewer
{
    GraphCanvas::GraphCanvasTreeItem* PrefabDependencyViewerConfig::CreateNodePaletteRoot()
    {
        const GraphCanvas::EditorId& editorId = PREFAB_DEPENDENCY_VIEWER_EDITOR_ID;
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", editorId);

        return rootItem;
    }

    /** Returns a bare minimum configuration to configure GraphCanvas UI
    for the purpose of visualizing the Prefab hierarchy. */
    PrefabDependencyViewerConfig* GetDefaultConfig()
    {
        // Make sure no memory leak happens here
        PrefabDependencyViewerConfig* config = new PrefabDependencyViewerConfig();
        config->m_editorId = PREFAB_DEPENDENCY_VIEWER_EDITOR_ID;
        config->m_baseStyleSheet = "PrefabDependencyViewer/StyleSheet/graphcanvas_style.json";

        return config;
    }

    PrefabDependencyViewerWidget::PrefabDependencyViewerWidget(QWidget* pParent)
        : GraphCanvas::AssetEditorMainWindow(GetDefaultConfig(), pParent)
    {
        AZ::Interface<PrefabDependencyViewerInterface>::Register(this);
    }

    void PrefabDependencyViewerWidget::SetupUI() {
        GraphCanvas::AssetEditorMainWindow::SetupUI();
        delete m_nodePalette;
        m_nodePalette = nullptr;
    }

    void PrefabDependencyViewerWidget::DisplayTree([[maybe_unused]]const Utils::DirectedGraph& graph)
    {
        m_sceneId = CreateNewGraph();
        CreateNodeUi(AzToolsFramework::Prefab::InvalidTemplateId);
    }

    void PrefabDependencyViewerWidget::CreateNodeUi([[maybe_unused]]const AzToolsFramework::Prefab::TemplateId& tid)
    {
        const char* nodeStyle = "";
        const AZ::Entity* graphCanvasNode = nullptr;;

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
            graphCanvasNode,
            &GraphCanvas::GraphCanvasRequests::CreateGeneralNodeAndActivate,
            nodeStyle);

        AZ_Assert(graphCanvasNode, "Unable to create GraphCanvas Node");

        AZ::EntityId nodeUiId = graphCanvasNode->GetId();
        GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetTitle, "Prefab");

        GraphCanvas::SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::AddNode, nodeUiId, AZ::Vector2(5, 5), false);
        GraphCanvas::SceneMemberUIRequestBus::Event(nodeUiId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        // AddNodeUiToScene()
    }

    PrefabDependencyViewerWidget::~PrefabDependencyViewerWidget() {
        AZ::Interface<PrefabDependencyViewerInterface>::Unregister(this);
    }
}

// Qt best practice for Q_OBJECT macro issues. File available at compile time.
#include <moc_PrefabDependencyViewerWidget.cpp>


