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

#include <AzCore/Interface/Interface.h>
#include <Core/Core.h>
#include <PrefabDependencyViewerWidget.h>
#include <QBoxLayout>
#include <QLabel>

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
        : GraphCanvas::AssetEditorMainWindow(GetDefaultConfig(), pParent) {}

    void PrefabDependencyViewerWidget::SetupUI() {
        GraphCanvas::AssetEditorMainWindow::SetupUI();
        delete m_nodePalette;
        m_nodePalette = nullptr;
    }

    void PrefabDependencyViewerWidget::OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget)
    {
        GraphCanvas::AssetEditorMainWindow::OnEditorOpened(dockWidget);
    }
        /* void PrefabDependencyViewerWidget::displayText()
    {
        
        setStyleSheet("QWidget{ background-color : rgba( 160, 160, 160, 255); border-radius : 7px;  }");
        QLabel* label = new QLabel(this);
        QHBoxLayout* layout = new QHBoxLayout();
        label->setText(AZStd::string("Random String").c_str());
        layout->addWidget(label);
        setLayout(layout);
    }

    void PrefabDependencyViewerWidget::displayTree(AzToolsFramework::Prefab::Instance& prefab)
    {
        setStyleSheet("QWidget{ background-color : rgba( 160, 160, 160, 255); border-radius : 7px;  }");
        QLabel* label = new QLabel(this);
        QLabel* label2 = new QLabel(this);
        QHBoxLayout* layout = new QHBoxLayout();

        label->setText(prefab.GetAbsoluteInstanceAliasPath().c_str());
        label2->setText(prefab.GetTemplateSourcePath().c_str());

        layout->addWidget(label);
        layout->addWidget(label2);
        setLayout(layout);
    }

    void PrefabDependencyViewerWidget::displayTree([[maybe_unused]]AzToolsFramework::Prefab::PrefabDom& prefab)
    {
        displayText();
        /*AZ_TracePrintf("Hello", "\n");
        setStyleSheet("QWidget{ background-color : rgba( 160, 160, 160, 255); border-radius : 7px;  }");
        QLabel* label = new QLabel(this);
        QLabel* label2 = new QLabel(this);
        QHBoxLayout* layout = new QHBoxLayout();

        label->setText();
        
        // label->setText(prefab["Source"]);


        layout->addWidget(label);
        layout->addWidget(label2);
        setLayout(layout);
        ///
    }
*/
    PrefabDependencyViewerWidget::~PrefabDependencyViewerWidget() {}
}

// Qt best practice for Q_OBJECT macro issues. File available at compile time.
#include <moc_PrefabDependencyViewerWidget.cpp>


