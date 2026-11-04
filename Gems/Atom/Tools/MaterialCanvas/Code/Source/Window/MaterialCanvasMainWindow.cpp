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
        const AZ::Crc32& toolId, AtomToolsFramework::GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent)
        : Base(toolId, "MaterialCanvasMainWindow", parent)
        , m_graphViewSettingsPtr(graphViewSettingsPtr)
        , m_styleManager(toolId, graphViewSettingsPtr->m_styleManagerPath)
    {
        m_assetBrowser->GetSearchWidget()->ClearTypeFilter();
        m_assetBrowser->GetSearchWidget()->SetTypeFilterVisible(false);
        m_assetBrowser->SetFileTypeFilters({
            { tr("Material").toUtf8().constData(), { "material" }, true },
            { tr("Material Graph").toUtf8().constData(), { "materialgraph", "materialgraphtemplate" }, true },
            { tr("Material Graph Node").toUtf8().constData(), { "materialgraphnode", "materialgraphnodetemplate" }, true },
            { tr("Material Type").toUtf8().constData(), { "materialtype" }, true },
            { tr("Material Pipeline").toUtf8().constData(), { "materialpipeline" }, true },
            { tr("Shader").toUtf8().constData(), { "shader" }, true },
            { tr("Shader Template").toUtf8().constData(), { "shader.template" }, true },
            { tr("Shader Variant List").toUtf8().constData(), { "shadervariantlist" }, true },
            { tr("Image").toUtf8().constData(), { "tif", "tiff", "png", "bmp", "jpg", "jpeg", "tga", "gif", "dds", "exr" }, true },
            { tr("Lua").toUtf8().constData(), { "lua" }, true },
            { tr("AZSL").toUtf8().constData(), { "azsl", "azsli", "srgi" }, true },
        });

        m_documentInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/DocumentInspector");
        AddDockWidget(tr("Inspector").toUtf8().constData(), m_documentInspector, Qt::RightDockWidgetArea);

        // Set up the toolbar that controls the viewport settings
        m_toolBar = new AtomToolsFramework::EntityPreviewViewportToolBar(m_toolId, this);

        // Create the dockable viewport widget that will be shared between all Material Canvas documents
        m_materialViewport = new AtomToolsFramework::EntityPreviewViewportWidget(m_toolId, this);

        // Initialize the entity context that will be used to create all of the entities displayed in the viewport
        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();

        // Initialize the atom scene and pipeline that will bind to the viewport window to render entities and presets
        auto viewportScene = AZStd::make_shared<AtomToolsFramework::EntityPreviewViewportScene>(
            m_toolId, m_materialViewport, entityContext, "MaterialCanvasViewportWidget", "passes/mainrenderpipeline.azasset");

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
        viewPortAndToolbar->layout()->setSpacing(0);
        viewPortAndToolbar->layout()->addWidget(m_toolBar);
        viewPortAndToolbar->layout()->addWidget(m_materialViewport);

        AddDockWidget(tr("Viewport").toUtf8().constData(), viewPortAndToolbar, Qt::BottomDockWidgetArea);

        m_viewportSettingsInspector = new AtomToolsFramework::EntityPreviewViewportSettingsInspector(m_toolId, this);
        AddDockWidget(tr("Viewport Settings").toUtf8().constData(), m_viewportSettingsInspector, Qt::LeftDockWidgetArea);
        SetDockWidgetVisible(tr("Viewport Settings").toUtf8().constData(), false);

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget(tr("Bookmarks").toUtf8().constData(), m_bookmarkDockWidget, Qt::BottomDockWidgetArea);
        SetDockWidgetVisible(tr("Bookmarks").toUtf8().constData(), false);

        AddDockWidget(tr("MiniMap").toUtf8().constData(), aznew GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::BottomDockWidgetArea);
        SetDockWidgetVisible(tr("MiniMap").toUtf8().constData(), false);

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewSettingsPtr->m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = m_graphViewSettingsPtr->m_nodeSaveIdentifier;

        m_nodePalette = aznew GraphCanvas::NodePaletteDockWidget(this, tr("Node Palette"), nodePaletteConfig);
        AddDockWidget(tr("Node Palette").toUtf8().constData(), m_nodePalette, Qt::LeftDockWidgetArea);

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(resolvedPath, m_graphViewSettingsPtr->m_translationPath.c_str());
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

    void MaterialCanvasMainWindow::PopulateSettingsInspector(AtomToolsFramework::InspectorWidget* inspector) const
    {
        m_materialCanvasCompileSettingsGroup = AtomToolsFramework::CreateSettingsPropertyGroup(
            tr("Material Canvas Settings").toUtf8().constData(),
            tr("Material Canvas Settings").toUtf8().constData(),
            { AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/EnableFasterShaderBuilds",
                  tr("Enable Faster Shader Builds").toUtf8().constData(),
                  tr("By default, some platforms perform an exhaustive compilation of shaders for multiple RHI. For example, the default "
                  "Windows shader builder settings automatically compiles shaders for DX12, Vulkan, and the Null renderer.\n\nThis option "
                  "overrides those registry settings and makes compilation and preview times much faster by only compiling shaders for the "
                  "currently active platform and RHI.\n\nThis also disables automatic shader variant generation.\n\nChanging this setting "
                  "requires restarting Material Canvas and the Asset Processor.\n\nChanging the active RHI with this setting enabled may "
                  "require clearing the cache to regenerate shaders for the new RHI.\n\nThe settings files containing the overrides will be "
                  "placed in the user/Registry folder for the current project.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ForceDeleteGeneratedFiles",
                  tr("Delete Files On Compile").toUtf8().constData(),
                  tr("This option forces files previously generated from the current graph to be deleted before creating new ones.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ForceClearAssetFingerprints",
                  tr("Clear Asset Fingerprints On Compile").toUtf8().constData(),
                  tr("This option forces the AP to reprocess generated files even if no differences were detected since last generated. This "
                  "guarantees that notifications are sent for assets like materials that may not be changed even if their dependent "
                  "material types or shaders are. This setting is most useful to ensure that other systems or applications are able to "
                  "recognize and not reload yeah materials after shaders are modified. Enabling this setting may affect the time it takes "
                  "for the viewport to reflect shader and material changes.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
                  tr("Enable Compile On Open").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is opened.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave",
                  tr("Enable Compile On Save").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is saved.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit",
                  tr("Enable Compile On Edit").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is edited.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted",
                  tr("Clear Viewport Material When Compiling Starts").toUtf8().constData(),
                  tr("Clear the viewport model's material whenever compiling shaders and materials starts.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed",
                  tr("Clear Viewport Material When Compiling Fails").toUtf8().constData(),
                  tr("Clear the viewport model's material whenever compiling shaders and materials fails.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging",
                  tr("Enable Compiler Logging").toUtf8().constData(),
                  tr("Toggle verbose logging for material graph generation.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/DynamicNode/EnablePropertyEditingOnNodeUI",
                  tr("Enable Property Editing On Nodes").toUtf8().constData(),
                  tr("Toggle settings to display properties and allow them to be added directly on graph nodes.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart",
                  tr("Create Untitled Graph Document On Start").toUtf8().constData(),
                  tr("Create a default, untitled graph document when Material Canvas starts.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/QueueGraphCompileIntervalMs",
                  tr("Queue Graph Compile Interval Ms").toUtf8().constData(),
                  tr("The delay (in milliseconds) before the graph is recompiled after changes.").toUtf8().constData(),
                  aznumeric_cast<AZ::s64>(500),
                  aznumeric_cast<AZ::s64>(0),
                  aznumeric_cast<AZ::s64>(1000)) });

        inspector->AddGroup(
            m_materialCanvasCompileSettingsGroup->m_name,
            m_materialCanvasCompileSettingsGroup->m_displayName,
            m_materialCanvasCompileSettingsGroup->m_description,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_materialCanvasCompileSettingsGroup.get(),
                m_materialCanvasCompileSettingsGroup.get(),
                azrtti_typeid<AtomToolsFramework::DynamicPropertyGroup>()));

        inspector->AddGroup(
            tr("Graph View Settings").toUtf8().constData(),
            tr("Graph View Settings").toUtf8().constData(),
            tr("Configuration settings for the graph view interaction, animation, and other behavior.").toUtf8().constData(),
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr->RTTI_Type()));

        Base::PopulateSettingsInspector(inspector);
    }

    void MaterialCanvasMainWindow::OnSettingsDialogClosed()
    {
        AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
        Base::OnSettingsDialogClosed();
    }

    AZStd::string MaterialCanvasMainWindow::GetHelpUrl() const
    {
        return "https://docs.o3de.org/docs/atom-guide/look-dev/tools/material-canvas/";
    }
} // namespace MaterialCanvas

