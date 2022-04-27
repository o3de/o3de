/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasViewportContent.h>

#include <QMessageBox>

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(
        const AZ::Crc32& toolId, const CreateNodePaletteItemsCallback& createNodePaletteItemsCallback, QWidget* parent)
        : Base(toolId, "MaterialCanvasMainWindow", parent)
        , m_styleManager(toolId, "MaterialCanvas/StyleSheet/graphcanvas_style.json")
    {
        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);

        m_toolBar = new AtomToolsFramework::EntityPreviewViewportToolBar(m_toolId, this);
        addToolBar(m_toolBar);

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/MaterialInspector");
        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea);

        // Set up the dockable viewport widget
        m_materialViewport = new AtomToolsFramework::EntityPreviewViewportWidget(m_toolId, this);

        // Initialize the entity context that will be used to create all of the entities displayed in the viewport
        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();

        // Initialize the atom scene and pipeline that will bind to the viewport window to render entities and presets
        auto viewportScene = AZStd::make_shared<AtomToolsFramework::EntityPreviewViewportScene>(
            m_toolId, m_materialViewport, entityContext, "MaterialCanvasViewportWidget", "passes/MainRenderPipeline.azasset");

        // Viewport content will instantiate all of the entities that will be displayed and controlled by the viewport
        auto viewportContent = AZStd::make_shared<MaterialCanvasViewportContent>(m_toolId, m_materialViewport, entityContext);

        // The input controller creates and binds input behaviors to control viewport objects
        auto viewportController = AZStd::make_shared<AtomToolsFramework::EntityPreviewViewportInputController>(m_toolId, m_materialViewport, viewportContent);

        // Inject the entity context, scene, content, and controller into the viewport widget
        m_materialViewport->Init(entityContext, viewportScene, viewportContent, viewportController);

        AddDockWidget("Viewport", m_materialViewport, Qt::RightDockWidgetArea);

        m_viewportSettingsInspector = new AtomToolsFramework::EntityPreviewViewportSettingsInspector(m_toolId, this);
        AddDockWidget("Viewport Settings", m_viewportSettingsInspector, Qt::LeftDockWidgetArea);
        SetDockWidgetVisible("Viewport Settings", false);

        AddDockWidget("MiniMap", aznew GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::RightDockWidgetArea);

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget("Bookmarks", m_bookmarkDockWidget, Qt::BottomDockWidgetArea);

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = createNodePaletteItemsCallback(m_toolId);
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

    void MaterialCanvasMainWindow::OnDocumentCleared(const AZ::Uuid& documentId)
    {
        Base::OnDocumentCleared(documentId);
        m_materialInspector->SetDocumentId(documentId);
    }

    void MaterialCanvasMainWindow::OnDocumentError(const AZ::Uuid& documentId)
    {
        Base::OnDocumentError(documentId);
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
            <p><h3><u>Camera Controls</u></h3></p>
            <p><b>LMB</b> - rotate camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> or <b>Alt+MMB</b> - pan camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)");
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
