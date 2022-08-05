/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/EditorActionsHandler.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzQtComponents/Components/Style.h>

#include <CryEdit.h>
#include <EditorCoreAPI.h>
#include <Editor/Undo/Undo.h>
#include <Editor/EditorViewportSettings.h>
#include <GameEngine.h>
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>
#include <MainWindow.h>
#include <QtViewPaneManager.h>

#include <QDesktopServices>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWidget>

static constexpr AZStd::string_view EditorMainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";

static constexpr AZStd::string_view AngleSnappingStateChangedUpdaterIdentifier = "o3de.updater.onAngleSnappingStateChanged";
static constexpr AZStd::string_view EntitySelectionChangedUpdaterIdentifier = "o3de.updater.onEntitySelectionChanged";
static constexpr AZStd::string_view GameModeStateChangedUpdaterIdentifier = "o3de.updater.onGameModeStateChanged";
static constexpr AZStd::string_view GridSnappingStateChangedUpdaterIdentifier = "o3de.updater.onGridSnappingStateChanged";
static constexpr AZStd::string_view LevelLoadedUpdaterIdentifier = "o3de.updater.onLevelLoaded";
static constexpr AZStd::string_view RecentFilesChangedUpdaterIdentifier = "o3de.updater.onRecentFilesChanged";
static constexpr AZStd::string_view UndoRedoUpdaterIdentifier = "o3de.updater.onUndoRedo";

static constexpr AZStd::string_view EditorMainWindowMenuBarIdentifier = "o3de.menubar.editor.mainwindow";

static constexpr AZStd::string_view FileMenuIdentifier = "o3de.menu.editor.file";
static constexpr AZStd::string_view RecentFilesMenuIdentifier = "o3de.menu.editor.file.recent";
static constexpr AZStd::string_view EditMenuIdentifier = "o3de.menu.editor.edit";
static constexpr AZStd::string_view EditModifyMenuIdentifier = "o3de.menu.editor.edit.modify";
static constexpr AZStd::string_view EditModifySnapMenuIdentifier = "o3de.menu.editor.edit.modify.snap";
static constexpr AZStd::string_view EditModifyModesMenuIdentifier = "o3de.menu.editor.edit.modify.modes";
static constexpr AZStd::string_view EditSettingsMenuIdentifier = "o3de.menu.editor.edit.settings";
static constexpr AZStd::string_view GameMenuIdentifier = "o3de.menu.editor.game";
static constexpr AZStd::string_view PlayGameMenuIdentifier = "o3de.menu.editor.game.play";
static constexpr AZStd::string_view GameAudioMenuIdentifier = "o3de.menu.editor.game.audio";
static constexpr AZStd::string_view GameDebuggingMenuIdentifier = "o3de.menu.editor.game.debugging";
static constexpr AZStd::string_view ToolsMenuIdentifier = "o3de.menu.editor.tools";
static constexpr AZStd::string_view ViewMenuIdentifier = "o3de.menu.editor.view";
static constexpr AZStd::string_view HelpMenuIdentifier = "o3de.menu.editor.help";
static constexpr AZStd::string_view HelpDocumentationMenuIdentifier = "o3de.menu.editor.help.documentation";
static constexpr AZStd::string_view HelpGameDevResourcesMenuIdentifier = "o3de.menu.editor.help.gamedevresources";

static constexpr AZStd::string_view ToolsToolBarIdentifier = "o3de.toolbar.editor.tools";
static constexpr AZStd::string_view PlayControlsToolBarIdentifier = "o3de.toolbar.editor.playcontrols";

static const int maxRecentFiles = 10;

bool IsLevelLoaded()
{
    auto cryEdit = CCryEditApp::instance();
    return !cryEdit->IsExportingLegacyData() && GetIEditor()->IsLevelLoaded();
}

bool IsEntitySelected()
{
    bool result = false;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        result, &AzToolsFramework::ToolsApplicationRequestBus::Handler::AreAnyEntitiesSelected);
    return result;
}

