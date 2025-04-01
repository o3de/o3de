/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/sort.h>
#include <EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/KeyboardShortcutsWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/LoadActorSettingsWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindowEventFilter.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PreferencesWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/ResetSettingsDialog.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/EMStudioSDK/Source/Workspace.h>
#include <Editor/SaveDirtyFilesCallbacks.h>
#include <MCore/Source/LogManager.h>

#include <Editor/ActorEditorBus.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MiscCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/FancyDocking.h>

// include Qt related
#include <QAbstractEventDispatcher>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStatusBar>
#include <QTextEdit>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <qnamespace.h>
AZ_PUSH_DISABLE_WARNING(4267, "-Wconversion")
#include <ISystem.h>
AZ_POP_DISABLE_WARNING
#include <LyViewPaneNames.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <IEditor.h>

namespace EMStudio
{
    class UndoMenuCallback
        : public MCore::CommandManagerCallback
    {
    public:
        UndoMenuCallback(MainWindow* mainWindow)
            : m_mainWindow(mainWindow)
        {}
        ~UndoMenuCallback() = default;

        void OnRemoveCommand([[maybe_unused]] size_t historyIndex) override          { m_mainWindow->UpdateUndoRedo(); }
        void OnSetCurrentCommand([[maybe_unused]] size_t index) override             { m_mainWindow->UpdateUndoRedo(); }
        void OnAddCommandToHistory([[maybe_unused]] size_t historyIndex, [[maybe_unused]] MCore::CommandGroup* group, [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine) override { m_mainWindow->UpdateUndoRedo(); }

        void OnPreExecuteCommand([[maybe_unused]] MCore::CommandGroup* group, [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine) override {}
        void OnPostExecuteCommand([[maybe_unused]] MCore::CommandGroup* group, [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine, [[maybe_unused]] bool wasSuccess, [[maybe_unused]] const AZStd::string& outResult) override {}
        void OnPreExecuteCommandGroup([[maybe_unused]] MCore::CommandGroup* group, [[maybe_unused]] bool undo) override {}
        void OnPostExecuteCommandGroup([[maybe_unused]] MCore::CommandGroup* group, [[maybe_unused]] bool wasSuccess) override {}
        void OnShowErrorReport([[maybe_unused]] const AZStd::vector<AZStd::string>& errors) override {}

    private:
        MainWindow* m_mainWindow;
    };

    ErrorWindow::ErrorWindow(QWidget* parent)
        : QDialog(parent)
    {
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);

        m_textEdit = new QTextEdit();
        m_textEdit->setTextInteractionFlags(Qt::NoTextInteraction | Qt::TextSelectableByMouse);
        mainLayout->addWidget(m_textEdit);

        setMinimumWidth(600);
        setMinimumHeight(400);
        setLayout(mainLayout);

        setStyleSheet("background-color: rgb(30,30,30);");
    }

    void ErrorWindow::Init(const AZStd::vector<AZStd::string>& errors)
    {
        const size_t numErrors = errors.size();

        // Update title of the dialog.
        AZStd::string windowTitle;
        if (numErrors > 1)
        {
            windowTitle = AZStd::string::format("%zu errors occurred", numErrors);
        }
        else
        {
            windowTitle = AZStd::string::format("%zu error occurred", numErrors);
        }

        setWindowTitle(windowTitle.c_str());

        // Iterate over the errors and construct the rich text string.
        AZStd::string text;
        for (size_t i = 0; i < numErrors; ++i)
        {
            text += AZStd::string::format("<font color=\"#F49C1C\"><b>#%zu</b>:</font> ", i + 1);
            text += "<font color=\"#CBCBCB\">";
            text += errors[i];
            text += "</font>";
            text += "<br><br>";
        }

        m_textEdit->setText(text.c_str());
    }

    constexpr AZStd::string_view AnimationEditorActionContextIdentifier = "o3de.context.animationEditor";

    MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
        : AzQtComponents::DockMainWindow(parent, flags)
        , m_prevSelectedActor(nullptr)
        , m_prevSelectedActorInstance(nullptr)
        , m_undoMenuCallback(nullptr)
        , m_fancyDockingManager(new AzQtComponents::FancyDocking(this, "emotionstudiosdk"))
    {
        m_loadingOptions                 = false;
        m_autosaveTimer                  = nullptr;
        m_preferencesWindow              = nullptr;
        m_applicationMode                = nullptr;
        m_dirtyFileManager               = nullptr;
        m_fileManager                    = nullptr;
        m_shortcutManager                = nullptr;
        m_nativeEventFilter              = nullptr;
        m_importActorCallback            = nullptr;
        m_removeActorCallback            = nullptr;
        m_removeActorInstanceCallback    = nullptr;
        m_importMotionCallback           = nullptr;
        m_removeMotionCallback           = nullptr;
        m_createMotionSetCallback        = nullptr;
        m_removeMotionSetCallback        = nullptr;
        m_loadMotionSetCallback          = nullptr;
        m_createAnimGraphCallback        = nullptr;
        m_removeAnimGraphCallback        = nullptr;
        m_loadAnimGraphCallback          = nullptr;
        m_selectCallback                 = nullptr;
        m_unselectCallback               = nullptr;
        m_clearSelectionCallback        = nullptr;
        m_saveWorkspaceCallback          = nullptr;

        // Register this window as the widget for the Animation Editor Action Context.
        AzToolsFramework::AssignWidgetToActionContextHelper(AnimationEditorActionContextIdentifier, this);
    }

    MainWindow::~MainWindow()
    {
        // Unregister this window as the widget for the Animation Editor Action Context.
        AzToolsFramework::RemoveWidgetFromActionContextHelper(AnimationEditorActionContextIdentifier, this);

        DisableUpdatingPlugins();

        if (m_nativeEventFilter)
        {
            QAbstractEventDispatcher::instance()->removeNativeEventFilter(m_nativeEventFilter);
            delete m_nativeEventFilter;
            m_nativeEventFilter = nullptr;
        }

        if (m_autosaveTimer)
        {
            m_autosaveTimer->stop();
        }

        PluginOptionsNotificationsBus::Router::BusRouterDisconnect();
        SavePreferences();

        // Unload everything from the Editor, so that reopening the editor
        // results in an empty scene
        Reset();
        CommandSystem::ClearMotionSetsCommand(); // Remove the default motion set.

        delete m_shortcutManager;
        delete m_fileManager;
        delete m_dirtyFileManager;

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_importActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_importMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_createMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_loadMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_createAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_loadAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_saveWorkspaceCallback, false);
        GetCommandManager()->RemoveCallback(&m_mainWindowCommandManagerCallback, false);
        delete m_importActorCallback;
        delete m_removeActorCallback;
        delete m_removeActorInstanceCallback;
        delete m_importMotionCallback;
        delete m_removeMotionCallback;
        delete m_createMotionSetCallback;
        delete m_removeMotionSetCallback;
        delete m_loadMotionSetCallback;
        delete m_createAnimGraphCallback;
        delete m_removeAnimGraphCallback;
        delete m_loadAnimGraphCallback;
        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
        delete m_saveWorkspaceCallback;

        EMotionFX::ActorEditorRequestBus::Handler::BusDisconnect();

        if (m_undoMenuCallback)
        {
            EMStudio::GetCommandManager()->RemoveCallback(m_undoMenuCallback);
        }
        EMotionFX::ActorEditorRequestBus::Handler::BusDisconnect();
    }

    void MainWindow::Reflect(AZ::ReflectContext* context)
    {
        GUIOptions::Reflect(context);
    }

    // init the main window
    void MainWindow::Init()
    {
        // tell the mystic Qt library about the main window
        MysticQt::GetMysticQt()->SetMainWindow(this);

        // enable drag&drop support
        setAcceptDrops(true);

        setDockNestingEnabled(true);

        setFocusPolicy(Qt::StrongFocus);

        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        selectionList.Clear();

        // create the menu bar
        QWidget* menuWidget = new QWidget();
        menuWidget->setObjectName("EMFX_Menu");

        // Give our custom menu widget the same size policy and minimum height as the default menu bar,
        // otherwise it will get shrunk
        menuWidget->setMinimumHeight(menuBar()->height());
        menuWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        QHBoxLayout* menuLayout = new QHBoxLayout(menuWidget);
        menuLayout->setMargin(0);
        menuLayout->setSpacing(0);

        QMenuBar* menuBar = new QMenuBar(menuWidget);
        menuLayout->addWidget(menuBar);

        m_applicationMode = new QComboBox();
        menuLayout->addWidget(m_applicationMode);

        setMenuWidget(menuWidget);

        // file actions
        QMenu* menu = menuBar->addMenu(tr("&File"));
        menu->setObjectName("EMFX.MainWindow.FileMenu");

        // reset action
        m_resetAction = menu->addAction(tr("&Reset"), this, &MainWindow::OnReset, QKeySequence::New);
        m_resetAction->setObjectName("EMFX.MainWindow.ResetAction");

        // save all
        m_saveAllAction = menu->addAction(tr("Save All..."), this, &MainWindow::OnSaveAll, QKeySequence::Save);
        m_saveAllAction->setObjectName("EMFX.MainWindow.SaveAllAction");

        // disable the reset and save all menus until one thing is loaded
        m_resetAction->setDisabled(true);
        m_saveAllAction->setDisabled(true);

        menu->addSeparator();

        // actor file actions
        QAction* openAction = menu->addAction(tr("&Open Actor"), this, &MainWindow::OnFileOpenActor, QKeySequence::Open);
        openAction->setObjectName("EMFX.MainWindow.OpenActorAction");
        m_mergeActorAction = menu->addAction(tr("&Merge Actor"), this, &MainWindow::OnFileMergeActor, Qt::CTRL + Qt::Key_I);
        m_mergeActorAction->setObjectName("EMFX.MainWindow.MergeActorAction");
        m_saveSelectedActorsAction = menu->addAction(tr("&Save Selected Actors"), this, &MainWindow::OnFileSaveSelectedActors);
        m_saveSelectedActorsAction->setObjectName("EMFX.MainWindow.SaveActorAction");

        // disable the merge actor menu until one actor is in the scene
        DisableMergeActorMenu();

        // disable the save selected actor menu until one actor is selected
        DisableSaveSelectedActorsMenu();

        // recent actors submenu
        m_recentActors.Init(menu, m_options.GetMaxRecentFiles(), "Recent Actors", "recentActorFiles");
        connect(&m_recentActors, &MysticQt::RecentFiles::OnRecentFile, this, &MainWindow::OnRecentFile);

        // workspace file actions
        menu->addSeparator();
        QAction* newWorkspaceAction = menu->addAction(tr("New Workspace"), this, &MainWindow::OnFileNewWorkspace);
        newWorkspaceAction->setObjectName("EMFX.MainWindow.NewWorkspaceAction");
        QAction* openWorkspaceAction = menu->addAction(tr("Open Workspace"), this, &MainWindow::OnFileOpenWorkspace);
        openWorkspaceAction->setObjectName("EMFX.MainWindow.OpenWorkspaceAction");
        QAction* saveWorkspaceAction = menu->addAction(tr("Save Workspace"), this, &MainWindow::OnFileSaveWorkspace);
        saveWorkspaceAction->setObjectName("EMFX.MainWindow.SaveWorkspaceAction");
        QAction* saveWorkspaceAsAction = menu->addAction(tr("Save Workspace As"), this, &MainWindow::OnFileSaveWorkspaceAs);
        saveWorkspaceAsAction->setObjectName("EMFX.MainWindow.SaveWorkspaceAsAction");

        // recent workspace submenu
        m_recentWorkspaces.Init(menu, m_options.GetMaxRecentFiles(), "Recent Workspaces", "recentWorkspaces");
        connect(&m_recentWorkspaces, &MysticQt::RecentFiles::OnRecentFile, this, &MainWindow::OnRecentFile);

        // edit menu
        menu = menuBar->addMenu(tr("&Edit"));
        menu->setObjectName("EMFX.MainWindow.EditMenu");
        m_undoAction = menu->addAction(
            tr("Undo"),
            this,
            &MainWindow::OnUndo,
            QKeySequence::Undo
        );
        m_undoAction->setObjectName("EMFX.MainWindow.UndoAction");
        m_redoAction = menu->addAction(
            tr("Redo"),
            this,
            &MainWindow::OnRedo,
            QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z)
        );
        m_redoAction->setObjectName("EMFX.MainWindow.RedoAction");
        m_undoAction->setDisabled(true);
        m_redoAction->setDisabled(true);
        menu->addSeparator();
        QAction* preferencesAction = menu->addAction(tr("&Preferences"), this, &MainWindow::OnPreferences);
        preferencesAction->setObjectName("EMFX.MainWindow.PrefsAction");

        // layouts item
        m_layoutsMenu = menuBar->addMenu(tr("&Layouts"));
        m_layoutsMenu->setObjectName("LayoutsMenu");
        UpdateLayoutsMenu();

        // reset the application mode selection and connect it
        m_applicationMode->setCurrentIndex(-1);
        connect(m_applicationMode, qOverload<int>(&QComboBox::currentIndexChanged), this, qOverload<int>(&MainWindow::ApplicationModeChanged));
        m_layoutLoaded = false;

        // view item
        menu = menuBar->addMenu(tr("&View"));
        m_createWindowMenu = menu;
        m_createWindowMenu->setObjectName("ViewMenu");

        // help menu
        menu = menuBar->addMenu(tr("&Help"));
        menu->setObjectName("EMFX.MainWindow.HelpMenu");

        menu->addAction("Documentation", this, []
        {
            QDesktopServices::openUrl(QUrl("https://o3de.org/docs/user-guide/visualization/animation/"));
        });

        menu->addAction("Forums", this, []
        {
            QDesktopServices::openUrl(QUrl("https://o3de.org/community/"));
        });

        menu->addSeparator();

        QMenu* folders = menu->addMenu("Folders");
        folders->setObjectName("EMFX.MainWindow.FoldersMenu");
        folders->addAction("Open autosave folder", this, &MainWindow::OnOpenAutosaveFolder);
        folders->addAction("Open settings folder", this, &MainWindow::OnOpenSettingsFolder);

        // Reset old workspace and start clean.
        GetManager()->GetWorkspace()->Reset();
        SetWindowTitleFromFileName("<not saved yet>");

        // create the autosave timer
        m_autosaveTimer = new QTimer(this);
        connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::OnAutosaveTimeOut);

