/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include "LUAEditorMainWindow.hxx"

#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Script/ScriptRemoteDebuggingConstants.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/UICore/SaveChangesDialog.hxx>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>
#include <AzToolsFramework/UI/LegacyFramework/MainWindowSavedState.h>
#include <AzToolsFramework/UI/UICore/TargetSelectorButton.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/UI/LegacyFramework/CustomMenus/CustomMenusAPI.h>
#include <Source/LUA/TargetContextButton.hxx>
#include <Source/LUA/LUAEditorDebuggerMessages.h>

#include "DebugAttachmentButton.hxx"
#include "ClassReferenceFilter.hxx"
#include "WatchesPanel.hxx"
#include "LUAEditorGoToLineDialog.hxx"
#include "LUAEditorView.hxx"
#include "LUAEditorContextMessages.h"
#include "LUABreakpointTrackerMessages.h"
#include "LUAEditorSettingsDialog.hxx"
#include "RecentFiles.h"

#include <Source/AssetDatabaseLocationListener.h>
#include <Source/LUA/ui_LUAEditorMainWindow.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QTimer>
#include <QDesktopServices>
#include <QLabel>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

void initSharedResources()
{
    Q_INIT_RESOURCE(sharedResources);
}

namespace
{
    [[maybe_unused]] const char* LUAEditorDebugName = "LUA Debug";
    [[maybe_unused]] const char* LUAEditorInfoName = "LUA Editor";
}


namespace LUAEditor
{
    extern AZ::Uuid ContextID;