void EditorActionsHandler::Initialize(QMainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    m_cryEditApp = CCryEditApp::instance();
    m_qtViewPaneManager = QtViewPaneManager::instance();

    m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
    AZ_Assert(m_actionManagerInterface, "EditorActionsHandler - could not get ActionManagerInterface on EditorActionsHandler construction.");

    m_actionManagerInternalInterface = AZ::Interface<AzToolsFramework::ActionManagerInternalInterface>::Get();
    AZ_Assert(
        m_actionManagerInternalInterface,
        "EditorActionsHandler - could not get ActionManagerInternalInterface on EditorActionsHandler construction.");
    
    m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
    AZ_Assert(m_hotKeyManagerInterface, "EditorActionsHandler - could not get HotKeyManagerInterface on EditorActionsHandler construction.");
    
    m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
    AZ_Assert(m_menuManagerInterface, "EditorActionsHandler - could not get MenuManagerInterface on EditorActionsHandler construction.");
    
    m_menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get();
    AZ_Assert(
        m_menuManagerInternalInterface, "EditorActionsHandler - could not get MenuManagerInternalInterface on EditorActionsHandler construction.");
    
    m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();
    AZ_Assert(m_toolBarManagerInterface, "EditorActionsHandler - could not get ToolBarManagerInterface on EditorActionsHandler construction.");

    InitializeActionContext();
    InitializeActionUpdaters();
    InitializeActions();
    InitializeWidgetActions();
    InitializeMenus();
    InitializeToolBars();

    // Ensure the tools menu and toolbar are refreshed when the viewpanes change.
    QObject::connect(
        m_qtViewPaneManager, &QtViewPaneManager::registeredPanesChanged, m_mainWindow,
        [&]()
        {
            RefreshToolActions();
        }
    );

    AzToolsFramework::EditorEventsBus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
    m_initialized = true;
}

EditorActionsHandler::~EditorActionsHandler()
{
    if (m_initialized)
    {
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
    }
}

void EditorActionsHandler::InitializeActionContext()
{
    AzToolsFramework::ActionContextProperties contextProperties;
    contextProperties.m_name = "O3DE Editor";

    m_actionManagerInterface->RegisterActionContext("", EditorMainWindowActionContextIdentifier, contextProperties, m_mainWindow);
}

