/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ISystem.h>
#include <IConsole.h>

#include <Editor/View/Windows/MainWindow.h>
#include <Editor/GraphCanvas/AutomationIds.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <QSplitter>
#include <QListView>
#include <QShortcut>
#include <QKeySequence>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneEvent>
#include <QMimeData>
#include <QCoreApplication>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QProgressDialog>
#include <QToolButton>

#include <ScriptEvents/ScriptEventsAsset.h>

#include <Editor/GraphCanvas/Components/MappingComponent.h>
#include <Editor/View/Dialogs/UnsavedChangesDialog.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/PropertyGrid.h>
#include <Editor/View/Widgets/CommandLine.h>
#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Widgets/CanvasWidget.h>
#include <Editor/View/Widgets/LogPanel.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingWindow.h>
#include <Editor/View/Widgets/MainWindowStatusWidget.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/StatisticsDialog/ScriptCanvasStatisticsDialog.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>
#include <Editor/View/Widgets/UnitTestPanel/UnitTestDockWidget.h>
#include <Editor/View/Widgets/ValidationPanel/GraphValidationDockWidget.h>

#include <Editor/View/Windows/ui_mainwindow.h>

#include <Editor/Model/EntityMimeDataHandler.h>

#include <Editor/Utilities/RecentAssetPath.h>

#include <Editor/Settings.h>
#include <Editor/Nodes/NodeCreateUtils.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>

#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>

#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/BookmarkContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CollapsedNodeGroupContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeGroupContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CommentContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>
#include <GraphCanvas/Types/ConstructPresets.h>

#include <Editor/View/Windows/ScriptCanvasContextMenus.h>
#include <Editor/View/Windows/ScriptEventMenu.h>
#include <Editor/View/Windows/EBusHandlerActionMenu.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Windows/Tools/InterpreterWidget/InterpreterWidget.h>
#include <Editor/View/Windows/Tools/UpgradeTool/UpgradeHelper.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <Editor/View/Widgets/VariablePanel/VariableConfigurationWidget.h>

// Save Format Conversion
#include <AzCore/Component/EntityUtils.h>
#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>
////

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>


#include <Editor/QtMetaTypes.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/LyViewPaneNames.h>

namespace ScriptCanvasEditor
{
    using namespace AzToolsFramework;

    namespace
    {
        template <typename T>
        class ScopedVariableSetter
        {
        public:
            ScopedVariableSetter(T& value)
                : m_oldValue(value)
                , m_value(value)
            {
            }
            ScopedVariableSetter(T& value, const T& newValue)
                : m_oldValue(value)
                , m_value(value)
            {
                m_value = newValue;
            }

            ~ScopedVariableSetter()
            {
                m_value = m_oldValue;
            }

        private:
            T m_oldValue;
            T& m_value;
        };

        template<typename MimeDataDelegateHandler, typename ... ComponentArgs>
        AZ::EntityId CreateMimeDataDelegate(ComponentArgs... componentArgs)
        {
            AZ::Entity* mimeDelegateEntity = aznew AZ::Entity("MimeData Delegate");

            mimeDelegateEntity->CreateComponent<MimeDataDelegateHandler>(AZStd::forward<ComponentArgs>(componentArgs) ...);
            mimeDelegateEntity->Init();
            mimeDelegateEntity->Activate();

            return mimeDelegateEntity->GetId();
        }
    } // anonymous namespace.

    void Workspace::Save()
    {
        auto workspace = AZ::UserSettings::CreateFind<EditorSettings::EditorWorkspace>(AZ_CRC("ScriptCanvasEditorWindowState", 0x10c47d36), AZ::UserSettings::CT_LOCAL);
        if (workspace)
        {
            workspace->Init(m_mainWindow->saveState(), m_mainWindow->saveGeometry());

            Widget::GraphTabBar* tabBar = m_mainWindow->m_tabBar;

            AZStd::vector<EditorSettings::EditorWorkspace::WorkspaceAssetSaveData> activeAssets;
            SourceHandle focusedAssetId = tabBar->FindAssetId(tabBar->currentIndex());

            if (m_rememberOpenCanvases)
            {
                activeAssets.reserve(tabBar->count());

                for (int i = 0; i < tabBar->count(); ++i)
                {
                    SourceHandle assetId = tabBar->FindAssetId(i);

                    const Tracker::ScriptCanvasFileState& fileState = m_mainWindow->GetAssetFileState(assetId);

                    if (fileState == Tracker::ScriptCanvasFileState::MODIFIED || fileState == Tracker::ScriptCanvasFileState::UNMODIFIED)
                    {
                        SourceHandle sourceId = GetSourceAssetId(assetId);
                        if (sourceId.IsGraphValid())
                        {
                            EditorSettings::EditorWorkspace::WorkspaceAssetSaveData assetSaveData;
                            assetSaveData.m_assetId = sourceId;

                            activeAssets.push_back(assetSaveData);
                        }
                    }
                    else if (assetId.AnyEquals(focusedAssetId))
                    {
                        focusedAssetId.Clear();
                    }
                }

                // The assetId needs to be the file AssetId to restore the workspace
                if (focusedAssetId.IsGraphValid())
                {
                    focusedAssetId = GetSourceAssetId(focusedAssetId);
                }

                // If our currently focused asset won't be restored, just show the first element.
                if (!focusedAssetId.IsGraphValid())
                {
                    if (!activeAssets.empty())
                    {
                        focusedAssetId = activeAssets.front().m_assetId;
                    }
                }
            }

            workspace->Clear();
            if (!activeAssets.empty())
            {
                workspace->ConfigureActiveAssets(focusedAssetId, activeAssets);
            }
        }
    }

    // Workspace

    void Workspace::Restore()
    {

        auto workspace = AZ::UserSettings::Find<EditorSettings::EditorWorkspace>(AZ_CRC("ScriptCanvasEditorWindowState", 0x10c47d36), AZ::UserSettings::CT_LOCAL);
        if (workspace)
        {
            workspace->Restore(qobject_cast<QMainWindow*>(m_mainWindow));

            if (m_rememberOpenCanvases)
            {
                for (const auto& assetSaveData : workspace->GetActiveAssetData())
                {
                    m_loadingAssets.push_back(assetSaveData.m_assetId);
                }

                if (m_loadingAssets.empty())
                {
                    m_mainWindow->OnWorkspaceRestoreEnd(SourceHandle());
                }
                else
                {
                    m_mainWindow->OnWorkspaceRestoreStart();
                }

                m_queuedAssetFocus = workspace->GetFocusedAssetId();

                // #sc-asset-editor
                //for (const auto& assetSaveData : workspace->GetActiveAssetData())
                {
                    // load all the files
//                     AssetTrackerNotificationBus::MultiHandler::BusConnect(assetSaveData.m_assetId);
//
//                     Callbacks::OnAssetReadyCallback onAssetReady = [this, assetSaveData](ScriptCanvasMemoryAsset& asset)
//                     {
//                         // If we get an error callback. Just remove it from out active lists.
//                         if (asset.IsSourceInError())
//                         {
//                             if (assetSaveData.m_assetId == m_queuedAssetFocus)
//                             {
//                                 m_queuedAssetFocus = SourceHandle();
//                             }
//
//                             SignalAssetComplete(asset.GetFileAssetId());
//                         }
//                     };
//
//                     bool loadedFile = true;
//                     AssetTrackerRequestBus::BroadcastResult(loadedFile, &AssetTrackerRequests::Load, assetSaveData.m_assetId, assetSaveData.m_assetType, onAssetReady);
//
//                     if (!loadedFile)
//                     {
//                         if (assetSaveData.m_assetId == m_queuedAssetFocus)
//                         {
//                             m_queuedAssetFocus = SourceHandle();
//                         }
//
//                         SignalAssetComplete(assetSaveData.m_assetId);
//                     }
                }
            }
            else
            {
                m_mainWindow->OnWorkspaceRestoreEnd(SourceHandle());
            }
        }
    }

    void Workspace::SignalAssetComplete(const SourceHandle& /*fileAssetId*/)
    {
        // When we are done loading all assets we can safely set the focus to the recorded asset
//         auto it = AZStd::find(m_loadingAssets.begin(), m_loadingAssets.end(), fileAssetId);
//         if (it != m_loadingAssets.end())
//         {
//             m_loadingAssets.erase(it);
//         }
//
//         if (m_loadingAssets.empty())
//         {
//             m_mainWindow->OnWorkspaceRestoreEnd(m_queuedAssetFocus);
//             m_queuedAssetFocus.SetInvalid();
//         }
    }

    SourceHandle Workspace::GetSourceAssetId(const SourceHandle& memoryAssetId) const
    {
        return memoryAssetId;
    }

