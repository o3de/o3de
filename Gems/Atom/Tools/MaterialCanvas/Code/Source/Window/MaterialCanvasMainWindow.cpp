/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AzCore/IO/FileIO.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasViewportContent.h>

#include <QMessageBox>

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(
        const AZ::Crc32& toolId, const AtomToolsFramework::GraphViewConfig& graphViewConfig, QWidget* parent)
        : Base(toolId, "MaterialCanvasMainWindow", parent)
        , m_graphViewConfig(graphViewConfig)
        , m_styleManager(toolId, graphViewConfig.m_styleManagerPath)
    {
        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);

        m_documentInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/DocumentInspector");
        AddDockWidget("Inspector", m_documentInspector, Qt::RightDockWidgetArea);

        // Set up the toolbar that controls the viewport settings
        m_toolBar = new AtomToolsFramework::EntityPreviewViewportToolBar(m_toolId, this);

        // Create the dockable viewport widget that will be shared between all material canvas documents
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

        // Combine the shared toolbar in viewport into stacked widget that will be docked as a single view
        auto viewPortAndToolbar = new QWidget(this);
        viewPortAndToolbar->setLayout(new QVBoxLayout(viewPortAndToolbar));
        viewPortAndToolbar->layout()->setContentsMargins(0, 0, 0, 0);
        viewPortAndToolbar->layout()->setMargin(0);
        viewPortAndToolbar->layout()->setSpacing(0);
        viewPortAndToolbar->layout()->addWidget(m_toolBar);
        viewPortAndToolbar->layout()->addWidget(m_materialViewport);

        AddDockWidget("Viewport", viewPortAndToolbar, Qt::BottomDockWidgetArea);

        m_viewportSettingsInspector = new AtomToolsFramework::EntityPreviewViewportSettingsInspector(m_toolId, this);
        AddDockWidget("Viewport Settings", m_viewportSettingsInspector, Qt::LeftDockWidgetArea);
        SetDockWidgetVisible("Viewport Settings", false);

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget("Bookmarks", m_bookmarkDockWidget, Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("Bookmarks", false);

        AddDockWidget("MiniMap", aznew GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("MiniMap", false);

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = m_graphViewConfig.m_createNodeTreeItemsFn(m_toolId);
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewConfig.m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = m_graphViewConfig.m_nodeSaveIdentifier;

        m_nodePalette = aznew GraphCanvas::NodePaletteDockWidget(this, "Node Palette", nodePaletteConfig);
        AddDockWidget("Node Palette", m_nodePalette, Qt::LeftDockWidgetArea);

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(resolvedPath, m_graphViewConfig.m_translationPath.c_str());
        const AZ::IO::FixedMaxPathString translationFilePath = resolvedPath.LexicallyNormal().FixedMaxPathString();
        if (m_translator.load(QLocale::Language::English, translationFilePath.c_str()))
        {
            if (!qApp->installTranslator(&m_translator))
            {
                AZ_Warning("MaterialCanvas", false, "Error installing translation %s!", translationFilePath.c_str());
            }
        }
        else
        {
            AZ_Warning("MaterialCanvas", false, "Error loading translation file %s", translationFilePath.c_str());
        }

        // Set up style sheet to fix highlighting in the node palette
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral(":/GraphView/GraphView.qss"));

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void MaterialCanvasMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_documentInspector->SetDocumentId(documentId);
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

    AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>> MaterialCanvasMainWindow::GetSettingsDialogGroups() const
    {
        AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>> groups;
        groups.push_back(AtomToolsFramework::CreateSettingsGroup(
            "Material Canvas Settings",
            "Material Canvas Settings",
           {
                AtomToolsFramework::CreatePropertyFromSetting(
                    "/O3DE/Atom/MaterialCanvas/EnableMinimalShaderBuilds",
                    "Enable Minimal Shader Builds",
                    "Improve shader and material iteration and preview times by limiting the asset processor and shader compiler to the "
                    "current platform and RHI. Changing this setting requires restarting Material Canvas and the asset processor.",
                    false),
            }));

        // Add base class settings after app specific settings
        AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>> groupsFromBase = Base::GetSettingsDialogGroups();
        groups.insert(groups.end(), groupsFromBase.begin(), groupsFromBase.end());
        return groups;
    }

    AZStd::string MaterialCanvasMainWindow::GetHelpDialogText() const
    {
        return R"(<html><head/><body>
            <p><h3><u>Shader Build Settings</u></h3></p>
            <p>Shaders, materials, and other assets will be generated as changes are applied to the graph.
            The viewport will update and display the generated materials and shaders once they have been
            compiled by the Asset Processor. This can take a few seconds. Compilation times and preview
            responsiveness can be improved by enabling the Minimal Shader Build settings in the Tools->Settings
            menu. Changing the settings will require restarting Material Canvas and the Asset Processor.</p>
            <p><h3><u>Camera Controls</u></h3></p>
            <p><b>LMB</b> - rotate camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> - pan camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)";
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