void EditorActionsHandler::InitializeActionUpdaters()
{
    m_actionManagerInterface->RegisterActionUpdater(AngleSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EntitySelectionChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(GameModeStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(GridSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(RecentFilesChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(UndoRedoUpdaterIdentifier);

    // If the Prefab system is not enabled, have a backup to update actions based on level loading.
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        m_isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (!m_isPrefabSystemEnabled)
    {
        m_actionManagerInterface->RegisterActionUpdater(LevelLoadedUpdaterIdentifier);
    }
}

void EditorActionsHandler::InitializeActions()
{
    // --- File Actions

    // New Level
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "New Level";
        actionProperties.m_description = "Create a new level";
        actionProperties.m_category = "Level";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.file.new", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnCreateLevel();
            }
        );

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.file.new", "Ctrl+N");
    }

    // Open Level
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open Level...";
        actionProperties.m_description = "Open an existing level";
        actionProperties.m_category = "Level";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.file.open", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnOpenLevel();
            }
        );

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.file.open", "Ctrl+O");
    }

    // Recent Files
    {
        RecentFileList* recentFiles = m_cryEditApp->GetRecentFileList();
        const int recentFilesSize = recentFiles->GetSize();

        for (int index = 0; index < maxRecentFiles; ++index)
        {
            AzToolsFramework::ActionProperties actionProperties;
            if (index < recentFilesSize)
            {
                actionProperties.m_name = AZStd::string::format("%i | %s", index + 1, (*recentFiles)[index].toUtf8().data());
            }
            else
            {
                actionProperties.m_name = AZStd::string::format("Recent File #%i", index + 1);
            }
            actionProperties.m_category = "Level";

            AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);

            m_actionManagerInterface->RegisterAction(
                EditorMainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [cryEdit = m_cryEditApp, index]()
                {
                    RecentFileList* recentFiles = cryEdit->GetRecentFileList();
                    const int recentFilesSize = recentFiles->GetSize();

                    if (index < recentFilesSize)
                    {
                        cryEdit->OpenDocumentFile((*recentFiles)[index].toUtf8().data(), true, COpenSameLevelOptions::ReopenLevelIfSame);
                    }
                }
            );

            m_actionManagerInterface->InstallEnabledStateCallback(
                actionIdentifier,
                [&, index]() -> bool
                {
                    return IsRecentFileActionActive(index);
                }
            );

            m_actionManagerInterface->AddActionToUpdater(RecentFilesChangedUpdaterIdentifier, actionIdentifier);
        }
    }

    // Clear Recent Files
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Clear All";
        actionProperties.m_description = "Clear the recent files list.";
        actionProperties.m_category = "Level";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.file.recent.clearAll",
            actionProperties,
            [&]()
            {
                RecentFileList* mruList = CCryEditApp::instance()->GetRecentFileList();

                // remove everything from the mru list
                for (int i = mruList->GetSize(); i > 0; i--)
                {
                    mruList->Remove(i - 1);
                }

                // save the settings immediately to the registry
                mruList->WriteList();

                // re-update the menus
                UpdateRecentFileActions();
            }
        );
    }

    // Save
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save";
        actionProperties.m_description = "Save the current level";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.file.save", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnFileSave();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.file.save", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, "o3de.action.file.save");
        
        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.file.save", "Ctrl+S");
    }

    // Save As...
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save As...";
        actionProperties.m_description = "Save the current level";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.file.saveAs", actionProperties,
            []()
            {
                CCryEditDoc* pDoc = GetIEditor()->GetDocument();
                pDoc->OnFileSaveAs();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.file.saveAs", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, "o3de.action.file.saveAs");
    }

    // Save Level Statistics
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save Level Statistics";
        actionProperties.m_description = "Logs Editor memory usage.";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.file.saveLevelStatistics", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnToolsLogMemoryUsage();
            }
        );

        // This action is required by python tests, but is always disabled.
        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.file.saveLevelStatistics",
            []() -> bool
            {
                return false;
            }
        );
    }

    // Edit Project Settings
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Edit Project Settings...";
        actionProperties.m_description = "Open the Project Settings panel.";
        actionProperties.m_category = "Project";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.project.editSettings", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnOpenProjectManagerSettings();
            }
        );
    }

    // Edit Platform Settings
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Edit Platform Settings...";
        actionProperties.m_description = "Open the Platform Settings panel.";
        actionProperties.m_category = "Platform";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.platform.editSettings", actionProperties,
            [qtViewPaneManager = m_qtViewPaneManager]()
            {
                qtViewPaneManager->OpenPane(LyViewPane::ProjectSettingsTool);
            }
        );
    }

    // New Project
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "New Project...";
        actionProperties.m_description = "Create a new project in the Project Manager.";
        actionProperties.m_category = "Project";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.project.new",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnOpenProjectManagerNew();
            }
        );
    }

    // Open Project
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open Project...";
        actionProperties.m_description = "Open a different project in the Project Manager.";
        actionProperties.m_category = "Project";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.project.open",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnOpenProjectManager();
            }
        );
    }

    // Show Log File
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Log File";
        actionProperties.m_category = "Project";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.file.showLog",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnFileEditLogFile();
            }
        );
    }

    // Editor Exit
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Exit";
        actionProperties.m_description = "Open a different project in the Project Manager.";
        actionProperties.m_category = "Project";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.editor.exit",
            actionProperties,
            [=]()
            {
                m_mainWindow->window()->close();
            }
        );
    }

    // --- Edit Actions

    // Undo
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&Undo";
        actionProperties.m_description = "Undo last operation";
        actionProperties.m_category = "Edit";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.undo",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnUndo();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.edit.undo",
            []() -> bool
            {
                return GetIEditor()->GetUndoManager()->IsHaveUndo();
            }
        );

        // Trigger update after every undo or redo operation
        m_actionManagerInterface->AddActionToUpdater(UndoRedoUpdaterIdentifier, "o3de.action.edit.undo");

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.edit.undo", "Ctrl+Z");
    }

    // Redo
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&Redo";
        actionProperties.m_description = "Redo last undo operation";
        actionProperties.m_category = "Edit";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.redo",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnRedo();
            }
        );

        auto outcome = m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.edit.redo",
            []() -> bool
            {
                return GetIEditor()->GetUndoManager()->IsHaveRedo();
            }
        );

        // Trigger update after every undo or redo operation
        m_actionManagerInterface->AddActionToUpdater(UndoRedoUpdaterIdentifier, "o3de.action.edit.redo");

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.edit.redo", "Ctrl+Shift+Z");
    }

    // Angle Snapping
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Angle snapping";
        actionProperties.m_description = "Toggle angle snapping";
        actionProperties.m_category = "Edit";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Angle.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.snap.toggleAngleSnapping",
            actionProperties,
            []()
            {
                SandboxEditor::SetAngleSnapping(!SandboxEditor::AngleSnappingEnabled());
            },
            []() -> bool
            {
                return SandboxEditor::AngleSnappingEnabled();
            }
        );

        // Trigger update when the angle snapping setting changes
        m_actionManagerInterface->AddActionToUpdater(AngleSnappingStateChangedUpdaterIdentifier, "o3de.action.edit.snap.toggleAngleSnapping");
    }

    // Grid Snapping
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Grid snapping";
        actionProperties.m_description = "Toggle grid snapping";
        actionProperties.m_category = "Edit";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Grid.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.snap.toggleGridSnapping",
            actionProperties,
            []()
            {
                SandboxEditor::SetGridSnapping(!SandboxEditor::GridSnappingEnabled());
            },
            []() -> bool
            {
                return SandboxEditor::GridSnappingEnabled();
            }
        );

        // Trigger update when the grid snapping setting changes
        m_actionManagerInterface->AddActionToUpdater(GridSnappingStateChangedUpdaterIdentifier, "o3de.action.edit.snap.toggleGridSnapping");
    }

    // Global Preferences
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Global Preferences...";
        actionProperties.m_category = "Editor";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.globalPreferences",
            actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnToolsPreferences();
            }
        );
    }

    // Editor Settings Manager
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Editor Settings Manager";
        actionProperties.m_category = "Editor";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.edit.editorSettingsManager",
            actionProperties,
            []()
            {
                QtViewPaneManager::instance()->OpenPane(LyViewPane::EditorSettingsManager);
            }
        );
    }

    // --- Game Actions

    // Play Game
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Play Game";
        actionProperties.m_description = "Activate the game input mode.";
        actionProperties.m_category = "Game";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Play.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.play", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnViewSwitchToGame();
            },
            []()
            {
                return GetIEditor()->IsInGameMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.game.play", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, "o3de.action.game.play");
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, "o3de.action.game.play");

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.game.play", "Ctrl+G");
    }

    // Play Game (Maximized)
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Play Game (Maximized)";
        actionProperties.m_description = "Activate the game input mode (maximized).";
        actionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.playMaximized", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnViewSwitchToGameFullScreen();
            },
            []()
            {
                return GetIEditor()->IsInGameMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.game.playMaximized", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, "o3de.action.game.playMaximized");
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, "o3de.action.game.playMaximized");
    }

    // Simulate
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Simulate";
        actionProperties.m_description = "Enable processing of Physics and AI.";
        actionProperties.m_category = "Game";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Simulate_Physics.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.simulate", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnSwitchPhysics();
            },
            [cryEdit = m_cryEditApp]()
            {
                return !cryEdit->IsExportingLegacyData() && GetIEditor()->GetGameEngine()->GetSimulationMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.game.simulate", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, "o3de.action.game.simulate");
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, "o3de.action.game.simulate");
    }

    // Export Selected Objects
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Export Selected Objects";
        actionProperties.m_description = "Export Selected Objects.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.exportSelectedObjects", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnExportSelectedObjects();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.game.exportSelectedObjects", IsEntitySelected);
        m_actionManagerInterface->AddActionToUpdater(EntitySelectionChangedUpdaterIdentifier, "o3de.action.game.exportSelectedObjects");
    }

    // Export Occlusion Objects
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Export Occlusion Mesh";
        actionProperties.m_description = "Export Occlusion Mesh.";
        actionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.exportOcclusionMesh", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnFileExportOcclusionMesh();
            }
        );
    }

    // Move Player and Camera Separately
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Move Player and Camera Separately";
        actionProperties.m_description = "Move Player and Camera Separately.";
        actionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            "o3de.action.game.movePlayerAndCameraSeparately",
            actionProperties,
            []()
            {
                GetIEditor()->GetGameEngine()->SyncPlayerPosition(!GetIEditor()->GetGameEngine()->IsSyncPlayerPosition());
            },
            []()
            {
                return !GetIEditor()->GetGameEngine()->IsSyncPlayerPosition();
            }
        );
    }

    // Stop All Sounds
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Stop All Sounds";
        actionProperties.m_description = "Stop All Sounds.";
        actionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.audio.stopAllSounds", actionProperties,
            []()
            {
                LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
                    &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
            }
        );
    }

    // Refresh Audio System
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Refresh";
        actionProperties.m_description = "Refresh Audio System.";
        actionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.game.audio.refresh", actionProperties,
            []()
            {
                AZStd::string levelName;
                AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequestBus::Events::GetLevelName);
                AZStd::to_lower(levelName.begin(), levelName.end());

                if (levelName == "untitled")
                {
                    levelName.clear();
                }

                LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
                    &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalRefreshAudio, AZStd::string_view{ levelName });
            }
        );
    }

    // --- Help Actions

    // Tutorials
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Tutorials";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.tutorials", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationTutorials();
            }
        );
    }

    // Open 3D Engine Documentation
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open 3D Engine Documentation";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.documentation.o3de", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationO3DE();
            }
        );
    }

    // GameLift Documentation
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "GameLift Documentation";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.documentation.gamelift", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationGamelift();
            }
        );
    }

    // Release Notes
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Release Notes";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.documentation.releasenotes", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationReleaseNotes();
            }
        );
    }

    // GameDev Blog
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "GameDev Blog";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.resources.gamedevblog", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationGameDevBlog();
            }
        );
    }

    // Forums
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Forums";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.resources.forums", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationForums();
            }
        );
    }

    // AWS Support
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "AWS Support";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.resources.awssupport", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnDocumentationAWSSupport();
            }
        );
    }

    // About O3DE
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&About O3DE";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.abouto3de", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnAppAbout();
            }
        );
    }

    // Welcome
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&Welcome";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier, "o3de.action.help.welcome", actionProperties,
            [cryEdit = m_cryEditApp]()
            {
                cryEdit->OnAppShowWelcomeScreen();
            }
        );
    }

}