    ////////////////
    // MainWindow
    ////////////////

    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent, Qt::Widget | Qt::WindowMinMaxButtonsHint)
        , ui(new Ui::MainWindow)
        , m_loadingNewlySavedFile(false)
        , m_isClosingTabs(false)
        , m_enterState(false)
        , m_ignoreSelection(false)
        , m_isRestoringWorkspace(false)
        , m_preventUndoStateUpdateCount(0)
        , m_queueCloseRequest(false)
        , m_hasQueuedClose(false)
        , m_isInAutomation(false)
        , m_allowAutoSave(true)
        , m_systemTickActions(0)
        , m_closeCurrentGraphAfterSave(false)
        , m_styleManager(ScriptCanvasEditor::AssetEditorId, "ScriptCanvas/StyleSheet/graphcanvas_style.json")
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        VariablePaletteRequestBus::Handler::BusConnect();
        GraphCanvas::AssetEditorAutomationRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@products@/translation/scriptcanvas_en_us.qm", unresolvedPath.data(), unresolvedPath.size());

        QString translationFilePath(unresolvedPath.data());
        if ( m_translator.load(QLocale::Language::English, translationFilePath) )
        {
            if ( !qApp->installTranslator(&m_translator) )
            {
                AZ_Warning("ScriptCanvas", false, "Error installing translation %s!", unresolvedPath.data());
            }
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Error loading translation file %s", unresolvedPath.data());
        }

        AzToolsFramework::AssetBrowser::AssetBrowserModel* assetBrowserModel = nullptr;
        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);

        {
            m_scriptEventsAssetModel = new ScriptCanvasAssetBrowserModel(this);

            AzToolsFramework::AssetBrowser::AssetGroupFilter* scriptEventAssetFilter = new AzToolsFramework::AssetBrowser::AssetGroupFilter();
            scriptEventAssetFilter->SetAssetGroup(ScriptEvents::ScriptEventsAsset::GetGroup());
            scriptEventAssetFilter->SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);

            m_scriptEventsAssetModel->setSourceModel(assetBrowserModel);
        }

        {
            m_scriptCanvasAssetModel = new ScriptCanvasAssetBrowserModel(this);

            AzToolsFramework::AssetBrowser::AssetGroupFilter* scriptCanvasAssetFilter = new AzToolsFramework::AssetBrowser::AssetGroupFilter();
            scriptCanvasAssetFilter->SetAssetGroup(ScriptCanvas::SubgraphInterfaceAssetDescription().GetGroupImpl());
            scriptCanvasAssetFilter->SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);

            m_scriptCanvasAssetModel->setSourceModel(assetBrowserModel);
        }

        m_nodePaletteModel.AssignAssetModel(m_scriptCanvasAssetModel);

        ui->setupUi(this);

        CreateMenus();
        UpdateRecentMenu();

        m_host = new QWidget();

        m_layout = new QVBoxLayout();

        m_emptyCanvas = aznew GraphCanvas::GraphCanvasEditorEmptyDockWidget(this);
        m_emptyCanvas->SetDragTargetText(tr("Use the File Menu or drag out a node from the Node Palette to create a new script.").toStdString().c_str());

        m_emptyCanvas->SetEditorId(ScriptCanvasEditor::AssetEditorId);

        m_emptyCanvas->RegisterAcceptedMimeType(Widget::NodePaletteDockWidget::GetMimeType());
        m_emptyCanvas->RegisterAcceptedMimeType(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

        m_editorToolbar = aznew GraphCanvas::AssetEditorToolbar(ScriptCanvasEditor::AssetEditorId);

        // Custom Actions
        {
            m_assignToSelectedEntity = new QToolButton();
            m_assignToSelectedEntity->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/attach_to_entity.png"));
            m_assignToSelectedEntity->setToolTip("Assigns the currently active graph to all of the currently selected entities.");

            m_selectedEntityMenu = new QMenu();

            m_assignToSelectedEntity->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
            m_assignToSelectedEntity->setMenu(m_selectedEntityMenu);
            m_assignToSelectedEntity->setEnabled(false);

            m_editorToolbar->AddCustomAction(m_assignToSelectedEntity);

            QObject::connect(m_selectedEntityMenu, &QMenu::aboutToShow, this, &MainWindow::OnSelectedEntitiesAboutToShow);
            QObject::connect(m_assignToSelectedEntity, &QToolButton::clicked, this, &MainWindow::OnAssignToSelectedEntities);
        }

        // Creation Actions
        {
            m_createScriptCanvas = new QToolButton();
            m_createScriptCanvas->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/create_graph.png"));
            m_createScriptCanvas->setToolTip("Creates a new Script Canvas Graph");

            QObject::connect(m_createScriptCanvas, &QToolButton::clicked, this, &MainWindow::OnFileNew);

            m_editorToolbar->AddCreationAction(m_createScriptCanvas);
            RegisterObject(AutomationIds::CreateScriptCanvasButton, m_createScriptCanvas);

        }

        {
            m_createFunctionInput = new QToolButton();
            m_createFunctionInput->setToolTip("Creates an Execution Nodeling on the leftmost side of the graph to be used as input for the graph.");
            m_createFunctionInput->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/create_function_input.png"));
            m_createFunctionInput->setEnabled(false);
        }

        m_editorToolbar->AddCustomAction(m_createFunctionInput);
        connect(m_createFunctionInput, &QToolButton::clicked, this, &MainWindow::CreateFunctionInput);

        {
            m_createFunctionOutput = new QToolButton();
            m_createFunctionOutput->setToolTip("Creates an Execution Nodeling on the rightmost side of the graph to be used as output for the graph.");
            m_createFunctionOutput->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/create_function_output.png"));
            m_createFunctionOutput->setEnabled(false);
        }

        m_editorToolbar->AddCustomAction(m_createFunctionOutput);
        connect(m_createFunctionOutput, &QToolButton::clicked, this, &MainWindow::CreateFunctionOutput);

        {
            m_validateGraphToolButton = new QToolButton();
            m_validateGraphToolButton->setToolTip("Will run a validation check on the current graph and report any warnings/errors discovered.");
            m_validateGraphToolButton->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/validate_icon.png"));
            m_validateGraphToolButton->setEnabled(false);
        }

        m_editorToolbar->AddCustomAction(m_validateGraphToolButton);

        // Screenshot
        {
            m_takeScreenshot = new QToolButton();
            m_takeScreenshot->setToolTip("Captures a full resolution screenshot of the entire graph or selected nodes into the clipboard");
            m_takeScreenshot->setIcon(QIcon(":/ScriptCanvasEditorResources/Resources/scriptcanvas_screenshot.png"));
            m_takeScreenshot->setEnabled(false);
        }

        m_editorToolbar->AddCustomAction(m_takeScreenshot);
        connect(m_takeScreenshot, &QToolButton::clicked, this, &MainWindow::OnScreenshot);


        connect(m_validateGraphToolButton, &QToolButton::clicked, this, &MainWindow::OnValidateCurrentGraph);

        m_layout->addWidget(m_editorToolbar);

        // Tab bar
        {
            m_tabWidget = new AzQtComponents::TabWidget(m_host);
            m_tabBar = new Widget::GraphTabBar(m_tabWidget);
            m_tabWidget->setCustomTabBar(m_tabBar);
            m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::OnTabCloseButtonPressed);
            connect(m_tabBar, &Widget::GraphTabBar::TabCloseNoButton, this, &MainWindow::OnTabCloseRequest);
            connect(m_tabBar, &Widget::GraphTabBar::SaveTab, this, &MainWindow::SaveTab);
            connect(m_tabBar, &Widget::GraphTabBar::CloseAllTabsSignal, this, &MainWindow::CloseAllTabs);
            connect(m_tabBar, &Widget::GraphTabBar::CloseAllTabsButSignal, this, &MainWindow::CloseAllTabsBut);
            connect(m_tabBar, &Widget::GraphTabBar::CopyPathToClipboard, this, &MainWindow::CopyPathToClipboard);
            connect(m_tabBar, &Widget::GraphTabBar::OnActiveFileStateChanged, this, &MainWindow::OnActiveFileStateChanged);

            AzQtComponents::TabWidget::applySecondaryStyle(m_tabWidget, false);
            m_tabWidget->setObjectName("ScriptCanvasTabs");

            m_layout->addWidget(m_tabWidget);
        }

        m_commandLine = new Widget::CommandLine(this);
        m_commandLine->setBaseSize(QSize(size().width(), m_commandLine->size().height()));
        m_commandLine->setObjectName("CommandLine");

        m_layout->addWidget(m_commandLine);
        m_layout->addWidget(m_emptyCanvas);

        // Minimap should be a child of the dock widget. But until performance concerns are resolved
        // we want to hide it(mostly to avoid re-setting up all of the structural code around it).
        //
        // If this is a child, it appears on the default context menu to show/hide.
        m_minimap = aznew GraphCanvas::MiniMapDockWidget(ScriptCanvasEditor::AssetEditorId);
        m_minimap->setObjectName("MiniMapDockWidget");

        m_statusWidget = aznew MainWindowStatusWidget(this);
        statusBar()->addWidget(m_statusWidget,1);

        QObject::connect(m_statusWidget, &MainWindowStatusWidget::OnErrorButtonPressed, this, &MainWindow::OnShowValidationErrors);
        QObject::connect(m_statusWidget, &MainWindowStatusWidget::OnWarningButtonPressed, this, &MainWindow::OnShowValidationWarnings);

        m_nodePaletteModel.RepopulateModel();

        // Order these are created denotes the order for an auto-generate Qt menu. Keeping this construction order
        // in sync with the order we display under tools for consistency.
        {
            const bool isInContextMenu = false;
            Widget::ScriptCanvasNodePaletteConfig nodePaletteConfig(m_nodePaletteModel, m_scriptEventsAssetModel, isInContextMenu);

            m_nodePalette = aznew Widget::NodePaletteDockWidget(tr("Node Palette"), this, nodePaletteConfig);
            m_nodePalette->setObjectName("NodePalette");

            RegisterObject(AutomationIds::NodePaletteDockWidget, m_nodePalette);
            RegisterObject(AutomationIds::NodePaletteWidget, m_nodePalette->GetNodePaletteWidget());
        }

        m_propertyGrid = new Widget::PropertyGrid(this, "Node Inspector");
        m_propertyGrid->setObjectName("NodeInspector");

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(ScriptCanvasEditor::AssetEditorId, this);

        QObject::connect(m_variableDockWidget, &VariableDockWidget::OnVariableSelectionChanged, this, &MainWindow::OnVariableSelectionChanged);

        // This needs to happen after the node palette is created, because we scrape for the variable data from inside
        // of there.
        m_variableDockWidget->PopulateVariablePalette(m_variablePaletteTypes);

        m_validationDockWidget = aznew GraphValidationDockWidget(this);
        m_validationDockWidget->setObjectName("ValidationDockWidget");
        // End Construction list

        m_ebusHandlerActionMenu = aznew EBusHandlerActionMenu();

        m_statisticsDialog = aznew StatisticsDialog(m_nodePaletteModel, m_scriptCanvasAssetModel, nullptr);
        m_statisticsDialog->hide();

        m_presetEditor = aznew GraphCanvas::ConstructPresetDialog(nullptr);
        m_presetEditor->SetEditorId(ScriptCanvasEditor::AssetEditorId);

        m_presetWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        m_presetWrapper->setGuest(m_presetEditor);
        m_presetWrapper->hide();

        m_host->setLayout(m_layout);

        setCentralWidget(m_host);

        m_workspace = new Workspace(this);

        QTimer::singleShot(0, [this]() {
            SetDefaultLayout();

            if (m_activeGraph.IsGraphValid())
            {
                m_queuedFocusOverride = m_activeGraph;
            }

            m_workspace->Restore();
            m_workspace->Save();
        });

        m_entityMimeDelegateId = CreateMimeDataDelegate<ScriptCanvasEditor::EntityMimeDataHandler>();

        ScriptCanvasEditor::GeneralRequestBus::Handler::BusConnect();
        ScriptCanvasEditor::AutomationRequestBus::Handler::BusConnect();

        UIRequestBus::Handler::BusConnect();
        UndoNotificationBus::Handler::BusConnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        ScriptCanvas::BatchOperationNotificationBus::Handler::BusConnect();
        AssetGraphSceneBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
        ScriptCanvas::ScriptCanvasSettingsRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();

        UINotificationBus::Broadcast(&UINotifications::MainWindowCreationEvent, this);

        m_userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (m_userSettings)
        {
            m_allowAutoSave = m_userSettings->m_autoSaveConfig.m_enabled;
            m_showUpgradeTool = m_userSettings->m_showUpgradeDialog;
            m_autoSaveTimer.setInterval(m_userSettings->m_autoSaveConfig.m_timeSeconds * 1000);

            m_userSettings->m_constructPresets.SetEditorId(ScriptCanvasEditor::AssetEditorId);
        }

        // These should be created after we load up the user settings so we can
        // initialize the user presets
        m_sceneContextMenu = aznew SceneContextMenu(m_nodePaletteModel, m_scriptEventsAssetModel);
        m_connectionContextMenu = aznew ConnectionContextMenu(m_nodePaletteModel, m_scriptEventsAssetModel);

        connect(m_nodePalette, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_minimap, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_propertyGrid, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_bookmarkDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_variableDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_loggingWindow, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
        connect(m_validationDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);

        m_autoSaveTimer.setSingleShot(true);
        connect(&m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::OnAutoSave);
        UpdateMenuState(false);
    }

    MainWindow::~MainWindow()
    {
        m_workspace->Save();

        ScriptCanvas::BatchOperationNotificationBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        UndoNotificationBus::Handler::BusDisconnect();
        UIRequestBus::Handler::BusDisconnect();
        ScriptCanvasEditor::GeneralRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorAutomationRequestBus::Handler::BusDisconnect();
        ScriptCanvas::ScriptCanvasSettingsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        Clear();

        delete m_nodePalette;

        delete m_unitTestDockWidget;
        delete m_statisticsDialog;
        delete m_presetEditor;

        delete m_workspace;

        delete m_sceneContextMenu;
        delete m_connectionContextMenu;
    }

    void MainWindow::CreateMenus()
    {
        // File menu
        connect(ui->action_New_Script, &QAction::triggered, this, &MainWindow::OnFileNew);
        ui->action_New_Script->setShortcut(QKeySequence(QKeySequence::New));

        connect(ui->action_Open, &QAction::triggered, this, &MainWindow::OnFileOpen);
        ui->action_Open->setShortcut(QKeySequence(QKeySequence::Open));

        connect(ui->action_UpgradeTool, &QAction::triggered, this, &MainWindow::RunUpgradeTool);
        ui->action_UpgradeTool->setVisible(true);

        connect(ui->action_Interpreter, &QAction::triggered, this, &MainWindow::ShowInterpreter);
        ui->action_Interpreter->setVisible(true);

        connect(ui->actionAdd_Script_Event_Helpers, &QAction::triggered, this, &MainWindow::OnScriptEventAddHelpers);
        connect(ui->actionClear_Script_Event_Status, &QAction::triggered, this, &MainWindow::OnScriptEventClearStatus);
        connect(ui->actionOpen_Script_Event, &QAction::triggered, this, &MainWindow::OnScriptEventOpen);
        connect(ui->actionParse_As_Script_Event, &QAction::triggered, this, &MainWindow::OnScriptEventParseAs);
        connect(ui->actionSave_As_ScriptEvent, &QAction::triggered, this, &MainWindow::OnScriptEventSaveAs);
        connect(ui->menuScript_Events_PREVIEW, &QMenu::aboutToShow, this, &MainWindow::OnScriptEventMenuPreShow);

        // List of recent files.
        {
            QMenu* recentMenu = new QMenu("Open &Recent");

            for (int i = 0; i < m_recentActions.size(); ++i)
            {
                QAction* action = new QAction(this);
                action->setVisible(false);
                m_recentActions[i] = AZStd::make_pair(action, QMetaObject::Connection());
                recentMenu->addAction(action);
            }

            connect(recentMenu, &QMenu::aboutToShow, this, &MainWindow::UpdateRecentMenu);

            recentMenu->addSeparator();

            // Clear Recent Files.
            {
                QAction* action = new QAction("&Clear Recent Files", this);

                QObject::connect(action,
                    &QAction::triggered,
                    [this](bool /*checked*/)
                {
                    ClearRecentFile();
                    UpdateRecentMenu();
                });

                recentMenu->addAction(action);

            }

            ui->menuFile->insertMenu(ui->action_Save, recentMenu);
            ui->menuFile->insertSeparator(ui->action_Save);
        }

        connect(ui->action_Save, &QAction::triggered, this, &MainWindow::OnFileSaveCaller);
        ui->action_Save->setShortcut(QKeySequence(QKeySequence::Save));

        connect(ui->action_Save_As, &QAction::triggered, this, &MainWindow::OnFileSaveAsCaller);
        ui->action_Save_As->setShortcut(QKeySequence(tr("Ctrl+Shift+S", "File|Save As...")));

        QObject::connect(ui->action_Close,
            &QAction::triggered,
            [this](bool /*checked*/)
        {
            m_tabBar->tabCloseRequested(m_tabBar->currentIndex());
        });
        ui->action_Close->setShortcut(QKeySequence(QKeySequence::Close));

        // Edit Menu
        SetupEditMenu();

        // View menu
        connect(ui->action_ViewNodePalette, &QAction::triggered, this, &MainWindow::OnViewNodePalette);
        connect(ui->action_ViewMiniMap, &QAction::triggered, this, &MainWindow::OnViewMiniMap);

        connect(ui->action_ViewProperties, &QAction::triggered, this, &MainWindow::OnViewProperties);
        connect(ui->action_ViewBookmarks, &QAction::triggered, this, &MainWindow::OnBookmarks);

        m_variableDockWidget = new VariableDockWidget(this);
        m_variableDockWidget->setObjectName("VariableManager");
        connect(ui->action_ViewVariableManager, &QAction::triggered, this, &MainWindow::OnVariableManager);
        connect(m_variableDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);

        m_loggingWindow = aznew LoggingWindow(this);
        m_loggingWindow->setObjectName("LoggingWindow");
        connect(ui->action_ViewLogWindow, &QAction::triggered, this, &MainWindow::OnViewLogWindow);
        connect(m_loggingWindow, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);

        connect(ui->action_ViewDebugger, &QAction::triggered, this, &MainWindow::OnViewDebugger);
        connect(ui->action_ViewCommandLine, &QAction::triggered, this, &MainWindow::OnViewCommandLine);
        connect(ui->action_ViewLog, &QAction::triggered, this, &MainWindow::OnViewLog);

        connect(ui->action_GraphValidation, &QAction::triggered, this, &MainWindow::OnViewGraphValidation);
        connect(ui->action_Debugging, &QAction::triggered, this, &MainWindow::OnViewDebuggingWindow);

        connect(ui->action_ViewUnitTestManager, &QAction::triggered, this, &MainWindow::OnViewUnitTestManager);
        connect(ui->action_NodeStatistics, &QAction::triggered, this, &MainWindow::OnViewStatisticsPanel);
        connect(ui->action_PresetsEditor, &QAction::triggered, this, &MainWindow::OnViewPresetsEditor);

        connect(ui->action_ViewRestoreDefaultLayout, &QAction::triggered, this, &MainWindow::OnRestoreDefaultLayout);
    }

    void MainWindow::SignalActiveSceneChanged(SourceHandle assetId)
    {
        AZ::EntityId graphId;
        if (assetId.IsGraphValid())
        {
            EditorGraphRequestBus::EventResult(graphId, assetId.Get()->GetScriptCanvasId(), &EditorGraphRequests::GetGraphCanvasGraphId);
        }

        m_autoSaveTimer.stop();

        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, graphId);
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);

        // The paste action refreshes based on the scene's mimetype
        RefreshPasteAction();

        bool enabled = false;

        if (graphId.IsValid())
        {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

            if (viewId.IsValid())
            {
                GraphCanvas::ViewNotificationBus::Handler::BusDisconnect();
                GraphCanvas::ViewNotificationBus::Handler::BusConnect(viewId);

                enabled = true;
            }
            else
            {
                AZ_Error("ScriptCanvasEditor", viewId.IsValid(), "SceneRequest must return a valid ViewId");
            }
        }

        UpdateMenuState(enabled);
    }

    void MainWindow::UpdateRecentMenu()
    {
        QStringList recentFiles = ReadRecentFiles();

        int recentCount = 0;
        for (auto filename : recentFiles)
        {
            if (!QFile::exists(filename))
            {
                continue;
            }

            auto& recent = m_recentActions[recentCount++];

            recent.first->setText(QString("&%1 %2").arg(QString::number(recentCount), filename));
            recent.first->setData(filename);
            recent.first->setVisible(true);

            QObject::disconnect(recent.second);
            recent.second = QObject::connect(recent.first,
                &QAction::triggered,
                [this, filename](bool /*checked*/)
            {
                OpenFile(filename.toUtf8().data());
            });
        }

        for (int i = recentCount; i < m_recentActions.size(); ++i)
        {
            auto& recent = m_recentActions[recentCount++];
            recent.first->setVisible(false);
        }
    }

    void MainWindow::OnViewVisibilityChanged(bool)
    {
        UpdateViewMenu();
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        // If we are in the middle of saving a graph. We don't want to close ourselves down and potentially retrigger the saving logic.
        if (m_queueCloseRequest)
        {
            m_hasQueuedClose = true;
            event->ignore();
            return;
        }

        for (int tabCounter = 0; tabCounter < m_tabBar->count(); ++tabCounter)
        {
            SourceHandle assetId = m_tabBar->FindAssetId(tabCounter);

            const Tracker::ScriptCanvasFileState& fileState = GetAssetFileState(assetId);

            if (fileState == Tracker::ScriptCanvasFileState::UNMODIFIED)
            {
                continue;
            }

            // Query the user.
            SetActiveAsset(assetId);

            QString tabName = m_tabBar->tabText(tabCounter);
            UnsavedChangesOptions shouldSaveResults = ShowSaveDialog(tabName);

            if (shouldSaveResults == UnsavedChangesOptions::SAVE)
            {
                if (fileState == Tracker::ScriptCanvasFileState::NEW)
                {
                    SaveAssetImpl(assetId, Save::As);
                }
                else
                {
                    SaveAssetImpl(assetId, Save::InPlace);
                }
                event->ignore();
                return;
            }
            else if (shouldSaveResults == UnsavedChangesOptions::CANCEL_WITHOUT_SAVING)
            {
                event->ignore();
                return;
            }
            else if (shouldSaveResults == UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING &&
                    (fileState == Tracker::ScriptCanvasFileState::NEW || fileState == Tracker::ScriptCanvasFileState::SOURCE_REMOVED))
            {
                CloseScriptCanvasAsset(assetId);
                --tabCounter;
            }
        }

        m_workspace->Save();
        event->accept();
    }

    UnsavedChangesOptions MainWindow::ShowSaveDialog(const QString& filename)
    {
        bool wasActive = m_autoSaveTimer.isActive();

        if (wasActive)
        {
            m_autoSaveTimer.stop();
        }

        UnsavedChangesOptions shouldSaveResults = UnsavedChangesOptions::INVALID;
        UnsavedChangesDialog dialog(filename, this);
        dialog.exec();
        shouldSaveResults = dialog.GetResult();

        // If the auto save timer was active, and we cancelled our save dialog, we want
        // to resume the auto save timer.
        if (shouldSaveResults == UnsavedChangesOptions::CANCEL_WITHOUT_SAVING
            || shouldSaveResults == UnsavedChangesOptions::INVALID)
        {
            RestartAutoTimerSave(wasActive);
        }

        return shouldSaveResults;
    }

    void MainWindow::TriggerUndo()
    {
        GeneralEditorNotificationBus::Event(GetActiveScriptCanvasId(), &GeneralEditorNotifications::OnUndoRedoBegin);
        DequeuePropertyGridUpdate();

        UndoRequestBus::Event(GetActiveScriptCanvasId(), &UndoRequests::Undo);
        SignalSceneDirty(m_activeGraph);

        m_propertyGrid->ClearSelection();
        GeneralEditorNotificationBus::Event(GetActiveScriptCanvasId(), &GeneralEditorNotifications::OnUndoRedoEnd);
    }

    void MainWindow::TriggerRedo()
    {
        GeneralEditorNotificationBus::Event(GetActiveScriptCanvasId(), &GeneralEditorNotifications::OnUndoRedoBegin);
        DequeuePropertyGridUpdate();

        UndoRequestBus::Event(GetActiveScriptCanvasId(), &UndoRequests::Redo);
        SignalSceneDirty(m_activeGraph);

        m_propertyGrid->ClearSelection();
        GeneralEditorNotificationBus::Event(GetActiveScriptCanvasId(), &GeneralEditorNotifications::OnUndoRedoEnd);
    }

    void MainWindow::RegisterVariableType(const ScriptCanvas::Data::Type& variableType)
    {
        m_variablePaletteTypes.insert(ScriptCanvas::Data::ToAZType(variableType));
    }

    bool MainWindow::IsValidVariableType(const ScriptCanvas::Data::Type& dataType) const
    {
        return m_variableDockWidget->IsValidVariableType(dataType);
    }

    VariablePaletteRequests::VariableConfigurationOutput MainWindow::ShowVariableConfigurationWidget
        ( const VariablePaletteRequests::VariableConfigurationInput& input, const QPoint& scenePosition)
    {
        VariablePaletteRequests::VariableConfigurationOutput output;
        m_slotTypeSelector = new VariableConfigurationWidget(GetActiveScriptCanvasId(), input, this); // Recreate the widget every time because of https://bugreports.qt.io/browse/QTBUG-76509
        m_slotTypeSelector->PopulateVariablePalette(m_variablePaletteTypes);

        // Only set the slot name if the user has already configured this slot, so if they are creating
        // for the first time they will see the placeholder text instead
        bool isValidVariableType = false;
        VariablePaletteRequestBus::BroadcastResult(isValidVariableType, &VariablePaletteRequests::IsValidVariableType, input.m_currentType);
        if (isValidVariableType)
        {
            m_slotTypeSelector->SetSlotName(input.m_currentName);
        }

        m_slotTypeSelector->move(scenePosition);
        m_slotTypeSelector->setEnabled(true);
        m_slotTypeSelector->update();

        if (m_slotTypeSelector->exec() != QDialog::Rejected)
        {
            output.m_name = m_slotTypeSelector->GetSlotName();
            output.m_type = Data::FromAZType(m_slotTypeSelector->GetSelectedType());
            output.m_actionIsValid = true;
            output.m_nameChanged = input.m_currentName != output.m_name;
            output.m_typeChanged = input.m_currentType != output.m_type;
        }

        delete m_slotTypeSelector;
        return output;
    }

    void MainWindow::OpenValidationPanel()
    {
        if (!m_validationDockWidget->isVisible())
        {
            OnViewGraphValidation();
        }
    }

    void MainWindow::PostUndoPoint(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        bool isIdle = true;
        UndoRequestBus::EventResult(isIdle, scriptCanvasId, &UndoRequests::IsIdle);

        if (m_preventUndoStateUpdateCount == 0 && isIdle)
        {
            ScopedUndoBatch scopedUndoBatch("Modify Graph Canvas Scene");
            UndoRequestBus::Event(scriptCanvasId, &UndoRequests::AddGraphItemChangeUndo, "Graph Change");
            UpdateFileState(m_activeGraph, Tracker::ScriptCanvasFileState::MODIFIED);
        }

        const bool forceTimer = true;
        RestartAutoTimerSave(forceTimer);
    }

    void MainWindow::SourceFileChanged
        ( AZStd::string relativePath
        , AZStd::string scanFolder
        , AZ::Uuid fileAssetId)
    {
        auto handle = SourceHandle::FromRelativePathAndScanFolder(scanFolder, relativePath, fileAssetId);

        if (!IsRecentSave(handle))
        {
            UpdateFileState(handle, Tracker::ScriptCanvasFileState::MODIFIED);
        }
    }

    void MainWindow::SourceFileRemoved
        ( AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder
        , AZ::Uuid fileAssetId)
    {
        SourceHandle handle = SourceHandle::FromRelativePath(nullptr, fileAssetId, relativePath);
        {
            if (!IsRecentSave(handle))
            {
                UpdateFileState(handle, Tracker::ScriptCanvasFileState::SOURCE_REMOVED);
            }
        }
    }

    void MainWindow::SignalSceneDirty(SourceHandle assetId)
    {
        UpdateFileState(assetId, Tracker::ScriptCanvasFileState::MODIFIED);
    }

    void MainWindow::PushPreventUndoStateUpdate()
    {
        ++m_preventUndoStateUpdateCount;
    }

    void MainWindow::PopPreventUndoStateUpdate()
    {
        if (m_preventUndoStateUpdateCount > 0)
        {
            --m_preventUndoStateUpdateCount;
        }
    }

    void MainWindow::ClearPreventUndoStateUpdate()
    {
        m_preventUndoStateUpdateCount = 0;
    }

    void MainWindow::UpdateFileState(const SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState)
    {
        m_tabBar->UpdateFileState(assetId, fileState);
    }

    AZ::Outcome<int, AZStd::string> MainWindow::OpenScriptCanvasAssetId(const SourceHandle& fileAssetId, Tracker::ScriptCanvasFileState fileState)
    {
        if (fileAssetId.Id().IsNull())
        {
            return AZ::Failure(AZStd::string("Unable to open asset with invalid asset id"));
        }

        int outTabIndex = m_tabBar->FindTab(fileAssetId);

        if (outTabIndex >= 0)
        {
            m_tabBar->SelectTab(fileAssetId);
            return AZ::Success(outTabIndex);
        }

        auto result = LoadFromFile(fileAssetId.AbsolutePath().c_str());
        if (!result)
        {
            return AZ::Failure(AZStd::string::format("Failed to load graph at %s", fileAssetId.AbsolutePath().c_str()));
        }

        AZ_Warning("ScriptCanvas", result.m_deserializeResult.m_jsonResults.empty()
            , "ScriptCanvas graph loaded with skippable errors: %s", result.m_deserializeResult.m_jsonResults.c_str());

        auto loadedGraph = result.m_handle;
        CompleteDescriptionInPlace(loadedGraph);
        outTabIndex = CreateAssetTab(loadedGraph, fileState);

        if (outTabIndex >= 0)
        {
            AddRecentFile(loadedGraph.AbsolutePath().c_str());
            OpenScriptCanvasAssetImplementation(loadedGraph, fileState);
            return AZ::Success(outTabIndex);
        }
        else
        {
            return AZ::Failure(AZStd::string("Specified asset is in an error state and cannot be properly displayed."));
        }
    }

    AZ::Outcome<int, AZStd::string> MainWindow::OpenScriptCanvasAssetImplementation(const SourceHandle& scriptCanvasAsset, Tracker::ScriptCanvasFileState fileState, int tabIndex)
    {
        const SourceHandle& fileAssetId = scriptCanvasAsset;
//         if (!fileAssetId.IsDescriptionValid())
//         {
//             return AZ::Failure(AZStd::string("Unable to open asset with invalid asset id"));
//         }
//
//         if (!scriptCanvasAsset.IsDescriptionValid())
//         {
//             if (!m_isRestoringWorkspace)
//             {
//                 AZStd::string errorPath = scriptCanvasAsset.Path().c_str();
//
//                 if (errorPath.empty())
//                 {
//                     errorPath = m_errorFilePath;
//                 }
//
//                 if (m_queuedFocusOverride.AnyEquals(fileAssetId))
//                 {
//                     m_queuedFocusOverride = fileAssetId;
//                 }
//
//                 QMessageBox::warning(this, "Unable to open source file", QString("Source File(%1) is in error and cannot be opened").arg(errorPath.c_str()), QMessageBox::StandardButton::Ok);
//             }
//
//             return AZ::Failure(AZStd::string("Source File is in error"));
//         }

        int outTabIndex = m_tabBar->FindTab(fileAssetId);

        if (outTabIndex >= 0)
        {
            m_tabBar->setCurrentIndex(outTabIndex);
            SetActiveAsset(scriptCanvasAsset);
            return AZ::Success(outTabIndex);
        }

        outTabIndex = CreateAssetTab(fileAssetId, fileState, tabIndex);
        SetActiveAsset(scriptCanvasAsset);

        if (outTabIndex == -1)
        {
            return AZ::Failure(AZStd::string::format("Unable to open existing Script Canvas Asset with id %s in the Script Canvas Editor"
                , fileAssetId.ToString().c_str()));
        }

        m_tabBar->setCurrentIndex(outTabIndex);

        AZStd::string assetPath = scriptCanvasAsset.AbsolutePath().c_str();
        if (!assetPath.empty() && !m_loadingNewlySavedFile)
        {
            AddRecentFile(assetPath.c_str());
        }

        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId(scriptCanvasAsset.Get()->GetScriptCanvasId());
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnGraphLoaded, graphCanvasGraphId);
        GeneralAssetNotificationBus::Event(fileAssetId, &GeneralAssetNotifications::OnAssetVisualized);
        return AZ::Success(outTabIndex);
    }

    AZ::Outcome<int, AZStd::string> MainWindow::OpenScriptCanvasAsset(SourceHandle scriptCanvasAssetId, Tracker::ScriptCanvasFileState fileState, int tabIndex)
    {
        if (scriptCanvasAssetId.IsGraphValid())
        {
            return OpenScriptCanvasAssetImplementation(scriptCanvasAssetId, fileState, tabIndex);
        }
        else
        {
            return OpenScriptCanvasAssetId(scriptCanvasAssetId, fileState);
        }
    }

    int MainWindow::CreateAssetTab(const SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState, int tabIndex)
    {
        return m_tabBar->InsertGraphTab(tabIndex, assetId, fileState);
    }

    void MainWindow::RemoveScriptCanvasAsset(const SourceHandle& assetId)
    {
        m_assetCreationRequests.erase(assetId);
        GeneralAssetNotificationBus::Event(assetId, &GeneralAssetNotifications::OnAssetUnloaded);

        if (assetId.IsGraphValid())
        {
            // Disconnect scene and asset editor buses
            GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(assetId.Get()->GetScriptCanvasId());
            GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId
                , &GraphCanvas::AssetEditorNotifications::OnGraphUnloaded, assetId.Get()->GetGraphCanvasGraphId());
        }

        int tabIndex = m_tabBar->FindTab(assetId);
        QVariant tabdata = m_tabBar->tabData(tabIndex);
        if (tabdata.isValid())
        {
            auto tabAssetId = tabdata.value<Widget::GraphTabMetadata>();
            SetActiveAsset(tabAssetId.m_assetId);
        }
    }

    int MainWindow::CloseScriptCanvasAsset(const SourceHandle& assetId)
    {
        int tabIndex = -1;
        if (IsTabOpen(assetId, tabIndex))
        {
            OnTabCloseRequest(tabIndex);
        }

        return tabIndex;
    }

    bool MainWindow::CreateScriptCanvasAssetFor(const TypeDefs::EntityComponentId& requestingEntityId)
    {
        for (auto createdAssetPair : m_assetCreationRequests)
        {
            if (createdAssetPair.second == requestingEntityId)
            {
                return OpenScriptCanvasAssetId(createdAssetPair.first, Tracker::ScriptCanvasFileState::NEW).IsSuccess();
            }
        }

        SourceHandle previousAssetId = m_activeGraph;

        OnFileNew();

        bool createdNewAsset = !(m_activeGraph.AnyEquals(previousAssetId));

        if (createdNewAsset)
        {
            m_assetCreationRequests[m_activeGraph] = requestingEntityId;
        }

        if (m_isRestoringWorkspace)
        {
            m_queuedFocusOverride = m_activeGraph;
        }

        return createdNewAsset;
    }

    bool MainWindow::IsScriptCanvasAssetOpen(const SourceHandle& assetId) const
    {
        return m_tabBar->FindTab(assetId) >= 0;
    }

    const CategoryInformation* MainWindow::FindNodePaletteCategoryInformation(AZStd::string_view categoryPath) const
    {
        return m_nodePaletteModel.FindBestCategoryInformation(categoryPath);
    }

    const NodePaletteModelInformation* MainWindow::FindNodePaletteModelInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const
    {
        return m_nodePaletteModel.FindNodePaletteInformation(nodeType);
    }

    void MainWindow::OpenFile(const char* fullPath)
    {
        auto tabIndex = m_tabBar->FindTabByPath(fullPath);
        if (tabIndex.IsGraphValid())
        {
            SetActiveAsset(tabIndex);
            return;
        }

        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool sourceInfoFound{};
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult
            ( sourceInfoFound
            , &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, fullPath, assetInfo, watchFolder);

        if (!sourceInfoFound)
        {
            QMessageBox::warning(this, "Invalid Source Asset", QString("'%1' is not a valid asset path.").arg(fullPath), QMessageBox::Ok);
            m_errorFilePath = fullPath;
            AZ_Warning("ScriptCanvas", false, "Unable to open file as a ScriptCanvas graph: %s", fullPath);
            return;
        }

        auto result = LoadFromFile(fullPath);
        if (!result)
        {
            QMessageBox::warning(this, "Invalid Source File"
                , QString("'%1' failed to load properly.\nFailure: %2").arg(fullPath).arg(result.m_fileReadErrors.c_str()), QMessageBox::Ok);
            m_errorFilePath = fullPath;
            AZ_Warning("ScriptCanvas", false, "Unable to open file as a ScriptCanvas graph: %s. Failure: %s"
                , fullPath, result.m_fileReadErrors.c_str());
            return;
        }
        else
        {
            AZ_Warning("ScriptCanvas", result.m_deserializeResult.m_jsonResults.empty()
                , "File loaded succesfully with deserialiation errors: %s", result.m_deserializeResult.m_jsonResults.c_str());
        }

        m_errorFilePath.clear();


        auto activeGraph =  SourceHandle::FromRelativePath(result.m_handle, assetInfo.m_assetId.m_guid, assetInfo.m_relativePath);
        activeGraph = SourceHandle::MarkAbsolutePath(activeGraph, fullPath);

        auto openOutcome = OpenScriptCanvasAsset(activeGraph, Tracker::ScriptCanvasFileState::UNMODIFIED);
        if (openOutcome)
        {
            RunGraphValidation(false);
            SetActiveAsset(activeGraph);
            SetRecentAssetId(activeGraph);
        }
        else
        {
            AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
        }
    }

    GraphCanvas::Endpoint MainWindow::HandleProposedConnection(const GraphCanvas::GraphId&, const GraphCanvas::ConnectionId&
        , const GraphCanvas::Endpoint& endpoint, const GraphCanvas::NodeId& nodeId, const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint retVal;

        GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
        GraphCanvas::SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetConnectionType);

        GraphCanvas::NodeId currentTarget = nodeId;

        while (!retVal.IsValid() && currentTarget.IsValid())
        {
            AZStd::vector<AZ::EntityId> targetSlotIds;
            GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, currentTarget, &GraphCanvas::NodeRequests::GetSlotIds);

            AZStd::list< GraphCanvas::Endpoint > endpoints;

            for (const auto& targetSlotId : targetSlotIds)
            {
                GraphCanvas::Endpoint proposedEndpoint(currentTarget, targetSlotId);

                bool canCreate = false;
                GraphCanvas::SlotRequestBus::EventResult(canCreate, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::CanCreateConnectionTo, proposedEndpoint);

                if (canCreate)
                {
                    GraphCanvas::SlotGroup slotGroup = GraphCanvas::SlotGroups::Invalid;
                    GraphCanvas::SlotRequestBus::EventResult(slotGroup, targetSlotId, &GraphCanvas::SlotRequests::GetSlotGroup);

                    bool isVisible = slotGroup != GraphCanvas::SlotGroups::Invalid;
                    GraphCanvas::SlotLayoutRequestBus::EventResult(isVisible, currentTarget, &GraphCanvas::SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

                    if (isVisible)
                    {
                        endpoints.push_back(proposedEndpoint);
                    }
                }
            }

            if (!endpoints.empty())
            {
                if (endpoints.size() == 1)
                {
                    retVal = endpoints.front();
                }
                else
                {
                    QMenu menu;

                    for (GraphCanvas::Endpoint proposedEndpoint : endpoints)
                    {
                        QAction* action = aznew EndpointSelectionAction(proposedEndpoint);
                        menu.addAction(action);
                    }

                    QAction* result = menu.exec(screenPoint);

                    if (result != nullptr)
                    {
                        EndpointSelectionAction* selectedEnpointAction = static_cast<EndpointSelectionAction*>(result);
                        retVal = selectedEnpointAction->GetEndpoint();
                    }
                    else
                    {
                        retVal.Clear();
                    }
                }

                if (retVal.IsValid())
                {
                    // Double safety check. This should be gauranteed by the previous checks. But just extra safety.
                    bool canCreateConnection = false;
                    GraphCanvas::SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::CanCreateConnectionTo, retVal);

                    if (!canCreateConnection)
                    {
                        retVal.Clear();
                    }
                }
            }
            else
            {
                retVal.Clear();
            }

            if (!retVal.IsValid())
            {
                bool isWrapped = false;
                GraphCanvas::NodeRequestBus::EventResult(isWrapped, currentTarget, &GraphCanvas::NodeRequests::IsWrapped);

                if (isWrapped)
                {
                    GraphCanvas::NodeRequestBus::EventResult(currentTarget, currentTarget, &GraphCanvas::NodeRequests::GetWrappingNode);
                }
                else
                {
                    currentTarget.SetInvalid();
                }
            }
        }

        return retVal;
    }

    void MainWindow::OnFileNew()
    {
        static int scriptCanvasEditorDefaultNewNameCount = 0;

        AZStd::string assetPath;
        AZStd::string newAssetName;

        for (;;)
        {
            newAssetName = AZStd::string::format(SourceDescription::GetAssetNamePattern()
                , ++scriptCanvasEditorDefaultNewNameCount);

            AZStd::array<char, AZ::IO::MaxPathLength> assetRootArray;
            if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath(SourceDescription::GetSuggestedSavePath()
                , assetRootArray.data(), assetRootArray.size()))
            {
                AZ_ErrorOnce("Script Canvas", false, "Unable to resolve @projectroot@ path");
            }

            AzFramework::StringFunc::Path::Join(assetRootArray.data(), (newAssetName + SourceDescription::GetFileExtension()).data(), assetPath);
            AZ::Data::AssetInfo assetInfo;

            if (!AssetHelpers::GetSourceInfo(assetPath, assetInfo))
            {
                break;
            }
        }

        auto createOutcome = CreateScriptCanvasAsset(newAssetName);
        if (!createOutcome.IsSuccess())
        {
            AZ_Warning("Script Canvas", createOutcome, "%s", createOutcome.GetError().data());
        }
    }

    int MainWindow::InsertTabForAsset(AZStd::string_view assetPath, SourceHandle assetId, int tabIndex)
    {
        int outTabIndex = -1;

        {
            // Insert tab block
            AZStd::string tabName;
            AzFramework::StringFunc::Path::GetFileName(assetPath.data(), tabName);
            m_tabBar->InsertGraphTab(tabIndex, assetId, Tracker::ScriptCanvasFileState::NEW);

            if (!IsTabOpen(assetId, outTabIndex))
            {
                AZ_Assert(false, AZStd::string::format("Unable to open new Script Canvas Asset with id %s in the Script Canvas Editor", assetId.ToString().c_str()).c_str());
                return -1;
            }

            m_tabBar->setTabToolTip(outTabIndex, assetPath.data());
        }

        return outTabIndex;
    }

    void MainWindow::UpdateUndoCache(SourceHandle)
    {
        UndoCache* undoCache = nullptr;
        UndoRequestBus::EventResult(undoCache, GetActiveScriptCanvasId(), &UndoRequests::GetSceneUndoCache);
        if (undoCache)
        {
            undoCache->UpdateCache(GetActiveScriptCanvasId());
        }
    }

    AZ::Outcome<int, AZStd::string> MainWindow::CreateScriptCanvasAsset(AZStd::string_view assetPath, int tabIndex)
    {
        int outTabIndex = -1;

        ScriptCanvas::DataPtr graph = EditorGraph::Create();
        AZ::Uuid assetId = AZ::Uuid::CreateRandom();
        auto relativeOption = ScriptCanvasEditor::CreateFromAnyPath(SourceHandle(graph, assetId), assetPath);
        SourceHandle handle = relativeOption ? *relativeOption : SourceHandle(graph, assetId);

        outTabIndex = InsertTabForAsset(assetPath, handle, tabIndex);

        if (outTabIndex == -1)
        {
            return AZ::Failure(AZStd::string::format("Script Canvas Asset %.*s is not open in a tab"
                , static_cast<int>(assetPath.size()), assetPath.data()));
        }

        SetActiveAsset(handle);
        PushPreventUndoStateUpdate();

        AZ::EntityId scriptCanvasEntityId = graph->GetGraph()->GetScriptCanvasId();
        GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(scriptCanvasEntityId);
        AZ::EntityId graphCanvasGraphId = GetGraphCanvasGraphId(scriptCanvasEntityId);

        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId
            , &GraphCanvas::AssetEditorNotifications::OnGraphRefreshed, graphCanvasGraphId, graphCanvasGraphId);

        if (IsTabOpen(handle, tabIndex))
        {
            AZStd::string tabName;
            AzFramework::StringFunc::Path::GetFileName(assetPath.data(), tabName);
            m_tabBar->setTabToolTip(tabIndex, assetPath.data());
            m_tabBar->SetTabText(tabIndex, tabName.c_str(), Tracker::ScriptCanvasFileState::NEW);
        }

        if (graphCanvasGraphId.IsValid())
        {
            GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(graphCanvasGraphId);
            GraphCanvas::SceneMimeDelegateRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneMimeDelegateRequests::AddDelegate, m_entityMimeDelegateId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetMimeType, Widget::NodePaletteDockWidget::GetMimeType());
            GraphCanvas::SceneMemberNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneMemberNotifications::OnSceneReady);
        }

        if (IsTabOpen(handle, outTabIndex))
        {
            RefreshActiveAsset();
        }

        PopPreventUndoStateUpdate();


        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId
            , &GraphCanvas::AssetEditorNotifications::OnGraphLoaded, graphCanvasGraphId);

        return AZ::Success(outTabIndex);
    }

    bool MainWindow::OnFileSave()
    {
        auto metaData = m_tabBar->GetTabData(m_activeGraph);
        if (!metaData)
        {
            return false;
        }

        if (metaData && metaData->m_fileState == Tracker::ScriptCanvasFileState::NEW)
        {
            return SaveAssetImpl(m_activeGraph, Save::As);
        }
        else
        {
            return SaveAssetImpl(m_activeGraph, Save::InPlace);
        }
    }

    bool MainWindow::OnFileSaveAs()
    {
        return SaveAssetImpl(m_activeGraph, Save::As);
    }

    bool MainWindow::SaveAssetImpl(const SourceHandle& sourceHandleIn, Save save)
    {
        SourceHandle sourceHandle = sourceHandleIn;

        if (!sourceHandle.IsGraphValid())
        {
            return false;
        }

        if (sourceHandle.Get()->IsScriptEventExtension())
        {
            QMessageBox mb
                ( QMessageBox::Warning
                , QObject::tr("Select ScriptCanvas or ScriptEvent source type:")
                , QObject::tr("Graph defines a ScriptEvent. Press 'Discard' and use Script Event menu to save it as .scriptevent, or 'Ok' to continue to save as .scriptcanvas")
                , QMessageBox::Ok | QMessageBox::Discard
                , nullptr);

            if (mb.exec() == QMessageBox::Discard)
            {
                return false;
            }
        }

        if (sourceHandle.AbsolutePath().Extension() == ".scriptevents")
        {
            auto newPath = sourceHandle.AbsolutePath();
            newPath.ReplaceExtension(".scriptcanvas");

            if (auto relativeOption = ScriptCanvasEditor::CreateFromAnyPath(sourceHandle, newPath))
            {
                sourceHandle = *relativeOption;
            }

            sourceHandle = SourceHandle::MarkAbsolutePath(sourceHandle, newPath);
        }

        if (!m_activeGraph.AnyEquals(sourceHandle))
        {
            OnChangeActiveGraphTab(sourceHandle);
        }

        PrepareAssetForSave(sourceHandle);

        AZStd::string suggestedFilename;
        AZStd::string suggestedDirectoryPath;
        AZStd::string suggestedFileFilter;
        bool isValidFileName = false;

        AZ::IO::FixedMaxPath projectSourcePath = AZ::Utils::GetProjectPath();
        projectSourcePath /= "ScriptCanvas//";
        QString selectedFile;

        if (save == Save::InPlace)
        {
            isValidFileName = true;
            suggestedFileFilter = SourceDescription::GetFileExtension();

            auto sourceHandlePath = sourceHandleIn.AbsolutePath();
            selectedFile = sourceHandleIn.AbsolutePath().Native().c_str();
            suggestedFilename = sourceHandleIn.AbsolutePath().Filename().Native();
            sourceHandlePath.RemoveFilename();
            suggestedDirectoryPath = sourceHandlePath.Native();
        }
        else
        {
            suggestedFileFilter = SourceDescription::GetFileExtension();

            if (sourceHandle.RelativePath().empty() || sourceHandle.RelativePath() == sourceHandle.RelativePath().Filename())
            {
                suggestedDirectoryPath = projectSourcePath.Native();
                suggestedFilename += sourceHandle.RelativePath().Filename().Native();
            }
            else
            {
                auto sourceHandlePath = sourceHandle.AbsolutePath();
                suggestedFilename = sourceHandle.AbsolutePath().Native();
                sourceHandlePath.RemoveFilename();
                suggestedDirectoryPath = sourceHandlePath.Native();
            }

            selectedFile = suggestedFilename.c_str();
        }

        QString filter = suggestedFileFilter.c_str();

        while (!isValidFileName)
        {
            selectedFile = AzQtComponents::FileDialog::GetSaveFileName(this, QObject::tr("Save As..."), suggestedDirectoryPath.data(), QObject::tr("All ScriptCanvas Files (*.scriptcanvas)"));

            // If the selected file is empty that means we just cancelled.
            // So we want to break out.
            if (!selectedFile.isEmpty())
            {
                AZStd::string filePath = selectedFile.toUtf8().data();

                if (!AZ::StringFunc::EndsWith(filePath, SourceDescription::GetFileExtension(), false))
                {
                    filePath += SourceDescription::GetFileExtension();
                }

                AZStd::string fileName;

                if (AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName))
                {
                    isValidFileName = !(fileName.empty());
                }
                else
                {
                    QMessageBox::information(this, "Unable to Save", "File name cannot be empty");
                }
            }
            else
            {
                break;
            }
        }

        if (isValidFileName)
        {
            AZStd::string internalStringFile = selectedFile.toUtf8().data();


            if (!AZ::StringFunc::EndsWith(internalStringFile, SourceDescription::GetFileExtension(), false))
            {
                internalStringFile += SourceDescription::GetFileExtension();
            }

            if (!AssetHelpers::IsValidSourceFile(internalStringFile, GetActiveScriptCanvasId()))
            {
                QMessageBox::warning(this, "Unable to Save", QString("File\n'%1'\n\nDoes not match the asset type of the current Graph.").arg(selectedFile));
                return false;
            }

            SaveAs(internalStringFile, sourceHandle);
            m_newlySavedFile = internalStringFile;
            // Forcing the file add here, since we are creating a new file
            AddRecentFile(m_newlySavedFile.c_str());
            return true;
        }

        return false;
    }

    void MainWindow::OnSaveCallBack(const VersionExplorer::FileSaveResult& result)
    {
        const bool saveSuccess = result.IsSuccess();

        int saveTabIndex = -1;
        SourceHandle memoryAsset;
        {
            int saverIndex = m_tabBar->FindTab(m_fileSaver->GetSource());
            if (saverIndex >= 0)
            {
                saveTabIndex = saverIndex;
                memoryAsset = m_fileSaver->GetSource();
            }
            else
            {
                auto completeDescription = CompleteDescription(m_fileSaver->GetSource());
                if (completeDescription)
                {
                    memoryAsset = *completeDescription;
                    saveTabIndex = m_tabBar->FindTab(memoryAsset);
                }
            }
        }

        AZ_VerifyWarning("ScriptCanvas", saveTabIndex >= 0, "MainWindow::OnSaveCallback failed to find saved graph in tab. Data has been saved, but the ScriptCanvas Editor needs to be closed and re-opened.s")

        AZ::IO::Path fileName = result.absolutePath.Filename();
        fileName = fileName.ReplaceExtension();
        AZStd::string tabName = fileName.Native();

        if (saveSuccess)
        {
            SourceHandle& fileAssetId = memoryAsset;
            int currentTabIndex = m_tabBar->currentIndex();

            AZ::Data::AssetInfo assetInfo;
            fileAssetId = SourceHandle::FromRelativePath(fileAssetId, assetInfo.m_assetId.m_guid, assetInfo.m_relativePath);

            // this line is the most important, as it the assetInfo is as yet unknown for newly saved graphs
            fileAssetId = SourceHandle::MarkAbsolutePath(fileAssetId, result.absolutePath);

            // this path is questionable, this is a save request that is not the current graph
            // We've saved as over a new graph, so we need to close the old one.
            if (saveTabIndex != currentTabIndex)
            {
                // Invalidate the file asset id so we don't delete trigger the asset flow.
                m_tabBar->setTabData(saveTabIndex, QVariant::fromValue(Widget::GraphTabMetadata()));
                m_tabBar->CloseTab(saveTabIndex);
                saveTabIndex = -1;
            }

            if (tabName.at(tabName.size() - 1) == '*' || tabName.at(tabName.size() - 1) == '^')
            {
                tabName = tabName.substr(0, tabName.size() - 2);
            }

            auto tabData = m_tabBar->GetTabData(saveTabIndex);
            tabData->m_fileState = Tracker::ScriptCanvasFileState::UNMODIFIED;
            tabData->m_assetId = fileAssetId;
            m_tabBar->SetTabData(*tabData, saveTabIndex);
            m_tabBar->SetTabText(saveTabIndex, tabName.c_str());
            m_activeGraph = fileAssetId;
        }
        else
        {
            const auto failureMessage = AZStd::string::format("Failed to save %s: %s", tabName.c_str(), result.fileSaveError.c_str());
            QMessageBox::critical(this, QString(), QObject::tr(failureMessage.data()));
        }

        if (m_tabBar->currentIndex() != saveTabIndex && saveTabIndex >= 0)
        {
            m_tabBar->setCurrentIndex(saveTabIndex);
        }

        UpdateAssignToSelectionState();

        OnSaveToast toast(tabName, GetActiveGraphCanvasGraphId(), saveSuccess);

        const bool displayAsNotification = true;
        RunGraphValidation(displayAsNotification);

        m_closeCurrentGraphAfterSave = false;

        EnableAssetView(memoryAsset);

        UpdateSaveState(true);
        UnblockCloseRequests();
        m_fileSaver.reset();
    }

    bool MainWindow::ActivateAndSaveAsset(const SourceHandle& unsavedAssetId)
    {
        SetActiveAsset(unsavedAssetId);
        return OnFileSave();
    }

    void MainWindow::SaveAs(AZStd::string_view path, SourceHandle sourceHandle)
    {
        // clear the AZ::Uuid because it will change
        if (auto relativeOption = ScriptCanvasEditor::CreateFromAnyPath(SourceHandle(sourceHandle, AZ::Uuid::CreateNull()), path))
        {
            sourceHandle = *relativeOption;
        }
        else
        {
            sourceHandle = SourceHandle::FromRelativePath(SourceHandle(sourceHandle, AZ::Uuid::CreateNull()), path);
        }

        DisableAssetView(sourceHandle);
        UpdateSaveState(false);
        m_fileSaver = AZStd::make_unique<VersionExplorer::FileSaver>
                ( nullptr
                , [this](const VersionExplorer::FileSaveResult& fileSaveResult) { OnSaveCallBack(fileSaveResult); });

        MarkRecentSave(sourceHandle);
        m_fileSaver->Save(sourceHandle, path);

        BlockCloseRequests();
    }

    void MainWindow::OnFileOpen()
    {
        const auto sourcePath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "scriptcanvas";
        const QStringList nameFilters = { "All ScriptCanvas Files (*.scriptcanvas)" };

        QFileDialog dialog(nullptr, tr("Open..."), sourcePath.c_str());
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setNameFilters(nameFilters);

        if (dialog.exec() == QDialog::Accepted)
        {
            m_filesToOpen = dialog.selectedFiles();

            OpenNextFile();
        }
    }

    void MainWindow::SetupEditMenu()
    {
        ui->action_Undo->setShortcut(QKeySequence::Undo);
        ui->action_Cut->setShortcut(QKeySequence(QKeySequence::Cut));
        ui->action_Copy->setShortcut(QKeySequence(QKeySequence::Copy));
        ui->action_Paste->setShortcut(QKeySequence(QKeySequence::Paste));
        ui->action_Delete->setShortcut(QKeySequence(QKeySequence::Delete));

        connect(ui->menuEdit, &QMenu::aboutToShow, this, &MainWindow::OnEditMenuShow);

        // Edit Menu
        connect(ui->action_Undo, &QAction::triggered, this, &MainWindow::TriggerUndo);
        connect(ui->action_Redo, &QAction::triggered, this, &MainWindow::TriggerRedo);
        connect(ui->action_Cut, &QAction::triggered, this, &MainWindow::OnEditCut);
        connect(ui->action_Copy, &QAction::triggered, this, &MainWindow::OnEditCopy);
        connect(ui->action_Paste, &QAction::triggered, this, &MainWindow::OnEditPaste);
        connect(ui->action_Duplicate, &QAction::triggered, this, &MainWindow::OnEditDuplicate);
        connect(ui->action_Delete, &QAction::triggered, this, &MainWindow::OnEditDelete);
        connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::RefreshPasteAction);
        connect(ui->action_RemoveUnusedNodes, &QAction::triggered, this, &MainWindow::OnRemoveUnusedNodes);
        connect(ui->action_RemoveUnusedVariables, &QAction::triggered, this, &MainWindow::OnRemoveUnusedVariables);
        connect(ui->action_RemoveUnusedElements, &QAction::triggered, this, &MainWindow::OnRemoveUnusedElements);
        connect(ui->action_Screenshot, &QAction::triggered, this, &MainWindow::OnScreenshot);
        connect(ui->action_SelectAll, &QAction::triggered, this, &MainWindow::OnSelectAll);
        connect(ui->action_SelectInputs, &QAction::triggered, this, &MainWindow::OnSelectInputs);
        connect(ui->action_SelectOutputs, &QAction::triggered, this, &MainWindow::OnSelectOutputs);
        connect(ui->action_SelectConnected, &QAction::triggered, this, &MainWindow::OnSelectConnected);
        connect(ui->action_ClearSelection, &QAction::triggered, this, &MainWindow::OnClearSelection);
        connect(ui->action_EnableSelection, &QAction::triggered, this, &MainWindow::OnEnableSelection);
        connect(ui->action_DisableSelection, &QAction::triggered, this, &MainWindow::OnDisableSelection);
        connect(ui->action_AlignTop, &QAction::triggered, this, &MainWindow::OnAlignTop);
        connect(ui->action_AlignBottom, &QAction::triggered, this, &MainWindow::OnAlignBottom);
        connect(ui->action_AlignLeft, &QAction::triggered, this, &MainWindow::OnAlignLeft);
        connect(ui->action_AlignRight, &QAction::triggered, this, &MainWindow::OnAlignRight);

        ui->action_ZoomIn->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Plus),
                                          QKeySequence(Qt::CTRL + Qt::Key_Equal)
                                        });

        // View Menu
        connect(ui->action_ShowEntireGraph, &QAction::triggered, this, &MainWindow::OnShowEntireGraph);
        connect(ui->action_ZoomIn, &QAction::triggered, this, &MainWindow::OnZoomIn);
        connect(ui->action_ZoomOut, &QAction::triggered, this, &MainWindow::OnZoomOut);
        connect(ui->action_ZoomSelection, &QAction::triggered, this, &MainWindow::OnZoomToSelection);
        connect(ui->action_GotoStartOfChain, &QAction::triggered, this, &MainWindow::OnGotoStartOfChain);
        connect(ui->action_GotoEndOfChain, &QAction::triggered, this, &MainWindow::OnGotoEndOfChain);


        connect(ui->action_GlobalPreferences, &QAction::triggered, [this]()
        {
            ScriptCanvasEditor::SettingsDialog(ui->action_GlobalPreferences->text(), ScriptCanvas::ScriptCanvasId(), this).exec();

            if (m_userSettings)
            {
                if (m_userSettings->m_autoSaveConfig.m_enabled)
                {
                    m_allowAutoSave = true;
                    m_autoSaveTimer.setInterval(m_userSettings->m_autoSaveConfig.m_timeSeconds * 1000);
                }
                else
                {
                    m_allowAutoSave = false;
                }
            }
        });

        connect(ui->action_GraphPreferences, &QAction::triggered, [this]() {
            ScriptCanvas::ScriptCanvasId scriptCanvasId = GetActiveScriptCanvasId();
            if (!scriptCanvasId.IsValid())
            {
                return;
            }

            m_autoSaveTimer.stop();

            ScriptCanvasEditor::SettingsDialog(ui->action_GraphPreferences->text(), scriptCanvasId, this).exec();
        });
    }

    void MainWindow::OnEditMenuShow()
    {
        RefreshGraphPreferencesAction();

        ui->action_Screenshot->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
        ui->menuSelect->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
        ui->action_ClearSelection->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
        ui->menuAlign->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
    }

    void MainWindow::RefreshPasteAction()
    {
        AZStd::string copyMimeType;
        GraphCanvas::SceneRequestBus::EventResult(copyMimeType, GetActiveGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetCopyMimeType);

        const bool pasteableClipboard = (!copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str()))
                                        || GraphVariablesTableView::HasCopyVariableData();

        ui->action_Paste->setEnabled(pasteableClipboard);
    }

    void MainWindow::RefreshGraphPreferencesAction()
    {
        ui->action_GraphPreferences->setEnabled(GetActiveGraphCanvasGraphId().IsValid());
    }

    void MainWindow::OnEditCut()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CutSelection);
    }

    void MainWindow::OnEditCopy()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CopySelection);
    }

    void MainWindow::OnEditPaste()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Paste);
    }

    void MainWindow::OnEditDuplicate()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
    }

    void MainWindow::OnEditDelete()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
    }

    void MainWindow::OnRemoveUnusedVariables()
    {
        AZ::EntityId scriptCanvasGraphId = GetActiveScriptCanvasId();
        EditorGraphRequestBus::Event(scriptCanvasGraphId, &EditorGraphRequests::RemoveUnusedVariables);
    }

    void MainWindow::OnRemoveUnusedNodes()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
    }

    void MainWindow::OnRemoveUnusedElements()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::RemoveUnusedElements);
    }

    void MainWindow::OnScreenshot()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
    }

    void MainWindow::OnSelectAll()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SelectAll);
    }

    void MainWindow::OnSelectInputs()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Input);
    }

    void MainWindow::OnSelectOutputs()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Output);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);
    }

    void MainWindow::OnSelectConnected()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
    }

    void MainWindow::OnClearSelection()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
    }

    void MainWindow::OnEnableSelection()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::EnableSelection);
    }

    void MainWindow::OnDisableSelection()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::DisableSelection);
    }

    void MainWindow::OnAlignTop()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Top;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MainWindow::OnAlignBottom()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MainWindow::OnAlignLeft()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Left;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MainWindow::OnAlignRight()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Right;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MainWindow::AlignSelected(const GraphCanvas::AlignConfig& alignConfig)
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        AZStd::vector< GraphCanvas::NodeId > selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);

        GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void MainWindow::OnShowEntireGraph()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
    }

    void MainWindow::OnZoomIn()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomIn);
    }

    void MainWindow::OnZoomOut()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomOut);
    }

    void MainWindow::OnZoomToSelection()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnSelection);
    }

    void MainWindow::OnGotoStartOfChain()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnStartOfChain);
    }

    void MainWindow::OnGotoEndOfChain()
    {
        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnEndOfChain);
    }

    void MainWindow::OnCanUndoChanged(bool canUndo)
    {
        ui->action_Undo->setEnabled(canUndo);
    }

    void MainWindow::OnCanRedoChanged(bool canRedo)
    {
        ui->action_Redo->setEnabled(canRedo);
    }

    bool MainWindow::CanShowNetworkSettings()
    {
        return m_userSettings->m_experimentalSettings.GetShowNetworkProperties();
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::HandleContextMenu(GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
        GraphCanvas::GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        editorContextMenu.RefreshActions(graphCanvasGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        GraphCanvas::ContextMenuAction* contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(result);

        if (contextMenuAction)
        {
            return contextMenuAction->TriggerAction(graphCanvasGraphId, sceneVector);
        }
        else
        {
            return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }
    }

    void MainWindow::OnAutoSave()
    {
        if (m_allowAutoSave)
        {
            const Tracker::ScriptCanvasFileState& fileState = GetAssetFileState(m_activeGraph);
            if (fileState != Tracker::ScriptCanvasFileState::INVALID && fileState != Tracker::ScriptCanvasFileState::NEW)
            {
                OnFileSaveCaller();
            }
        }
    }

    //! GeneralRequestBus
    void MainWindow::OnChangeActiveGraphTab(SourceHandle assetId)
    {
        SetActiveAsset(assetId);
    }

    AZ::EntityId MainWindow::GetActiveGraphCanvasGraphId() const
    {
        AZ::EntityId graphId{};

        if (m_activeGraph.IsGraphValid())
        {
            EditorGraphRequestBus::EventResult
                ( graphId, m_activeGraph.Get()->GetScriptCanvasId(), &EditorGraphRequests::GetGraphCanvasGraphId);
        }

        return graphId;
    }

    ScriptCanvas::ScriptCanvasId MainWindow::GetActiveScriptCanvasId() const
    {
        return FindScriptCanvasIdByAssetId(m_activeGraph);
    }

    GraphCanvas::GraphId MainWindow::GetGraphCanvasGraphId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        AZ::EntityId graphId{};
        EditorGraphRequestBus::EventResult(graphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);
        return graphId;
    }

    GraphCanvas::GraphId MainWindow::FindGraphCanvasGraphIdByAssetId(const SourceHandle& assetId) const
    {
        AZ::EntityId graphId{};

        if (assetId.IsGraphValid())
        {
            EditorGraphRequestBus::EventResult
                ( graphId, assetId.Get()->GetScriptCanvasId(), &EditorGraphRequests::GetGraphCanvasGraphId);
        }

        return graphId;
    }

    ScriptCanvas::ScriptCanvasId MainWindow::FindScriptCanvasIdByAssetId(const SourceHandle& assetId) const
    {
        return assetId.IsGraphValid() ? assetId.Get()->GetScriptCanvasId() : ScriptCanvas::ScriptCanvasId{};
    }

    ScriptCanvas::ScriptCanvasId MainWindow::GetScriptCanvasId(const GraphCanvas::GraphId& graphCanvasGraphId) const
    {
        return m_tabBar->FindScriptCanvasIdFromGraphCanvasId(graphCanvasGraphId);
    }

    bool MainWindow::IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const
    {
        bool isActive = false;
        UndoRequestBus::EventResult(isActive, GetScriptCanvasId(graphCanvasGraphId), &UndoRequests::IsActive);
        return isActive;
    }

    bool MainWindow::IsScriptCanvasInUndoRedo(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        if (GetActiveScriptCanvasId() == scriptCanvasId)
        {
            bool isInUndoRedo = false;
            UndoRequestBus::BroadcastResult(isInUndoRedo, &UndoRequests::IsActive);
            return isInUndoRedo;
        }

        return false;
    }

    bool MainWindow::IsActiveGraphInUndoRedo() const
    {
        bool isActive = false;
        UndoRequestBus::EventResult(isActive, GetActiveScriptCanvasId(), &UndoRequests::IsActive);
        return isActive;
    }

    QVariant MainWindow::GetTabData(const SourceHandle& assetId)
    {
        for (int tabIndex = 0; tabIndex < m_tabBar->count(); ++tabIndex)
        {
            QVariant tabdata = m_tabBar->tabData(tabIndex);
            if (tabdata.isValid())
            {
                auto tabAssetId = tabdata.value<Widget::GraphTabMetadata>();
                if (tabAssetId.m_assetId.AnyEquals(assetId))
                {
                    return tabdata;
                }
            }
        }
        return QVariant();
    }

    bool MainWindow::IsTabOpen(const SourceHandle& fileAssetId, int& outTabIndex) const
    {
        int tabIndex = m_tabBar->FindTab(fileAssetId);
        if (-1 != tabIndex)
        {
            outTabIndex = tabIndex;
            return true;
        }
        return false;
    }

    void MainWindow::ReconnectSceneBuses(SourceHandle previousAsset, SourceHandle nextAsset)
    {
        // Disconnect previous asset
        AZ::EntityId previousScriptCanvasSceneId;
        if (previousAsset.IsGraphValid())
        {
            previousScriptCanvasSceneId = previousAsset.Get()->GetScriptCanvasId();
            GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(previousScriptCanvasSceneId);
        }

        AZ::EntityId nextAssetGraphCanvasId;
        if (nextAsset.IsGraphValid())
        {
            // Connect the next asset
            EditorGraphRequestBus::EventResult(nextAssetGraphCanvasId, nextAsset.Get()->GetScriptCanvasId(), &EditorGraphRequests::GetGraphCanvasGraphId);

            if (nextAssetGraphCanvasId.IsValid())
            {
                GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(nextAssetGraphCanvasId);
                GraphCanvas::SceneMimeDelegateRequestBus::Event(nextAssetGraphCanvasId, &GraphCanvas::SceneMimeDelegateRequests::AddDelegate, m_entityMimeDelegateId);

                GraphCanvas::SceneRequestBus::Event(nextAssetGraphCanvasId, &GraphCanvas::SceneRequests::SetMimeType, Widget::NodePaletteDockWidget::GetMimeType());
                GraphCanvas::SceneMemberNotificationBus::Event(nextAssetGraphCanvasId, &GraphCanvas::SceneMemberNotifications::OnSceneReady);
            }
        }

        // Notify about the graph refresh
        GraphCanvas::AssetEditorNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphCanvas::AssetEditorNotifications::OnGraphRefreshed, previousScriptCanvasSceneId, nextAssetGraphCanvasId);
    }

    void MainWindow::SetActiveAsset(const SourceHandle& fileAssetId)
    {
        if (m_activeGraph.AnyEquals(fileAssetId))
        {
            return;
        }

        if (fileAssetId.IsGraphValid())
        {
            if (m_tabBar->FindTab(fileAssetId) >= 0)
            {
                QSignalBlocker signalBlocker(m_tabBar);
                m_tabBar->SelectTab(fileAssetId);
            }
        }

        if (m_activeGraph.IsGraphValid())
        {
            // If we are saving the asset, the Id may have changed from the in-memory to the file asset Id, in that case,
            // there's no need to hide the view or remove the widget
            auto oldTab = m_tabBar->FindTab(m_activeGraph);
            if (auto view = m_tabBar->ModTabView(oldTab))
            {
                view->hide();
                m_layout->removeWidget(view);
                m_tabBar->ClearTabView(oldTab);
            }
        }

        if (fileAssetId.IsGraphValid())
        {
            SourceHandle previousAssetId = m_activeGraph;
            m_activeGraph = fileAssetId;
            RefreshActiveAsset();
            ReconnectSceneBuses(previousAssetId, m_activeGraph);
        }
        else
        {
            SourceHandle previousAssetId = m_activeGraph;
            m_activeGraph.Clear();
            m_emptyCanvas->show();
            ReconnectSceneBuses(previousAssetId, m_activeGraph);
            SignalActiveSceneChanged(SourceHandle());
        }

        UpdateUndoCache(fileAssetId);
        RefreshSelection();
    }

    void MainWindow::RefreshActiveAsset()
    {
        if (m_activeGraph.IsGraphValid())
        {
            if (auto view = m_tabBar->ModOrCreateTabView(m_tabBar->FindTab(m_activeGraph)))
            {
                view->ShowScene(m_activeGraph.Get()->GetScriptCanvasId());
                m_layout->addWidget(view);
                view->show();
                m_emptyCanvas->hide();
                SignalActiveSceneChanged(m_activeGraph);
            }
            else
            {
                SetActiveAsset({});
            }
        }
    }

    void MainWindow::Clear()
    {
        m_tabBar->CloseAllTabs();
        SetActiveAsset({});
    }

    void MainWindow::OnTabCloseButtonPressed(int index)
    {
        QVariant tabdata = m_tabBar->tabData(index);
        if (tabdata.isValid())
        {
            Widget::GraphTabMetadata tabMetadata = tabdata.value<Widget::GraphTabMetadata>();
            Tracker::ScriptCanvasFileState fileState = tabMetadata.m_fileState;
            UnsavedChangesOptions saveDialogResults = UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING;

            if (fileState == Tracker::ScriptCanvasFileState::NEW
            || fileState == Tracker::ScriptCanvasFileState::MODIFIED
            || fileState == Tracker::ScriptCanvasFileState::SOURCE_REMOVED)
            {
                SetActiveAsset(tabMetadata.m_assetId);
                saveDialogResults = ShowSaveDialog(m_tabBar->tabText(index).toUtf8().constData());
            }

            if (saveDialogResults == UnsavedChangesOptions::SAVE)
            {
                m_closeCurrentGraphAfterSave = true;
                SaveAssetImpl(tabMetadata.m_assetId, fileState == Tracker::ScriptCanvasFileState::NEW ? Save::As : Save::InPlace);
            }
            else if (saveDialogResults == UnsavedChangesOptions::CONTINUE_WITHOUT_SAVING)
            {
                OnTabCloseRequest(index);
            }
        }
    }

    void MainWindow::SaveTab(int index)
    {
        QVariant tabdata = m_tabBar->tabData(index);
        if (tabdata.isValid())
        {
            auto assetId = tabdata.value<Widget::GraphTabMetadata>();
            SaveAssetImpl(assetId.m_assetId, Save::InPlace);
        }
    }

    void MainWindow::CloseAllTabs()
    {
        m_isClosingTabs = true;
        m_skipTabOnClose.Clear();

        CloseNextTab();
    }

    void MainWindow::CloseAllTabsBut(int index)
    {
        QVariant tabdata = m_tabBar->tabData(index);
        if (tabdata.isValid())
        {
            auto assetId = tabdata.value<Widget::GraphTabMetadata>().m_assetId;

            m_isClosingTabs = true;
            m_skipTabOnClose = assetId;
            CloseNextTab();
        }
    }

    void MainWindow::CopyPathToClipboard(int index)
    {
        QVariant tabdata = m_tabBar->tabData(index);

        if (tabdata.isValid())
        {
            QClipboard* clipBoard = QGuiApplication::clipboard();

            auto assetId = tabdata.value<Widget::GraphTabMetadata>();
            if (!assetId.m_assetId.AbsolutePath().empty())
            {
                clipBoard->setText(assetId.m_assetId.AbsolutePath().c_str());
            }
            else
            {
                clipBoard->setText(m_tabBar->tabText(index));
            }
        }
    }

    void MainWindow::OnActiveFileStateChanged()
    {
        UpdateAssignToSelectionState();
    }

    void MainWindow::CloseNextTab()
    {
        if (m_isClosingTabs)
        {
            if (m_tabBar->count() == 0
                || (m_tabBar->count() == 1 && m_skipTabOnClose.IsGraphValid()))
            {
                m_isClosingTabs = false;
                m_skipTabOnClose.Clear();
                return;
            }

            int tab = 0;

            while (tab < m_tabBar->count())
            {
                QVariant tabdata = m_tabBar->tabData(tab);
                if (tabdata.isValid())
                {
                    auto assetId = tabdata.value<Widget::GraphTabMetadata>();

                    if (!assetId.m_assetId.AnyEquals(m_skipTabOnClose))
                    {
                        break;
                    }
                }

                tab++;
            }

            m_tabBar->tabCloseRequested(tab);
        }
    }

    void MainWindow::OnTabCloseRequest(int index)
    {
        QVariant tabdata = m_tabBar->tabData(index);
        if (tabdata.isValid())
        {
            auto tabAssetId = tabdata.value<Widget::GraphTabMetadata>();


            if (tabAssetId.m_canvasWidget)
            {
                tabAssetId.m_canvasWidget->hide();
            }

            bool activeSet = false;

            if (tabAssetId.m_assetId.AnyEquals(m_activeGraph))
            {
                SetActiveAsset({});
                activeSet = true;
            }

            m_tabBar->CloseTab(index);
            m_tabBar->update();
            RemoveScriptCanvasAsset(tabAssetId.m_assetId);

            if (!activeSet && m_tabBar->count() == 0)
            {
                // The last tab has been removed.
                SetActiveAsset({});
            }

            // Handling various close all events because the save is async need to deal with this in a bunch of different ways
            // Always want to trigger this, even if we don't have any active tabs to avoid doubling the clean-up
            // information
            AddSystemTickAction(SystemTickActionFlag::CloseNextTabAction);
        }
    }

    void MainWindow::OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste)
    {
        // Handle special-case where if a method node is created that has an AZ::Event output slot,
        // we will automatically create the AZ::Event Handler node for the user
        GraphCanvas::GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        AZStd::vector<GraphCanvas::SlotId> outputDataSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(outputDataSlotIds, nodeId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::DataSlot);

        for (const auto& slotId : outputDataSlotIds)
        {

            if (!IsInUndoRedo(graphCanvasGraphId) && !isPaste && CreateAzEventHandlerSlotMenuAction::FindBehaviorMethodWithAzEventReturn(graphCanvasGraphId, slotId))
            {
                CreateAzEventHandlerSlotMenuAction eventHandlerAction(this);
                eventHandlerAction.RefreshAction(graphCanvasGraphId, slotId);

                AZ::Vector2 position;
                GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

                eventHandlerAction.TriggerAction(graphCanvasGraphId, position);
                break;
            }
        }
    }

    void MainWindow::OnSelectionChanged()
    {
        QueuePropertyGridUpdate();
    }

    void MainWindow::OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variablePropertyIds)
    {
        m_selectedVariableIds = variablePropertyIds;

        QueuePropertyGridUpdate();
    }

    void MainWindow::QueuePropertyGridUpdate()
    {
        // Selection will be ignored when a delete operation is are taking place to prevent slowdown from processing
        // too many events at once.
        if (!m_ignoreSelection && !m_isInAutomation)
        {
            AddSystemTickAction(SystemTickActionFlag::RefreshPropertyGrid);
        }
    }

    void MainWindow::DequeuePropertyGridUpdate()
    {
        RemoveSystemTickAction(SystemTickActionFlag::RefreshPropertyGrid);
    }

    void MainWindow::SetDefaultLayout()
    {
        // Disable updates while we restore the layout to avoid temporary glitches
        // as the panes are moved around
        setUpdatesEnabled(false);

        if (m_commandLine)
        {
            m_commandLine->hide();
        }

        if (m_validationDockWidget)
        {
            addDockWidget(Qt::BottomDockWidgetArea, m_validationDockWidget);
            m_validationDockWidget->setFloating(false);
            m_validationDockWidget->hide();
        }

        if (m_logPanel)
        {
            addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);
            m_logPanel->setFloating(false);
            m_logPanel->hide();
        }

        if (m_minimap)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_minimap);
            m_minimap->setFloating(false);
            m_minimap->show();
        }

        if (m_nodePalette)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_nodePalette);
            m_nodePalette->setFloating(false);
            m_nodePalette->show();
        }

        if (m_variableDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_variableDockWidget);
            m_variableDockWidget->setFloating(false);
            m_variableDockWidget->show();
        }

        if (m_unitTestDockWidget)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_unitTestDockWidget);
            m_unitTestDockWidget->setFloating(false);
            m_unitTestDockWidget->hide();
        }

        if (m_loggingWindow)
        {
            addDockWidget(Qt::BottomDockWidgetArea, m_loggingWindow);
            m_loggingWindow->setFloating(false);
            m_loggingWindow->hide();
        }

        if (m_propertyGrid)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_propertyGrid);
            m_propertyGrid->setFloating(false);
            m_propertyGrid->show();
        }

        if (m_bookmarkDockWidget)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_bookmarkDockWidget);
            m_bookmarkDockWidget->setFloating(false);
            m_bookmarkDockWidget->hide();
        }

        if (m_minimap)
        {
            addDockWidget(Qt::RightDockWidgetArea, m_minimap);
            m_minimap->setFloating(false);
            m_minimap->show();
        }

        resizeDocks(
        { m_nodePalette, m_propertyGrid },
        { static_cast<int>(size().width() * 0.15f), static_cast<int>(size().width() * 0.2f) },
            Qt::Horizontal);

        resizeDocks({ m_nodePalette, m_minimap },
        { static_cast<int>(size().height() * 0.70f), static_cast<int>(size().height() * 0.30f) },
            Qt::Vertical);


        resizeDocks({ m_propertyGrid, m_variableDockWidget },
        { static_cast<int>(size().height() * 0.70f), static_cast<int>(size().height() * 0.30f) },
            Qt::Vertical);

        resizeDocks({ m_validationDockWidget }, { static_cast<int>(size().height() * 0.01) }, Qt::Vertical);

        // Disabled until debugger is implemented
        //resizeDocks({ m_logPanel }, { static_cast<int>(size().height() * 0.1f) }, Qt::Vertical);

        // Re-enable updates now that we've finished adjusting the layout
        setUpdatesEnabled(true);

        m_defaultLayout = saveState();

        UpdateViewMenu();
    }

    void MainWindow::RefreshSelection()
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId = GetActiveScriptCanvasId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        bool hasCopiableSelection = false;
        bool hasSelection = false;

        if (m_activeGraph.IsGraphValid())
        {
            if (graphCanvasGraphId.IsValid())
            {
                // Get the selected nodes.
                GraphCanvas::SceneRequestBus::EventResult(hasCopiableSelection, graphCanvasGraphId, &GraphCanvas::SceneRequests::HasCopiableSelection);
            }

            AZStd::vector< AZ::EntityId > selection;
            GraphCanvas::SceneRequestBus::EventResult(selection, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetSelectedItems);

            selection.reserve(selection.size() + m_selectedVariableIds.size());
            selection.insert(selection.end(), m_selectedVariableIds.begin(), m_selectedVariableIds.end());

            if (!selection.empty())
            {
                hasSelection = true;
                m_propertyGrid->SetSelection(selection);
            }
            else
            {
                m_propertyGrid->ClearSelection();
            }
        }
        else
        {
            m_propertyGrid->ClearSelection();
        }

        // cut, copy and duplicate only works for specified items
        ui->action_Cut->setEnabled(hasCopiableSelection);
        ui->action_Copy->setEnabled(hasCopiableSelection);
        ui->action_Duplicate->setEnabled(hasCopiableSelection);

        // Delete will work for anything that is selectable
        ui->action_Delete->setEnabled(hasSelection);
    }

    void MainWindow::OnViewNodePalette()
    {
        if (m_nodePalette)
        {
            m_nodePalette->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewMiniMap()
    {
        if (m_minimap)
        {
            m_minimap->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewLogWindow()
    {
        if (m_loggingWindow)
        {
            m_loggingWindow->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewGraphValidation()
    {
        if (m_validationDockWidget)
        {
            m_validationDockWidget->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewDebuggingWindow()
    {
        if (m_loggingWindow)
        {
            m_loggingWindow->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewUnitTestManager()
    {
        if (m_unitTestDockWidget == nullptr)
        {
            CreateUnitTestWidget();
        }

        if (m_unitTestDockWidget)
        {
            m_unitTestDockWidget->show();
            m_unitTestDockWidget->raise();
            m_unitTestDockWidget->activateWindow();
        }
    }

    void MainWindow::OnViewStatisticsPanel()
    {
        if (m_statisticsDialog)
        {
            m_statisticsDialog->InitStatisticsWindow();
            m_statisticsDialog->show();
            m_statisticsDialog->raise();
            m_statisticsDialog->activateWindow();
        }
    }

    void MainWindow::OnViewPresetsEditor()
    {
        if (m_presetEditor && m_presetWrapper)
        {
            QSize boundingBox = size();
            QPointF newPosition = mapToGlobal(QPoint(aznumeric_cast<int>(boundingBox.width() * 0.5f), aznumeric_cast<int>(boundingBox.height() * 0.5f)));

            m_presetEditor->show();

            m_presetWrapper->show();
            m_presetWrapper->raise();
            m_presetWrapper->activateWindow();

            QRect geometry = m_presetWrapper->geometry();
            QSize originalSize = geometry.size();

            newPosition.setX(newPosition.x() - geometry.width() * 0.5f);
            newPosition.setY(newPosition.y() - geometry.height() * 0.5f);

            geometry.setTopLeft(newPosition.toPoint());

            geometry.setWidth(originalSize.width());
            geometry.setHeight(originalSize.height());

            m_presetWrapper->setGeometry(geometry);
        }
    }

    void MainWindow::OnViewProperties()
    {
        if (m_propertyGrid)
        {
            m_propertyGrid->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnViewDebugger()
    {
    }

    void MainWindow::OnViewCommandLine()
    {
        if (m_commandLine->isVisible())
        {
            m_commandLine->hide();
        }
        else
        {
            m_commandLine->show();
        }
    }

    void MainWindow::OnViewLog()
    {
        if (m_logPanel)
        {
            m_logPanel->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnBookmarks()
    {
        if (m_bookmarkDockWidget)
        {
            m_bookmarkDockWidget->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnVariableManager()
    {
        if (m_variableDockWidget)
        {
            m_variableDockWidget->toggleViewAction()->trigger();
        }
    }

    void MainWindow::OnRestoreDefaultLayout()
    {
        if (!m_defaultLayout.isEmpty())
        {
            restoreState(m_defaultLayout);
            UpdateViewMenu();
        }
    }

    void MainWindow::UpdateViewMenu()
    {
        if (ui->action_ViewBookmarks->isChecked() != m_bookmarkDockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewBookmarks);
            ui->action_ViewBookmarks->setChecked(m_bookmarkDockWidget->isVisible());
        }

        if (ui->action_ViewMiniMap->isChecked() != m_minimap->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewMiniMap);
            ui->action_ViewMiniMap->setChecked(m_minimap->isVisible());
        }

        if (ui->action_ViewNodePalette->isChecked() != m_nodePalette->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewNodePalette);
            ui->action_ViewNodePalette->setChecked(m_nodePalette->isVisible());
        }

        if (ui->action_ViewProperties->isChecked() != m_propertyGrid->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewProperties);
            ui->action_ViewProperties->setChecked(m_propertyGrid->isVisible());
        }

        if (ui->action_ViewVariableManager->isChecked() != m_variableDockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewVariableManager);
            ui->action_ViewVariableManager->setChecked(m_variableDockWidget->isVisible());
        }

        if (ui->action_ViewLogWindow->isChecked() != m_loggingWindow->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_ViewLogWindow);
            ui->action_ViewLogWindow->setChecked(m_loggingWindow->isVisible());
        }

        if (ui->action_GraphValidation->isChecked() != m_validationDockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(ui->action_GraphValidation);

            ui->action_GraphValidation->setChecked(m_validationDockWidget->isVisible());
        }

        if (ui->action_Debugging->isChecked() != m_loggingWindow->isVisible())
        {
            ui->action_Debugging->setChecked(m_loggingWindow->isVisible());
        }

        // Want these two elements to be mutually exclusive.
        if (m_statusWidget->isVisible() == m_validationDockWidget->isVisible())
        {
            statusBar()->setVisible(!m_validationDockWidget->isVisible());
            m_statusWidget->setVisible(!m_validationDockWidget->isVisible());
        }
    }

    void MainWindow::DeleteNodes(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<AZ::EntityId>& nodes)
    {
        // clear the selection then delete the nodes that were selected
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ nodes.begin(), nodes.end() });
    }

    void MainWindow::DeleteConnections(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<AZ::EntityId>& connections)
    {
        ScopedVariableSetter<bool> scopedIgnoreSelection(m_ignoreSelection, true);
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ connections.begin(), connections.end() });
    }

    void MainWindow::DisconnectEndpoints(const AZ::EntityId& graphCanvasGraphId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints)
    {
        AZStd::unordered_set<AZ::EntityId> connections;
        for (const auto& endpoint : endpoints)
        {
            AZStd::vector<AZ::EntityId> endpointConnections;
            GraphCanvas::SceneRequestBus::EventResult(endpointConnections, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetConnectionsForEndpoint, endpoint);
            connections.insert(endpointConnections.begin(), endpointConnections.end());
        }
        DeleteConnections(graphCanvasGraphId, { connections.begin(), connections.end() });
    }

    void MainWindow::ShowInterpreter()
    {
        using namespace ScriptCanvasEditor;

        if (!m_interpreterWidget)
        {
            m_interpreterWidget = AZStd::make_unique<InterpreterWidget>();
        }

        if (m_interpreterWidget)
        {
            m_interpreterWidget->show();
            m_interpreterWidget->raise();
            m_interpreterWidget->activateWindow();
        }
    }

    void MainWindow::RunUpgradeTool()
    {
        using namespace VersionExplorer;
        auto versionExplorer = AZStd::make_unique<Controller>(this);
        versionExplorer->exec();

        const ModificationResults* result = nullptr;
        ModelRequestsBus::BroadcastResult(result, &ModelRequestsTraits::GetResults);
        if (result && !result->m_failures.empty())
        {
            // If there are graphs that need manual correction, show the helper
            UpgradeHelper* upgradeHelper = new UpgradeHelper(this);
            upgradeHelper->show();
        }
    }

    void MainWindow::OnShowValidationErrors()
    {
        m_userSettings->m_showValidationErrors = true;

        if (!m_validationDockWidget->isVisible())
        {
            OnViewGraphValidation();

            // If the window wasn't visible, it doesn't seem to get the signals.
            // So need to manually prompt it to get the desired result
            m_validationDockWidget->OnShowErrors();
        }
    }

    void MainWindow::OnShowValidationWarnings()
    {
        m_userSettings->m_showValidationWarnings = true;

        if (!m_validationDockWidget->isVisible())
        {
            OnViewGraphValidation();

            // If the window wasn't visible, it doesn't seem to get the signals.
            // So need to manually prompt it to get the desired result
            m_validationDockWidget->OnShowWarnings();
        }
    }

    void MainWindow::OnValidateCurrentGraph()
    {
        const bool displayToastNotification = false;
        RunGraphValidation(displayToastNotification);
    }

    void MainWindow::RunGraphValidation(bool displayToastNotification)
    {
        m_validationDockWidget->OnRunValidator(displayToastNotification);

        if (m_validationDockWidget->HasValidationIssues())
        {
            OpenValidationPanel();
        }
    }

    void MainWindow::OnViewParamsChanged(const GraphCanvas::ViewParams& viewParams)
    {
        AZ_UNUSED(viewParams);
        RestartAutoTimerSave();
    }

    void MainWindow::OnZoomChanged(qreal)
    {
        RestartAutoTimerSave();
    }

    void MainWindow::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList&, const AzToolsFramework::EntityIdList&)
    {
        UpdateAssignToSelectionState();
    }

    void MainWindow::UpdateMenuState(bool enabled)
    {
        m_validateGraphToolButton->setEnabled(enabled);
        ui->menuRemove_Unused->setEnabled(enabled);
        ui->action_RemoveUnusedNodes->setEnabled(enabled);
        ui->action_RemoveUnusedVariables->setEnabled(enabled);
        ui->action_RemoveUnusedElements->setEnabled(enabled);

        ui->action_ZoomIn->setEnabled(enabled);
        ui->action_ZoomOut->setEnabled(enabled);
        ui->action_ZoomSelection->setEnabled(enabled);
        ui->action_ShowEntireGraph->setEnabled(enabled);
        ui->menuGo_To->setEnabled(enabled);
        ui->action_GotoStartOfChain->setEnabled(enabled);
        ui->action_GotoEndOfChain->setEnabled(enabled);
        ui->actionZoom_To->setEnabled(enabled);

        ui->action_EnableSelection->setEnabled(enabled);
        ui->action_DisableSelection->setEnabled(enabled);

        m_createFunctionOutput->setEnabled(enabled);
        m_createFunctionInput->setEnabled(enabled);
        m_takeScreenshot->setEnabled(enabled);

        // File Menu
        ui->action_Close->setEnabled(enabled);

        RefreshGraphPreferencesAction();

        UpdateAssignToSelectionState();
        UpdateUndoRedoState();
    }

    void MainWindow::OnWorkspaceRestoreStart()
    {
        m_isRestoringWorkspace = true;
    }

    void MainWindow::OnWorkspaceRestoreEnd(SourceHandle lastFocusAsset)
    {
        if (m_isRestoringWorkspace)
        {
            m_isRestoringWorkspace = false;

            if (m_queuedFocusOverride.IsGraphValid())
            {
                SetActiveAsset(m_queuedFocusOverride);
                m_queuedFocusOverride.Clear();
            }
            else if (lastFocusAsset.IsGraphValid())
            {
                SetActiveAsset(lastFocusAsset);
            }

            if (!m_activeGraph.IsGraphValid())
            {
                if (m_tabBar->count() > 0)
                {
                    if (m_tabBar->currentIndex() != 0)
                    {
                        m_tabBar->setCurrentIndex(0);
                    }
                    else
                    {
                        SetActiveAsset(m_tabBar->FindAssetId(0));
                    }
                }
                else
                {
                    SetActiveAsset({});
                }
            }
        }
    }

    void MainWindow::UpdateAssignToSelectionState()
    {
        bool buttonEnabled = m_activeGraph.IsGraphValid();

        if (buttonEnabled)
        {
            const Tracker::ScriptCanvasFileState& fileState = GetAssetFileState(m_activeGraph);
            if (fileState == Tracker::ScriptCanvasFileState::INVALID || fileState == Tracker::ScriptCanvasFileState::NEW || fileState == Tracker::ScriptCanvasFileState::SOURCE_REMOVED)
            {
                buttonEnabled = false;
            }

            m_assignToSelectedEntity->setEnabled(buttonEnabled);
        }
        else
        {
            m_assignToSelectedEntity->setEnabled(false);
        }
    }

    void MainWindow::UpdateUndoRedoState()
    {
        bool isEnabled = false;
        UndoRequestBus::EventResult(isEnabled, GetActiveScriptCanvasId(), &UndoRequests::CanUndo);

        ui->action_Undo->setEnabled(isEnabled);

        isEnabled = false;
        UndoRequestBus::EventResult(isEnabled, GetActiveScriptCanvasId(), &UndoRequests::CanRedo);

        ui->action_Redo->setEnabled(isEnabled);
    }

    void MainWindow::UpdateSaveState(bool enabled)
    {
         ui->action_Save->setEnabled(enabled);
         ui->action_Save_As->setEnabled(enabled);
    }

    void MainWindow::CreateFunctionInput()
    {
        PushPreventUndoStateUpdate();
        CreateFunctionDefinitionNode(-1);
        PopPreventUndoStateUpdate();

        PostUndoPoint(GetActiveScriptCanvasId());
    }

    void MainWindow::CreateFunctionOutput()
    {
        PushPreventUndoStateUpdate();
        CreateFunctionDefinitionNode(1);
        PopPreventUndoStateUpdate();

        PostUndoPoint(GetActiveScriptCanvasId());
    }

    void MainWindow::CreateFunctionDefinitionNode(int positionOffset)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId = GetActiveScriptCanvasId();

        GraphCanvas::GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        QRectF viewBounds;
        GraphCanvas::ViewRequestBus::EventResult(viewBounds, viewId, &GraphCanvas::ViewRequests::GetCompleteArea);

        const bool isInput = positionOffset < 0;
        const AZStd::string rootName = isInput ? "New Input" : "New Output";
        NodeIdPair nodeIdPair = Nodes::CreateFunctionDefinitionNode(scriptCanvasId, isInput, rootName);

        GraphCanvas::SceneRequests* sceneRequests = GraphCanvas::SceneRequestBus::FindFirstHandler(graphCanvasGraphId);

        if (sceneRequests == nullptr)
        {
            return;
        }

        QPointF pasteOffset = sceneRequests->SignalGenericAddPositionUseBegin();
        sceneRequests->AddNode(nodeIdPair.m_graphCanvasId, GraphCanvas::ConversionUtils::QPointToVector(pasteOffset), false);
        sceneRequests->SignalGenericAddPositionUseEnd();

        if (!viewBounds.isEmpty())
        {
            QPointF topLeftPoint = viewBounds.center();

            int widthOffset = aznumeric_cast<int>((viewBounds.width() * 0.5f) * positionOffset);

            topLeftPoint.setX(topLeftPoint.x() + widthOffset);

            QGraphicsItem* graphicsItem = nullptr;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsItem, nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

            GraphCanvas::NodeUIRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeUIRequests::AdjustSize);

            qreal width = graphicsItem->sceneBoundingRect().width();

            // If we are going negative we need to move over the width of the node.
            if (positionOffset < 0)
            {
                topLeftPoint.setX(topLeftPoint.x() - width);
            }

            // Center the node.
            qreal height = graphicsItem->sceneBoundingRect().height();
            topLeftPoint.setY(topLeftPoint.y() - height * 0.5);

            // Offset by the width step.
            AZ::Vector2 minorStep = AZ::Vector2::CreateZero();

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);
            GraphCanvas::GridRequestBus::EventResult(minorStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            QRectF sceneBoundaries = sceneRequests->AsQGraphicsScene()->sceneRect();

            sceneBoundaries.adjust(minorStep.GetX(), minorStep.GetY(), -minorStep.GetX(), -minorStep.GetY());

            topLeftPoint.setX(topLeftPoint.x() + minorStep.GetX() * positionOffset);

            // Sanitizes the position of the node to ensure it's always 'visible'
            while (topLeftPoint.x() + width <= sceneBoundaries.left())
            {
                topLeftPoint.setX(topLeftPoint.x() + width);
            }

            while (topLeftPoint.x() >= sceneBoundaries.right())
            {
                topLeftPoint.setX(topLeftPoint.x() - width);
            }

            while (topLeftPoint.y() + height <= sceneBoundaries.top())
            {
                topLeftPoint.setY(topLeftPoint.y() + height);
            }

            while (topLeftPoint.y() >= sceneBoundaries.bottom())
            {
                topLeftPoint.setY(topLeftPoint.y() - height);
            }
            ////

            GraphCanvas::GeometryRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::GeometryRequests::SetPosition, GraphCanvas::ConversionUtils::QPointToVector(topLeftPoint));

            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnArea, graphicsItem->sceneBoundingRect());
        }
    }

    NodeIdPair MainWindow::ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos)
    {
        if (!m_isInAutomation)
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        }

        NodeIdPair retVal;

        if (azrtti_istypeof<CreateNodeMimeEvent>(mimeEvent))
        {
            CreateNodeMimeEvent* createEvent = static_cast<CreateNodeMimeEvent*>(mimeEvent);

            if (createEvent->ExecuteEvent(nodeCreationPos, nodeCreationPos, graphCanvasGraphId))
            {
                retVal = createEvent->GetCreatedPair();
            }
        }
        else if (azrtti_istypeof<SpecializedCreateNodeMimeEvent>(mimeEvent))
        {
            SpecializedCreateNodeMimeEvent* specializedCreationEvent = static_cast<SpecializedCreateNodeMimeEvent*>(mimeEvent);
            retVal = specializedCreationEvent->ConstructNode(graphCanvasGraphId, nodeCreationPos);
        }

        return retVal;
    }

    const GraphCanvas::GraphCanvasTreeItem* MainWindow::GetNodePaletteRoot() const
    {
        return m_nodePalette->GetTreeRoot();
    }

    void MainWindow::SignalAutomationBegin()
    {
        m_isInAutomation = true;
    }

    void MainWindow::SignalAutomationEnd()
    {
        m_isInAutomation = false;
    }

    void MainWindow::ForceCloseActiveAsset()
    {
        OnTabCloseRequest(m_tabBar->currentIndex());
    }

    bool MainWindow::RegisterObject(AZ::Crc32 elementId, QObject* object)
    {
        auto lookupIter = m_automationLookUpMap.find(elementId);

        if (lookupIter != m_automationLookUpMap.end())
        {
            AZ_Error("ScriptCanvas", false, "Attempting to register two elements with the id %llu", (unsigned int)elementId);
            return false;
        }

        m_automationLookUpMap[elementId] = object;
        return true;
    }

    bool MainWindow::UnregisterObject(AZ::Crc32 elementId)
    {
        auto eraseCount = m_automationLookUpMap.erase(elementId);
        return eraseCount > 0;
    }

    QObject* MainWindow::FindObject(AZ::Crc32 elementId)
    {
        auto lookupIter = m_automationLookUpMap.find(elementId);

        if (lookupIter != m_automationLookUpMap.end())
        {
            return lookupIter->second;
        }

        return nullptr;
    }

    QObject* MainWindow::FindElementByName(QString elementName)
    {
        return findChild<QObject*>(elementName);
    }

    AZ::EntityId MainWindow::FindEditorNodeIdByAssetNodeId([[maybe_unused]] const SourceHandle& assetId
        , [[maybe_unused]] AZ::EntityId assetNodeId) const
    {
        AZ::EntityId editorEntityId{};
//         AssetTrackerRequestBus::BroadcastResult
//             ( editorEntityId, &AssetTrackerRequests::GetEditorEntityIdFromSceneEntityId, assetId.Id(), assetNodeId);
        // #sc_editor_asset_redux fix logger
        return editorEntityId;
    }

    AZ::EntityId MainWindow::FindAssetNodeIdByEditorNodeId([[maybe_unused]] const SourceHandle& assetId
        , [[maybe_unused]] AZ::EntityId editorNodeId) const
    {
        AZ::EntityId sceneEntityId{};
        // AssetTrackerRequestBus::BroadcastResult
        // ( sceneEntityId, &AssetTrackerRequests::GetSceneEntityIdFromEditorEntityId, assetId.Id(), editorNodeId);
        // #sc_editor_asset_redux fix logger
        return sceneEntityId;
    }

    GraphCanvas::Endpoint MainWindow::CreateNodeForProposalWithGroup(const AZ::EntityId& connectionId
        , const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint, AZ::EntityId groupTarget)
    {
        PushPreventUndoStateUpdate();

        GraphCanvas::Endpoint retVal;

        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        // Handle the special-case if we are creating a node proposal for an AZ::Event, then we show
        // a small menu with only that applicable action
        if (CreateAzEventHandlerSlotMenuAction::FindBehaviorMethodWithAzEventReturn(graphCanvasGraphId, endpoint.GetSlotId()))
        {
            GraphCanvas::EditorContextMenu menu(ScriptCanvasEditor::AssetEditorId);

            menu.AddMenuAction(aznew CreateAzEventHandlerSlotMenuAction(&menu));

            HandleContextMenu(menu, endpoint.GetSlotId(), screenPoint, scenePoint);
        }
        // For everything else, show the full scene context menu
        else
        {
            m_sceneContextMenu->FilterForSourceSlot(graphCanvasGraphId, endpoint.GetSlotId());
            m_sceneContextMenu->RefreshActions(graphCanvasGraphId, connectionId);
            m_sceneContextMenu->SetupDisplayForProposal();

            QAction* action = m_sceneContextMenu->exec(screenPoint);

            // If the action returns null. We need to check if it was our widget, or just a close command.
            if (action == nullptr)
            {
                GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_sceneContextMenu->GetNodePalette()->GetContextMenuEvent();

                if (mimeEvent)
                {
                    NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, graphCanvasGraphId, AZ::Vector2(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y())));

                    if (finalNode.m_graphCanvasId.IsValid())
                    {
                        GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, false);
                        retVal = HandleProposedConnection(graphCanvasGraphId, connectionId, endpoint, finalNode.m_graphCanvasId, screenPoint);
                    }

                    if (retVal.IsValid())
                    {
                        AZStd::unordered_set<GraphCanvas::ConnectionId> createdConnections = GraphCanvas::GraphUtils::CreateOpportunisticConnectionsBetween(endpoint, retVal);
                        GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, true);

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, retVal.GetNodeId(), &GraphCanvas::GeometryRequests::GetPosition);

                        QPointF connectionPoint;
                        GraphCanvas::SlotUIRequestBus::EventResult(connectionPoint, retVal.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                        qreal verticalOffset = connectionPoint.y() - position.GetY();
                        position.SetY(aznumeric_cast<float>(scenePoint.y() - verticalOffset));

                        qreal horizontalOffset = connectionPoint.x() - position.GetX();
                        position.SetX(aznumeric_cast<float>(scenePoint.x() - horizontalOffset));

                        GraphCanvas::GeometryRequestBus::Event(retVal.GetNodeId(), &GraphCanvas::GeometryRequests::SetPosition, position);

                        GraphCanvas::GraphUtils::AddElementToGroup(finalNode.m_graphCanvasId, groupTarget);

                        GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                    else
                    {
                        GraphCanvas::GraphUtils::DeleteOutermostNode(graphCanvasGraphId, finalNode.m_graphCanvasId);
                    }
                }
            }
        }

        PopPreventUndoStateUpdate();

        return retVal;
    }

    void MainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(wrapperNode) != nullptr)
        {
            m_ebusHandlerActionMenu->SetEbusHandlerNode(wrapperNode);

            // We don't care about the result, since the actions are done on demand with the menu
            m_ebusHandlerActionMenu->exec(screenPoint);
        }
        else if (ScriptCanvasWrapperNodeDescriptorRequestBus::FindFirstHandler(wrapperNode) != nullptr)
        {
            ScriptCanvasWrapperNodeDescriptorRequestBus::Event(wrapperNode, &ScriptCanvasWrapperNodeDescriptorRequests::OnWrapperAction, actionWidgetBoundingRect, scenePoint, screenPoint);
        }
    }

    void MainWindow::OnSelectionManipulationBegin()
    {
        m_ignoreSelection = true;
    }

    void MainWindow::OnSelectionManipulationEnd()
    {
        m_ignoreSelection = false;
        OnSelectionChanged();
    }

    AZ::EntityId MainWindow::CreateNewGraph()
    {
        AZ::EntityId graphId;

        OnFileNew();

        if (m_activeGraph.IsGraphValid())
        {
            graphId = GetActiveGraphCanvasGraphId();
        }

        return graphId;
    }

    bool MainWindow::ContainsGraph(const GraphCanvas::GraphId&) const
    {
        return false;
    }

    bool MainWindow::CloseGraph(const GraphCanvas::GraphId&)
    {
        return false;
    }

    void MainWindow::CustomizeConnectionEntity(AZ::Entity* connectionEntity)
    {
        connectionEntity->CreateComponent<SceneMemberMappingComponent>();
    }

    void MainWindow::ShowAssetPresetsMenu(GraphCanvas::ConstructType constructType)
    {
        OnViewPresetsEditor();

        if (m_presetEditor)
        {
            m_presetEditor->SetActiveConstructType(constructType);
        }
    }

    //! Hook for receiving context menu events for each QGraphicsScene
    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowSceneContextMenuWithGroup(const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget)
    {
        bool tryDaisyChain = (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

        GraphCanvas::GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();
        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId = GetActiveScriptCanvasId();

        if (!graphCanvasGraphId.IsValid() || !scriptCanvasGraphId.IsValid())
        {
            // Nothing to do.
            return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        m_sceneContextMenu->ResetSourceSlotFilter();
        m_sceneContextMenu->RefreshActions(graphCanvasGraphId, AZ::EntityId());
        QAction* action = m_sceneContextMenu->exec(screenPoint);

        GraphCanvas::ContextMenuAction::SceneReaction reaction = GraphCanvas::ContextMenuAction::SceneReaction::Nothing;

        if (action == nullptr)
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_sceneContextMenu->GetNodePalette()->GetContextMenuEvent();

            NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, graphCanvasGraphId, AZ::Vector2(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y())));

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

            if (finalNode.m_graphCanvasId.IsValid())
            {
                GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, true);

                AZ::Vector2 position;
                GraphCanvas::GeometryRequestBus::EventResult(position, finalNode.m_graphCanvasId, &GraphCanvas::GeometryRequests::GetPosition);
                GraphCanvas::GeometryRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::GeometryRequests::SetPosition, position);

                // If we have a valid group target. We're going to want to add the element to the group.
                GraphCanvas::GraphUtils::AddElementToGroup(finalNode.m_graphCanvasId, groupTarget);

                GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);

                if (tryDaisyChain)
                {
                    QTimer::singleShot(50, [graphCanvasGraphId, finalNode, screenPoint, scenePoint, groupTarget]()
                    {
                        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::HandleProposalDaisyChainWithGroup, finalNode.m_graphCanvasId, GraphCanvas::SlotTypes::ExecutionSlot, GraphCanvas::CT_Output, screenPoint, scenePoint, groupTarget);
                    });
                }
            }
        }
        else
        {
            GraphCanvas::ContextMenuAction* contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(action);

            if (contextMenuAction)
            {
                PushPreventUndoStateUpdate();
                AZ::Vector2 mousePoint(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
                reaction = contextMenuAction->TriggerAction(graphCanvasGraphId, mousePoint);
                PopPreventUndoStateUpdate();
            }
        }

        return reaction;
    }

    //! Hook for receiving context menu events for each QGraphicsScene
    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);
        NodeDescriptorType descriptorType = NodeDescriptorType::Unknown;
        NodeDescriptorRequestBus::EventResult(descriptorType, nodeId, &NodeDescriptorRequests::GetType);

        if (descriptorType == NodeDescriptorType::GetVariable || descriptorType == NodeDescriptorType::SetVariable)
        {
            contextMenu.AddMenuAction(aznew ConvertVariableNodeToReferenceAction(&contextMenu));
        }

        if (descriptorType == NodeDescriptorType::FunctionDefinitionNode)
        {
            NodeDescriptorComponent* descriptor = nullptr;
            NodeDescriptorRequestBus::EventResult(descriptor, nodeId, &NodeDescriptorRequests::GetDescriptorComponent);
            contextMenu.AddMenuAction(aznew RenameFunctionDefinitionNodeAction(descriptor, &contextMenu));
            contextMenu.addSeparator();
        }

        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowCommentContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CommentContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeGroupContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);

        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CollapsedNodeGroupContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::BookmarkContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowConnectionContextMenuWithGroup(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget)
    {
        PushPreventUndoStateUpdate();

        GraphCanvas::ContextMenuAction::SceneReaction reaction = GraphCanvas::ContextMenuAction::SceneReaction::Nothing;

        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
        GraphCanvas::GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        m_connectionContextMenu->RefreshActions(graphCanvasGraphId, connectionId);

        QAction* result = m_connectionContextMenu->exec(screenPoint);

        GraphCanvas::ContextMenuAction* contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(result);

        // If the action returns null. We need to check if it was our widget, or just a close command.
        if (contextMenuAction)
        {
            reaction = contextMenuAction->TriggerAction(graphCanvasGraphId, sceneVector);
        }
        else
        {
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_connectionContextMenu->GetNodePalette()->GetContextMenuEvent();

            if (mimeEvent)
            {
                NodeIdPair finalNode = ProcessCreateNodeMimeEvent(mimeEvent, graphCanvasGraphId, AZ::Vector2(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y())));

                GraphCanvas::Endpoint sourceEndpoint;
                GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                GraphCanvas::Endpoint targetEndpoint;
                GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                if (finalNode.m_graphCanvasId.IsValid())
                {
                    GraphCanvas::ConnectionSpliceConfig spliceConfig;
                    spliceConfig.m_allowOpportunisticConnections = true;

                    if (!GraphCanvas::GraphUtils::SpliceNodeOntoConnection(finalNode.m_graphCanvasId, connectionId, spliceConfig))
                    {
                        GraphCanvas::GraphUtils::DeleteOutermostNode(graphCanvasGraphId, finalNode.m_graphCanvasId);
                    }
                    else
                    {
                        reaction = GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;

                        // Now we can deal with the alignment of the node.
                        GraphCanvas::VisualRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::VisualRequests::SetVisible, true);

                        AZ::Vector2 position(0,0);
                        GraphCanvas::GeometryRequestBus::EventResult(position, finalNode.m_graphCanvasId, &GraphCanvas::GeometryRequests::GetPosition);

                        QPointF sourceConnectionPoint(0,0);
                        GraphCanvas::SlotUIRequestBus::EventResult(sourceConnectionPoint, spliceConfig.m_splicedSourceEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                        QPointF targetConnectionPoint(0,0);
                        GraphCanvas::SlotUIRequestBus::EventResult(targetConnectionPoint, spliceConfig.m_splicedTargetEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                        // Average our two points so we splice roughly in the center of our node.
                        QPointF connectionPoint = (sourceConnectionPoint + targetConnectionPoint) * 0.5f;

                        qreal verticalOffset = connectionPoint.y() - position.GetY();
                        position.SetY(aznumeric_cast<float>(scenePoint.y() - verticalOffset));

                        qreal horizontalOffset = connectionPoint.x() - position.GetX();
                        position.SetX(aznumeric_cast<float>(scenePoint.x() - horizontalOffset));

                        GraphCanvas::GeometryRequestBus::Event(finalNode.m_graphCanvasId, &GraphCanvas::GeometryRequests::SetPosition, position);

                        if (IsNodeNudgingEnabled())
                        {
                            GraphCanvas::NodeNudgingController nudgingController(graphCanvasGraphId, { finalNode.m_graphCanvasId });
                            nudgingController.FinalizeNudging();
                        }

                        GraphCanvas::GraphUtils::AddElementToGroup(finalNode.m_graphCanvasId, groupTarget);

                        GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }
            }
        }

        PopPreventUndoStateUpdate();

        return reaction;
    }

    GraphCanvas::ContextMenuAction::SceneReaction MainWindow::ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::SlotContextMenu contextMenu(ScriptCanvasEditor::AssetEditorId);

        contextMenu.AddMenuAction(aznew ConvertReferenceToVariableNodeAction(&contextMenu));
        contextMenu.AddMenuAction(aznew ExposeSlotMenuAction(&contextMenu));
        contextMenu.AddMenuAction(aznew CreateAzEventHandlerSlotMenuAction(&contextMenu));

        auto setSlotTypeAction = aznew SetDataSlotTypeMenuAction(&contextMenu);
        contextMenu.AddMenuAction(setSlotTypeAction);

        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    void MainWindow::OnSystemTick()
    {
        if (HasSystemTickAction(SystemTickActionFlag::RefreshPropertyGrid))
        {
            RemoveSystemTickAction(SystemTickActionFlag::RefreshPropertyGrid);
            RefreshSelection();
        }

        if (HasSystemTickAction(SystemTickActionFlag::CloseWindow))
        {
            RemoveSystemTickAction(SystemTickActionFlag::CloseWindow);
            qobject_cast<QWidget*>(parent())->close();
        }

        if (HasSystemTickAction(SystemTickActionFlag::CloseCurrentGraph))
        {
            RemoveSystemTickAction(SystemTickActionFlag::CloseCurrentGraph);

            if (m_tabBar)
            {
                m_tabBar->tabCloseRequested(m_tabBar->currentIndex());
            }
        }

        if (HasSystemTickAction(SystemTickActionFlag::CloseNextTabAction))
        {
            RemoveSystemTickAction(SystemTickActionFlag::CloseNextTabAction);
            CloseNextTab();
        }

        ClearStaleSaves();
    }

    void MainWindow::OnCommandStarted(AZ::Crc32)
    {
        PushPreventUndoStateUpdate();
    }

    void MainWindow::OnCommandFinished(AZ::Crc32)
    {
        PopPreventUndoStateUpdate();
    }

    void MainWindow::PrepareActiveAssetForSave()
    {
        PrepareAssetForSave(m_activeGraph);
    }

    void MainWindow::PrepareAssetForSave(const SourceHandle& /*assetId*/)
    {
    }

    void MainWindow::RestartAutoTimerSave(bool forceTimer)
    {
        if (m_autoSaveTimer.isActive() || forceTimer)
        {
            m_autoSaveTimer.stop();
            m_autoSaveTimer.start();
        }
    }

    void MainWindow::OnSelectedEntitiesAboutToShow()
    {
        AzToolsFramework::EntityIdList selectedEntityIds;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        m_selectedEntityMenu->clear();

        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            bool isLayerEntity = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(isLayerEntity, entityId, &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            if (isLayerEntity)
            {
                continue;
            }

            AZ::NamedEntityId namedEntityId(entityId);

            QAction* actionElement = new QAction(namedEntityId.GetName().data(), m_selectedEntityMenu);

            QObject::connect(actionElement, &QAction::triggered, [this, entityId]() {
                OnAssignToEntity(entityId);
            });

            m_selectedEntityMenu->addAction(actionElement);
        }
    }

    void MainWindow::OnAssignToSelectedEntities()
    {
        Tracker::ScriptCanvasFileState fileState = GetAssetFileState(m_activeGraph);;

        bool isDocumentOpen = false;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(isDocumentOpen, &AzToolsFramework::EditorRequests::IsLevelDocumentOpen);

        if (fileState == Tracker::ScriptCanvasFileState::NEW || fileState == Tracker::ScriptCanvasFileState::SOURCE_REMOVED || !isDocumentOpen)
        {
            return;
        }

        AzToolsFramework::EntityIdList selectedEntityIds;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        auto selectedEntityIdIter = selectedEntityIds.begin();

        bool isLayerAmbiguous = false;
        AZ::EntityId targetLayer;

        while (selectedEntityIdIter != selectedEntityIds.end())
        {
            bool isLayerEntity = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(isLayerEntity, (*selectedEntityIdIter), &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            if (isLayerEntity)
            {
                if (targetLayer.IsValid())
                {
                    isLayerAmbiguous = true;
                }

                targetLayer = (*selectedEntityIdIter);

                selectedEntityIdIter = selectedEntityIds.erase(selectedEntityIdIter);
            }
            else
            {
                ++selectedEntityIdIter;
            }
        }

        if (selectedEntityIds.empty())
        {
            AZ::EntityId createdId;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(createdId, &AzToolsFramework::EditorRequests::CreateNewEntity, AZ::EntityId());

            selectedEntityIds.emplace_back(createdId);

            if (targetLayer.IsValid() && !isLayerAmbiguous)
            {
                AZ::TransformBus::Event(createdId, &AZ::TransformBus::Events::SetParent, targetLayer);
            }
        }

        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            AssignGraphToEntityImpl(entityId);
        }
    }

    void MainWindow::OnAssignToEntity(const AZ::EntityId& entityId)
    {
        Tracker::ScriptCanvasFileState fileState = GetAssetFileState(m_activeGraph);

        if (fileState == Tracker::ScriptCanvasFileState::MODIFIED
            || fileState == Tracker::ScriptCanvasFileState::UNMODIFIED)
        {
            AssignGraphToEntityImpl(entityId);
        }
    }

    ScriptCanvasEditor::Tracker::ScriptCanvasFileState MainWindow::GetAssetFileState(SourceHandle assetId) const
    {
        auto dataOptional = m_tabBar->GetTabData(assetId);
        return dataOptional ? dataOptional->m_fileState : Tracker::ScriptCanvasFileState::INVALID;
    }

    void MainWindow::AssignGraphToEntityImpl(const AZ::EntityId& entityId)
    {
        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(isLayerEntity, entityId, &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        if (isLayerEntity)
        {
            return;
        }

        EditorScriptCanvasComponentRequests* firstRequestBus = nullptr;
        EditorScriptCanvasComponentRequests* firstEmptyRequestBus = nullptr;

        EditorScriptCanvasComponentRequestBus::EnumerateHandlersId(entityId, [&firstRequestBus, &firstEmptyRequestBus](EditorScriptCanvasComponentRequests* scriptCanvasRequests)
        {
            if (firstRequestBus == nullptr)
            {
                firstRequestBus = scriptCanvasRequests;
            }

            if (!scriptCanvasRequests->HasAssetId())
            {
                firstEmptyRequestBus = scriptCanvasRequests;
            }

            return firstRequestBus == nullptr || firstEmptyRequestBus == nullptr;
        });

        auto usableRequestBus = firstEmptyRequestBus;

        if (usableRequestBus == nullptr)
        {
            usableRequestBus = firstRequestBus;
        }

        if (usableRequestBus == nullptr)
        {
            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ entityId }
                                                                                                                        , AZ::ComponentTypeList{ azrtti_typeid<EditorScriptCanvasComponent>() });

            usableRequestBus = EditorScriptCanvasComponentRequestBus::FindFirstHandler(entityId);
        }

        if (usableRequestBus)
        {
            usableRequestBus->SetAssetId(m_activeGraph.Describe());
        }
    }

    bool MainWindow::HasSystemTickAction(SystemTickActionFlag action)
    {
        return (m_systemTickActions & action) != 0;
    }

    void MainWindow::RemoveSystemTickAction(SystemTickActionFlag action)
    {
        m_systemTickActions = m_systemTickActions & (~action);
    }

    void MainWindow::AddSystemTickAction(SystemTickActionFlag action)
    {
        m_systemTickActions |= action;
    }

    void MainWindow::BlockCloseRequests()
    {
        m_queueCloseRequest = true;
    }

    void MainWindow::UnblockCloseRequests()
    {
        if (m_queueCloseRequest)
        {
            m_queueCloseRequest = false;

            if (m_hasQueuedClose)
            {
                qobject_cast<QWidget*>(parent())->close();
            }
        }
    }

    void MainWindow::OpenNextFile()
    {
        if (!m_filesToOpen.empty())
        {
            QString nextFile = m_filesToOpen.front();
            m_filesToOpen.pop_front();

            OpenFile(nextFile.toUtf8().data());
            OpenNextFile();
        }
        else
        {
            m_errorFilePath.clear();
        }
    }

    double MainWindow::GetSnapDistance() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_snapDistance;
        }

        return 10.0;
    }

    bool MainWindow::IsGroupDoubleClickCollapseEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_enableGroupDoubleClickCollapse;
        }

        return true;
    }

    bool MainWindow::IsBookmarkViewportControlEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_allowBookmarkViewpointControl;
        }

        return false;
    }

    bool MainWindow::IsDragNodeCouplingEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_dragNodeCouplingConfig.m_enabled;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDragCouplingTime() const
    {
        if (m_userSettings)
        {
            return AZStd::chrono::milliseconds(m_userSettings->m_dragNodeCouplingConfig.m_timeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    bool MainWindow::IsDragConnectionSpliceEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_dragNodeSplicingConfig.m_enabled;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDragConnectionSpliceTime() const
    {
        if (m_userSettings)
        {
            return AZStd::chrono::milliseconds(m_userSettings->m_dragNodeSplicingConfig.m_timeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    bool MainWindow::IsDropConnectionSpliceEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_dropNodeSplicingConfig.m_enabled;
        }

        return false;
    }

    AZStd::chrono::milliseconds MainWindow::GetDropConnectionSpliceTime() const
    {
        if (m_userSettings)
        {
            return AZStd::chrono::milliseconds(m_userSettings->m_dropNodeSplicingConfig.m_timeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    bool MainWindow::IsNodeNudgingEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_allowNodeNudging;
        }

        return false;
    }

    bool MainWindow::IsShakeToDespliceEnabled() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_shakeDespliceConfig.m_enabled;
        }

        return false;
    }

    int MainWindow::GetShakesToDesplice() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_shakeDespliceConfig.m_shakeCount;
        }

        return 3;
    }

    float MainWindow::GetMinimumShakePercent() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_shakeDespliceConfig.GetMinimumShakeLengthPercent();
        }

        return 0.03f;
    }

    float MainWindow::GetShakeDeadZonePercent() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_shakeDespliceConfig.GetDeadZonePercent();
        }

        return 0.01f;
    }

    float MainWindow::GetShakeStraightnessPercent() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_shakeDespliceConfig.GetStraightnessPercent();
        }

        return 0.75f;
    }

    AZStd::chrono::milliseconds MainWindow::GetMaximumShakeDuration() const
    {
        if (m_userSettings)
        {
            return AZStd::chrono::milliseconds(m_userSettings->m_shakeDespliceConfig.m_maximumShakeTimeMS);
        }

        return AZStd::chrono::milliseconds(500);
    }

    AZStd::chrono::milliseconds MainWindow::GetAlignmentTime() const
    {
        if (m_userSettings)
        {
            return AZStd::chrono::milliseconds(m_userSettings->m_alignmentTimeMS);
        }

        return AZStd::chrono::milliseconds(250);
    }

    float MainWindow::GetMaxZoom() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_zoomSettings.GetMaxZoom();
        }

        return 2.0f;
    }

    float MainWindow::GetEdgePanningPercentage() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_edgePanningSettings.GetEdgeScrollPercent();
        }

        return 0.1f;
    }

    float MainWindow::GetEdgePanningScrollSpeed() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_edgePanningSettings.GetEdgeScrollSpeed();
        }

        return 100.0f;
    }

    GraphCanvas::EditorConstructPresets* MainWindow::GetConstructPresets() const
    {
        if (m_userSettings)
        {
            return &m_userSettings->m_constructPresets;
        }

        return nullptr;
    }

    const GraphCanvas::ConstructTypePresetBucket* MainWindow::GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const
    {
        GraphCanvas::EditorConstructPresets* presets = GetConstructPresets();

        if (presets)
        {
            return presets->FindPresetBucket(constructType);
        }

        return nullptr;
    }

    GraphCanvas::Styling::ConnectionCurveType MainWindow::GetConnectionCurveType() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_stylingSettings.GetConnectionCurveType();
        }

        return GraphCanvas::Styling::ConnectionCurveType::Straight;
    }

    GraphCanvas::Styling::ConnectionCurveType MainWindow::GetDataConnectionCurveType() const
    {
        if (m_userSettings)
        {
            return m_userSettings->m_stylingSettings.GetDataConnectionCurveType();
        }

        return GraphCanvas::Styling::ConnectionCurveType::Straight;
    }

    bool MainWindow::AllowNodeDisabling() const
    {
        return true;
    }

    bool MainWindow::AllowDataReferenceSlots() const
    {
        return true;
    }

    void MainWindow::CreateUnitTestWidget()
    {
        // Dock Widget will be unable to dock with this as it doesn't have a parent.
        // Going to orphan this as a floating window to more mimic its behavior as a pop-up window rather then a dock widget.
        m_unitTestDockWidget = aznew UnitTestDockWidget(this);
        m_unitTestDockWidget->setObjectName("TestManager");
        m_unitTestDockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
        m_unitTestDockWidget->setFloating(true);
        m_unitTestDockWidget->hide();

        // Restore this if we want the dock widget to again be a toggleable thing.
        //connect(m_unitTestDockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::OnViewVisibilityChanged);
    }

    void MainWindow::DisableAssetView(const SourceHandle& memoryAssetId)
    {
        if (auto view = m_tabBar->ModTabView(m_tabBar->FindTab(memoryAssetId)))
        {
            view->DisableView();
        }

        m_tabBar->setEnabled(false);
        m_bookmarkDockWidget->setEnabled(false);
        m_variableDockWidget->setEnabled(false);
        m_propertyGrid->DisableGrid();
        m_editorToolbar->OnViewDisabled();

        m_createFunctionInput->setEnabled(false);
        m_createFunctionOutput->setEnabled(false);
        m_createScriptCanvas->setEnabled(false);

        UpdateMenuState(false);

        ui->action_New_Script->setEnabled(false);

        m_autoSaveTimer.stop();
    }

    void MainWindow::EnableAssetView(const SourceHandle& memoryAssetId)
    {
        if (auto view = m_tabBar->ModTabView(m_tabBar->FindTab(memoryAssetId)))
        {
            view->EnableView();
        }

        m_tabBar->setEnabled(true);
        m_bookmarkDockWidget->setEnabled(true);
        m_variableDockWidget->setEnabled(true);
        m_propertyGrid->EnableGrid();
        m_editorToolbar->OnViewEnabled();

        m_createScriptCanvas->setEnabled(true);

        ui->action_New_Script->setEnabled(true);

        UpdateMenuState(true);

        UpdateUndoRedoState();
    }


    void MainWindow::ClearStaleSaves()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        auto timeNow = AZStd::chrono::steady_clock::now();
        AZStd::erase_if(m_saves, [&timeNow](const auto& item)
        {
            AZStd::sys_time_t delta = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(timeNow - item.second).count();
            return delta > 2.0f;
        });
    }

    bool MainWindow::IsRecentSave(const SourceHandle& handle) const
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(const_cast<MainWindow*>(this)->m_mutex);
        AZStd::string key = handle.AbsolutePath().Native();
        AZStd::to_lower(key.begin(), key.end());
        auto iter = m_saves.find(key);
        return iter != m_saves.end();
    }

    void MainWindow::MarkRecentSave(const SourceHandle& handle)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        AZStd::string key = handle.AbsolutePath().Native();
        AZStd::to_lower(key.begin(), key.end());
        m_saves[key] = AZStd::chrono::steady_clock::now();
    }

    void MainWindow::OnScriptEventAddHelpers()
    {
        if (ScriptEvents::Editor::MakeHelpersAction(m_activeGraph).first)
        {
            GraphCanvas::GraphModelRequestBus::Event
                ( m_activeGraph.Mod()->GetEntityId()
                , &GraphCanvas::GraphModelRequests::RequestUndoPoint);
        }
    }

    void MainWindow::OnScriptEventClearStatus()
    {
        ScriptEvents::Editor::ClearStatusAction(m_activeGraph);
    }

    void MainWindow::OnScriptEventMenuPreShow()
    {
        auto result = ScriptEvents::Editor::UpdateMenuItemsEnabled(m_activeGraph);
        ui->actionAdd_Script_Event_Helpers->setEnabled(result.m_addHelpers);
        ui->actionClear_Script_Event_Status->setEnabled(result.m_clear);
        ui->actionParse_As_Script_Event->setEnabled(result.m_parse);
        ui->actionSave_As_ScriptEvent->setEnabled(result.m_save);
    }

    void MainWindow::OnScriptEventOpen()
    {
        AZStd::pair<ScriptCanvas::SourceHandle, AZStd::string> result = ScriptEvents::Editor::OpenAction();

        if (result.first.Get())
        {
            OpenScriptCanvasAssetImplementation(result.first, Tracker::ScriptCanvasFileState::UNMODIFIED);
        }
        else
        {
            if (!result.second.empty())
            {
                QMessageBox mb
                    ( QMessageBox::Warning
                    , tr("Failed to open ScriptEvent file into ScriptCanvas Editor.")
                    , result.second.c_str()
                    , QMessageBox::Close
                    , nullptr);

                mb.exec();
            }
        }
    }

    void MainWindow::OnScriptEventParseAs()
    {
        if (!m_activeGraph.IsGraphValid())
        {
            return;
        }

        AZStd::pair<bool, AZStd::vector<AZStd::string>> result = ScriptEvents::Editor::ParseAsAction(m_activeGraph);

        if (result.first)
        {
            QMessageBox mb
                ( QMessageBox::Information
                , QObject::tr("Success!")
                , QObject::tr("Graph parsed as ScriptEvent, and may be saved as one.")
                , QMessageBox::Close
                , nullptr);

            mb.exec();
        }
        else
        {
            AZStd::string parseErrorString;

            if (!result.second.empty())
            {
                parseErrorString = "Parse Errors:\n";

                for (auto& entry : result.second)
                {
                    parseErrorString += "* ";
                    parseErrorString += entry;
                    parseErrorString += "\n";
                }
            }

            QMessageBox mb
                ( QMessageBox::Warning
                , QObject::tr("Graph did not parse as ScriptEvent, please fix issues below to save as a ScriptEvent")
                , parseErrorString.c_str()
                , QMessageBox::Close
                , nullptr);

            mb.exec();
        }
    }

    void MainWindow::OnScriptEventSaveAs()
    {
        auto result = ScriptEvents::Editor::SaveAsAction(m_activeGraph);
        if (result.first)
        {
            OnSaveToast toast
                ( result.second
                , GetActiveGraphCanvasGraphId()
                , true
                , AZStd::string("Graph Saved .scriptevent, and this editor can open that file.\n"
                    "No .scriptcanvas file was saved from this graph."));
        }
        else
        {
            QMessageBox mb
                ( QMessageBox::Warning
                , QObject::tr("Failed to Save As Script Event")
                , result.second.c_str()
                , QMessageBox::Close
                , nullptr);

            mb.exec();
        }
    }

#include <Editor/View/Windows/moc_MainWindow.cpp>
}
