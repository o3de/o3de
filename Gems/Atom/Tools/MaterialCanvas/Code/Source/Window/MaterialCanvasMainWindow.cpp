/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Document/InMemoryShaderCompiler.h>
#include <Document/MaterialGraphCompiler.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasViewportContent.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
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

        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    MaterialCanvasMainWindow::~MaterialCanvasMainWindow()
    {
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasMainWindow::CreateMenus(QMenuBar* menuBar)
    {
        Base::CreateMenus(menuBar);

        // Apply sits next to Save because the two are halves of one decision. Save writes the graph; Apply publishes the material the
        // graph describes. An edit refreshes only the reduced preview build, so without Apply the material the rest of the engine loads
        // would change on no occasion other than a save, and there would be no way to try a change in a level without committing it to
        // the source file first.
        //
        // Built by hand rather than through the base's CreateActionAtPosition. That is a function template defined in
        // AtomToolsDocumentMainWindow.cpp rather than in its header, so it can only be instantiated inside that translation unit;
        // calling it from here leaves a declared but undefined instantiation over a lambda's internal linkage closure type, which is
        // MSVC's C5046 and, warnings as errors aside, an unresolved external at link time. These are the five lines it would have run.
        m_actionApply = new QAction(tr("A&pply"), m_menuFile);
        m_actionApply->setShortcut(QKeySequence("Ctrl+Shift+A"));
        m_actionApply->setShortcutContext(Qt::WindowShortcut);
        QObject::connect(
            m_actionApply,
            &QAction::triggered,
            m_menuFile,
            [this]()
            {
                AtomToolsFramework::GraphDocumentRequestBus::Event(
                    GetCurrentDocumentId(), &AtomToolsFramework::GraphDocumentRequestBus::Events::QueueApplyGraph);
            });
        m_menuFile->insertAction(m_actionSaveAsCopy, m_actionApply);

        // Measurement only, and deliberately manual. The spike answers what a shader costs to build in process, with the Asset
        // Processor out of the way; running it automatically on every compile would add a second azslc invocation to the loop it is
        // meant to be measuring.
        if (!m_menuTools)
        {
            return;
        }

        m_menuTools->addAction(
            tr("Run In-Memory Shader Spike..."),
            [this]()
            {
                // Preprocessed AZSL, which the Asset Processor keeps as a cache product next to every shader it builds. Starting the
                // dialog in the cache saves hunting for one; any *_dx12.azslin will do.
                const QString cacheFolder =
                    QString("%1/Cache/pc").arg(QString::fromUtf8(AZ::Utils::GetProjectPath().c_str()));

                const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    tr("Select an intermediate AZSL file"),
                    cacheFolder,
                    // .azsl runs the whole chain including MCPP; .azslin starts at azslc, which is useful if the reconstructed
                    // include paths turn out to be wrong for this project.
                    tr("AZSL (*.azsl *.azslin)"));
                if (selectedPath.isEmpty())
                {
                    return;
                }

                // Run on a worker, never on the UI thread. RHI::ExecuteShaderCompiler waits for azslc with a busy spin that has no
                // sleep in it -- while(IsProcessRunning()) { PeekError(); PeekOutput(); } -- so on the main thread it saturates a core
                // hammering pipe syscalls, blocks the Qt event loop, and starves the very process it is waiting on. Measured on this
                // thread the same shader took 5,994 ms and then 7,028 ms against 670 ms for an identical azslc command line run from
                // a script. The Asset Processor never sees this because its builders are separate processes with nothing else to do.
                //
                // The real in-memory path has the same constraint, and GraphDocument::CompileGraph already meets it by running the
                // compile as a job.
                const AZStd::string inputPath = selectedPath.toUtf8().constData();
                auto spikeJob = AZ::CreateJobFunction(
                    [this, inputPath]()
                    {
                        const auto spikeResult = RunInMemoryShaderSpike(inputPath);

                        // Back to the UI thread to say so. The result is copied into the queued call because the job's frame is gone
                        // by the time it runs.
                        QMetaObject::invokeMethod(
                            this,
                            [this, spikeResult]()
                            {
                                if (!spikeResult.m_succeeded)
                                {
                                    QMessageBox::warning(
                                        this, tr("In-Memory Shader Spike"), tr("Failed: %1").arg(spikeResult.m_failure.c_str()));
                                    return;
                                }

                                QMessageBox::information(
                                    this,
                                    tr("In-Memory Shader Spike"),
                                    tr("MCPP: %1 ms\nazslc: %2 ms\nreflection: %3 ms\ntotal: %4 ms\n\n"
                                       "%5 preprocessed lines from %6 files.\n"
                                       "%7 SRGs, %8 shader options, %9 lines of HLSL.\n\n"
                                       "DXC and dxsc add about 298 ms on top of this. Compare against roughly 2.2 s for the same "
                                       "shader through the Asset Processor.")
                                        .arg(qRound(spikeResult.m_preprocessMs))
                                        .arg(qRound(spikeResult.m_azslcMs))
                                        .arg(qRound(spikeResult.m_reflectionMs))
                                        .arg(qRound(spikeResult.m_totalMs))
                                        .arg(spikeResult.m_preprocessedLineCount)
                                        .arg(spikeResult.m_includedFileCount)
                                        .arg(spikeResult.m_srgCount)
                                        .arg(spikeResult.m_shaderOptionCount)
                                        .arg(spikeResult.m_hlslLineCount));
                            },
                            Qt::QueuedConnection);
                    },
                    true);
                spikeJob->Start();
            });

        // The other half. This one needs no process spawned and no reimplementation: CreateMaterialTypeAsset is the same public
        // call FinalStage makes, so if it works here it works, and the 300 ms the Asset Processor spends on that job is overhead
        // rather than computation.
        m_menuTools->addAction(
            tr("Run In-Memory Material Spike..."),
            [this]()
            {
                const QString previewFolder = QString("%1/Assets/MaterialCanvasPreview")
                                                  .arg(QString::fromUtf8(AZ::Utils::GetProjectPath().c_str()));

                const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    // The abstract one the canvas wrote, not the generated one. The intermediate is found from it.
                    tr("Select a preview material type"),
                    previewFolder,
                    tr("Material Type (*.materialtype)"));
                if (selectedPath.isEmpty())
                {
                    return;
                }

                const AZStd::string inputPath = selectedPath.toUtf8().constData();
                auto spikeJob = AZ::CreateJobFunction(
                    [this, inputPath]()
                    {
                        const auto spikeResult = RunInMemoryMaterialSpike(inputPath);

                        QMetaObject::invokeMethod(
                            this,
                            [this, spikeResult]()
                            {
                                if (!spikeResult.m_succeeded)
                                {
                                    QMessageBox::warning(
                                        this, tr("In-Memory Material Spike"), tr("Failed: %1").arg(spikeResult.m_failure.c_str()));
                                    return;
                                }

                                QMessageBox::information(
                                    this,
                                    tr("In-Memory Material Spike"),
                                    tr("locate + load: %1 ms\nmaterial type asset: %2 ms\ntotal: %3 ms\n\n"
                                       "%4 properties, %5 shader collections.\n\n"
                                       "The Asset Processor spends about 300 ms on the FinalStage job that does this.")
                                        .arg(qRound(spikeResult.m_locateMs))
                                        .arg(qRound(spikeResult.m_createMaterialTypeMs))
                                        .arg(qRound(spikeResult.m_totalMs))
                                        .arg(spikeResult.m_propertyCount)
                                        .arg(spikeResult.m_shaderCount));
                            },
                            Qt::QueuedConnection);
                    },
                    true);
                spikeJob->Start();
            });
    }

    void MaterialCanvasMainWindow::UpdateMenus(QMenuBar* menuBar)
    {
        Base::UpdateMenus(menuBar);

        // The base constructor builds the menus, and a menu update can be queued before this class has finished adding to them.
        if (!m_actionApply)
        {
            return;
        }

        const AZ::Uuid documentId = GetCurrentDocumentId();

        bool isOpen = false;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            isOpen, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsOpen);

        bool applyNeeded = false;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            applyNeeded, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::IsApplyGraphNeeded);

        // Hidden rather than disabled when preview output is off. There is no second output to publish then, so every compile has
        // already produced the real material and an Apply that could never do anything would only raise the question of what it is for.
        m_actionApply->setVisible(MaterialGraphCompiler::IsPreviewOutputEnabled());
        m_actionApply->setEnabled(isOpen && applyNeeded);

        // Greying out is the indicator, in the same way it is in other material editors: enabled means the material in the level is
        // behind the graph. The tooltip says which state this is, because a disabled item on its own reads as broken rather than as done.
        m_actionApply->setToolTip(
            applyNeeded ? tr("Rebuild the material for use outside Material Canvas. It is currently behind this graph.")
                        : tr("The material outside Material Canvas is up to date with this graph."));
    }

    void MaterialCanvasMainWindow::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        if (documentId != GetCurrentDocumentId())
        {
            return;
        }

        // The menu only says this while it is open, so the state is also put somewhere permanently visible. This runs after the compile
        // has finished reporting its own status, so it is not competing with those messages.
        bool applyNeeded = false;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            applyNeeded, documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::IsApplyGraphNeeded);

        if (MaterialGraphCompiler::IsPreviewOutputEnabled() && applyNeeded)
        {
            SetStatusWarning("Preview is current. The material outside Material Canvas is out of date -- File, Apply to rebuild it.");
        }

        QueueUpdateMenus(false);
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
                  "/O3DE/Atom/MaterialCanvas/EnablePreviewOnlyMaterialPipeline",
                  tr("Use Preview-Only Material Pipeline").toUtf8().constData(),
                  tr("An abstract material type is expanded into one shader per render pass, for every enabled material pipeline. With "
                  "the default MainPipeline and LowEndPipeline, a Standard lighting model produces 21 shaders, and every one of them is "
                  "rebuilt whenever the graph changes.\n\nThis option replaces both with a single trimmed pipeline that builds only the "
                  "shaders the Material Canvas viewport actually draws with: depth, shadow, forward, and transparent. Four shaders "
                  "instead of 21.\n\nThe cost is reduced fidelity for a few features while it is enabled. Per-pixel depth offset and "
                  "alpha cutout fall back to un-offset depth and shadow silhouettes, tinted transparent materials do not draw, light "
                  "culling gets less precise depth bounds for transparent surfaces, and there are no motion vectors for TAA or motion "
                  "blur.\n\nThe pipeline is declared on each material type Material Canvas generates, so it applies to the graphs "
                  "being edited and to nothing else in the project. It takes effect on the next compile; no restart is needed. Note that "
                  "the generated material type is still the one other systems load, so while this is enabled that material carries "
                  "preview shaders rather than production ones.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/EnableInMemoryPreviewMaterial",
                  tr("Build Preview Material In Memory").toUtf8().constData(),
                  tr("Builds the preview material inside Material Canvas instead of waiting for the Asset Processor to build it.\n\n"
                  "Two of the four jobs an edit currently triggers, the material type final stage and the material builder, were "
                  "measured at 300 ms and 268 ms of Asset Processor time for 16 ms and roughly 20 ms of actual work. The remainder "
                  "is hashing, dependency fingerprinting, product copies and catalog updates around builders that barely do "
                  "anything.\n\nNothing is reimplemented to skip them: CreateMaterialTypeAsset and MaterialAssetCreator are the same "
                  "calls those builders make, so the preview material is built the same way, just here. The shader is still built by "
                  "the Asset Processor and this depends on it, so whenever the shader is not ready yet the viewport falls back to "
                  "the normal path and waits, exactly as before.\n\nRequires preview output to be enabled. Production materials are "
                  "unaffected: this only changes how the viewport gets its preview.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ProductionMaterialPipelines",
                  tr("Production Material Pipelines").toUtf8().constData(),
                  tr("Comma separated list of material pipelines the production material type is built through. Leave this empty to use "
                  "every pipeline the project registers, which by default is MainPipeline and LowEndPipeline.\n\nThose two produce nearly "
                  "identical shaders: for the same transparent Standard PBR material they measure 13,201 and 13,181 preprocessed lines, "
                  "and 1,287 ms and 1,286 ms of azslc. A project with no low end target therefore pays for a second complete set of "
                  "shaders on every save and never loads them. Setting this to \"MainPipeline\" removes that.\n\nThis affects the "
                  "production output only; the preview set is unaffected. An unrecognised name is reported as a warning and the default "
                  "list is used, so a mistake here costs a log line rather than a material that does not render.").toUtf8().constData(),
                  AZStd::string("")),
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

        if (auto registry = AZ::SettingsRegistry::Get())
        {
            registry->Remove("/O3DE/Atom/MaterialCanvas/PaneWindowState");
        }

        const AZ::IO::FixedMaxPath settingsFilePath(
            AZStd::string::format("%s/user/Registry/usersettings.materialcanvas.setreg", AZ::Utils::GetProjectPath().c_str()));
        AtomToolsFramework::SaveSettingsToFile(
            settingsFilePath,
            { "/O3DE/AtomToolsFramework", "/O3DE/Atom/Tools", "/O3DE/Atom/GraphView", "/O3DE/Atom/MaterialCanvas" });
    }

    AZStd::string MaterialCanvasMainWindow::GetHelpUrl() const
    {
        return "https://docs.o3de.org/docs/atom-guide/look-dev/tools/material-canvas/";
    }
} // namespace MaterialCanvas