        // load preferences
        PluginOptionsNotificationsBus::Router::BusRouterConnect();
        LoadPreferences();
        m_autosaveTimer->setInterval(m_options.GetAutoSaveInterval() * 60 * 1000);

        // Create the dirty file manager and register the workspace callback.
        m_dirtyFileManager = new DirtyFileManager;
        m_dirtyFileManager->AddCallback(new SaveDirtyActorFilesCallback());
        m_dirtyFileManager->AddCallback(new SaveDirtyMotionFilesCallback());
        m_dirtyFileManager->AddCallback(new SaveDirtyMotionSetFilesCallback());
        m_dirtyFileManager->AddCallback(new SaveDirtyAnimGraphFilesCallback());
        m_dirtyFileManager->AddCallback(new SaveDirtyWorkspaceCallback);

        // init the file manager
        m_fileManager = new EMStudio::FileManager(this);

        ////////////////////////////////////////////////////////////////////////
        // Keyboard Shortcut Manager
        ////////////////////////////////////////////////////////////////////////

        // create the shortcut manager
        m_shortcutManager = new MysticQt::KeyboardShortcutManager();

        // load the old shortcuts
        LoadKeyboardShortcuts();

        // add the application mode group
        constexpr AZStd::string_view layoutGroupName = "Layouts";
        QAction* animGraphLayoutAction = new QAction(
            "AnimGraph",
            this);
        animGraphLayoutAction->setShortcut(Qt::Key_1 | Qt::AltModifier);
        m_shortcutManager->RegisterKeyboardShortcut(animGraphLayoutAction, layoutGroupName, false);
        connect(animGraphLayoutAction, &QAction::triggered, [this]{ m_applicationMode->setCurrentIndex(0); });
        addAction(animGraphLayoutAction);

        QAction* animationLayoutAction = new QAction(
            "Animation",
            this);
        animationLayoutAction->setShortcut(Qt::Key_2 | Qt::AltModifier);
        m_shortcutManager->RegisterKeyboardShortcut(animationLayoutAction, layoutGroupName, false);
        connect(animationLayoutAction, &QAction::triggered, [this]{ m_applicationMode->setCurrentIndex(1); });
        addAction(animationLayoutAction);

        QAction* characterLayoutAction = new QAction(
            "Character",
            this);
        characterLayoutAction->setShortcut(Qt::Key_3 | Qt::AltModifier);
        m_shortcutManager->RegisterKeyboardShortcut(characterLayoutAction, layoutGroupName, false);
        connect(characterLayoutAction, &QAction::triggered, [this]{ m_applicationMode->setCurrentIndex(2); });
        addAction(characterLayoutAction);

        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();

        m_undoMenuCallback = new UndoMenuCallback(this);
        EMStudio::GetCommandManager()->RegisterCallback(m_undoMenuCallback);
        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();

        // create and register the command callbacks
        m_importActorCallback = new CommandImportActorCallback(false);
        m_removeActorCallback = new CommandRemoveActorCallback(false);
        m_removeActorInstanceCallback = new CommandRemoveActorInstanceCallback(false);
        m_importMotionCallback = new CommandImportMotionCallback(false);
        m_removeMotionCallback = new CommandRemoveMotionCallback(false);
        m_createMotionSetCallback = new CommandCreateMotionSetCallback(false);
        m_removeMotionSetCallback = new CommandRemoveMotionSetCallback(false);
        m_loadMotionSetCallback = new CommandLoadMotionSetCallback(false);
        m_createAnimGraphCallback = new CommandCreateAnimGraphCallback(false);
        m_removeAnimGraphCallback = new CommandRemoveAnimGraphCallback(false);
        m_loadAnimGraphCallback = new CommandLoadAnimGraphCallback(false);
        m_selectCallback = new CommandSelectCallback(false);
        m_unselectCallback = new CommandUnselectCallback(false);
        m_clearSelectionCallback = new CommandClearSelectionCallback(false);
        m_saveWorkspaceCallback = new CommandSaveWorkspaceCallback(false);
        GetCommandManager()->RegisterCommandCallback("ImportActor", m_importActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActor", m_removeActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_removeActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("ImportMotion", m_importMotionCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotion", m_removeMotionCallback);
        GetCommandManager()->RegisterCommandCallback("CreateMotionSet", m_createMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", m_removeMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("LoadMotionSet", m_loadMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("CreateAnimGraph", m_createAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", m_removeAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("LoadAnimGraph", m_loadAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("SaveWorkspace", m_saveWorkspaceCallback);

        GetCommandManager()->RegisterCallback(&m_mainWindowCommandManagerCallback);

        AZ_Assert(!m_nativeEventFilter, "Double initialization?");
        m_nativeEventFilter = new NativeEventFilter(this);
        QAbstractEventDispatcher::instance()->installNativeEventFilter(m_nativeEventFilter);

        EnableUpdatingPlugins();
    }

    MainWindow::MainWindowCommandManagerCallback::MainWindowCommandManagerCallback()
    {
        m_skipClearRecorderCommands =
        {
            CommandSystem::CommandRecorderClear::s_RecorderClearCmdName,
            CommandSystem::CommandStopAllMotionInstances::s_stopAllMotionInstancesCmdName,
            CommandSystem::CommandSelect::s_SelectCmdName,
            CommandSystem::CommandUnselect::s_unselectCmdName,
            CommandSystem::CommandClearSelection::s_clearSelectionCmdName,
            CommandSystem::CommandToggleLockSelection::s_toggleLockSelectionCmdName,
        };
    }

    bool MainWindow::MainWindowCommandManagerCallback::NeedToClearRecorder(MCore::Command* command, const MCore::CommandLine& commandLine) const
    {
        if (AZStd::find(m_skipClearRecorderCommands.begin(), m_skipClearRecorderCommands.end(), command->GetNameString()) != m_skipClearRecorderCommands.end())
        {
            return false;
        }

        if (command->GetNameString() == "AnimGraphAdjustNode")
        {
            if (!commandLine.CheckIfHasParameter("newName") &&
                !commandLine.CheckIfHasParameter("enabled") &&
                !commandLine.CheckIfHasParameter("attributesString"))
            {
                return false;
            }
        }

        return true;
    }

    void MainWindow::MainWindowCommandManagerCallback::OnPreExecuteCommand([[maybe_unused]] MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (NeedToClearRecorder(command, commandLine))
        {
            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommandInsideCommand(CommandSystem::CommandRecorderClear::s_RecorderClearCmdName, commandResult))
            {
                AZ_Warning("Editor", false, "Clear recorder command failed: %s", commandResult.c_str());
            }
        }
    }