void EditorActionsHandler::InitializeWidgetActions()
{
    // Help - Search Documentation Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Search Documentation";
        widgetActionProperties.m_category = "Help";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.help.searchDocumentation",
            widgetActionProperties,
            [&]()
            {
                return CreateDocsSearchWidget();
            }
        );
    }

    // Expander
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Expander";
        widgetActionProperties.m_category = "Widgets";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.expander",
            widgetActionProperties,
            [&]()
            {
                return CreateExpander();
            }
        );
    }

    // Play Controls - Label
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Play Controls Label";
        widgetActionProperties.m_category = "Game";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.game.playControlsLabel",
            widgetActionProperties,
            [&]()
            {
                return CreatePlayControlsLabel();
            }
        );
    }
}

void EditorActionsHandler::InitializeMenus()
{
    // Register MenuBar
    m_menuManagerInterface->RegisterMenuBar(EditorMainWindowMenuBarIdentifier);

    // Initialize Menus
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&File";
        m_menuManagerInterface->RegisterMenu(FileMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Open Recent";
            m_menuManagerInterface->RegisterMenu(RecentFilesMenuIdentifier, menuProperties);

            // Legacy - the menu should update when the files list is changed.
            QMenu* menu = m_menuManagerInternalInterface->GetMenu(FileMenuIdentifier);
            QObject::connect(
                menu,
                &QMenu::aboutToShow,
                m_mainWindow,
                [&]()
                {
                    UpdateRecentFileActions();
                }
            );
        }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Edit";
        m_menuManagerInterface->RegisterMenu(EditMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Modify";
            m_menuManagerInterface->RegisterMenu(EditModifyMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Snap";
            m_menuManagerInterface->RegisterMenu(EditModifySnapMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Transform Mode";
            m_menuManagerInterface->RegisterMenu(EditModifyModesMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Editor Settings";
            m_menuManagerInterface->RegisterMenu(EditSettingsMenuIdentifier, menuProperties);
        }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Game";
        m_menuManagerInterface->RegisterMenu(GameMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Play Game";
                m_menuManagerInterface->RegisterMenu(PlayGameMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Audio";
                m_menuManagerInterface->RegisterMenu(GameAudioMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Debugging";
                m_menuManagerInterface->RegisterMenu(GameDebuggingMenuIdentifier, menuProperties);
        }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Tools";
        m_menuManagerInterface->RegisterMenu(ToolsMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&View";
        m_menuManagerInterface->RegisterMenu(ViewMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Help";
        m_menuManagerInterface->RegisterMenu(HelpMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Documentation";
            m_menuManagerInterface->RegisterMenu(HelpDocumentationMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "GameDev Resources";
            m_menuManagerInterface->RegisterMenu(HelpGameDevResourcesMenuIdentifier, menuProperties);
        }

    // Add Menus to MenuBar
    // We space the sortkeys by 100 to allow external systems to add menus in-between.
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, FileMenuIdentifier, 100);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, EditMenuIdentifier, 200);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, GameMenuIdentifier, 300);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, ToolsMenuIdentifier, 400);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, ViewMenuIdentifier, 500);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, HelpMenuIdentifier, 600);

    // Set the menu bar for this window
    m_mainWindow->setMenuBar(m_menuManagerInternalInterface->GetMenuBar(EditorMainWindowMenuBarIdentifier));

    // Add actions to each menu

    // File
    {
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.new", 100);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.open", 200);
        m_menuManagerInterface->AddSubMenuToMenu(FileMenuIdentifier, RecentFilesMenuIdentifier, 300);
        {
            for (int index = 0; index < maxRecentFiles; ++index)
            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);
                m_menuManagerInterface->AddActionToMenu(RecentFilesMenuIdentifier, actionIdentifier, 100);
            }
            m_menuManagerInterface->AddSeparatorToMenu(RecentFilesMenuIdentifier, 200);
            m_menuManagerInterface->AddActionToMenu(RecentFilesMenuIdentifier, "o3de.action.file.recent.clearAll", 300);
        }
        m_menuManagerInterface->AddSeparatorToMenu(FileMenuIdentifier, 400);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.save", 500);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.saveAs", 600);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.saveLevelStatistics", 700);
        m_menuManagerInterface->AddSeparatorToMenu(FileMenuIdentifier, 800);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.project.editSettings", 900);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.platform.editSettings", 1000);
        m_menuManagerInterface->AddSeparatorToMenu(FileMenuIdentifier, 1100);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.project.new", 1200);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.project.open", 1300);
        m_menuManagerInterface->AddSeparatorToMenu(FileMenuIdentifier, 1400);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.file.showLog", 1500);
        m_menuManagerInterface->AddSeparatorToMenu(FileMenuIdentifier, 1600);
        m_menuManagerInterface->AddActionToMenu(FileMenuIdentifier, "o3de.action.editor.exit", 1700);
    }

    // Edit
    {
        m_menuManagerInterface->AddActionToMenu(EditMenuIdentifier, "o3de.action.edit.undo", 100);
        m_menuManagerInterface->AddActionToMenu(EditMenuIdentifier, "o3de.action.edit.redo", 200);

        m_menuManagerInterface->AddSubMenuToMenu(EditMenuIdentifier, EditModifyMenuIdentifier, 1800);
        {
            m_menuManagerInterface->AddSubMenuToMenu(EditModifyMenuIdentifier, EditModifySnapMenuIdentifier, 100);
            {
                m_menuManagerInterface->AddActionToMenu(EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleAngleSnapping", 100);
                m_menuManagerInterface->AddActionToMenu(EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleGridSnapping", 200);
            }
            m_menuManagerInterface->AddSubMenuToMenu(EditModifyMenuIdentifier, EditModifyModesMenuIdentifier, 200);
        }
        m_menuManagerInterface->AddSeparatorToMenu(EditMenuIdentifier, 1900);
        m_menuManagerInterface->AddSubMenuToMenu(EditMenuIdentifier, EditSettingsMenuIdentifier, 2000);
        {
            m_menuManagerInterface->AddActionToMenu(EditSettingsMenuIdentifier, "o3de.action.edit.globalPreferences", 100);
            m_menuManagerInterface->AddActionToMenu(EditSettingsMenuIdentifier, "o3de.action.edit.editorSettingsManager", 200);
        }
    }

    // Game
    {
        m_menuManagerInterface->AddSubMenuToMenu(GameMenuIdentifier, PlayGameMenuIdentifier, 100);
        {
            m_menuManagerInterface->AddActionToMenu(PlayGameMenuIdentifier, "o3de.action.game.play", 100);
            m_menuManagerInterface->AddActionToMenu(PlayGameMenuIdentifier, "o3de.action.game.playMaximized", 200);
        }
        m_menuManagerInterface->AddActionToMenu(GameMenuIdentifier, "o3de.action.game.simulate", 200);
        m_menuManagerInterface->AddSeparatorToMenu(GameMenuIdentifier, 300);
        m_menuManagerInterface->AddActionToMenu(GameMenuIdentifier, "o3de.action.game.exportSelectedObjects", 400);
        m_menuManagerInterface->AddActionToMenu(GameMenuIdentifier, "o3de.action.game.exportOcclusionMesh", 500);
        m_menuManagerInterface->AddSeparatorToMenu(GameMenuIdentifier, 600);
        m_menuManagerInterface->AddActionToMenu(GameMenuIdentifier, "o3de.action.game.movePlayerAndCameraSeparately", 700);
        m_menuManagerInterface->AddSeparatorToMenu(GameMenuIdentifier, 800);
        m_menuManagerInterface->AddSubMenuToMenu(GameMenuIdentifier, GameAudioMenuIdentifier, 900);
        {
            m_menuManagerInterface->AddActionToMenu(GameAudioMenuIdentifier, "o3de.action.game.audio.stopAllSounds", 100);
            m_menuManagerInterface->AddActionToMenu(GameAudioMenuIdentifier, "o3de.action.game.audio.refresh", 200);
        }
        m_menuManagerInterface->AddSeparatorToMenu(GameMenuIdentifier, 1000);
        m_menuManagerInterface->AddSubMenuToMenu(GameMenuIdentifier, GameDebuggingMenuIdentifier, 1100);
        {
        }
    }

    // Help
    {
        m_menuManagerInterface->AddWidgetToMenu(HelpMenuIdentifier, "o3de.widgetAction.help.searchDocumentation", 100);
        m_menuManagerInterface->AddActionToMenu(HelpMenuIdentifier, "o3de.action.help.tutorials", 200);
        m_menuManagerInterface->AddSubMenuToMenu(HelpMenuIdentifier, HelpDocumentationMenuIdentifier, 300);
        {
            m_menuManagerInterface->AddActionToMenu(HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.o3de", 100);
            m_menuManagerInterface->AddActionToMenu(HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.gamelift", 200);
            m_menuManagerInterface->AddActionToMenu(HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.releasenotes", 300);
        }
        m_menuManagerInterface->AddSubMenuToMenu(HelpMenuIdentifier, HelpGameDevResourcesMenuIdentifier, 400);
        {
            m_menuManagerInterface->AddActionToMenu(HelpGameDevResourcesMenuIdentifier, "o3de.action.help.resources.gamedevblog", 100);
            m_menuManagerInterface->AddActionToMenu(HelpGameDevResourcesMenuIdentifier, "o3de.action.help.resources.forums", 200);
            m_menuManagerInterface->AddActionToMenu(HelpGameDevResourcesMenuIdentifier, "o3de.action.help.resources.awssupport", 300);
        }
        m_menuManagerInterface->AddSeparatorToMenu(HelpMenuIdentifier, 500);
        m_menuManagerInterface->AddActionToMenu(HelpMenuIdentifier, "o3de.action.help.abouto3de", 600);
        m_menuManagerInterface->AddActionToMenu(HelpMenuIdentifier, "o3de.action.help.welcome", 700);
    }
}

void EditorActionsHandler::InitializeToolBars()
{
    // Initialize ToolBars
    {
        AzToolsFramework::ToolBarProperties toolBarProperties;
        toolBarProperties.m_name = "Tools";
        m_toolBarManagerInterface->RegisterToolBar(ToolsToolBarIdentifier, toolBarProperties);
    }

    {
        AzToolsFramework::ToolBarProperties toolBarProperties;
        toolBarProperties.m_name = "Play Controls";
        m_toolBarManagerInterface->RegisterToolBar(PlayControlsToolBarIdentifier, toolBarProperties);
    }

    // Set the toolbars
    m_mainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_toolBarManagerInterface->GetToolBar(ToolsToolBarIdentifier));
    m_mainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, m_toolBarManagerInterface->GetToolBar(PlayControlsToolBarIdentifier));
    
    // Add actions to each toolbar

    // Play Controls
    {
        m_toolBarManagerInterface->AddWidgetToToolBar(PlayControlsToolBarIdentifier, "o3de.widgetAction.expander", 100);
        m_toolBarManagerInterface->AddSeparatorToToolBar(PlayControlsToolBarIdentifier, 200);
        m_toolBarManagerInterface->AddWidgetToToolBar(PlayControlsToolBarIdentifier, "o3de.widgetAction.game.playControlsLabel", 300);
        m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(PlayControlsToolBarIdentifier, "o3de.action.game.play", PlayGameMenuIdentifier, 400);
        m_toolBarManagerInterface->AddSeparatorToToolBar(PlayControlsToolBarIdentifier, 500);
        m_toolBarManagerInterface->AddActionToToolBar(PlayControlsToolBarIdentifier, "o3de.action.game.simulate", 600);
    }
}

QWidget* EditorActionsHandler::CreateExpander()
{
    QWidget* expander = new QWidget(m_mainWindow);
    expander->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    expander->setVisible(true);
    return expander;
}

QWidget* EditorActionsHandler::CreatePlayControlsLabel()
{
    QLabel* label = new QLabel(m_mainWindow);
    label->setText("Play Controls");
    return label;
}

QWidget* EditorActionsHandler::CreateDocsSearchWidget()
{
    QWidget* containerWidget = new QWidget(m_mainWindow);
    auto lineEdit = new AzQtComponents::SearchLineEdit(m_mainWindow);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(lineEdit);
    containerWidget->setLayout(layout);
    containerWidget->setContentsMargins(2, 0, 2, 0);
    lineEdit->setPlaceholderText(QObject::tr("Search documentation..."));

    auto searchAction = [lineEdit]()
    {
        auto text = lineEdit->text();
        if (text.isEmpty())
        {
            QDesktopServices::openUrl(QUrl("https://www.o3de.org/docs/"));
        }
        else
        {
            QUrl docSearchUrl("https://www.o3de.org/search/");
            QUrlQuery docSearchQuery;
            docSearchQuery.addQueryItem("query", text);
            docSearchUrl.setQuery(docSearchQuery);
            QDesktopServices::openUrl(docSearchUrl);
        }
        lineEdit->clear();
    };
    QObject::connect(lineEdit, &QLineEdit::returnPressed, m_mainWindow, searchAction);

    QMenu* helpMenu = m_menuManagerInternalInterface->GetMenu(HelpMenuIdentifier);

    QObject::connect(helpMenu, &QMenu::aboutToHide, lineEdit, &QLineEdit::clear);
    QObject::connect(helpMenu, &QMenu::aboutToShow, lineEdit, &QLineEdit::clearFocus);

    return containerWidget;
}

void EditorActionsHandler::OnViewPaneOpened(const char* viewPaneName)
{
    AZStd::string toolActionIdentifier = AZStd::string::format("o3de.action.tool.%s", viewPaneName);
    m_actionManagerInterface->UpdateAction(toolActionIdentifier);
}

void EditorActionsHandler::OnViewPaneClosed(const char* viewPaneName)
{
    AZStd::string toolActionIdentifier = AZStd::string::format("o3de.action.tool.%s", viewPaneName);
    m_actionManagerInterface->UpdateAction(toolActionIdentifier);
}

void EditorActionsHandler::OnStartPlayInEditor()
{
    m_actionManagerInterface->TriggerActionUpdater(GameModeStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnStopPlayInEditor()
{
    // Wait one frame for the game mode to actually be shut off.
    QTimer::singleShot(
        0,
        nullptr,
        [actionManagerInterface = m_actionManagerInterface]()
        {
            actionManagerInterface->TriggerActionUpdater(GameModeStateChangedUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::OnEntityStreamLoadSuccess()
{
    if (!m_isPrefabSystemEnabled)
    {
        m_actionManagerInterface->TriggerActionUpdater(LevelLoadedUpdaterIdentifier);
    }
}

void EditorActionsHandler::AfterEntitySelectionChanged(
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlySelectedEntities,
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
{
    m_actionManagerInterface->TriggerActionUpdater(EntitySelectionChangedUpdaterIdentifier);
}

void EditorActionsHandler::AfterUndoRedo()
{
    // Wait one frame for the undo stack to actually be updated.
    QTimer::singleShot(
        0,
        nullptr,
        [actionManagerInterface = m_actionManagerInterface]()
        {
            actionManagerInterface->TriggerActionUpdater(UndoRedoUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::OnEndUndo([[maybe_unused]] const char* label, [[maybe_unused]] bool changed)
{
    // Wait one frame for the undo stack to actually be updated.
    QTimer::singleShot(
        0,
        nullptr,
        [actionManagerInterface = m_actionManagerInterface]()
        {
            actionManagerInterface->TriggerActionUpdater(UndoRedoUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::OnAngleSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(AngleSnappingStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnGridSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(GridSnappingStateChangedUpdaterIdentifier);
}

bool EditorActionsHandler::IsRecentFileActionActive(int index)
{
    RecentFileList* recentFiles = m_cryEditApp->GetRecentFileList();
    return (index < recentFiles->GetSize());
}

void EditorActionsHandler::UpdateRecentFileActions()
{
    RecentFileList* recentFiles = m_cryEditApp->GetRecentFileList();
    const int recentFilesSize = recentFiles->GetSize();

    // Update all names
    for (int index = 0; index < maxRecentFiles; ++index)
    {
        AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);
        if (index < recentFilesSize)
        {
            m_actionManagerInterface->SetActionName(
                actionIdentifier, AZStd::string::format("%i | %s", index + 1, (*recentFiles)[index].toUtf8().data()));
        }
        else
        {
            m_actionManagerInterface->SetActionName(actionIdentifier, AZStd::string::format("Recent File #%i", index + 1));
        }
    }

    // Trigger the updater
    m_actionManagerInterface->TriggerActionUpdater(RecentFilesChangedUpdaterIdentifier);
}

void EditorActionsHandler::RefreshToolActions()
{
    // If the tools are being displayed in the menu or toolbar already, remove them.
    m_menuManagerInterface->RemoveActionsFromMenu(ToolsMenuIdentifier, m_toolActionIdentifiers);
    m_toolBarManagerInterface->RemoveActionsFromToolBar(ToolsToolBarIdentifier, m_toolActionIdentifiers);
    m_toolActionIdentifiers.clear();

    AZStd::vector<AZStd::pair<AZStd::string, int>> toolsMenuItems;
    AZStd::vector<AZStd::pair<AZStd::string, int>> toolsToolBarItems;

    // Place all actions in the same sort index in the menu and toolbar.
    // This will display them in order of addition (alphabetical) and ensure no external tool
    // can add items in-between tools without passing through the QtViewPanes system.
    const int sortKey = 0;

    // Get the tools list and refresh the menu.
    const QtViewPanes& viewpanes = m_qtViewPaneManager->GetRegisteredPanes();
    for (const auto& viewpane : viewpanes)
    {
        if (viewpane.IsViewportPane())
        {
            continue;
        }

        AZStd::string toolActionIdentifier = AZStd::string::format("o3de.action.tool.%s", viewpane.m_name.toUtf8().data());

        // Create the action if it does not already exist.
        if (m_actionManagerInternalInterface->GetAction(toolActionIdentifier) == nullptr)
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name =
                viewpane.m_options.optionalMenuText.length() > 0
                ? viewpane.m_options.optionalMenuText.toUtf8().data()
                : viewpane.m_name.toUtf8().data();
            actionProperties.m_category = "Tool";
            actionProperties.m_iconPath = viewpane.m_options.toolbarIcon;

            m_actionManagerInterface->RegisterCheckableAction(
                EditorMainWindowActionContextIdentifier,
                toolActionIdentifier,
                actionProperties,
                [viewpaneManager = m_qtViewPaneManager, viewpaneName = viewpane.m_name]
                {
                    viewpaneManager->TogglePane(viewpaneName);
                },
                [viewpaneManager = m_qtViewPaneManager, viewpaneName = viewpane.m_name]() -> bool
                {
                    return viewpaneManager->IsVisible(viewpaneName);
                }
            );
        }

        m_toolActionIdentifiers.push_back(toolActionIdentifier);

        if (viewpane.m_options.showInMenu)
        {
            toolsMenuItems.push_back({ toolActionIdentifier, sortKey });
        }

        if (viewpane.m_options.showOnToolsToolbar)
        {
            toolsToolBarItems.push_back({ toolActionIdentifier, sortKey });
        }
    }

    m_menuManagerInterface->AddActionsToMenu(ToolsMenuIdentifier, toolsMenuItems);
    m_toolBarManagerInterface->AddActionsToToolBar(ToolsToolBarIdentifier, toolsToolBarItems);
}
