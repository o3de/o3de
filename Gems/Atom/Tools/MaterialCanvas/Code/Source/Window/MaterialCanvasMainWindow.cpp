/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

#include <QMessageBox>

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, "MaterialCanvasMainWindow",  parent)
        , m_styleManager(toolId, "MaterialCanvas/StyleSheet/graphcanvas_style.json")
    {
        m_toolBar = new MaterialCanvasToolBar(m_toolId, this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/MaterialInspector");
        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea);

        m_materialViewport = new MaterialCanvasViewportWidget(m_toolId, this);
        m_materialViewport->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        AddDockWidget("Viewport", m_materialViewport, Qt::RightDockWidgetArea);

        AddDockWidget("Viewport Settings", new ViewportSettingsInspector(m_toolId, this), Qt::LeftDockWidgetArea);
        SetDockWidgetVisible("Viewport Settings", false);

        AddDockWidget("MiniMap", aznew GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::RightDockWidgetArea);

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget("Bookmarks", m_bookmarkDockWidget, Qt::BottomDockWidgetArea);

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = GetNodePaletteRootTreeItem();
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = "materialcanvas/node-palette-mime-event";
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = "MaterialCanvas_ContextMenu";

        m_nodePalette = aznew GraphCanvas::NodePaletteDockWidget(this, "Node Palette", nodePaletteConfig);
        AddDockWidget("Node Palette", m_nodePalette, Qt::LeftDockWidgetArea);

        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(
            "@products@/translation/materialcanvas_en_us.qm", unresolvedPath.data(), unresolvedPath.size());

        QString translationFilePath(unresolvedPath.data());
        if (m_translator.load(QLocale::Language::English, translationFilePath))
        {
            if (!qApp->installTranslator(&m_translator))
            {
                AZ_Warning("MaterialCanvas", false, "Error installing translation %s!", unresolvedPath.data());
            }
        }
        else
        {
            AZ_Warning("MaterialCanvas", false, "Error loading translation file %s", unresolvedPath.data());
        }

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialCanvasMainWindow::~MaterialCanvasMainWindow()
    {
    }

    void MaterialCanvasMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_materialInspector->SetDocumentId(documentId);
    }

    void MaterialCanvasMainWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_materialViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(
            m_materialViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(), m_materialViewport->size().width(),
            m_materialViewport->size().height());

        [[maybe_unused]] QSize newDeviceSize = m_materialViewport->size();
        AZ_Warning(
            "Material Canvas",
            static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialCanvasMainWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialCanvasMainWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    void MaterialCanvasMainWindow::OpenSettings()
    {
    }

    void MaterialCanvasMainWindow::OpenHelp()
    {
        QMessageBox::information(
            this, windowTitle(),
            R"(<html><head/><body>
            <p><h3><u>Material Canvas Controls</u></h3></p>
            <p><b>LMB</b> - pan camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> or <b>Alt+MMB</b> - move camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)");
    }

    GraphCanvas::GraphCanvasTreeItem* MaterialCanvasMainWindow::GetNodePaletteRootTreeItem() const
    {
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", m_toolId);
        auto nodeCategory1 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 1", m_toolId);
        nodeCategory1->SetTitlePalette("NodeCategory1");
        auto nodeCategory2 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 2", m_toolId);
        nodeCategory2->SetTitlePalette("NodeCategory2");
        auto nodeCategory3 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 3", m_toolId);
        nodeCategory3->SetTitlePalette("NodeCategory3");
        auto nodeCategory4 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 4", m_toolId);
        nodeCategory4->SetTitlePalette("NodeCategory4");
        auto nodeCategory5 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 5", m_toolId);
        nodeCategory5->SetTitlePalette("NodeCategory5");
        return rootItem;
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