    void MainWindow::MainWindowCommandManagerCallback::OnPreUndoCommand(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        OnPreExecuteCommand(nullptr, command, commandLine);
    }

    // Called when the errors shall be shown.
    void MainWindow::MainWindowCommandManagerCallback::OnShowErrorReport(const AZStd::vector<AZStd::string>& errors)
    {
        if (!m_errorWindow)
        {
            m_errorWindow = AZStd::make_unique<ErrorWindow>(GetMainWindow());
        }

        EMStudio::GetApp()->restoreOverrideCursor();
        m_errorWindow->Init(errors);
        m_errorWindow->open();
    }

    bool MainWindow::CommandImportActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();
        mainWindow->UpdateResetAndSaveAllMenus();

        return true;
    }


    bool MainWindow::CommandImportActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();
        mainWindow->UpdateResetAndSaveAllMenus();

        return true;
    }


    bool MainWindow::CommandRemoveActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();
        mainWindow->UpdateResetAndSaveAllMenus();

        return true;
    }


    bool MainWindow::CommandRemoveActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();
        mainWindow->UpdateResetAndSaveAllMenus();

        return true;
    }


    bool MainWindow::CommandRemoveActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandRemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandImportMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandImportMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandCreateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandLoadAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetManager()->GetMainWindow()->UpdateResetAndSaveAllMenus();
        return true;
    }


    bool MainWindow::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudio::MainWindow* mainWindow = GetManager()->GetMainWindow();
        mainWindow->UpdateSaveActorsMenu();
        mainWindow->BroadcastSelectionNotifications();

        return true;
    }


    bool MainWindow::CommandSaveWorkspaceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string filename;
        commandLine.GetValue("filename", command, &filename);
        GetManager()->GetMainWindow()->OnWorkspaceSaved(filename.c_str());
        return true;
    }


    bool MainWindow::CommandSaveWorkspaceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    void MainWindow::OnWorkspaceSaved(const char* filename)
    {
        m_recentWorkspaces.AddRecentFile(filename);
        SetWindowTitleFromFileName(filename);
    }


    void MainWindow::UpdateResetAndSaveAllMenus()
    {
        // enable the menus if at least one actor
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            m_resetAction->setEnabled(true);
            m_saveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one motion
        if (EMotionFX::GetMotionManager().GetNumMotions() > 0)
        {
            m_resetAction->setEnabled(true);
            m_saveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one motion set
        bool emptyDefaultMotionSet = false;
        if (EMotionFX::GetMotionManager().GetNumMotionSets() == 1)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
            if (motionSet->GetNumChildSets() == 0 &&
                motionSet->GetNumMotionEntries() == 0 &&
                motionSet->GetNameString() == CommandSystem::s_defaultMotionSetName)
            {
                emptyDefaultMotionSet = true;
            }
        }

        if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0 && !emptyDefaultMotionSet)
        {
            m_resetAction->setEnabled(true);
            m_saveAllAction->setEnabled(true);
            return;
        }

        // enable the menus if at least one anim graph
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() > 0)
        {
            m_resetAction->setEnabled(true);
            m_saveAllAction->setEnabled(true);
            return;
        }

        // nothing loaded, disable the menus
        m_resetAction->setDisabled(true);
        m_saveAllAction->setDisabled(true);
    }


    void MainWindow::EnableMergeActorMenu()
    {
        m_mergeActorAction->setEnabled(true);
    }


    void MainWindow::DisableMergeActorMenu()
    {
        m_mergeActorAction->setDisabled(true);
    }


    void MainWindow::UpdateSaveActorsMenu()
    {
        // enable the merge menu only if one actor is in the scene
        if (EMotionFX::GetActorManager().GetNumActors() > 0)
        {
            EnableMergeActorMenu();
        }
        else
        {
            DisableMergeActorMenu();
        }

        // enable the actor save selected menu only if one actor or actor instance is selected
        // it's needed to check here because if one actor is removed it's not selected anymore
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numSelectedActors = selectionList.GetNumSelectedActors();
        const size_t numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        if ((numSelectedActors > 0) || (numSelectedActorInstances > 0))
        {
            EnableSaveSelectedActorsMenu();
        }
        else
        {
            DisableSaveSelectedActorsMenu();
        }
    }


    void MainWindow::EnableSaveSelectedActorsMenu()
    {
        m_saveSelectedActorsAction->setEnabled(true);
    }


    void MainWindow::DisableSaveSelectedActorsMenu()
    {
        m_saveSelectedActorsAction->setDisabled(true);
    }


    void MainWindow::SetWindowTitleFromFileName(const AZStd::string& fileName)
    {
        // get only the version number of EMotion FX
        AZStd::string emfxVersionString = EMotionFX::GetEMotionFX().GetVersionString();
        AzFramework::StringFunc::Replace(emfxVersionString, "EMotion FX ", "", true /* case sensitive */);

        // set the window title
        // only set the EMotion FX version if the filename is empty
        AZStd::string windowTitle;
        windowTitle = AZStd::string::format("EMotion Studio %s (BUILD %s)", emfxVersionString.c_str(), MCORE_DATE);
        if (fileName.empty() == false)
        {
            windowTitle += AZStd::string::format(" - %s", fileName.c_str());
        }
        setWindowTitle(windowTitle.c_str());
    }


    // update the items inside the Window->Create menu item
    void MainWindow::UpdateCreateWindowMenu()
    {
        PluginManager* pluginManager = GetPluginManager();

        const size_t numRegisteredPlugins = pluginManager->GetNumRegisteredPlugins();
        const PluginManager::PluginVector& registeredPlugins = pluginManager->GetRegisteredPlugins();

        AZStd::vector<AZStd::string> sortedPluginNames;
        sortedPluginNames.reserve(numRegisteredPlugins);
        for (const EMStudioPlugin* plugin : registeredPlugins)
        {
            sortedPluginNames.push_back(plugin->GetName());
        }
        AZStd::sort(sortedPluginNames.begin(), sortedPluginNames.end());

        // clear the window menu
        m_createWindowMenu->clear();

        // for all registered plugins, create a menu items
        for (const AZStd::string& pluginTypeString : sortedPluginNames)
        {
            const size_t pluginIndex = pluginManager->FindRegisteredPluginIndex(pluginTypeString.c_str());
            EMStudioPlugin* plugin = registeredPlugins[pluginIndex];

            // check if multiple instances allowed
            // on this case the plugin is not one action but one submenu
            if (plugin->AllowMultipleInstances())
            {
                // create the menu
                m_createWindowMenu->addMenu(plugin->GetName());

                // TODO: add each instance inside the submenu
            }
            else
            {
                // create the action
                QAction* action = m_createWindowMenu->addAction(plugin->GetName());
                action->setData(plugin->GetName());

                // connect the action to activate the plugin when clicked on it
                connect(action, &QAction::triggered, this, &MainWindow::OnWindowCreate);

                // set the action checkable
                action->setCheckable(true);

                // set the checked state of the action
                EMStudioPlugin* activePlugin = pluginManager->FindActivePlugin(plugin->GetClassID());
                action->setChecked(activePlugin != nullptr);

                // Create any children windows this plugin might want to create
                if (activePlugin)
                {
                    // must use the active plugin, as it needs to be initialized to create window entries
                    activePlugin->AddWindowMenuEntries(m_createWindowMenu);
                }
            }
        }
    }


    // create a new given window
    void MainWindow::OnWindowCreate(bool checked)
    {
        // get the plugin name
        QAction* action = (QAction*)sender();
        const QString pluginName = action->data().toString();

        // checked is the new state
        // activate the plugin if the menu is not checked
        // show and focus on the actual window if the menu is already checked
        if (checked)
        {
            // try to create the new window
            EMStudioPlugin* newPlugin = EMStudio::GetPluginManager()->CreateWindowOfType(FromQtString(pluginName).c_str());
            if (newPlugin == nullptr)
            {
                MCore::LogError("Failed to create window using plugin '%s'", FromQtString(pluginName).c_str());
                return;
            }

            // if we have a dock widget plugin here, making floatable and change its window size
            if (newPlugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_WINDOW)
            {
                DockWidgetPlugin* dockPlugin = static_cast<DockWidgetPlugin*>(newPlugin);
                QRect dockRect;
                dockRect.setSize(dockPlugin->GetInitialWindowSize());
                dockRect.moveCenter(geometry().center());
                m_fancyDockingManager->makeDockWidgetFloating(dockPlugin->GetDockWidget(), dockRect);
            }
        }
        else // (checked == false)
        {
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePluginByTypeString(FromQtString(pluginName).c_str());
            AZ_Assert(plugin, "Failed to get plugin, since it was checked it should be active");
            EMStudio::GetPluginManager()->RemoveActivePlugin(plugin);
        }

        // update the window menu
        UpdateCreateWindowMenu();
    }

    // open the autosave folder
    void MainWindow::OnOpenAutosaveFolder()
    {
        const QUrl url(("file:///" + GetManager()->GetAutosavesFolder()).c_str());
        QDesktopServices::openUrl(url);
    }


    // open the settings folder
    void MainWindow::OnOpenSettingsFolder()
    {
        const QUrl url(("file:///" + GetManager()->GetAppDataFolder()).c_str());
        QDesktopServices::openUrl(url);
    }

    // show the preferences dialog
    void MainWindow::OnPreferences()
    {
        if (m_preferencesWindow == nullptr)
        {
            m_preferencesWindow = new PreferencesWindow(this);
            m_preferencesWindow->Init();

            AzToolsFramework::ReflectedPropertyEditor* generalPropertyWidget = m_preferencesWindow->AddCategory("General");
            generalPropertyWidget->ClearInstances();
            generalPropertyWidget->InvalidateAll();

            generalPropertyWidget->AddInstance(&m_options, azrtti_typeid(m_options));

            const PluginManager::PluginVector& activePlugins = GetPluginManager()->GetActivePlugins();
            for (EMStudioPlugin* plugin : activePlugins)
            {
                if (PluginOptions* pluginOptions = plugin->GetOptions())
                {
                    generalPropertyWidget->AddInstance(pluginOptions, azrtti_typeid(pluginOptions));
                }
            }

            const PluginManager::PersistentPluginVector& persistentPlugins = GetPluginManager()->GetPersistentPlugins();
            for (const AZStd::unique_ptr<PersistentPlugin>& plugin : persistentPlugins)
            {
                if (PluginOptions* pluginOptions = plugin->GetOptions())
                {
                    generalPropertyWidget->AddInstance(pluginOptions, azrtti_typeid(pluginOptions));
                }
            }


            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return;
            }
            generalPropertyWidget->SetAutoResizeLabels(true);
            generalPropertyWidget->Setup(serializeContext, nullptr, true);
            generalPropertyWidget->show();
            generalPropertyWidget->ExpandAll();
            generalPropertyWidget->InvalidateAll();

            // Keyboard shortcuts
            KeyboardShortcutsWindow* shortcutsWindow = new KeyboardShortcutsWindow(m_preferencesWindow);
            m_preferencesWindow->AddCategory(shortcutsWindow, "Keyboard shortcuts");
        }

        m_preferencesWindow->exec();
        SavePreferences();
    }


    // save the preferences
    void MainWindow::SavePreferences()
    {
        // open the config file
        QSettings settings(this);
        m_options.Save(settings, *this);
    }


    // load the preferences
    void MainWindow::LoadPreferences()
    {
        // When a setting changes, OnOptionChanged will save. To avoid saving while settings are being
        // loaded, we use this flag
        m_loadingOptions = true;

        // open the config file
        QSettings settings(this);
        m_options = GUIOptions::Load(settings, *this);

        m_loadingOptions = false;
    }

    void MainWindow::AddRecentActorFile(const QString& fileName)
    {
        m_recentActors.AddRecentFile(fileName.toUtf8().data());
    }

    void MainWindow::LoadKeyboardShortcuts()
    {
        QSettings shortcutSettings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioKeyboardShortcuts.cfg").c_str(), QSettings::IniFormat, this);
        m_shortcutManager->Load(&shortcutSettings);
    }

    void MainWindow::LoadActor(const char* fileName, bool replaceCurrentScene)
    {
        // create the final command
        AZStd::string commandResult;

        // set the command group name based on the parameters
        const AZStd::string commandGroupName = (replaceCurrentScene) ? "Open actor" : "Merge actor";

        // create the command group
        AZStd::string outResult;
        MCore::CommandGroup commandGroup(commandGroupName.c_str());

        // clear the scene if not merging
        // clear the actors and actor instances selection if merging
        if (replaceCurrentScene)
        {
            CommandSystem::ClearScene(true, true, &commandGroup);
        }
        else
        {
            commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");
        }

        // create the load command
        AZStd::string loadActorCommand;

        // add the import command
        loadActorCommand = AZStd::string::format("ImportActor -filename \"%s\" ", fileName);

        // add the load actor settings
        LoadActorSettingsWindow::LoadActorSettings loadActorSettings;
        loadActorCommand += "-loadMeshes " + AZStd::to_string(loadActorSettings.m_loadMeshes);
        loadActorCommand += " -loadTangents " + AZStd::to_string(loadActorSettings.m_loadTangents);
        loadActorCommand += " -autoGenTangents " + AZStd::to_string(loadActorSettings.m_autoGenerateTangents);
        loadActorCommand += " -loadLimits " + AZStd::to_string(loadActorSettings.m_loadLimits);
        loadActorCommand += " -loadGeomLods " + AZStd::to_string(loadActorSettings.m_loadGeometryLoDs);
        loadActorCommand += " -loadMorphTargets " + AZStd::to_string(loadActorSettings.m_loadMorphTargets);
        loadActorCommand += " -loadCollisionMeshes " + AZStd::to_string(loadActorSettings.m_loadCollisionMeshes);
        loadActorCommand += " -loadMaterialLayers " + AZStd::to_string(loadActorSettings.m_loadStandardMaterialLayers);
        loadActorCommand += " -loadSkinningInfo " + AZStd::to_string(loadActorSettings.m_loadSkinningInfo);
        loadActorCommand += " -loadSkeletalLODs " + AZStd::to_string(loadActorSettings.m_loadSkeletalLoDs);
        loadActorCommand += " -dualQuatSkinning " + AZStd::to_string(loadActorSettings.m_dualQuaternionSkinning);

        // add the load and the create instance commands
        commandGroup.AddCommandString(loadActorCommand.c_str());

        // execute the group command
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError("Could not load actor '%s'.", fileName);
        }

        // add the actor in the recent actor list
        // if the same actor is already in the list, the duplicate is removed
        m_recentActors.AddRecentFile(fileName);
    }



    void MainWindow::LoadCharacter(const AZ::Data::AssetId& actorAssetId, const AZ::Data::AssetId& animgraphId, const AZ::Data::AssetId& motionSetId)
    {
        m_characterFiles.clear();
        AZStd::string cachePath = gEnv->pFileIO->GetAlias("@products@");
        AZStd::string filename;
        AzFramework::StringFunc::AssetDatabasePath::Normalize(cachePath);

        AZStd::string actorFilename;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            actorFilename, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, actorAssetId);
        AzFramework::StringFunc::AssetDatabasePath::Join(cachePath.c_str(), actorFilename.c_str(), filename);
        actorFilename = filename;

        AZStd::string animgraphFilename;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            animgraphFilename, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, animgraphId);
        bool found;
        if (!animgraphFilename.empty())
        {
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                found,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                animgraphFilename.c_str(),
                filename);

            if (found)
            {
                animgraphFilename = filename;
            }
        }

        AZStd::string motionSetFilename;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            motionSetFilename, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, motionSetId);
        if (!motionSetFilename.empty())
        {
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                found,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                motionSetFilename.c_str(),
                filename);

            if (found)
            {
                motionSetFilename = filename;
            }
        }

        // if the name is empty we stop looking for it
        bool foundActor = actorFilename.empty();
        bool foundAnimgraph = animgraphFilename.empty();
        bool foundMotionSet = motionSetFilename.empty();

        // Gather the list of dirty files
        AZStd::vector<AZStd::string> filenames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> objects;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> dirtyObjects;

        const size_t numDirtyFilesCallbacks = m_dirtyFileManager->GetNumCallbacks();
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = m_dirtyFileManager->GetCallback(i);
            callback->GetDirtyFileNames(&filenames, &objects);
            const size_t numFileNames = filenames.size();
            for (size_t j = 0; j < numFileNames; ++j)
            {
                // bypass if the filename is empty
                // it's the case when the file is not already saved
                if (filenames[j].empty())
                {
                    continue;
                }

                if (!foundActor && filenames[j] == actorFilename)
                {
                    foundActor = true;
                }
                else if (!foundAnimgraph && filenames[j] == animgraphFilename)
                {
                    foundAnimgraph = true;
                }
                else if (!foundMotionSet && filenames[j] == motionSetFilename)
                {
                    foundMotionSet = true;
                }
            }
        }

        // Dont reload dirty files that are already open.
        if (!foundActor)
        {
            m_characterFiles.push_back(actorFilename);
        }
        if (!foundAnimgraph)
        {
            m_characterFiles.push_back(animgraphFilename);
        }
        if (!foundMotionSet)
        {
            m_characterFiles.push_back(motionSetFilename);
        }

        if (isVisible() && m_layoutLoaded)
        {
            LoadCharacterFiles();
        }
    }

    void MainWindow::OnFileNewWorkspace()
    {
        // save all files that have been changed
        if (m_dirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            return;
        }

        // Are you sure?
        if (QMessageBox::warning(this, "New Workspace", "Are you sure you want to create a new workspace?\n\nThis will reset the entire scene.\n\nClick Yes to reset the scene and create a new workspace, No in case you want to cancel the process.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        {
            return;
        }

        // create th command group
        MCore::CommandGroup commandGroup("New workspace", 32);

        // clear everything
        Reset(true, true, true, true, &commandGroup);

        // execute the group command
        AZStd::string result;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            // clear the history
            GetCommandManager()->ClearHistory();

            // set the window title to not saved yet
            SetWindowTitleFromFileName("<not saved yet>");

            GetCommandManager()->SetUserOpenedWorkspaceFlag(true);
        }
        else
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        Workspace* workspace = GetManager()->GetWorkspace();
        workspace->SetFilename("");
        workspace->SetDirtyFlag(false);
    }


    void MainWindow::OnFileOpenWorkspace()
    {
        const AZStd::string filename = m_fileManager->LoadWorkspaceFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        LoadFile(filename.c_str());
    }


    void MainWindow::OnSaveAll()
    {
        m_dirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    }


    void MainWindow::OnFileSaveWorkspace()
    {
        // save all files that have been changed, filter to not show the workspace files
        if (m_dirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, SaveDirtyWorkspaceCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        Workspace* workspace = GetManager()->GetWorkspace();
        AZStd::string command;

        // save using the current filename or show the dialog
        if (workspace->GetFilenameString().empty())
        {
            // open up save as dialog so that we can choose a filename
            const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
            if (filename.empty())
            {
                return;
            }

            // save the workspace using the newly selected filename
            command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());
        }
        else
        {
            command = AZStd::string::format("SaveWorkspace -filename \"%s\"", workspace->GetFilename());
        }
        AZStd::string result;
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS,
                "Workspace <font color=green>successfully</font> saved");
        }
        else
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR,
                AZStd::string::format("Workspace <font color=red>failed</font> to save<br/><br/>%s", result.c_str()).c_str());
        }
    }


    void MainWindow::OnFileSaveWorkspaceAs()
    {
        // save all files that have been changed, filter to not show the workspace files
        if (m_dirtyFileManager->SaveDirtyFiles(MCORE_INVALIDINDEX32, SaveDirtyWorkspaceCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        // open up save as dialog so that we can choose a filename
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
        if (filename.empty())
        {
            return;
        }

        // save the workspace using the newly selected filename
        const AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());

        AZStd::string result;
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS,
                "Workspace <font color=green>successfully</font> saved");
        }
        else
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR,
                AZStd::string::format("Workspace <font color=red>failed</font> to save<br/><br/>%s", result.c_str()).c_str());
        }
    }


    void MainWindow::Reset(bool clearActors, bool clearMotionSets, bool clearMotions, bool clearAnimGraphs, MCore::CommandGroup* commandGroup, bool addDefaultMotionSet)
    {
        // create and relink to a temporary new command group in case the input command group has not been specified
        MCore::CommandGroup newCommandGroup("Reset Scene");

        // add commands in the command group if one is valid
        if (commandGroup == nullptr)
        {
            if (clearActors)
            {
                CommandSystem::ClearScene(true, true, &newCommandGroup);
            }
            if (clearAnimGraphs)
            {
                CommandSystem::ClearAnimGraphsCommand(&newCommandGroup);
            }
            if (clearMotionSets)
            {
                CommandSystem::ClearMotionSetsCommand(&newCommandGroup);
                if (addDefaultMotionSet)
                {
                    CommandSystem::CreateDefaultMotionSet(/*forceCreate=*/true, &newCommandGroup);
                }
            }
            if (clearMotions)
            {
                CommandSystem::ClearMotions(&newCommandGroup, true);
            }
        }
        else
        {
            if (clearActors)
            {
                CommandSystem::ClearScene(true, true, commandGroup);
            }
            if (clearAnimGraphs)
            {
                CommandSystem::ClearAnimGraphsCommand(commandGroup);
            }
            if (clearMotionSets)
            {
                CommandSystem::ClearMotionSetsCommand(commandGroup);
                if (addDefaultMotionSet)
                {
                    CommandSystem::CreateDefaultMotionSet(/*forceCreate=*/true, commandGroup);
                }
            }
            if (clearMotions)
            {
                CommandSystem::ClearMotions(commandGroup, true);
            }
        }

        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(newCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        GetCommandManager()->ClearHistory();

        Workspace* workspace = GetManager()->GetWorkspace();
        workspace->SetDirtyFlag(false);
    }

    void MainWindow::OnReset()
    {
        if (m_dirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            return;
        }

        ResetSettingsDialog* resetDialog = new ResetSettingsDialog(this);
        resetDialog->setObjectName("EMFX.MainWindow.ResetSettingsDialog");
        EMStudio::ResetSettingsDialog::connect(resetDialog, &QDialog::finished, [=](int resultCode)
        {
            resetDialog->deleteLater();

            if (resultCode == QDialog::Accepted)
            {
                Reset(
                    resetDialog->IsActorsChecked(),
                    resetDialog->IsMotionSetsChecked(),
                    resetDialog->IsMotionsChecked(),
                    resetDialog->IsAnimGraphsChecked()
                );
            }
        }
        );
        resetDialog->open();
    }

    void MainWindow::OnOptionChanged(const AZStd::string& optionChanged)
    {
        if (optionChanged == GUIOptions::s_maxRecentFilesOptionName)
        {
            // Set the maximum number of recent files
            m_recentActors.SetMaxRecentFiles(m_options.GetMaxRecentFiles());
            m_recentWorkspaces.SetMaxRecentFiles(m_options.GetMaxRecentFiles());
        }
        else if (optionChanged == GUIOptions::s_maxHistoryItemsOptionName)
        {
            // Set the maximum number of history items in the command manager
            GetCommandManager()->SetMaxHistoryItems(m_options.GetMaxHistoryItems());
        }
        else if (optionChanged == GUIOptions::s_notificationVisibleTimeOptionName)
        {
            // Set the notification visible time
            GetNotificationWindowManager()->SetVisibleTime(m_options.GetNotificationInvisibleTime());
        }
        else if (optionChanged == GUIOptions::s_enableAutosaveOptionName)
        {
            // Enable or disable the autosave timer
            if (m_options.GetEnableAutoSave())
            {
                m_autosaveTimer->setInterval(m_options.GetAutoSaveInterval() * 60 * 1000);
                m_autosaveTimer->start();
            }
            else
            {
                m_autosaveTimer->stop();
            }
        }
        else if (optionChanged == GUIOptions::s_autosaveIntervalOptionName)
        {
            // Set the autosave interval
            m_autosaveTimer->stop();
            m_autosaveTimer->setInterval(m_options.GetAutoSaveInterval() * 60 * 1000);
            m_autosaveTimer->start();
        }
        else if (optionChanged == GUIOptions::s_importerLogDetailsEnabledOptionName)
        {
            // Set if the detail logging of the importer is enabled or not
            EMotionFX::GetImporter().SetLogDetails(m_options.GetImporterLogDetailsEnabled());
        }
        else if (optionChanged == GUIOptions::s_autoLoadLastWorkspaceOptionName)
        {
            // Set if auto loading the last workspace is enabled or not
            GetManager()->SetAutoLoadLastWorkspace(m_options.GetAutoLoadLastWorkspace());
        }

        // Save preferences
        if (!m_loadingOptions)
        {
            SavePreferences();
        }
    }

    // open an actor
    void MainWindow::OnFileOpenActor()
    {
        if (m_dirtyFileManager->SaveDirtyFiles({azrtti_typeid<EMotionFX::Actor>()}) == DirtyFileManager::CANCELED)
        {
            return;
        }

        AZStd::vector<AZStd::string> filenames = m_fileManager->LoadActorsFileDialog(this);
        activateWindow();
        if (filenames.empty())
        {
            return;
        }

        const size_t numFilenames = filenames.size();
        for (size_t i = 0; i < numFilenames; ++i)
        {
            LoadActor(filenames[i].c_str(), i == 0);
        }
    }


    // merge an actor
    void MainWindow::OnFileMergeActor()
    {
        AZStd::vector<AZStd::string> filenames = m_fileManager->LoadActorsFileDialog(this);
        activateWindow();
        if (filenames.empty())
        {
            return;
        }

        for (const AZStd::string& filename : filenames)
        {
            LoadActor(filename.c_str(), false);
        }
    }


    // save selected actors
    void MainWindow::OnFileSaveSelectedActors()
    {
        // get the current selection list
        const CommandSystem::SelectionList& selectionList             = GetCommandManager()->GetCurrentSelection();
        const size_t                        numSelectedActors         = selectionList.GetNumSelectedActors();
        const size_t                        numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();

        // create the saving actor array
        AZStd::vector<EMotionFX::Actor*> savingActors;
        savingActors.reserve(numSelectedActors + numSelectedActorInstances);

        // add all selected actors to the list
        for (size_t i = 0; i < numSelectedActors; ++i)
        {
            savingActors.push_back(selectionList.GetActor(i));
        }

        // check all actors of all selected actor instances and put them in the list if they are not in yet
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::Actor* actor = selectionList.GetActorInstance(i)->GetActor();

            if (AZStd::find(savingActors.begin(), savingActors.end(), actor) == savingActors.end())
            {
                savingActors.push_back(actor);
            }
        }

        // Save all selected actors.
        const size_t numActors = savingActors.size();
        for (size_t i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = savingActors[i];
            GetMainWindow()->GetFileManager()->SaveActor(actor);
        }
    }


    void MainWindow::OnRecentFile(QAction* action)
    {
        const AZStd::string filename = action->data().toString().toUtf8().data();

        // Load the recent file.
        // No further error handling needed here as the commands do that all internally.
        LoadFile(filename.c_str(), 0, 0, false);
    }


    // save the current layout to a file
    void MainWindow::OnLayoutSaveAs()
    {
        EMStudio::GetLayoutManager()->SaveLayoutAs();
    }


    // update the layouts menu
    void MainWindow::UpdateLayoutsMenu()
    {
        // clear the current menu
        m_layoutsMenu->clear();

        // generate the layouts path
        QDir layoutsPath = QDir{ QString(MysticQt::GetDataDir().c_str()) }.filePath("Layouts");

        // open the dir
        QDir dir(layoutsPath);
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        // add each layout
        m_layoutNames.clear();
        AZStd::string filename;
        const QFileInfoList list = dir.entryInfoList();
        const int listSize = list.size();
        for (int i = 0; i < listSize; ++i)
        {
            // get the filename
            const QFileInfo fileInfo = list[i];
            FromQtString(fileInfo.fileName(), &filename);

            // check the extension, only ".layout" are accepted
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);
            AZStd::to_lower(extension.begin(), extension.end());
            if (extension == "layout")
            {
                AzFramework::StringFunc::Path::GetFileName(filename.c_str(), filename);
                m_layoutNames.emplace_back(filename);
            }
        }

        // add each menu
        for (const AZStd::string& layoutName : m_layoutNames)
        {
            QAction* action = m_layoutsMenu->addAction(layoutName.c_str());
            connect(action, &QAction::triggered, this, &MainWindow::OnLoadLayout);
        }

        // add the separator only if at least one layout
        if (!m_layoutNames.empty())
        {
            m_layoutsMenu->addSeparator();
        }

        // add the save current menu
        QAction* saveCurrentAction = m_layoutsMenu->addAction("Save Current");
        connect(saveCurrentAction, &QAction::triggered, this, &MainWindow::OnLayoutSaveAs);

        // remove menu is needed only if at least one layout
        if (!m_layoutNames.empty())
        {
            // add the remove menu
            QMenu* removeMenu = m_layoutsMenu->addMenu("Remove");
            removeMenu->setObjectName("RemoveMenu");

            // add each layout in the remove menu
            for (const AZStd::string& layoutName : m_layoutNames)
            {
                // User cannot remove the default layout. This layout is referenced in the qrc file, removing it will
                // cause compiling issue too.
                if (layoutName == "AnimGraph")
                {
                    continue;
                }
                QAction* action = removeMenu->addAction(layoutName.c_str());
                connect(action, &QAction::triggered, this, &MainWindow::OnRemoveLayout);
            }
        }

        // disable signals to avoid to switch of layout
        m_applicationMode->blockSignals(true);

        // update the combo box
        m_applicationMode->clear();
        for (const AZStd::string& layoutName : m_layoutNames)
        {
            m_applicationMode->addItem(layoutName.c_str());
        }

        // update the current selection of combo box
        const int layoutIndex = m_applicationMode->findText(QString(m_options.GetApplicationMode().c_str()));
        m_applicationMode->setCurrentIndex(layoutIndex);

        // enable signals
        m_applicationMode->blockSignals(false);
    }

    void MainWindow::ApplicationModeChanged(int index)
    {
        QString text = m_applicationMode->itemText(index);
        ApplicationModeChanged(text);
    }

    // called when the application mode combo box changed
    void MainWindow::ApplicationModeChanged(const QString& text)
    {
        if (text.isEmpty())
        {
            // If the text is empty, this means no .layout files exist on disk.
            // In this case, load the built-in layout
            GetLayoutManager()->LoadLayout(":/EMotionFX/AnimGraph.layout");
            return;
        }

        // update the last used layout and save it in the preferences file
        m_options.SetApplicationMode(text.toUtf8().data());
        SavePreferences();

        // generate the filename
        const auto filename = AZ::IO::Path(MysticQt::GetDataDir()) / AZStd::string::format("Layouts/%s.layout", FromQtString(text).c_str());

        // try to load it
        if (GetLayoutManager()->LoadLayout(filename.c_str()) == false)
        {
            MCore::LogError("Failed to load layout from file '%s'", filename.c_str());
        }
    }

    void MainWindow::OnRemoveLayoutButtonClicked([[maybe_unused]] QAbstractButton* button)
    {
        if (!m_reallyRemoveLayoutDialog)
        {
            return;
        }

        if (m_reallyRemoveLayoutDialog->buttonRole(m_reallyRemoveLayoutDialog->clickedButton()) == QMessageBox::YesRole)
        {
            // try to remove the file
            QFile file(m_layoutFileBeingRemoved);
            if (file.remove() == false)
            {
                MCore::LogError("Failed to remove layout file '%s'", FromQtString(m_layoutFileBeingRemoved).c_str());
                m_reallyRemoveLayoutDialog->close();
                m_reallyRemoveLayoutDialog = nullptr;
                return;
            }
            else
            {
                MCore::LogInfo("Successfullly removed layout file '%s'", FromQtString(m_layoutFileBeingRemoved).c_str());
            }

            // check if the layout removed is the current used
            if (QString(m_options.GetApplicationMode().c_str()) == m_removeLayoutNameText)
            {
                // find the layout index on the application mode combo box
                const int layoutIndex = m_applicationMode->findText(m_removeLayoutNameText);

                // set the new layout index, take the previous if the last layout is removed, the next is taken otherwise
                const int newLayoutIndex = (layoutIndex == (m_applicationMode->count() - 1)) ? layoutIndex - 1 : layoutIndex + 1;

                // select the layout, it also keeps it and saves to config
                m_applicationMode->setCurrentIndex(newLayoutIndex);
            }

            // update the layouts menu
            UpdateLayoutsMenu();
        }

        m_reallyRemoveLayoutDialog->close();
        m_reallyRemoveLayoutDialog = nullptr;
    }

    // remove a given layout
    void MainWindow::OnRemoveLayout()
    {
        // generate the filename
        QAction* action = qobject_cast<QAction*>(sender());
        m_layoutFileBeingRemoved = QDir(MysticQt::GetDataDir().c_str()).filePath(QString("Layouts/") + action->text() + ".layout");
        m_removeLayoutNameText = action->text();

        // make sure we really want to remove it
        m_reallyRemoveLayoutDialog = new QMessageBox(QMessageBox::Warning, "Remove The Selected Layout?", "Are you sure you want to remove the selected layout?<br>Note: This cannot be undone.", QMessageBox::Yes | QMessageBox::No, this);
        m_reallyRemoveLayoutDialog->setTextFormat(Qt::RichText);
        m_reallyRemoveLayoutDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_reallyRemoveLayoutDialog, &QMessageBox::buttonClicked, this, &MainWindow::OnRemoveLayoutButtonClicked);

        m_reallyRemoveLayoutDialog->open();
    }

    QMessageBox* MainWindow::GetRemoveLayoutDialog()
    {
        return m_reallyRemoveLayoutDialog;
    }

    // load a given layout
    void MainWindow::OnLoadLayout()
    {
        // get the menu action
        QAction* action = qobject_cast<QAction*>(sender());

        // update the last used layout and save it in the preferences file
        m_options.SetApplicationMode(action->text().toUtf8().data());
        SavePreferences();

        // generate the filename
        const auto filename = AZ::IO::Path(MysticQt::GetDataDir()) / AZStd::string::format("Layouts/%s.layout", FromQtString(action->text()).c_str());

        // try to load it
        if (GetLayoutManager()->LoadLayout(filename.c_str()))
        {
            // update the combo box
            m_applicationMode->blockSignals(true);
            const int layoutIndex = m_applicationMode->findText(action->text());
            m_applicationMode->setCurrentIndex(layoutIndex);
            m_applicationMode->blockSignals(false);
        }
        else
        {
            MCore::LogError("Failed to load layout from file '%s'", filename.c_str());
        }
    }


    // undo
    void MainWindow::OnUndo()
    {
        // check if we can undo
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() >= 0)
        {
            // perform the undo
            AZStd::string outResult;
            const bool result = GetCommandManager()->Undo(outResult);

            // log the results if there are any
            if (!outResult.empty())
            {
                if (result == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }
        }

        // enable or disable the undo/redo menu options
        UpdateUndoRedo();
    }


    // redo
    void MainWindow::OnRedo()
    {
        // check if we can redo
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() < (int32)GetCommandManager()->GetNumHistoryItems() - 1)
        {
            // perform the redo
            AZStd::string outResult;
            const bool result = GetCommandManager()->Redo(outResult);

            // log the results if there are any
            if (!outResult.empty())
            {
                if (result == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }
        }

        // enable or disable the undo/redo menu options
        UpdateUndoRedo();
    }


    // update the undo and redo status in the menu (disabled or enabled)
    void MainWindow::UpdateUndoRedo()
    {
        // check the undo status
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() >= 0)
        {
            m_undoAction->setEnabled(true);
        }
        else
        {
            m_undoAction->setEnabled(false);
        }

        // check the redo status
        if (GetCommandManager()->GetNumHistoryItems() > 0 && GetCommandManager()->GetHistoryIndex() < (int32)GetCommandManager()->GetNumHistoryItems() - 1)
        {
            m_redoAction->setEnabled(true);
        }
        else
        {
            m_redoAction->setEnabled(false);
        }
    }


    // disable undo/redo
    void MainWindow::DisableUndoRedo()
    {
        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);
    }


    void MainWindow::LoadFile(const AZStd::string& fileName, int32 contextMenuPosX, int32 contextMenuPosY, bool contextMenuEnabled, bool reload)
    {
        AZStd::vector<AZStd::string> filenames;
        filenames.push_back(AZStd::string(fileName.c_str()));
        LoadFiles(filenames, contextMenuPosX, contextMenuPosY, contextMenuEnabled, reload);
    }


    void MainWindow::LoadFiles(const AZStd::vector<AZStd::string>& filenames, int32 contextMenuPosX, int32 contextMenuPosY, bool contextMenuEnabled, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }

        AZStd::vector<AZStd::string> actorFilenames;
        AZStd::vector<AZStd::string> motionFilenames;
        AZStd::vector<AZStd::string> animGraphFilenames;
        AZStd::vector<AZStd::string> workspaceFilenames;
        AZStd::vector<AZStd::string> motionSetFilenames;

        // get the number of urls and iterate over them
        AZStd::string extension;
        for (const AZStd::string& filename : filenames)
        {
            // get the complete file name and extract the extension
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

            if (AzFramework::StringFunc::Equal(extension.c_str(), "actor"))
            {
                actorFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "motion"))
            {
                motionFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "animgraph"))
            {
                // Force-load from asset source folder.
                AZStd::string assetSourceFilename = filename;
                if (GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(assetSourceFilename))
                {
                    animGraphFilenames.push_back(assetSourceFilename);
                }
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "emfxworkspace"))
            {
                workspaceFilenames.push_back(filename);
            }
            else
            if (AzFramework::StringFunc::Equal(extension.c_str(), "motionset"))
            {
                // Force-load from asset source folder.
                AZStd::string assetSourceFilename = filename;
                if (GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(assetSourceFilename))
                {
                    motionSetFilenames.push_back(assetSourceFilename);
                }
            }
        }

        //--------------------

        const size_t actorCount = actorFilenames.size();
        if (actorCount == 1)
        {
            m_droppedActorFileName = actorFilenames[0].c_str();
            m_recentActors.AddRecentFile(m_droppedActorFileName.c_str());

            if (contextMenuEnabled)
            {
                if (EMotionFX::GetActorManager().GetNumActors() > 0)
                {
                    // create the drop context menu
                    QMenu menu(this);
                    QAction* openAction = menu.addAction("Open Actor");
                    QAction* mergeAction = menu.addAction("Merge Actor");
                    connect(openAction, &QAction::triggered, this, &MainWindow::OnOpenDroppedActor);
                    connect(mergeAction, &QAction::triggered, this, &MainWindow::OnMergeDroppedActor);

                    // show the menu at the given position
                    menu.exec(mapToGlobal(QPoint(contextMenuPosX, contextMenuPosY)));
                }
                else
                {
                    OnOpenDroppedActor();
                }
            }
            else
            {
                OnOpenDroppedActor();
            }
        }
        else
        {
            // Load and merge all actors.
            for (const AZStd::string& actorFilename : actorFilenames)
            {
                LoadActor(actorFilename.c_str(), false);
            }
        }



        //--------------------

        if (!motionFilenames.empty())
        {
            CommandSystem::LoadMotionsCommand(motionFilenames, reload);
        }
        if (!motionSetFilenames.empty())
        {
            CommandSystem::LoadMotionSetsCommand(motionSetFilenames, reload, false);
        }

        CommandSystem::LoadAnimGraphsCommand(animGraphFilenames, reload);

        //--------------------

        const size_t numWorkspaces = workspaceFilenames.size();
        if (numWorkspaces > 0)
        {
            // make sure we did not cancel load workspace
            if (m_dirtyFileManager->SaveDirtyFiles() != DirtyFileManager::CANCELED)
            {
                // add the workspace in the recent workspace list
                // if the same workspace is already in the list, the duplicate is removed
                m_recentWorkspaces.AddRecentFile(workspaceFilenames[0]);

                // create the command group
                MCore::CommandGroup workspaceCommandGroup("Load workspace", 64);

                // clear everything before laoding a new workspace file
                Reset(/*clearActors=*/true, /*clearMotionSets=*/true, /*clearMotions=*/true, /*clearAnimGraphs=*/true, &workspaceCommandGroup, /*addDefaultMotionSet=*/false);
                workspaceCommandGroup.SetReturnFalseAfterError(true);

                // load the first workspace of the list as more doesn't make sense anyway
                Workspace* workspace = GetManager()->GetWorkspace();
                if (workspace->Load(workspaceFilenames[0].c_str(), &workspaceCommandGroup))
                {
                    // execute the group command
                    AZStd::string result;
                    if (GetCommandManager()->ExecuteCommandGroup(workspaceCommandGroup, result))
                    {
                        // set the workspace not dirty
                        workspace->SetDirtyFlag(false);

                        const PluginManager::PluginVector& activePlugins = GetPluginManager()->GetActivePlugins();
                        for (EMStudioPlugin* plugin : activePlugins)
                        {
                            plugin->OnAfterLoadProject();
                        }

                        GetCommandManager()->ClearHistory();

                        // set the window title using the workspace filename
                        SetWindowTitleFromFileName(workspaceFilenames[0].c_str());

                        GetCommandManager()->SetUserOpenedWorkspaceFlag(true);
                    }
                    else
                    {
                        // result could arrive with some '%', since AZ_Error, assumes that the string being passed is a format, we could
                        // produce issues. To be safe, here we escape '%'
                        AzFramework::StringFunc::Replace(result, "%", "%%", true /* case sensitive since it is faster */);
                        AZ_Error("EMotionFX", false, result.c_str());
                    }
                }
            }
        }
    }

    void MainWindow::Activate(const AZ::Data::AssetId& actorAssetId, const EMotionFX::AnimGraph* animGraph, const EMotionFX::MotionSet* motionSet)
    {
        AZStd::string cachePath = gEnv->pFileIO->GetAlias("@products@");
        AZStd::string filename;
        AzFramework::StringFunc::AssetDatabasePath::Normalize(cachePath);

        AZStd::string actorFilename;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            actorFilename, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, actorAssetId);
        AzFramework::StringFunc::AssetDatabasePath::Join(cachePath.c_str(), actorFilename.c_str(), filename);
        actorFilename = filename;

        MCore::CommandGroup commandGroup("Animgraph and motion set activation");
        AZStd::string commandString;

        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (!actorInstance || actorFilename != actorInstance->GetActor()->GetFileName())
            {
                continue;
            }

            commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d",
                    actorInstance->GetID(),
                    animGraph->GetID(),
                    motionSet->GetID());
            commandGroup.AddCommandString(commandString);
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void MainWindow::LoadLayoutAfterShow()
    {
        if (!m_layoutLoaded)
        {
            m_layoutLoaded = true;

            LoadDefaultLayout();
            if (m_characterFiles.empty() && GetManager()->GetAutoLoadLastWorkspace())
            {
                // load last workspace
                const AZStd::string lastRecentWorkspace = m_recentWorkspaces.GetLastRecentFileName();
                if (!lastRecentWorkspace.empty())
                {
                    m_characterFiles.push_back(lastRecentWorkspace);
                }
            }
            if (!m_characterFiles.empty())
            {
                // Need to defer loading the character until the layout is ready. We also
                // need a couple of initializeGL/paintGL to happen before the character
                // is being loaded.
                QTimer::singleShot(1000, this, &MainWindow::LoadCharacterFiles);
            }
        }
    }

    void MainWindow::RaiseFloatingWidgets()
    {
        const QList<QDockWidget*> dockWidgetList = findChildren<QDockWidget*>();
        for (QDockWidget* dockWidget : dockWidgetList)
        {
            if (dockWidget->isFloating())
            {
                // There is some weird behavior with floating QDockWidget. After showing it,
                // the widget doesn't seem to remain when we move/maximize or do some changes in the
                // window that contains it. Setting it as floating false then true seems to workaround
                // the problem
                dockWidget->setFloating(false);
                dockWidget->setFloating(true);

                dockWidget->show();
                dockWidget->raise();
            }
        }
    }

    // Load default layout.
    void MainWindow::LoadDefaultLayout()
    {
        if (m_applicationMode->count() == 0)
        {
            // When the combo box is empty, the call to setCurrentIndex will
            // not cause any slots to be fired, so dispatch the call manually.
            // Pass an empty string to duplicate the behavior of calling
            // currentText() on an empty combo box
            ApplicationModeChanged(0);
            return;
        }

        int layoutIndex = m_applicationMode->findText(m_options.GetApplicationMode().c_str());

        // If searching for the last used layout fails load the default or viewer layout if they exist
        if (layoutIndex == -1)
        {
            layoutIndex = m_applicationMode->findText("AnimGraph");
        }
        if (layoutIndex == -1)
        {
            layoutIndex = m_applicationMode->findText("Character");
        }
        if (layoutIndex == -1)
        {
            layoutIndex = m_applicationMode->findText("Animation");
        }

        m_applicationMode->setCurrentIndex(layoutIndex);
    }


    EMotionFX::ActorInstance* MainWindow::GetSelectedActorInstance()
    {
        return GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
    }


    EMotionFX::Actor* MainWindow::GetSelectedActor()
    {
        return GetCommandManager()->GetCurrentSelection().GetSingleActor();
    }


    void MainWindow::BroadcastSelectionNotifications()
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();

        // Handle actor selection changes.
        EMotionFX::Actor* selectedActor = selectionList.GetSingleActor();
        if (m_prevSelectedActor != selectedActor)
        {
            EMotionFX::ActorEditorNotificationBus::Broadcast(&EMotionFX::ActorEditorNotifications::ActorSelectionChanged, selectedActor);
        }
        m_prevSelectedActor = selectedActor;

        // Handle actor instance selection changes.
        EMotionFX::ActorInstance* selectedActorInstance = selectionList.GetSingleActorInstance();
        if (m_prevSelectedActorInstance != selectedActorInstance)
        {
            EMotionFX::ActorEditorNotificationBus::Broadcast(&EMotionFX::ActorEditorNotifications::ActorInstanceSelectionChanged, selectedActorInstance);
        }
        m_prevSelectedActorInstance = selectedActorInstance;
    }


    void MainWindow::LoadCharacterFiles()
    {
        if (!m_characterFiles.empty())
        {
            LoadFiles(m_characterFiles, 0, 0, false, true);
            m_characterFiles.clear();

            const PluginManager::PluginVector& activePlugins = GetPluginManager()->GetActivePlugins();
            for (EMStudioPlugin* plugin : activePlugins)
            {
                plugin->OnAfterLoadActors();
            }
        }
    }

    void MainWindow::OnSaveLayoutDialogAccept()
    {
        EMStudio::GetLayoutManager()->SaveDialogAccepted();
    }

    void MainWindow::OnSaveLayoutDialogReject()
    {
        EMStudio::GetLayoutManager()->SaveDialogRejected();
    }

    // accept drops
    void MainWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    // gets called when the user drag&dropped an actor to the application and then chose to open it in the context menu
    void MainWindow::OnOpenDroppedActor()
    {
        if (m_dirtyFileManager->SaveDirtyFiles({azrtti_typeid<EMotionFX::Actor>()}) == DirtyFileManager::CANCELED)
        {
            return;
        }
        LoadActor(m_droppedActorFileName.c_str(), true);
    }


    // gets called when the user drag&dropped an actor to the application and then chose to merge it in the context menu
    void MainWindow::OnMergeDroppedActor()
    {
        LoadActor(m_droppedActorFileName.c_str(), false);
    }


    // handle drop events
    void MainWindow::dropEvent(QDropEvent* event)
    {
        // check if we dropped any files to the application
        const QMimeData* mimeData = event->mimeData();

        AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*> entries;
        if (AzToolsFramework::AssetBrowser::Utils::FromMimeData(mimeData, entries))
        {
            AZStd::vector<AZStd::string> fileNames;
            for (const auto& entry : entries)
            {
                AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*> productEntries;
                entry->GetChildrenRecursively<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry>(productEntries);
                for (const auto& productEntry : productEntries)
                {
                    fileNames.emplace_back(FileManager::GetAssetFilenameFromAssetId(productEntry->GetAssetId()));
                }
            }
            LoadFiles(fileNames, event->pos().x(), event->pos().y());
            event->acceptProposedAction();

        }
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (m_dirtyFileManager->SaveDirtyFiles() == DirtyFileManager::CANCELED)
        {
            event->ignore();
        }
        else
        {
            m_autosaveTimer->stop();

            PluginManager* pluginManager = GetPluginManager();

            // The close event does not hide floating widgets, so we are doing that manually here
            const QList<QDockWidget*> dockWidgetList = findChildren<QDockWidget*>();
            for (QDockWidget* dockWidget : dockWidgetList)
            {
                if (dockWidget->isFloating())
                {
                    dockWidget->hide();
                }
            }

            // get a copy of the active plugins since some plugins may choose
            // to get inactive when the main window closes
            const AZStd::vector<EMStudioPlugin*> activePlugins = pluginManager->GetActivePlugins();
            for (EMStudioPlugin* activePlugin : activePlugins)
            {
                AZ_Assert(activePlugin, "Unexpected null active plugin");
                activePlugin->OnMainWindowClosed();
            }

            QMainWindow::closeEvent(event);
        }

        // We mark it as false so next time is shown the layout is re-loaded if
        // necessary
        m_layoutLoaded = false;
    }


    void MainWindow::showEvent(QShowEvent* event)
    {
        if (m_options.GetEnableAutoSave())
        {
            m_autosaveTimer->setInterval(m_options.GetAutoSaveInterval() * 60 * 1000);
            m_autosaveTimer->start();
        }

        // EMotionFX dock widget is created the first time it's opened, so we need to load layout after that
        // The singleShot is needed because show event is fired before the dock widget resizes (in the same function dock widget is created)
        // So we want to load layout after that. It's a bit hacky, but most sensible at the moment.
        if (!m_layoutLoaded)
        {
            QTimer::singleShot(0, this, &MainWindow::LoadLayoutAfterShow);
        }

        QMainWindow::showEvent(event);

        // This show event ends up being called twice from QtViewPaneManager::OpenPane. At the end of the method
        // is doing a "raise" on this window. Since we cannot intercept that raise (raise is a slot and doesn't
        // have an event associated) we are deferring a call to RaiseFloatingWidgets which will raise the floating
        // widgets (this needs to happen after the raise from OpenPane).
        QTimer::singleShot(0, this, &MainWindow::RaiseFloatingWidgets);
    }

    // get the name of the currently active layout
    const char* MainWindow::GetCurrentLayoutName() const
    {
        // get the selected layout
        const int currentLayoutIndex = m_applicationMode->currentIndex();

        // if the index is out of range, return empty name
        if ((currentLayoutIndex < 0) || (currentLayoutIndex >= (int32)GetNumLayouts()))
        {
            return "";
        }

        // return the layout name
        return GetLayoutName(currentLayoutIndex);
    }

    const char* MainWindow::GetEMotionFXPaneName()
    {
        return LyViewPane::AnimationEditor;
    }


    void MainWindow::OnAutosaveTimeOut()
    {
        AZStd::vector<AZStd::string> filenames;
        AZStd::vector<AZStd::string> dirtyFileNames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> objects;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer> dirtyObjects;

        const size_t numDirtyFilesCallbacks = m_dirtyFileManager->GetNumCallbacks();
        for (size_t i = 0; i < numDirtyFilesCallbacks; ++i)
        {
            SaveDirtyFilesCallback* callback = m_dirtyFileManager->GetCallback(i);
            callback->GetDirtyFileNames(&filenames, &objects);
            const size_t numFileNames = filenames.size();
            for (size_t j = 0; j < numFileNames; ++j)
            {
                // bypass if the filename is empty
                // it's the case when the file is not already saved
                if (filenames[j].empty())
                {
                    continue;
                }

                // avoid duplicate of filename and object
                if (AZStd::find(dirtyFileNames.begin(), dirtyFileNames.end(), filenames[j]) == dirtyFileNames.end())
                {
                    dirtyFileNames.push_back(filenames[j]);
                    dirtyObjects.push_back(objects[j]);
                }
            }
        }

        // Skip directly in case there are no dirty files.
        if (dirtyFileNames.empty() && dirtyObjects.empty())
        {
            return;
        }

        // create the command group
        AZStd::string command;
        MCore::CommandGroup commandGroup("Autosave");

        // get the autosaves folder
        const AZStd::string autosavesFolder = GetManager()->GetAutosavesFolder();

        // save each dirty object
        QStringList entryList;
        AZStd::string filename, extension, startWithAutosave;
        const size_t numFileNames = dirtyFileNames.size();
        for (size_t i = 0; i < numFileNames; ++i)
        {
            // get the full path
            filename = dirtyFileNames[i];

            // get the base name with autosave
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), startWithAutosave);
            startWithAutosave += "_Autosave";

            // get the extension
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

            // open the dir and get the file list
            const QDir dir = QDir(autosavesFolder.c_str());
            entryList = dir.entryList(QDir::Files, QDir::Time | QDir::Reversed);

            // generate the autosave file list
            int maxAutosaveFileNumber = 0;
            QList<QString> autosaveFileList;
            const int numEntry = entryList.size();
            for (int j = 0; j < numEntry; ++j)
            {
                // get the file info
                const QFileInfo fileInfo = QFileInfo(QString::fromStdString(autosavesFolder.data()) + entryList[j]);

                // check the extension
                if (fileInfo.suffix() != extension.c_str())
                {
                    continue;
                }

                // check the base name
                const QString baseName = fileInfo.baseName();
                if (baseName.startsWith(startWithAutosave.c_str()))
                {
                    // extract the number
                    const int numberExtracted = baseName.mid(static_cast<int>(startWithAutosave.size())).toInt();

                    // check if the number is valid
                    if (numberExtracted > 0)
                    {
                        // add the file in the list
                        autosaveFileList.append(QString::fromStdString(autosavesFolder.data()) + entryList[j]);
                        AZ_Printf("EMotionFX", "Appending '%s' #%i\n", entryList[j].toUtf8().data(), numberExtracted);

                        // Update the maximum autosave file number that already exists on disk.
                        maxAutosaveFileNumber = MCore::Max<int>(maxAutosaveFileNumber, numberExtracted);
                    }
                }
            }

            // check if the length is upper than the max num files
            if (autosaveFileList.length() >= m_options.GetAutoSaveNumberOfFiles())
            {
                // number of files to delete
                // one is added because one space needs to be free for the new file
                const int numFilesToDelete = m_options.GetAutoSaveNumberOfFiles() ? (autosaveFileList.size() - m_options.GetAutoSaveNumberOfFiles() + 1) : autosaveFileList.size();

                // delete each file
                for (int j = 0; j < numFilesToDelete; ++j)
                {
                    AZ_Printf("EMotionFX", "Removing '%s'\n", autosaveFileList[j].toUtf8().data());
                    QFile::remove(autosaveFileList[j]);
                }
            }

            // Set the new autosave file number and prevent an integer overflow.
            int newAutosaveFileNumber = maxAutosaveFileNumber + 1;
            if (newAutosaveFileNumber == std::numeric_limits<int>::max())
            {
                // Restart counting autosave file numbers from the beginning.
                newAutosaveFileNumber = 1;
            }

            // save the new file
            AZStd::string newFileFilename;
            newFileFilename = AZStd::string::format("%s%s%d.%s", autosavesFolder.c_str(), startWithAutosave.c_str(), newAutosaveFileNumber, extension.c_str());
            AZ_Printf("EMotionFX", "Saving to '%s'\n", newFileFilename.c_str());

            // Backing up actors and motions doesn't work anymore as we just update the .assetinfos and the asset processor does the rest.
            if (dirtyObjects[i].m_motionSet)
            {
                command = AZStd::string::format("SaveMotionSet -motionSetID %i -filename \"%s\" -updateFilename false -updateDirtyFlag false -sourceControl false", dirtyObjects[i].m_motionSet->GetID(), newFileFilename.c_str());
                commandGroup.AddCommandString(command);
            }
            else if (dirtyObjects[i].m_animGraph)
            {
                const size_t animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(dirtyObjects[i].m_animGraph);
                command = AZStd::string::format("SaveAnimGraph -index %zu -filename \"%s\" -updateFilename false -updateDirtyFlag false -sourceControl false", animGraphIndex, newFileFilename.c_str());
                commandGroup.AddCommandString(command);
            }
            else if (dirtyObjects[i].m_workspace)
            {
                Workspace* workspace = GetManager()->GetWorkspace();
                workspace->Save(newFileFilename.c_str(), false, false);
            }
        }

        // execute the command group
        AZStd::string result;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result, false))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS,
                "Autosave <font color=green>completed</font>");
        }
        else
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR,
                AZStd::string::format("Autosave <font color=red>failed</font><br/><br/>%s", result.c_str()).c_str());
        }
    }


    void MainWindow::moveEvent(QMoveEvent* event)
    {
        MCORE_UNUSED(event);
        GetManager()->GetNotificationWindowManager()->OnMovedOrResized();
    }


    void MainWindow::resizeEvent(QResizeEvent* event)
    {
        MCORE_UNUSED(event);
        GetManager()->GetNotificationWindowManager()->OnMovedOrResized();
    }


    void MainWindow::OnUpdateRenderPlugins()
    {
        const PluginManager::PluginVector& activePlugins = GetPluginManager()->GetActivePlugins();
        for (EMStudioPlugin* plugin : activePlugins)
        {
            if (plugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_RENDERING)
            {
                plugin->ProcessFrame(0.0f);
            }
        }
    }

    void MainWindow::UpdatePlugins(float timeDelta)
    {
        EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
        if (!pluginManager)
        {
            return;
        }

        const PluginManager::PluginVector& activePlugins = pluginManager->GetActivePlugins();
        for (EMStudioPlugin* plugin : activePlugins)
        {
            plugin->ProcessFrame(timeDelta);
        }

        const PluginManager::PersistentPluginVector& persistentPlugins = pluginManager->GetPersistentPlugins();
        for (const AZStd::unique_ptr<PersistentPlugin>& plugin : persistentPlugins)
        {
            plugin->Update(timeDelta);
        }
    }

    void MainWindow::EnableUpdatingPlugins()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void MainWindow::DisableUpdatingPlugins()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MainWindow::OnTick(float delta, AZ::ScriptTimePoint timePoint)
    {
        AZ_UNUSED(timePoint);

        // Check if we are in game mode.
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        const bool inGameMode = editor ? editor->IsInGameMode() : false;

        // Update all the animation editor plugins (redraw viewports, timeline, and graph windows etc).
        // But only update this when the main window is visible and we are in game mode.
        const bool isEditorActive = !visibleRegion().isEmpty() && !inGameMode;

        if (isEditorActive)
        {
            UpdatePlugins(delta);
        }
    }

    int MainWindow::GetTickOrder()
    {
        return AZ::TICK_UI;
    }
} // namespace EMStudio
