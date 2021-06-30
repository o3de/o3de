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
    {
        Q_OBJECT;
	public:
       explicit PrefabDependencyViewerWidget(QWidget* pParent = NULL);
       virtual ~PrefabDependencyViewerWidget();

       /** Sets up the GraphCanvas UI without the Node Palette. */
       virtual void SetupUI();

    protected:
       /** Overriding RefreshMenu in order to remove the
       unnecessary menu bar on the top. As a bonus, this
       also removes the ability to revive NodePalette from the UI. */
       virtual void RefreshMenu() override {}
       virtual void OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget) override;
        /* void displayText();
       virtual void displayTree(AzToolsFramework::Prefab::Instance& prefab) override;
       virtual void displayTree(AzToolsFramework::Prefab::PrefabDom& prefab) override;
       */
    };
}; // namespace AzToolsFramework