    //////////////////////////////////////////////////////////////////////////
    //LUAEditorMainWindow
    LUAEditorMainWindow::LUAEditorMainWindow(QStandardItemModel* dataModel, bool connectedState, QWidget* parent, Qt::WindowFlags flags)
        : QMainWindow(parent, flags)
        , m_lastFocusedAssetId()
        , m_ptrFindDialog(nullptr)
        , m_settingsDialog(nullptr)
        , m_actionClearRecentFiles(nullptr)
    {
        initSharedResources();
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath engineRootPath;
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AzQtComponents::StyleManager* m_styleSheet = new AzQtComponents::StyleManager(this);
        m_styleSheet->initialize(qApp, engineRootPath);

        LUAViewMessages::Bus::Handler::BusConnect();

        //m_currentTabContextMenuUUID = AZ::Uuid::CreateNull();
        m_gui = azcreate(Ui::LUAEditorMainWindow, ());
        m_gui->setupUi(this);
        setAcceptDrops(true);
        m_bIgnoreFocusRequests = false;
        m_bAutocompleteEnabled = true; // the default

        QMenu* theMenu = new QMenu(this);
        (void)theMenu->addAction(
            "Close Lua Editor App",
            this,
            SLOT(OnMenuCloseCurrentWindow()),
            QKeySequence("Alt+F4")
            );

        AzToolsFramework::FrameworkMessages::Bus::Broadcast(
            &AzToolsFramework::FrameworkMessages::Bus::Events::PopulateApplicationMenu, theMenu);
        menuBar()->insertMenu(m_gui->menuFile->menuAction(), theMenu);

        m_ptrFindDialog = aznew LUAEditorFindDialog(this);
        m_settingsDialog = aznew LUAEditorSettingsDialog(this);

        actionTabForwards = new QAction(tr("Next Document Tab"), this);
        actionTabBackwards = new QAction(tr("Previous Document Tab"), this);

        actionTabForwards->setShortcut(QKeySequence("Ctrl+Tab"));
        connect(actionTabForwards, SIGNAL(triggered(bool)), this, SLOT(OnTabForwards()));
        actionTabBackwards->setShortcut(QKeySequence("Ctrl+Shift+Tab"));
        connect(actionTabBackwards, SIGNAL(triggered(bool)), this, SLOT(OnTabBackwards()));
        m_gui->menuView->addAction(actionTabForwards);
        m_gui->menuView->addAction(actionTabBackwards);

        connect(m_gui->m_findResults1, &FindResults::ResultSelected, this, &LUAEditorMainWindow::OnFindResultClicked);
        connect(m_gui->m_findResults2, &FindResults::ResultSelected, this, &LUAEditorMainWindow::OnFindResultClicked);
        connect(m_gui->m_findResults3, &FindResults::ResultSelected, this, &LUAEditorMainWindow::OnFindResultClicked);
        connect(m_gui->m_findResults4, &FindResults::ResultSelected, this, &LUAEditorMainWindow::OnFindResultClicked);
        m_gui->findTabWidget->setCurrentIndex(0);

        connect(m_gui->actionCut, SIGNAL(triggered()), this, SLOT(OnEditMenuCut()));
        connect(m_gui->actionCopy, SIGNAL(triggered()), this, SLOT(OnEditMenuCopy()));
        connect(m_gui->actionSettings, SIGNAL(triggered()), this, SLOT(OnSettings()));

        connect(m_gui->actionLuaDocumentation, SIGNAL(triggered()), this, SLOT(OnLuaDocumentation()));

        m_gui->localsTreeView->SetOperatingMode(WATCHES_MODE_LOCALS);

        m_gui->m_logPanel->SetStorageID(AZ_CRC_CE("LUA Editor Log Panel"));
        connect(m_gui->m_logPanel, &AzToolsFramework::LogPanel::BaseLogPanel::TabsReset, this, &LUAEditorMainWindow::OnLogTabsReset);

        //m_lastProgramCounterAssetId = "";

        QMainWindow* pCentralWidget = new QMainWindow();
        setCentralWidget(pCentralWidget);
        pCentralWidget->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
        pCentralWidget->setDockOptions(AllowNestedDocks | AllowTabbedDocks | AnimatedDocks);

        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

        m_pTargetButton = aznew AzToolsFramework::TargetSelectorButtonAction(AzFramework::LuaToolsKey, this);
        m_gui->debugToolbar->addAction(m_pTargetButton);
        m_gui->menuDebug->addAction(m_pTargetButton);

        m_pContextButton = aznew LUA::TargetContextButtonAction(this);
        m_gui->debugToolbar->addAction(m_pContextButton);
        m_gui->menuDebug->addAction(m_pContextButton);

        m_pDebugAttachmentButton = aznew LUAEditor::DebugAttachmentButtonAction(this);
        m_gui->debugToolbar->addAction(m_pDebugAttachmentButton);
        m_gui->menuDebug->addAction(m_pDebugAttachmentButton);
        m_pDebugAttachmentButton->setEnabled(false);

        //turn these off by default
        m_settingsDialog->hide();
        m_ptrFindDialog->hide();
        m_gui->watchDockWidget->hide();
        m_gui->stackDockWidget->hide();
        m_gui->localsDockWidget->hide();
        m_gui->breakpointsDockWidget->hide();
        m_gui->findResultsDockWidget->hide();

        QTimer::singleShot(0, this, SLOT(RestoreWindowState()));

        m_gui->watchDockWidget->toggleViewAction()->setIcon(QIcon(":/general/watch_window"));
        m_gui->stackDockWidget->toggleViewAction()->setIcon(QIcon(":/general/callstack"));
        m_gui->localsDockWidget->toggleViewAction()->setIcon(QIcon(":/general/lua_locals"));
        m_gui->breakpointsDockWidget->toggleViewAction()->setIcon(QIcon(":/general/breakpoints"));
        m_gui->findResultsDockWidget->toggleViewAction()->setIcon(QIcon(":/general/find_results"));

        //construct the viewToolBar and menuView from toggle view actions
        m_gui->viewToolBar->addAction(m_gui->watchDockWidget->toggleViewAction());
        m_gui->viewToolBar->addAction(m_gui->breakpointsDockWidget->toggleViewAction());
        m_gui->viewToolBar->addAction(m_gui->stackDockWidget->toggleViewAction());
        m_gui->viewToolBar->addAction(m_gui->localsDockWidget->toggleViewAction());
        m_gui->viewToolBar->addAction(m_gui->findResultsDockWidget->toggleViewAction());

        m_gui->menuView->addAction(m_gui->watchDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->breakpointsDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->stackDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->localsDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->findResultsDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->classReferenceDockWidget->toggleViewAction());
        m_gui->menuView->addAction(m_gui->m_dockLog->toggleViewAction());
        m_gui->menuView->addAction(m_gui->luaFilesDockWidget->toggleViewAction());

        LUAEditorMainWindowMessages::Handler::BusConnect();

        LUABreakpointTrackerMessages::Handler::BusConnect();

        Context_DebuggerManagement::Bus::Broadcast(&Context_DebuggerManagement::Bus::Events::CleanUpBreakpoints);

        SetDebugControlsToInitial();
        SetEditContolsToNoFilesOpen();
        // whats the current target anyway?

        // dataModel is the sole point of contact between our Context and its LUA debugger class information
        m_ClassReferenceFilter = aznew ClassReferenceFilterModel(this);
        m_ClassReferenceFilter->setSourceModel(dataModel);
        m_gui->classReferenceTreeView->setModel(m_ClassReferenceFilter);

        connect(m_gui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &LUAEditorMainWindow::luaClassFilterTextChanged);

        connect(m_gui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(OnFileMenuOpen()));

        UpdateOpenRecentMenu();

        connect(m_gui->actionAutocomplete, SIGNAL(triggered(bool)), this, SLOT(OnAutocompleteChanged(bool)));

        auto newState = AZ::UserSettings::CreateFind<LUAEditorMainWindowSavedState>(AZ_CRC_CE("LUA EDITOR MAIN WINDOW STATE"), AZ::UserSettings::CT_LOCAL);
        m_gui->actionAutoReloadUnmodifiedFiles->setChecked(newState->m_bAutoReloadUnmodifiedFiles);

        connect(m_gui->actionAutoReloadUnmodifiedFiles, &QAction::triggered, this, [](bool newValue)
        {
            auto newState = AZ::UserSettings::CreateFind<LUAEditorMainWindowSavedState>(AZ_CRC_CE("LUA EDITOR MAIN WINDOW STATE"), AZ::UserSettings::CT_LOCAL);
            newState->m_bAutoReloadUnmodifiedFiles = newValue;
        });

        connect(this, &LUAEditorMainWindow::OnReferenceDataChanged, this, [this]() {
            luaClassFilterTextChanged(m_ClassReferenceFilter->GetFilter());
            if (auto* view = GetCurrentView())
            {
                // Update syntax highlighting now that the project libraries are loaded
                view->UpdateFont();
            }
         });

        // preset our running state based on outside conditions when we are created
        if (connectedState)
        {
            OnConnectedToTarget();
        }
        else
        {
            OnDisconnectedFromTarget();
        }

        {
            // an attempt to make this all more readable!
            using namespace AzToolsFramework;
            typedef FrameworkMessages::Bus HotkeyBus;

            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUALinesUpTranspose"), m_gui->actionLinesUpTranspose);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUALinesDnTranspose"), m_gui->actionLinesDnTranspose);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("GeneralOpenAssetBrowser"), m_gui->actionOpen);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAFind"), m_gui->actionFind);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAQuickFindLocal"), m_gui->actionFindLocal);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAQuickFindLocalReverse"), m_gui->actionFindLocalReverse);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAFindInFiles"), m_gui->actionFindInAllOpen);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAReplace"), m_gui->actionReplace);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAReplaceInFiles"), m_gui->actionReplaceInAllOpen);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAGoToLine"), m_gui->actionGoToLine);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAFold"), m_gui->actionFoldAll);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAUnfold"), m_gui->actionUnfoldAll);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUACloseAllExceptCurrent"), m_gui->actionCloseAllExcept);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUACloseAll"), m_gui->actionCloseAll);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAComment"), m_gui->actionComment_Selected_Block);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAUncomment"), m_gui->actionUnComment_Selected_Block);
            HotkeyBus::Broadcast(&HotkeyBus::Events::RegisterActionToHotkey, AZ_CRC_CE("LUAResetZoom"), m_gui->actionResetZoom);
        }

        //m_StoredTabAssetId = AZ::Uuid::CreateNull();
        installEventFilter(this);

        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::Application, theMenu);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::File, m_gui->menuFile);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::Edit, m_gui->menuEdit);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::View, m_gui->menuView);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::Debug, m_gui->menuDebug);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::SourceControl, m_gui->menuSource_Control);
        LegacyFramework::CustomMenusMessages::Bus::Broadcast(&LegacyFramework::CustomMenusMessages::Bus::Events::RegisterMenu, LegacyFramework::CustomMenusCommon::LUAEditor::Options, m_gui->menu_Options);

        QObject::connect(m_gui->menu_Options, &QMenu::aboutToShow, this, &LUAEditorMainWindow::OnOptionsMenuRequested);


        connect(m_gui->m_logPanel, &AzToolsFramework::LogPanel::TracePrintFLogPanel::LogLineSelected, this, &LUAEditorMainWindow::LogLineSelectionChanged);
    }

    void LUAEditorMainWindow::OnOptionsMenuRequested()
    {
        m_gui->actionAutocomplete->blockSignals(true);
        m_gui->actionAutocomplete->setCheckable(true);
        m_gui->actionAutocomplete->setChecked(m_bAutocompleteEnabled);
        m_gui->actionAutocomplete->blockSignals(false);
    }

    void LUAEditorMainWindow::UpdateOpenRecentMenu()
    {
        const QStringList recentFiles = ReadRecentFiles();

        QList<QAction*> actions = m_gui->menuOpenRecent->actions();

        for (int i = actions.size() - 1; i >= 0; i--)
        {
            m_gui->menuOpenRecent->removeAction(actions[i]);
        }

        for (auto& fileName : recentFiles)
        {
            QAction* action = new QAction(fileName, this);
            connect(
                action,
                &QAction::triggered,
                this,
                [fileName]([[maybe_unused]] bool checked)
                {
                    constexpr bool errorOnNotFound = true;
                    Context_DocumentManagement::Bus::Broadcast(
                        &Context_DocumentManagement::Bus::Events::OnLoadDocument, fileName.toStdString().c_str(), errorOnNotFound);
                });
            m_gui->menuOpenRecent->addAction(action);
        }

        m_gui->menuOpenRecent->addSeparator();

        m_actionClearRecentFiles = new QAction("Clear Recent Files", this);

        connect(
            m_actionClearRecentFiles,
            &QAction::triggered,
            this,
            [this]([[maybe_unused]] bool checked)
            {
                ClearRecentFile();
                UpdateOpenRecentMenu();
            });

        m_gui->menuOpenRecent->addAction(m_actionClearRecentFiles);

        m_gui->menuOpenRecent->setEnabled(!recentFiles.isEmpty());
        m_actionClearRecentFiles->setEnabled(!recentFiles.isEmpty());
    }

    LUAEditorMainWindow::~LUAEditorMainWindow(void)
    {
        removeEventFilter(this);

        LUAViewMessages::Bus::Handler::BusDisconnect();
        //ClientInterface::Handler::BusDisconnect();
        LUAEditorMainWindowMessages::Handler::BusDisconnect();
        LUABreakpointTrackerMessages::Handler::BusDisconnect();

        azdestroy(m_gui);

        delete m_assetDatabaseListener;
        m_assetDatabaseListener = nullptr;
    }

    void LUAEditorMainWindow::SetupLuaFilesPanel()
    {
        if (m_assetDatabaseListener != nullptr)
        {
            // We have already setup the panel, so nothing to do
            return;
        }

        AZStd::string cacheRoot;
        bool cacheRootFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(cacheRootFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetAbsoluteAssetDatabaseLocation, cacheRoot);
        if (!cacheRootFound)
        {
            return;
        }

        m_assetDatabaseListener = new AssetDatabaseLocationListener();
        m_assetDatabaseListener->Init(cacheRoot.c_str());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::StartMonitoringAssets);

        // Get the asset browser model
        AzToolsFramework::AssetBrowser::AssetBrowserModel* assetBrowserModel = nullptr;
        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);
        AZ_Assert(assetBrowserModel, "Failed to get filebrowser model");

        // Hook up the data set to the tree view
        m_filterModel = aznew AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(this);
        m_filterModel->setSourceModel(assetBrowserModel);

        // Delay the setting of the filter until everything can be initialized
        QTimer::singleShot(1000, [this]()
        {
            m_filterModel->SetFilter(CreateFilter());
        });

        m_gui->m_assetBrowserTreeView->setModel(m_filterModel);

        m_gui->m_assetBrowserTreeView->SetShowSourceControlIcons(true);

        m_gui->m_assetBrowserTreeView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

        // Maintains the tree expansion state between runs
        m_gui->m_assetBrowserTreeView->SetName("LuaIDETreeView");

        connect(m_gui->m_assetBrowserTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex&)
        {
            auto selectedAssets = m_gui->m_assetBrowserTreeView->GetSelectedAssets();
            if (selectedAssets.size() == 1)
            {
                auto selectedAsset = selectedAssets.front();
                const AZStd::string filePath = selectedAsset->GetFullPath();
                auto entryType = selectedAsset->GetEntryType();
                if (entryType == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
                {
                    Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, filePath, true);
                }                    
            }
        });
    }

    QSharedPointer<AzToolsFramework::AssetBrowser::CompositeFilter> LUAEditorMainWindow::CreateFilter()
    {
        using namespace AzToolsFramework::AssetBrowser;

        // Only look at Script Assets (.lua files)
        // Propagate this down to cover all the parents of a script asset
        AssetTypeFilter* assetFilter = new AssetTypeFilter();
        assetFilter->SetAssetType(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());
        assetFilter->SetFilterPropagation(AssetTypeFilter::PropagateDirection::Down);

        // We only care about sources (not products)
        // Do not propagate this at all
        EntryTypeFilter* entryTypeFilter = new EntryTypeFilter();
        entryTypeFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);
        entryTypeFilter->SetFilterPropagation(AssetTypeFilter::PropagateDirection::None);

        // Add in a string filter that comes from user input
        StringFilter* stringFilter = new StringFilter();
        stringFilter->SetFilterPropagation(AssetTypeFilter::PropagateDirection::Up);

        connect(m_gui->m_assetBrowserSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, [&, stringFilter](const QString& newString)
        {
            stringFilter->SetFilterString(newString);
            if (newString.isEmpty())
            {
                m_gui->m_assetBrowserTreeView->collapseAll();
            }
            else
            {
                m_gui->m_assetBrowserTreeView->expandAll();
            }
        });

        // Construct the final filter where they are all and'd together
        // Propagate the final filter down so that any matches will show the hierarchy of folders down to the appropriate matching leaf node
        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(FilterConstType(stringFilter));
        finalFilter->AddFilter(FilterConstType(assetFilter));
        finalFilter->AddFilter(FilterConstType(entryTypeFilter));
        finalFilter->SetFilterPropagation(AssetTypeFilter::PropagateDirection::Down);

        return finalFilter;
    }

    void LUAEditorMainWindow::OnSettings()
    {
        m_settingsDialog->show();
    }

    void LUAEditorMainWindow::OnLuaDocumentation()
    {
        QDesktopServices::openUrl(QUrl("https://o3de.org/docs/user-guide/scripting/lua/"));
    }

    void LUAEditorMainWindow::OnMenuCloseCurrentWindow()
    {
        AzToolsFramework::FrameworkMessages::Bus::Broadcast(
            &AzToolsFramework::FrameworkMessages::Bus::Events::RequestMainWindowClose, ContextID);
    }

    void LUAEditorMainWindow::OnAutocompleteChanged(bool change)
    {
        m_bAutocompleteEnabled = change;
        m_gui->actionAutocomplete->blockSignals(true);
        m_gui->actionAutocomplete->setCheckable(true);
        m_gui->actionAutocomplete->setChecked(m_bAutocompleteEnabled);
        m_gui->actionAutocomplete->blockSignals(false);

        for (TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.begin(); viewInfoIter != m_dOpenLUAView.end(); ++viewInfoIter)
        {
            TrackedLUAView& viewInfo = viewInfoIter->second;
            viewInfo.luaViewWidget()->SetAutoCompletionEnabled(m_bAutocompleteEnabled);
        }
        //AZ_TracePrintf("LUA","change %d",change);
    }

    //////////////////////////////////////////////////////////////////////////

    void LUAEditorMainWindow::OnOpenLUAView(const DocumentInfo& docInfo)
    {
        //char output[64];
        //docInfo.m_assetId.ToString(output,AZ_ARRAY_SIZE(output),true,true);
        //AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("OnOpenLuaView %s = %s\n", (docInfo.m_assetName + ".lua").c_str(), output).c_str());

        show();

        //set focus if already created
        TrackedLUAViewMap::iterator viewIter = m_dOpenLUAView.find(docInfo.m_assetId);
        if (viewIter != m_dOpenLUAView.end())
        {
            viewIter->second.luaDockWidget()->show();
            viewIter->second.luaDockWidget()->raise();
            viewIter->second.luaViewWidget()->setFocus();
            return;
        }

        setAnimated(false);

        //make a new one
        LUADockWidget* luaDockWidget = aznew LUADockWidget(this->centralWidget());
        luaDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);//do not add floatable!
        luaDockWidget->setAssetId(docInfo.m_assetId);
        QWidget* luaLayout = new QWidget();

        luaLayout->setLayout(aznew LUAEditorMainWindowLayout(luaLayout));
        luaLayout->layout()->setContentsMargins(0, 0, 0, 0);

        LUAViewWidget* luaViewWidget = aznew LUAViewWidget();
        luaViewWidget->SetLuaDockWidget(luaDockWidget);
        luaDockWidget->setObjectName(QString::fromUtf8(docInfo.m_displayName.c_str()));

        luaViewWidget->setObjectName(QString::fromUtf8(docInfo.m_displayName.c_str()));
        luaDockWidget->setWidget(luaLayout);
        luaViewWidget->Initialize(docInfo);

        luaViewWidget->installEventFilter(this);

        m_ptrPerforceStatusWidget = new QLabel(tr("Pending Status"), this);
        m_ptrPerforceStatusWidget->setMargin(2);
        m_ptrPerforceStatusWidget->setStyleSheet(QString("background: rgba(192,192,192,255); color: black;  border-style: inset;\nborder-width: 1px;\nborder-color: rgba(100,100,100,255);\nborder-radius: 8px;"));
        m_ptrPerforceStatusWidget->setAutoFillBackground(true);
        m_ptrPerforceStatusWidget->setTextInteractionFlags(Qt::NoTextInteraction);
        m_ptrPerforceStatusWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        connect(luaViewWidget, SIGNAL(sourceControlStatusUpdated(QString)), m_ptrPerforceStatusWidget, SLOT(setText(QString)));

        luaLayout->layout()->addWidget(luaViewWidget);
        luaLayout->layout()->addWidget(m_ptrPerforceStatusWidget);

        //if we already have an open view tabbify, if not just add it to the side
        if (m_dOpenLUAView.size() && !m_lastFocusedAssetId.empty())
        {
            viewIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
            if (viewIter != m_dOpenLUAView.end())
            {
                qobject_cast<QMainWindow*>(this->centralWidget())->tabifyDockWidget(viewIter->second.luaDockWidget(), luaDockWidget);
            }
            else
            {
                qobject_cast<QMainWindow*>(this->centralWidget())->addDockWidget(static_cast<Qt::DockWidgetArea>(0x4), luaDockWidget, Qt::Horizontal);
            }
        }
        else
        {
            qobject_cast<QMainWindow*>(this->centralWidget())->addDockWidget(static_cast<Qt::DockWidgetArea>(0x4), luaDockWidget, Qt::Horizontal);
        }

        //track it
        if (m_lastFocusedAssetId.empty())
        {
            m_lastFocusedAssetId = docInfo.m_assetId;
        }
        m_dOpenLUAView.insert(AZStd::make_pair(docInfo.m_assetId, TrackedLUAView(luaDockWidget, luaViewWidget, docInfo.m_assetId)));
        m_CtrlTabOrder.push_front(docInfo.m_assetId);
        //if (m_dOpenLUAView.size() == 1)
        {
            QApplication::processEvents();
        }

        AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::OnDockWidgetLocationChanged, this, docInfo.m_assetId);

        luaDockWidget->show();
        luaDockWidget->raise();
        luaViewWidget->setFocus();
        m_ptrPerforceStatusWidget->raise();

        connect(luaDockWidget, SIGNAL(visibilityChanged(bool)), luaViewWidget, SLOT(OnVisibilityChanged(bool)));

        SetEditContolsToAtLeastOneFileOpen();
        setAnimated(true);
    };

    void LUAEditorMainWindow::OnDockWidgetLocationChanged(const AZStd::string assetId)
    {
        // find the widget:
        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(assetId);
        if (viewInfoIter == m_dOpenLUAView.end())
        {
            return;
        }

        QTabBar* pBar = nullptr;

        // We need to find out what we are docked to in order for this to work correctly.
        // this function was added in, and I can't find anything similar that will let me do the same thing.
        // Without this, we lose the ability to have the right click -> close all but this.
        //QTabBar *pBar = qobject_cast<QMainWindow*>(this->centralWidget())->tabBar(viewInfoIter->second.luaDockWidget());

        if (!pBar)
        {
            return;
        }

        // have we ever done this already?
        if (pBar->documentMode() == false)
        {
            pBar->setDocumentMode(true);
            pBar->setElideMode(Qt::ElideNone); // do not elide!
            pBar->setTabsClosable(true);
            connect(pBar, &QTabBar::tabCloseRequested, this,
                [assetId, this]()
                {
                    AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::RequestCloseDocument, this, assetId);
                });

            pBar->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(pBar,
                &QWidget::customContextMenuRequested,
                this,
                [assetId, this](const QPoint& point)
                {
                    this->showTabContextMenu(assetId, point);
                }
                );
        }
    }

    void LUAEditorMainWindow::showTabContextMenu(const AZStd::string& assetId, const QPoint& pos)
    {
        QTabBar* emitter = qobject_cast<QTabBar*>(sender());
        if (!emitter)
        {
            return;
        }

        int tabIdx = emitter->tabAt(pos);
        if (tabIdx < 0)
        {
            return;
        }

        m_currentTabContextMenuUUID = assetId;

        if (m_currentTabContextMenuUUID.empty())
        {
            return;
        }

        QMenu menu(this);
        menu.addAction("Close All Except This", this, SLOT(closeAllTabsExceptThisTabContextMenu()));
        menu.exec(emitter->mapToGlobal(pos));
    }

    void LUAEditorMainWindow::closeAllTabsExceptThisTabContextMenu()
    {
        if (m_currentTabContextMenuUUID.empty())
        {
            return;
        }

        for (TrackedLUAViewMap::iterator it = m_dOpenLUAView.begin(); it != m_dOpenLUAView.end(); ++it)
        {
            if (it->first != m_currentTabContextMenuUUID)
            {
                AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::RequestCloseDocument, this, it->first);
            }
        }
        m_currentTabContextMenuUUID = "";
    }

    void LUAEditorMainWindow::OnOpenWatchView()
    {
        show();
        m_gui->watchDockWidget->show();
        m_gui->watchDockWidget->setFocus();
    }

    void LUAEditorMainWindow::OnOpenReferenceView()
    {
        show();
        m_gui->classReferenceDockWidget->show();
        m_gui->classReferenceDockWidget->setFocus();
    }

    void LUAEditorMainWindow::OnOpenBreakpointsView()
    {
        show();
        m_gui->breakpointsDockWidget->show();
        m_gui->breakpointsDockWidget->raise();
        m_gui->breakpointsDockWidget->setFocus();
    }

    void LUAEditorMainWindow::OnOpenStackView()
    {
        show();
        m_gui->stackDockWidget->show();
        m_gui->stackDockWidget->raise();
        m_gui->stackDockWidget->setFocus();
    }

    void LUAEditorMainWindow::OnOpenLocalsView()
    {
        show();
        m_gui->localsDockWidget->show();
        m_gui->localsDockWidget->raise();
        m_gui->localsDockWidget->setFocus();
    }

    void LUAEditorMainWindow::OnOpenFindView(int index)
    {
        show();
        m_gui->findResultsDockWidget->show();
        m_gui->findResultsDockWidget->raise();
        m_gui->findResultsDockWidget->setFocus();
        m_gui->findTabWidget->setCurrentIndex(index);
    }

    void LUAEditorMainWindow::ResetSearchClicks()
    {
        m_dProcessFindListClicked.clear();
    }

    void LUAEditorMainWindow::MoveProgramCursor(const AZStd::string& assetId, int lineNumber)
    {
        if ((m_lastProgramCounterAssetId != assetId) && (!m_lastProgramCounterAssetId.empty()))
        {
            // the program counter has moved from one document to another.
            // remove it from the old one.
            TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastProgramCounterAssetId);

            if (viewInfoIter != m_dOpenLUAView.end())
            {
                TrackedLUAView& viewInfo = viewInfoIter->second;
                LUAViewWidget* pTextWidget = viewInfo.luaViewWidget();
                pTextWidget->UpdateCurrentExecutingLine(-1);
            }
        }

        m_lastProgramCounterAssetId = "";

        // now add it to the new one:
        {
            TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(assetId);

            if (viewInfoIter != m_dOpenLUAView.end())
            {
                TrackedLUAView& viewInfo = viewInfoIter->second;
                LUAViewWidget* pTextWidget = viewInfo.luaViewWidget();
                pTextWidget->UpdateCurrentExecutingLine(lineNumber);
                m_lastProgramCounterAssetId = assetId;
            }
        }

        if (lineNumber == -1)
        {
            m_lastProgramCounterAssetId = "";
        }
    }

    void LUAEditorMainWindow::MoveEditCursor(const AZStd::string& assetId, int lineNumber, bool withSelection)
    {
        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(assetId);

        if (viewInfoIter != m_dOpenLUAView.end())
        {
            TrackedLUAView& viewInfo = viewInfoIter->second;
            LUAViewWidget* pTextWidget = viewInfo.luaViewWidget();
            pTextWidget->UpdateCurrentEditingLine(lineNumber);
            if (withSelection)
            {
                pTextWidget->SetSelection(lineNumber, 0, lineNumber + 1, 0);
            }
        }
    }


    // debug menu items
    void LUAEditorMainWindow::ExecuteScript(bool executeLocally)
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }
        if (SyncDocumentToContext(m_lastFocusedAssetId))
        {
            Context_DebuggerManagement::Bus::Broadcast(
                &Context_DebuggerManagement::Bus::Events::ExecuteScriptBlob, m_lastFocusedAssetId, executeLocally);
        }
    }


    void LUAEditorMainWindow::OnDebugExecute()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        LUAViewWidget* view = GetCurrentView();
        if (view)
        {
            view->UpdateCurrentExecutingLine(-1);
        }

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);

        if (viewInfoIter != m_dOpenLUAView.end())
        {
            ExecuteScript(true);
        }
    }

    void LUAEditorMainWindow::OnDebugExecuteOnTarget()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        LUAViewWidget* view = GetCurrentView();
        if (view)
        {
            view->UpdateCurrentExecutingLine(-1);
        }
    }

    // execution control
    void LUAEditorMainWindow::OnDebugToggleBreakpoint()
    {
        // current view
        LUAViewWidget* view = GetCurrentView();
        if (view)
        {
            // current line
            int line, index;
            view->GetCursorPosition(line, index);
            view->BreakpointToggle(line);
        }
    }

    void LUAEditorMainWindow::OnDebugContinueRunning()
    {
        LUAEditorDebuggerMessages::Bus::Broadcast(&LUAEditorDebuggerMessages::Bus::Events::DebugRunContinue);
    }

    void LUAEditorMainWindow::OnDebugStepOver()
    {
        LUAEditorDebuggerMessages::Bus::Broadcast(&LUAEditorDebuggerMessages::Bus::Events::DebugRunStepOver);
    }

    void LUAEditorMainWindow::OnDebugStepIn()
    {
        LUAEditorDebuggerMessages::Bus::Broadcast(&LUAEditorDebuggerMessages::Bus::Events::DebugRunStepIn);
    }

    void LUAEditorMainWindow::OnDebugStepOut()
    {
        LUAEditorDebuggerMessages::Bus::Broadcast(&LUAEditorDebuggerMessages::Bus::Events::DebugRunStepOut);
    }

    //file menu

    void LUAEditorMainWindow::OnFileMenuOpen()
    {
        AssetSelectionModel selection;

        StringFilter* stringFilter = new StringFilter();
        stringFilter->SetName("Lua file (*.lua)");
        stringFilter->SetFilterString(".lua");
        stringFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
        auto stringFilterPtr = FilterConstType(stringFilter);

        selection.SetDisplayFilter(stringFilterPtr);
        selection.SetSelectionFilter(stringFilterPtr);

        AssetBrowserComponentRequestBus::Broadcast(
            &AssetBrowserComponentRequests::PickAssets, selection, AzToolsFramework::GetActiveWindow());

        if (!selection.IsValid())
        {
            return;
        }

        auto* result = selection.GetResult();

        if (!result)
        {
            AZ_Assert(false, "Lua script - Incorrect entry type selected during script instantiation.");
            return;
        }

        const AZStd::string assetId(result->GetFullPath().data());
        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, assetId, true);
        AzFramework::StringFunc::Path::Split(assetId.c_str(), nullptr, &m_lastOpenFilePath);
        AddRecentFile(result->GetFullPath().c_str());
        UpdateOpenRecentMenu();
    }

    void LUAEditorMainWindow::OnFileMenuNew()
    {
        AZStd::string assetId;
        if (!OnFileSaveDialog("", assetId))
        {
            return;
        }

        if (AzFramework::StringFunc::Find(assetId.c_str(), ".lua") == AZStd::string::npos)
        {
            assetId += ".lua";
        }

        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnNewDocument, assetId);
        SetEditContolsToAtLeastOneFileOpen();
    }

    bool LUAEditorMainWindow::SyncDocumentToContext(const AZStd::string& assetId)
    {
        if (assetId.empty())
        {
            return false;
        }

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(assetId);
        AZ_Assert(viewInfoIter != m_dOpenLUAView.end(), "OnFileMenuClose() : Cant find view Info.");
        TrackedLUAView& viewInfo = viewInfoIter->second;
        QByteArray viewBuffer = viewInfo.luaViewWidget()->GetText().toUtf8();
        AZStd::size_t viewSize = viewBuffer.size();

        Context_DocumentManagement::Bus::Broadcast(
            &Context_DocumentManagement::Bus::Events::UpdateDocumentData, assetId, viewBuffer.data(), viewSize);
        return true;
    }

    void LUAEditorMainWindow::OnFileMenuSave()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
        AZ_Assert(viewInfoIter != m_dOpenLUAView.end(), "OnFileMenuSave() : Cant find view Info.");
        TrackedLUAView& viewInfo = viewInfoIter->second;

        //has the views text changed?
        if (viewInfo.luaViewWidget()->IsReadOnly())
        {
            AZ_Warning("LUA Editor", false, "Cannot save document - it is read-only (Check out first)");
            return;
        }

        if (SyncDocumentToContext(m_lastFocusedAssetId))
        {
            Context_DocumentManagement::Bus::Broadcast(
                &Context_DocumentManagement::Bus::Events::OnSaveDocument, m_lastFocusedAssetId, false, false);
        }
    }

    void LUAEditorMainWindow::OnFileMenuSaveAs()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        if (SyncDocumentToContext(m_lastFocusedAssetId))
        {
            bool saveSuccess = false;
            Context_DocumentManagement::Bus::BroadcastResult(
                saveSuccess, &Context_DocumentManagement::Bus::Events::OnSaveDocumentAs, m_lastFocusedAssetId, false);
        }
    }

    void LUAEditorMainWindow::OnFileMenuSaveAll()
    {
        for (TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.begin(); viewInfoIter != m_dOpenLUAView.end(); ++viewInfoIter)
        {
            TrackedLUAView& viewInfo = viewInfoIter->second;

            //has the views text changed?
            if (viewInfo.luaViewWidget()->IsReadOnly())
            {
                continue;
            }

            if (SyncDocumentToContext(viewInfo.luaViewWidget()->m_Info.m_assetId))
            {
                Context_DocumentManagement::Bus::Broadcast(
                    &Context_DocumentManagement::Bus::Events::OnSaveDocument, viewInfo.luaViewWidget()->m_Info.m_assetId, false, false);
            }
        }
    }

    void LUAEditorMainWindow::OnFileMenuReload()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        auto currentView = GetCurrentView();
        if (!currentView)
        {
            return;
        }

        // if this view has been modified, prompt to be sure they want to lose the changes
        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
        if (viewInfoIter == m_dOpenLUAView.end())
        {
            return; // no such view, dafuq?
        }
        TrackedLUAView& viewInfo = viewInfoIter->second;

        //has the views text changed?
        if (!viewInfo.luaViewWidget()->IsReadOnly() &&
            viewInfo.luaViewWidget()->IsModified())
        {
            QMessageBox msgBox;
            msgBox.setText("This file has been modified.\nDo you really want to Reload and lose changes?");
            msgBox.setInformativeText(currentView->m_Info.m_assetName.c_str());
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::Warning);
            int ret = msgBox.exec();
            if (ret != QMessageBox::Ok)
            {
                return;
            }
        }

        // Need to store this off for use on the reload since it will be cleared out/changed as part of the OnCloseDocument call
        AZStd::string asset = m_lastFocusedAssetId;

        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnCloseDocument, m_lastFocusedAssetId);
        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, asset, true);

        //not sure about this... looks like legacy, may need to be removed?
        // instate the topmost tab as the current asset ID
        // so that the window we're going to reopen has something to tabify onto
        TrackedLUACtrlTabOrder::iterator tabIter = m_CtrlTabOrder.begin();
        if (tabIter != m_CtrlTabOrder.end())
        {
            m_lastFocusedAssetId = *tabIter;
        }
    }

    void LUAEditorMainWindow::OnFileMenuClose()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        RequestCloseDocument(m_lastFocusedAssetId);
    }

    void LUAEditorMainWindow::OnFileMenuCloseAll()
    {
        for (TrackedLUAViewMap::iterator it = m_dOpenLUAView.begin(); it != m_dOpenLUAView.end(); ++it)
        {
            AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::RequestCloseDocument, this, it->first);
        }
    }

    void LUAEditorMainWindow::OnFileMenuCloseAllExcept()
    {
        for (TrackedLUAViewMap::iterator it = m_dOpenLUAView.begin(); it != m_dOpenLUAView.end(); ++it)
        {
            if (it->first != m_lastFocusedAssetId)
            {
                AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::RequestCloseDocument, this, it->first);
            }
        }
    }

    bool LUAEditorMainWindow::RequestCloseDocument(const AZStd::string id)
    {
        AZStd::string assetId = id;

        //AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("OnFileMenuClose %s\n", assetId.c_str()).c_str());

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(assetId);
        if (viewInfoIter == m_dOpenLUAView.end())
        {
            return true; // no such view, probably a double click on close.
        }
        TrackedLUAView& viewInfo = viewInfoIter->second;
        //has the views text changed?
        if (!viewInfo.luaViewWidget()->IsReadOnly() &&
            viewInfo.luaViewWidget()->IsModified())
        {
            AzToolsFramework::SaveChangesDialog dialog(this);
            dialog.exec();
            AzToolsFramework::SaveChangesDialogResult dr = dialog.m_result;
            if (dr == AzToolsFramework::SCDR_Save)
            {
                //the user wants to save before closing
                if (!SyncDocumentToContext(assetId))
                {
                    return false;
                }

                Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnSaveDocument, assetId, true, false);
                return true;
            }
            else if (dr == AzToolsFramework::SCDR_DiscardAndContinue)
            {
                //the user has chosen to continue to close the document and lose changes
                Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnCloseDocument, assetId);
                return true;
            }
            else
            {
                //the user has canceled the close //or close was pressed
                return false;
            }
        }
        else
        {
            //no changes, just close the document
            Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnCloseDocument, assetId);
            return true;
        }
    }

    void LUAEditorMainWindow::OnCloseView(const AZStd::string& assetId)
    {
        //remove it from the tracking list
        setAnimated(false);
        TrackedLUAViewMap::iterator viewIter = m_dOpenLUAView.find(assetId);
        if (viewIter != m_dOpenLUAView.end())
        {
            viewIter->second.luaDockWidget()->deleteLater();
            m_dOpenLUAView.erase(viewIter);
            TrackedLUACtrlTabOrder::iterator tabIter = m_CtrlTabOrder.begin();
            while (tabIter != m_CtrlTabOrder.end())
            {
                if (*tabIter == assetId)
                {
                    m_CtrlTabOrder.erase(tabIter);
                    break;
                }

                ++tabIter;
            }
        }

        if (m_lastFocusedAssetId == assetId)
        {
            m_lastFocusedAssetId = "";
        }

        //if there are no open views then null out the last focused document it should be invalid now
        if (m_dOpenLUAView.empty())
        {
            m_lastFocusedAssetId = "";
            SetEditContolsToNoFilesOpen();

            AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("Last Focused Document ID to nullptr\n").c_str());
        }
        setAnimated(true);
    }

    void SendKeys(QWidget* pWidget, Qt::Key keyToSend, Qt::KeyboardModifier modifiers, QAction* pActionToDisable)
    {
        if (!pWidget)
        {
            return;
        }

        if (pActionToDisable)
        {
            pActionToDisable->setDisabled(true);
        }
        QKeyEvent PressIt(QEvent::KeyPress, keyToSend, modifiers);
        QKeyEvent ReleaseIt(QEvent::KeyRelease, keyToSend, modifiers);
        QApplication::sendEvent(pWidget, &PressIt);
        QApplication::sendEvent(pWidget, &ReleaseIt);
        if (pActionToDisable)
        {
            pActionToDisable->setDisabled(false);
        }
    }

    //edit menu
    void LUAEditorMainWindow::OnEditMenuUndo()
    {
        SendKeys(QApplication::focusWidget(), Qt::Key_Z, Qt::ControlModifier, m_gui->actionUndo);
    }

    void LUAEditorMainWindow::OnEditMenuRedo()
    {
        SendKeys(QApplication::focusWidget(), Qt::Key_Y, Qt::ControlModifier, m_gui->actionRedo);
    }

    void LUAEditorMainWindow::OnEditMenuCut()
    {
        auto currentView = GetCurrentView();
        if (!currentView)
        {
            return;
        }

        int lineFrom;
        int indexFrom;
        int lineTo;
        int indexTo;
        currentView->GetSelection(lineFrom, indexFrom, lineTo, indexTo);
        // on no selection
        if (lineFrom == -1)
        {
            // get the line and strip the whitespace
            currentView->GetCursorPosition(lineFrom, indexFrom);
            currentView->SetSelection(lineFrom, 0, lineFrom + 1, 0);
            QString cutThis = currentView->GetLineText(lineFrom);
            QString finalCut = cutThis.simplified();
            if (!finalCut.length())
            {
                // if the string is now empty, then it was all whitespace and so remove it but not to clipboard
                currentView->RemoveSelectedText();
                return;
            }
        }

        // drop through to standard cut to clipboard handling with the original or our newly selected line(s)
        currentView->Cut();
    }

    void LUAEditorMainWindow::OnEditMenuCopy()
    {
        auto currentView = GetCurrentView();
        if (!currentView)
        {
            return;
        }

        int lineFrom;
        int indexFrom;
        int lineTo;
        int indexTo;
        bool wasSelected = true;
        currentView->GetSelection(lineFrom, indexFrom, lineTo, indexTo);
        // on no selection, force this one line
        if (lineFrom == -1)
        {
            wasSelected = false;
            currentView->GetCursorPosition(lineFrom, indexFrom);
            currentView->SetSelection(lineFrom, 0, lineFrom + 1, 0);
        }

        currentView->Copy();

        if (!wasSelected)
        {
            currentView->SetCursorPosition(lineFrom, indexFrom);
        }
    }

    void LUAEditorMainWindow::OnEditMenuPaste()
    {
        SendKeys(QApplication::focusWidget(), Qt::Key_V, Qt::ControlModifier, m_gui->actionPaste);
    }

    void LUAEditorMainWindow::OnEditMenuFind()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->show();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(false);
        m_ptrFindDialog->SetNewSearchStarting();
        m_ptrFindDialog->ResetSearch();
        m_ptrFindDialog->activateWindow();
        m_ptrFindDialog->raise();
    }

    void LUAEditorMainWindow::OnEditMenuReplace()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->show();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(false);
        m_ptrFindDialog->SetNewSearchStarting();
        m_ptrFindDialog->ResetSearch();
        m_ptrFindDialog->activateWindow();
        m_ptrFindDialog->raise();
    }

    void LUAEditorMainWindow::OnEditMenuFindInAllOpen()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->show();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(true);
        m_ptrFindDialog->SetNewSearchStarting();
        m_ptrFindDialog->ResetSearch();
        m_ptrFindDialog->activateWindow();
        m_ptrFindDialog->raise();
    }

    void LUAEditorMainWindow::OnEditMenuReplaceInAllOpen()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->show();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(true);
        m_ptrFindDialog->SetNewSearchStarting();
        m_ptrFindDialog->ResetSearch();
        m_ptrFindDialog->activateWindow();
        m_ptrFindDialog->raise();
    }

    void LUAEditorMainWindow::OnEditMenuFindLocal()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(false);
        m_ptrFindDialog->SetNewSearchStarting(true, true);
        m_ptrFindDialog->OnFindNext();
    }

    void LUAEditorMainWindow::OnEditMenuFindLocalReverse()
    {
        m_ptrFindDialog->SaveState();
        m_ptrFindDialog->SetAnyDocumentsOpen(m_StateTrack.atLeastOneFileOpen);
        m_ptrFindDialog->SetToFindInAllOpen(false);
        m_ptrFindDialog->SetNewSearchStarting(true, false);
        m_ptrFindDialog->OnFindNext();
    }

    void LUAEditorMainWindow::OnEditMenuFindNext()
    {
        if (m_ptrFindDialog)
        {
            m_ptrFindDialog->OnFindNext();
        }
    }

    void LUAEditorMainWindow::OnEditMenuGoToLine()
    {
        auto currentView = GetCurrentView();
        if (!currentView)
        {
            return;
        }

        LUAEditorGoToLineDialog dlg(this);

        int lineNumber = 0, cursorColumn = 0;
        currentView->GetCursorPosition(lineNumber, cursorColumn);
        dlg.setLineNumber(lineNumber + 1);

        if (dlg.exec() != QDialog::Rejected)
        {
            // go to that line of the selected file.
            lineNumber = dlg.getLineNumber();

            currentView->SetCursorPosition(lineNumber, 0);
        }
    }

    void LUAEditorMainWindow::OnEditMenuFoldAll()
    {
        if (auto currentView = GetCurrentView())
        {
            currentView->FoldAll();
        }
    }

    void LUAEditorMainWindow::OnEditMenuUnfoldAll()
    {
        if (auto currentView = GetCurrentView())
        {
            currentView->UnfoldAll();
        }
    }

    void LUAEditorMainWindow::OnEditMenuSelectAll()
    {
        if (auto currentView = GetCurrentView())
        {
            currentView->SelectAll();
        }
    }
    void LUAEditorMainWindow::OnEditMenuSelectToBrace()
    {
        if (auto currentView = GetCurrentView())
        {
            currentView->SelectToMatchingBrace();
        }
    }

    void LUAEditorMainWindow::OnCommentSelectedBlock()
    {
        if (NeedsCheckout())
        {
            return;
        }

        if (auto currentView = GetCurrentView())
        {
            // must check read-only status at every operation that can modify the buffer
            if (!currentView->IsReadOnly())
            {
                currentView->CommentSelectedLines();
            }
        }
    }

    void LUAEditorMainWindow::OnUnCommentSelectedBlock()
    {
        if (NeedsCheckout())
        {
            return;
        }

        if (auto currentView = GetCurrentView())
        {
            // must check read-only status at every operation that can modify the buffer
            if (!currentView->IsReadOnly())
            {
                currentView->UncommentSelectedLines();
            }
        }
    }

    bool LUAEditorMainWindow::NeedsCheckout()
    {
        auto currentView = GetCurrentView();
        if (!currentView)
        {
            return false;
        }

        if (currentView->IsReadOnly())
        {
            QMessageBox msgBox;
            msgBox.setText("Checkout This File To Edit?");
            msgBox.setInformativeText(currentView->m_Info.m_assetName.c_str());
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::Warning);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Ok)
            {
                OnRequestCheckOut(currentView->m_Info.m_assetId);
            }

            return true;
        }
        return false;
    }

    void LUAEditorMainWindow::OnEditMenuTransposeUp()
    {
        if (NeedsCheckout())
        {
            return;
        }

        // must check read-only status at every operation that can modify the buffer
        if (auto currentView = GetCurrentView())
        {
            if (!currentView->IsReadOnly())
            {
                currentView->MoveSelectedLinesUp();
            }
        }
    }
    void LUAEditorMainWindow::OnEditMenuTransposeDn()
    {
        if (NeedsCheckout())
        {
            return;
        }

        // must check read-only status at every operation that can modify the buffer
        if (auto currentView = GetCurrentView())
        {
            if (!currentView->IsReadOnly())
            {
                currentView->MoveSelectedLinesDn();
            }
        }
    }

    //view menu
    void LUAEditorMainWindow::OnViewMenuBreakpoints()
    {
        OnOpenBreakpointsView();
    }

    void LUAEditorMainWindow::OnViewMenuStack()
    {
        OnOpenStackView();
    }

    void LUAEditorMainWindow::OnViewMenuLocals()
    {
        OnOpenLocalsView();
    }

    void LUAEditorMainWindow::OnViewMenuWatch()
    {
        OnOpenWatchView();
    }

    void LUAEditorMainWindow::OnViewMenuReference()
    {
        OnOpenReferenceView();
    }

    void LUAEditorMainWindow::OnViewMenuFind1()
    {
        OnOpenFindView(0);
    }

    void LUAEditorMainWindow::OnViewMenuFind2()
    {
        OnOpenFindView(1);
    }

    void LUAEditorMainWindow::OnViewMenuFind3()
    {
        OnOpenFindView(2);
    }

    void LUAEditorMainWindow::OnViewMenuFind4()
    {
        OnOpenFindView(3);
    }

    void LUAEditorMainWindow::OnViewMenuResetZoom()
    {
        if (auto currentView = GetCurrentView())
        {
            currentView->ResetZoom();
        }
    }

    //source control menu
    void LUAEditorMainWindow::OnSourceControlMenuCheckOut()
    {
        // if no last document id then return
        if (m_lastFocusedAssetId.empty())
        {
            return;
        }

        Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::RefreshAllDocumentPerforceStat);

        if (!SyncDocumentToContext(m_lastFocusedAssetId))
        {
            AZ_Warning(LUAEditorDebugName, false, "Could not sync doc data before checkout, data may be lost.");
            QMessageBox::warning(this, "Error!", "Could not sync document before checkout!");
            return;
        }

        Context_DocumentManagement::Bus::Broadcast(
            &Context_DocumentManagement::Bus::Events::DocumentCheckOutRequested, m_lastFocusedAssetId);
    }

    //tools menu

    // when the Editor Main window is requested to close, it is not destroyed.

    //////////////////////////////////////////////////////////////////////////
    // Qt Events
    void LUAEditorMainWindow::closeEvent(QCloseEvent* event)
    {
        OnMenuCloseCurrentWindow();
        event->ignore();
    }

    bool LUAEditorMainWindow::OnGetPermissionToShutDown()
    {
        bool willShutDown = true;

        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnGetPermissionToShutDown()\n");

        for (TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.begin(); viewInfoIter != m_dOpenLUAView.end(); ++viewInfoIter)
        {
            TrackedLUAView& viewInfo = viewInfoIter->second;

            //have the views' text changed?
            if (!viewInfo.luaViewWidget()->IsReadOnly() &&
                viewInfo.luaViewWidget()->IsModified())
            {
                this->show();
                viewInfo.luaDockWidget()->show();
                viewInfo.luaDockWidget()->raise();

                AzToolsFramework::SaveChangesDialog dialog(this);
                dialog.exec();
                AzToolsFramework::SaveChangesDialogResult dr = dialog.m_result;

                if (dr == AzToolsFramework::SCDR_Save)
                {
                    if (!SyncDocumentToContext(viewInfo.luaViewWidget()->m_Info.m_assetId))
                    {
                        AZ_Warning(LUAEditorDebugName, false, "Could not sync doc data before closing it, data may be lost.");
                        willShutDown = false;
                        AZ_TracePrintf(LUAEditorInfoName, "                            SyncDocumentToContext() failure\n");
                        break;
                    }

                    AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnGetPermissionToShutDown() SAVING %s\n", viewInfo.luaViewWidget()->m_Info.m_assetName.c_str());

                    Context_DocumentManagement::Bus::Broadcast(
                        &Context_DocumentManagement::Bus::Events::OnSaveDocument, viewInfo.luaViewWidget()->m_Info.m_assetId, false, false);
                }
                else if (dr == AzToolsFramework::SCDR_DiscardAndContinue)
                {
                    //the user has chosen to continue and lose changes
                    if (viewInfo.luaViewWidget()->m_Info.m_bUntitledDocument)
                    {
                        AZ_TracePrintf(LUAEditorDebugName, "                            Forced close\n");

                        // all untitled documents are force closed to clear any tracked states that will be serialized by their trackers
                        Context_DocumentManagement::Bus::Broadcast(
                            &Context_DocumentManagement::Bus::Events::OnCloseDocument, viewInfo.luaViewWidget()->m_Info.m_assetId);
                        viewInfoIter = m_dOpenLUAView.begin();
                    }
                    else // all titled (i.e. preexisting or saved files, simply reload
                    {
                        AZ_TracePrintf(LUAEditorDebugName, "                            Forced reload\n");

                        Context_DocumentManagement::Bus::Broadcast(
                            &Context_DocumentManagement::Bus::Events::OnReloadDocument, viewInfo.luaViewWidget()->m_Info.m_assetId);
                        viewInfoIter = m_dOpenLUAView.begin();
                    }
                }
                else
                {
                    //the user has canceled the close
                    willShutDown = false;
                    break;
                }
            }
        }
        
        return willShutDown;
    }

    void LUAEditorMainWindow::SaveWindowState()
    {
        // build state and store it.

        auto newState = AZ::UserSettings::CreateFind<LUAEditorMainWindowSavedState>(AZ_CRC_CE("LUA EDITOR MAIN WINDOW STATE"), AZ::UserSettings::CT_LOCAL);
        newState->Init(saveState(), saveGeometry());

        newState->m_bAutocompleteEnabled = m_bAutocompleteEnabled;

        // gather and store the open files in tab order
        newState->m_openAssetIds.clear();

        // the following is experimental, apparently widget child order doesn't match tab order, which is unfortunate
        QList<QDockWidget*> dockWidgets = qobject_cast<QMainWindow*>(this->centralWidget())->findChildren<QDockWidget*>();
        QList<QDockWidget*>::iterator qlit = dockWidgets.begin();
        while (qlit != dockWidgets.end())
        {
            LUADockWidget* ldw = qobject_cast<LUADockWidget*>(*qlit);
            if (ldw)
            {
                TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(ldw->assetId());

                if (viewInfoIter != m_dOpenLUAView.end())
                {
                    TrackedLUAView& viewInfo = viewInfoIter->second;
                    newState->m_openAssetIds.push_back(viewInfo.luaViewWidget()->m_Info.m_assetId);
                    AZ_TracePrintf(LUAEditorDebugName, "  -  TAB Saved %s\n", viewInfo.luaViewWidget()->m_Info.m_assetName.c_str());
                }
            }
            qlit++;
        }

        m_gui->m_logPanel->SaveState();

        auto savedState = AZ::UserSettings::CreateFind<AzToolsFramework::MainWindowSavedState>(AZ_CRC_CE("INNER_LUA_WINDOW"), AZ::UserSettings::CT_LOCAL);
        if (savedState)
        {
            // restore state.
            QMainWindow* pMainWindow = static_cast<QMainWindow*>(centralWidget());
            savedState->Init(pMainWindow->saveState(), pMainWindow->saveGeometry());
        }
    }

    void LUAEditorMainWindow::OnLogTabsReset()
    {
        m_gui->m_logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Lua Editor", "Lua Editor", ""));
    }

    void LUAEditorMainWindow::RestoreWindowState() // call this after you have rebuilt everything.
    {
        if (!m_gui->m_logPanel->LoadState())
        {
            OnLogTabsReset();
        }

        // load the state from our state block:
        auto pEditorMainSavedState = AZ::UserSettings::Find<LUAEditorMainWindowSavedState>(AZ_CRC_CE("LUA EDITOR MAIN WINDOW STATE"), AZ::UserSettings::CT_LOCAL);
        if (pEditorMainSavedState)
        {
            QByteArray editorGeomData((const char*)pEditorMainSavedState->m_windowGeometry.data(), (int)pEditorMainSavedState->m_windowGeometry.size());
            QByteArray editorStateData((const char*)pEditorMainSavedState->GetWindowState().data(), (int)pEditorMainSavedState->GetWindowState().size());

            for (const auto& assetId : pEditorMainSavedState->m_openAssetIds)
            {
                Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, assetId, false);
            }

            restoreGeometry(editorGeomData);
            if (this->isMaximized())
            {
                this->showNormal();
                this->showMaximized();
            }
            restoreState(editorStateData);

            m_bAutocompleteEnabled = pEditorMainSavedState->m_bAutocompleteEnabled;
            OnAutocompleteChanged(m_bAutocompleteEnabled);

            auto pWindowSavedState = AZ::UserSettings::Find<AzToolsFramework::MainWindowSavedState>(AZ_CRC_CE("INNER_LUA_WINDOW"), AZ::UserSettings::CT_LOCAL);
            if (pWindowSavedState)
            {
                // restore state.
                QByteArray windowGeomData((const char*)pWindowSavedState->m_windowGeometry.data(), (int)pWindowSavedState->m_windowGeometry.size());
                QByteArray windowStateData((const char*)pWindowSavedState->GetWindowState().data(), (int)pWindowSavedState->GetWindowState().size());
                QMainWindow* pMainWindow = static_cast<QMainWindow*>(centralWidget());
                pMainWindow->restoreState(windowStateData);
            }
        }
        else
        {
            // default state!
        }
    }

    LUAViewWidget*  LUAEditorMainWindow::GetCurrentView()
    {
        if (m_lastFocusedAssetId.empty())
        {
            return nullptr;
        }

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
        AZ_Assert(viewInfoIter != m_dOpenLUAView.end(), "OnFileMenuClose() : Cant find view Info.");
        TrackedLUAView& viewInfo = viewInfoIter->second;
        return viewInfo.luaViewWidget();
    }

    AZStd::vector<LUAViewWidget*> LUAEditorMainWindow::GetAllViews()
    {
        AZStd::vector<LUAViewWidget*> dViews;
        for (TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.begin(); viewInfoIter != m_dOpenLUAView.end(); ++viewInfoIter)
        {
            dViews.push_back(viewInfoIter->second.luaViewWidget());
        }

        return dViews;
    }

    FindResults*  LUAEditorMainWindow::GetFindResultsWidget(int index)
    {
        switch (index)
        {
        case 0:
            return m_gui->m_findResults1;
        case 1:
            return m_gui->m_findResults2;
        case 2:
            return m_gui->m_findResults3;
        case 3:
            return m_gui->m_findResults4;
        }

        return nullptr;
    }

    void LUAEditorMainWindow::SetCurrentFindListWidget(int index)
    {
        AZ_Assert(index >= 0 && index < 4, "Only 4 find windows currently");
        m_gui->findTabWidget->setCurrentIndex(index);
    }

    void LUAEditorMainWindow::OnFindResultClicked(FindResultsBlockInfo result)
    {
        if (OnRequestFocusView(result.m_assetId))
        {
            LUAViewWidget* pLUAViewWidget = GetCurrentView();
            if (!pLUAViewWidget)
            {
                return;
            }

            pLUAViewWidget->SetCursorPosition(result.m_lineNumber, result.m_firstMatchPosition);
        }
        else
        {
            // the document was probably closed, request it be reopened
            m_dProcessFindListClicked.push_back(result);
            AZ_Assert(false, "Fix assets!");
        }
    }

    void LUAEditorMainWindow::OnDataLoadedAndSet(const DocumentInfo& info, LUAViewWidget* pLUAViewWidget)
    {
        for (auto iter = m_dProcessFindListClicked.begin(); iter != m_dProcessFindListClicked.end(); ++iter)
        {
            if (iter->m_assetId == info.m_assetId)
            {
                AZ_Assert(iter->m_assignAssetId, "m_assignAssetId was never set");
                iter->m_assetId = pLUAViewWidget->m_Info.m_assetId;
                iter->m_assignAssetId(info.m_assetName, pLUAViewWidget->m_Info.m_assetId);

                AZ::SystemTickBus::QueueFunction(&LUAEditorMainWindow::OnFindResultClicked, this, *iter);
                m_dProcessFindListClicked.erase(iter);
                return;
            }
        }
    }

    bool LUAEditorMainWindow::OnFileSaveDialog(const AZStd::string& assetName, AZStd::string& newAssetName)
    {
        const QDir rootDir { AZ::Utils::GetProjectPath().c_str() };

        QString name = QFileDialog::getSaveFileName(this, QString(AZStd::string::format("Save File {%s}", assetName.c_str()).c_str()), m_lastOpenFilePath.size() > 0 ? m_lastOpenFilePath.c_str() : rootDir.absolutePath(), QString("*.lua"));
        if (name.isEmpty())
        {
            return false;
        }

        AzFramework::StringFunc::Path::Split(name.toUtf8().data(), nullptr, &m_lastOpenFilePath);
        newAssetName = name.toUtf8().data();

        return true;
    }

    bool LUAEditorMainWindow::OnFileSaveAsDialog(const AZStd::string& assetName, AZStd::string& newAssetName)
    {
        const char* rootDirString;
        AZ::ComponentApplicationBus::BroadcastResult(rootDirString, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);

        QDir rootDir;
        rootDir.setPath(rootDirString);
        rootDir.cdUp();

        QString name = QFileDialog::getSaveFileName(this, QString(AZStd::string::format("Save File As {%s}", assetName.c_str()).c_str()), rootDir.absolutePath(), QString("*.lua"));
        if (name.isEmpty())
        {
            return false;
        }

        //name has the full path in it, we need to convert it to an asset name
        AZStd::string projectRoot, databaseRoot, databasePath, databaseFile, fileExtension;
        if (!AzFramework::StringFunc::AssetDatabasePath::Split(name.toUtf8().data(), &projectRoot, &databaseRoot, &databasePath, &databaseFile, &fileExtension))
        {
            AZ_Warning("LUAEditorMainWindow", false, AZStd::string::format("<span severity=\"err\">Path is invalid: '%s'</span>", name.toUtf8().data()).c_str());
            return false;
        }

        AzFramework::StringFunc::AssetDatabasePath::Join(databasePath.c_str(), databaseFile.c_str(), newAssetName);

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // LUAEditorMainWindow Messages
    void LUAEditorMainWindow::OnFocusInEvent(const AZStd::string& assetId)
    {
        m_lastFocusedAssetId = assetId;

        if (!m_bIgnoreFocusRequests)
        {
            SetGUIToMatch(m_StateTrack);
        }
    }

    void LUAEditorMainWindow::OnFocusOutEvent(const AZStd::string&)
    {
    }

    void LUAEditorMainWindow::OnRequestCheckOut(const AZStd::string& assetId)
    {
        AZStd::string restoreAssetId = m_lastFocusedAssetId;
        m_lastFocusedAssetId = assetId;

        OnSourceControlMenuCheckOut();

        m_lastFocusedAssetId = restoreAssetId;
    }

    //////////////////////////////////////////////////////////////////////////

    bool LUAEditorMainWindow::OnRequestFocusView(const AZStd::string& assetId)
    {
        TrackedLUAViewMap::iterator viewIter = m_dOpenLUAView.find(assetId);
        if (viewIter != m_dOpenLUAView.end())
        {
            viewIter->second.luaDockWidget()->show();
            viewIter->second.luaDockWidget()->raise();
            viewIter->second.luaViewWidget()->RegainFocusFinal();
            return true;
        }
        return false;
    }

    void LUAEditorMainWindow::OnDocumentInfoUpdated(const DocumentInfo& docInfo)
    {
        // document has fresh information available (it was checked out or its data finished or etc)
        TrackedLUAViewMap::iterator viewIter = m_dOpenLUAView.find(docInfo.m_assetId);
        if (viewIter != m_dOpenLUAView.end())
        {
            viewIter->second.luaViewWidget()->OnDocumentInfoUpdated(docInfo);
        }
    }

    // preliminaries for making the debug action buttons context sensitive
    void LUAEditorMainWindow::BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints)
    {
        (void)uniqueBreakpoints;
    }

    void LUAEditorMainWindow::BreakpointHit(const LUAEditor::Breakpoint& breakpoint)
    {
        (void)breakpoint;
        SetDebugControlsToAtBreak();
    }

    void LUAEditorMainWindow::BreakpointResume()
    {
        SetDebugControlsToRunning();
    }

    //////////////////////////////////////////////////////////////////////////
    // externally driven context sensitive widget states
    void LUAEditorMainWindow::SetDebugControlsToInitial()
    {
        m_StateTrack.Init();
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::SetDebugControlsToRunning()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::SetDebugControlsToRunning()\n");

        m_StateTrack.scriptRunning = true;
        m_StateTrack.atBreak = false;
        m_StateTrack.hasExecuted = true;
        SetGUIToMatch(m_StateTrack);

        if (!m_lastFocusedAssetId.empty())
        {
            TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
            AZ_Assert(viewInfoIter != m_dOpenLUAView.end(), "OnFileMenuClose() : Cant find view Info.");
            TrackedLUAView& viewInfo = viewInfoIter->second;
            viewInfo.luaViewWidget()->UpdateCurrentExecutingLine(-1);
        }

        LUAEditor::LUAStackTrackerMessages::Bus::Broadcast(&LUAEditor::LUAStackTrackerMessages::Bus::Events::StackClear);
    }
    void LUAEditorMainWindow::SetDebugControlsToAtBreak()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::SetDebugControlsToAtBreak()\n");

        m_StateTrack.scriptRunning = false;
        m_StateTrack.atBreak = true;
        m_StateTrack.hasExecuted = true;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::SetEditContolsToNoFilesOpen()
    {
        m_StateTrack.atLeastOneFileOpen = false;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::SetEditContolsToAtLeastOneFileOpen()
    {
        m_StateTrack.atLeastOneFileOpen = true;
        SetGUIToMatch(m_StateTrack);
    }

    void LUAEditorMainWindow::luaClassFilterTextChanged(const QString& newPattern)
    {
        m_ClassReferenceFilter->SetFilter(newPattern);

        if (newPattern.isEmpty())
        {
            m_gui->classReferenceTreeView->collapseAll();
        }
        else
        {
            m_gui->classReferenceTreeView->expandAll();
        }
    }

    void LUAEditorMainWindow::OnConnectedToTarget()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnConnectedToTarget()\n");

        m_StateTrack.targetConnected = true;
        m_StateTrack.debuggerAttached = false;
        m_StateTrack.scriptRunning = false;
        m_StateTrack.atBreak = false;
        m_StateTrack.hasExecuted = false;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::OnDisconnectedFromTarget()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnDisconnectedFromTarget()\n");

        m_StateTrack.targetConnected = false;
        m_StateTrack.debuggerAttached = false;
        m_StateTrack.scriptRunning = false;
        m_StateTrack.atBreak = false;
        m_StateTrack.hasExecuted = false;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::OnConnectedToDebugger()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnConnectedToDebugger()\n");

        m_StateTrack.debuggerAttached = true;
        m_StateTrack.scriptRunning = false;
        m_StateTrack.atBreak = false;
        m_StateTrack.hasExecuted = false;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::OnDisconnectedFromDebugger()
    {
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditorMainWindow::OnDisconnectedFromDebugger()\n");

        m_StateTrack.debuggerAttached = false;
        m_StateTrack.scriptRunning = false;
        m_StateTrack.atBreak = false;
        m_StateTrack.hasExecuted = false;
        SetGUIToMatch(m_StateTrack);
    }
    void LUAEditorMainWindow::Repaint()
    {
        SetGUIToMatch(m_StateTrack);

        const AZStd::vector<LUAViewWidget*>& allViews = GetAllViews();
        for (LUAViewWidget* view : allViews)
        {
            view->UpdateFont();
        }
    }
    void LUAEditorMainWindow::OnExecuteScriptResult(bool success)
    {
        if (success)
        {
            m_StateTrack.hasExecuted = true;
            SetDebugControlsToRunning();
        }
    }

    void LUAEditorMainWindow::SetGUIToMatch(StateTrack& track)
    {
        if (track.atLeastOneFileOpen)
        {
            m_gui->actionSave->setEnabled(true);
            m_gui->actionClose->setEnabled(true);
            m_gui->actionSaveAll->setEnabled(true);
            m_gui->actionUndo->setEnabled(true);
            m_gui->actionRedo->setEnabled(true);
            m_gui->actionCut->setEnabled(true);
            m_gui->actionCopy->setEnabled(true);
            m_gui->actionPaste->setEnabled(true);
            m_gui->actionSaveAs->setEnabled(true);
            m_gui->actionCheckOut->setEnabled(true);
            m_gui->actionGoToLine->setEnabled(true);
            m_gui->action_execute->setEnabled(true);
            m_gui->action_togglebreak->setEnabled(true);
        }
        else
        {
            m_gui->actionSave->setEnabled(false);
            m_gui->actionClose->setEnabled(false);
            m_gui->actionSaveAll->setEnabled(false);
            m_gui->actionUndo->setEnabled(false);
            m_gui->actionRedo->setEnabled(false);
            m_gui->actionCut->setEnabled(false);
            m_gui->actionCopy->setEnabled(false);
            m_gui->actionPaste->setEnabled(false);
            m_gui->actionSaveAs->setEnabled(false);
            m_gui->actionCheckOut->setEnabled(false);
            m_gui->actionGoToLine->setEnabled(false);
            m_gui->action_execute->setEnabled(false);
            m_gui->action_togglebreak->setEnabled(false);
        }

        // special handling for the watches
        m_gui->watchDockWidget->setEnabled(false);
        if (track.debuggerAttached)
        {
            m_gui->watchDockWidget->setEnabled(true);
        }

        if ((!track.targetConnected) || (!track.debuggerAttached))
        {
            // turn off any little yellow arrows (QScintilla)
            TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(m_lastFocusedAssetId);
            if (viewInfoIter != m_dOpenLUAView.end())
            {
                TrackedLUAView& viewInfo = viewInfoIter->second;
                if (viewInfo.luaViewWidget())
                {
                    viewInfo.luaViewWidget()->UpdateCurrentExecutingLine(-1);
                }
            }
        }

        if (!track.targetConnected)
        {
            m_pDebugAttachmentButton->setEnabled(false);
            m_gui->action_continue->setEnabled(false);
            m_gui->action_ExecuteOnTarget->setEnabled(false);
            m_gui->action_stepover->setEnabled(false);
            m_gui->action_stepin->setEnabled(false);
            m_gui->action_stepout->setEnabled(false);

            // EARLY OUT
            return;
        }

        // TARGET CONNECTED TRUE IS ASSUMED BEYOND THIS POINT

        m_pDebugAttachmentButton->setEnabled(true);

        if (!track.debuggerAttached)
        {
            m_gui->action_ExecuteOnTarget->setEnabled(false);
            m_gui->action_stepover->setEnabled(false);
            m_gui->action_stepin->setEnabled(false);
            m_gui->action_stepout->setEnabled(false);
            m_gui->action_continue->setEnabled(false);
            // EARLY OUT
            return;
        }

        // DEBUGGER ATTACHED TRUE IS ASSUMED BEYOND THIS POINT

        if (track.scriptRunning)
        {
            if (track.atBreak) // running script and at a break
            {
                m_gui->action_ExecuteOnTarget->setEnabled(false);
                m_gui->action_stepover->setEnabled(true);
                m_gui->action_stepin->setEnabled(true);
                m_gui->action_stepout->setEnabled(true);
                m_gui->action_continue->setEnabled(true);
            }
            else // running script and NOT at a break
            {
                m_gui->action_ExecuteOnTarget->setEnabled(true);
                m_gui->action_stepover->setEnabled(false);
                m_gui->action_stepin->setEnabled(false);
                m_gui->action_stepout->setEnabled(false);
                m_gui->action_continue->setEnabled(false); // this will make it execute remotely...
            }
        }
        else // script NOT running
        {
            if (track.atBreak) // script is NOT running and we're at a break
            {
                m_gui->action_ExecuteOnTarget->setEnabled(false);
                m_gui->action_stepover->setEnabled(track.hasExecuted);
                m_gui->action_stepin->setEnabled(track.hasExecuted);
                m_gui->action_stepout->setEnabled(track.hasExecuted);
                m_gui->action_continue->setEnabled(track.hasExecuted);
            }
            else // script is NOT running and NOT at a break
            {
                if (track.atLeastOneFileOpen)
                {
                    m_gui->action_ExecuteOnTarget->setEnabled(true);
                    m_gui->action_stepover->setEnabled(track.hasExecuted);
                    m_gui->action_stepin->setEnabled(track.hasExecuted);
                    m_gui->action_stepout->setEnabled(track.hasExecuted);
                    m_gui->action_continue->setEnabled(track.hasExecuted);
                }
                else // NO files open
                {
                    m_gui->action_ExecuteOnTarget->setEnabled(false);
                    m_gui->action_stepover->setEnabled(false);
                    m_gui->action_stepin->setEnabled(false);
                    m_gui->action_stepout->setEnabled(false);
                    m_gui->action_continue->setEnabled(false);
                }
            }
        }
    }

    bool LUAEditorMainWindow::HasAtLeastOneFileOpen() const
    {
        return m_StateTrack.atLeastOneFileOpen;
    }

    bool LUAEditorMainWindow::eventFilter(QObject* obj, QEvent* event)
    {
        (void)obj;

        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_C && (keyEvent->modifiers() & Qt::ControlModifier))
            {
                OnEditMenuCopy();
                return true;
            }
            else if (keyEvent->key() == Qt::Key_X && (keyEvent->modifiers() & Qt::ControlModifier))
            {
                OnEditMenuCut();
                return true;
            }
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Control)
            {
                m_StoredTabAssetId = "";
            }
        }

        //return QObject::eventFilter(obj, event);
        return false;
    }

    void LUAEditorMainWindow::OnTabForwards()
    {
        TrackedLUACtrlTabOrder::iterator tabIter = m_CtrlTabOrder.begin();
        while (tabIter != m_CtrlTabOrder.end())
        {            
            if (*tabIter == m_lastFocusedAssetId)
            {
                break;
            }
            tabIter++;
        }

        if (tabIter == m_CtrlTabOrder.begin())
        {
            tabIter = m_CtrlTabOrder.end();
            --tabIter;
        }
        else
        {
            --tabIter;
        }

        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(*tabIter);
        if (viewInfoIter != m_dOpenLUAView.end())
        {
            viewInfoIter->second.luaDockWidget()->show();
            viewInfoIter->second.luaDockWidget()->raise();
            viewInfoIter->second.luaViewWidget()->setFocus();
            m_lastFocusedAssetId = *tabIter;
        }
    }

    void LUAEditorMainWindow::OnTabBackwards()
    {        
        TrackedLUACtrlTabOrder::iterator tabIter = m_CtrlTabOrder.begin();
        while (tabIter != m_CtrlTabOrder.end())
        {
            if (*tabIter == m_lastFocusedAssetId)
            {
                break;
            }
            tabIter++;
        }

        if (tabIter == m_CtrlTabOrder.end())
        {
            return;
        }

        tabIter++;
        if (tabIter == m_CtrlTabOrder.end())
        {
            tabIter = m_CtrlTabOrder.begin();
            
        }
      
        TrackedLUAViewMap::iterator viewInfoIter = m_dOpenLUAView.find(*tabIter);
        if (viewInfoIter != m_dOpenLUAView.end())
        {
            viewInfoIter->second.luaDockWidget()->show();
            viewInfoIter->second.luaDockWidget()->raise();
            viewInfoIter->second.luaViewWidget()->setFocus();
            m_lastFocusedAssetId = *tabIter;
        }
    }

    void LUAEditorMainWindow::dragEnterEvent(QDragEnterEvent* pEvent)
    {
        if (!pEvent->mimeData()->hasUrls())
        {
            return;
        }

        pEvent->acceptProposedAction();
    }

    void LUAEditorMainWindow::dropEvent(QDropEvent* pEvent)
    {
        if (!pEvent->mimeData()->hasUrls())
        {
            return;
        }

        pEvent->setDropAction(Qt::CopyAction);

        pEvent->accept();

        QList<QUrl> urls = pEvent->mimeData()->urls();
        for (int idx = 0; idx < urls.count(); ++idx)
        {
            QString path = urls[idx].toLocalFile();
            AZ_TracePrintf("Debug", "URL: %s\n", path.toUtf8().data());

            AZStd::string assetId(path.toUtf8().data());
            Context_DocumentManagement::Bus::Broadcast(&Context_DocumentManagement::Bus::Events::OnLoadDocument, assetId, true);
        }
    }

    QTabWidget* LUAEditorMainWindow::GetFindTabWidget()
    {
        return m_gui->findTabWidget;
    }

    void LUAEditorMainWindow::AddMessageToLog(AzToolsFramework::Logging::LogLine::LogType type, const char* window, const char* message, void* userData)
    {
        m_gui->m_logPanel->InsertLogLine(type, window, message, userData);
    }

    void LUAEditorMainWindow::LogLineSelectionChanged(const AzToolsFramework::Logging::LogLine& logLine)
    {
        CompilationErrorData* errorData = static_cast<CompilationErrorData*>(logLine.GetUserData());
        if (errorData)
        {
            // Use the data, if it exists from the logLine to make sure the right tab/line is highlighted in the editor
            if (OnRequestFocusView(errorData->m_filename))
            {
                LUAViewWidget* pLUAViewWidget = GetCurrentView();
                if (pLUAViewWidget)
                {
                    pLUAViewWidget->SetCursorPosition(errorData->m_lineNumber, 0);
                }
            }
        }
    }

    void LUAEditorMainWindowSavedState::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<LUAEditorMainWindowSavedState, AzToolsFramework::MainWindowSavedState   >()
                ->Version(5)
                ->Field("m_openAssetIds", &LUAEditorMainWindowSavedState::m_openAssetIds)
                ->Field("m_bAutocompleteEnabled", &LUAEditorMainWindowSavedState::m_bAutocompleteEnabled)
                ->Field("m_bAutoReloadUnmodifiedFiles", &LUAEditorMainWindowSavedState::m_bAutoReloadUnmodifiedFiles);
        }

        LUAEditorFindDialog::Reflect(reflection);
    }


    void LUAEditorMainWindowLayout::addItem(QLayoutItem* pChild)
    {
        children.push_back(pChild);
    }

    QLayoutItem* LUAEditorMainWindowLayout::itemAt(int index) const
    {
        if (index >= (int)children.size())
        {
            return nullptr;
        }

        return children[index];
    }

    QLayoutItem* LUAEditorMainWindowLayout::takeAt(int index)
    {
        QLayoutItem* pItem = nullptr;

        if (index >= (int)children.size())
        {
            return nullptr;
        }

        pItem = children[index];
        children.erase(children.begin() + index);
        return pItem;
    }

    LUAEditorMainWindowLayout::LUAEditorMainWindowLayout(QWidget* pParent)
    {
        (void)pParent;
    }

    LUAEditorMainWindowLayout::~LUAEditorMainWindowLayout()
    {
        QLayoutItem* item;
        item = takeAt(0);
        while (item)
        {
            delete item;
            item = takeAt(0);
        }
    }

    int LUAEditorMainWindowLayout::count() const
    {
        return (int)children.size();
    }

    void LUAEditorMainWindowLayout::setGeometry (const QRect& r)
    {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effectiveRect = r.adjusted(+left, +top, -right, -bottom);
        for (int pos = 0; pos < (int)children.size() - 1; ++pos)
        {
            QLayoutItem* pItem = children[pos];
            pItem->setGeometry(effectiveRect);
        }

        if (children.size())
        {
            // if we have any elements, the last element is top right aligned:
            QLayoutItem* pItem = children[children.size() - 1];
            QSize lastItemSize = pItem->minimumSize();

            const int magicalRightEdgeOffset = pItem->widget()->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            QRect topRightCorner(effectiveRect.topRight() - QPoint(lastItemSize.width() + magicalRightEdgeOffset, 0) + QPoint(-2, 2), lastItemSize);
            if (pItem->geometry() != topRightCorner)
            {
                pItem->setGeometry(topRightCorner);
            }
        }
    }

    Qt::Orientations LUAEditorMainWindowLayout::expandingDirections() const
    {
        return Qt::Orientations();
    }

    QSize LUAEditorMainWindowLayout::sizeHint() const
    {
        return minimumSize();
    }

    QSize LUAEditorMainWindowLayout::minimumSize() const
    {
        QSize size;

        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);

        for (int pos = 0; pos < (int)children.size(); ++pos)
        {
            QLayoutItem* item = children[pos];
            size = size.expandedTo(item->minimumSize());
        }

        size += (QSize(left + right, top + bottom));

        return size;
    }
}//namespace LUAEditor

#include <Source/LUA/moc_LUAEditorMainWindow.cpp>
