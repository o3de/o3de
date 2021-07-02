/*
/* Copyright (c) Contributors to the Open 3D Engine Project
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*/

#pragma once

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <QWidget>
#include <PrefabDependencyViewerInterface.h>

namespace PrefabDependencyViewer
{
    struct PrefabDependencyViewerConfig
        : GraphCanvas::AssetEditorWindowConfig
    {
        /** Return an empty NodePalette tree */
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteRoot() override;
    };

    class PrefabDependencyViewerWidget 
        : public GraphCanvas::AssetEditorMainWindow
        , public PrefabDependencyViewerInterface
    {
        Q_OBJECT;
	public:
        explicit PrefabDependencyViewerWidget(QWidget* pParent = NULL);
        virtual ~PrefabDependencyViewerWidget();

        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Sets up the GraphCanvas UI without the Node Palette. */
        virtual void SetupUI() override;

        ////////////////// PrefabDependencyViewerWidget overrides ////////////////////
        virtual void DisplayTree(const AzToolsFramework::Prefab::TemplateId& tid) override;

        void CreateNodeUi(const AzToolsFramework::Prefab::TemplateId& tid);

    protected:
        ////////////////// GraphCanvas::AssetEditorMainWindow overrides //////////////
        /** Overriding RefreshMenu in order to remove the
        unnecessary menu bar on the top. As a bonus, this
        also removes the ability to revive NodePalette from the UI. */
        virtual void RefreshMenu() override {}

    private:
        AZ::EntityId m_sceneId;
    };
}; // namespace AzToolsFramework


