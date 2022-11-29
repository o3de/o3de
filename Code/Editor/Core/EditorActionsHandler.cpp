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
#include <AzToolsFramework/Viewport/LocalViewBookmarkLoader.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzQtComponents/Components/Style.h>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

#include <Core/Widgets/PrefabEditVisualModeWidget.h>
#include <CryEdit.h>
#include <EditorCoreAPI.h>
#include <Editor/Undo/Undo.h>
#include <Editor/EditorViewportCamera.h>
#include <Editor/EditorViewportSettings.h>
#include <GameEngine.h>
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>
#include <MainWindow.h>
#include <QtViewPaneManager.h>
#include <ToolBox.h>
#include <ToolsConfigPage.h>
#include <Util/PathUtil.h>

#include <QDesktopServices>
#include <QDir>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWidget>

static constexpr AZStd::string_view EditorMainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";

static constexpr AZStd::string_view AngleSnappingStateChangedUpdaterIdentifier = "o3de.updater.onAngleSnappingStateChanged";
static constexpr AZStd::string_view DrawHelpersStateChangedUpdaterIdentifier = "o3de.updater.onViewportDrawHelpersStateChanged";
static constexpr AZStd::string_view EntitySelectionChangedUpdaterIdentifier = "o3de.updater.onEntitySelectionChanged";
static constexpr AZStd::string_view GameModeStateChangedUpdaterIdentifier = "o3de.updater.onGameModeStateChanged";
static constexpr AZStd::string_view GridSnappingStateChangedUpdaterIdentifier = "o3de.updater.onGridSnappingStateChanged";
static constexpr AZStd::string_view IconsStateChangedUpdaterIdentifier = "o3de.updater.onViewportIconsStateChanged";
static constexpr AZStd::string_view OnlyShowHelpersForSelectedEntitiesIdentifier =  "o3de.updater.onOnlyShowHelpersForSelectedEntitiesChanged";
static constexpr AZStd::string_view LevelLoadedUpdaterIdentifier = "o3de.updater.onLevelLoaded";
static constexpr AZStd::string_view RecentFilesChangedUpdaterIdentifier = "o3de.updater.onRecentFilesChanged";
static constexpr AZStd::string_view UndoRedoUpdaterIdentifier = "o3de.updater.onUndoRedo";
static constexpr AZStd::string_view ViewportDisplayInfoStateChangedUpdaterIdentifier = "o3de.updater.onViewportDisplayInfoStateChanged";

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
static constexpr AZStd::string_view ToolBoxMacrosMenuIdentifier = "o3de.menu.editor.toolbox.macros";
static constexpr AZStd::string_view ToolsMenuIdentifier = "o3de.menu.editor.tools";
static constexpr AZStd::string_view ViewMenuIdentifier = "o3de.menu.editor.view";
static constexpr AZStd::string_view LayoutsMenuIdentifier = "o3de.menu.editor.view.layouts";
static constexpr AZStd::string_view ViewportMenuIdentifier = "o3de.menu.editor.viewport";
static constexpr AZStd::string_view GoToLocationMenuIdentifier = "o3de.menu.editor.goToLocation";
static constexpr AZStd::string_view SaveLocationMenuIdentifier = "o3de.menu.editor.saveLocation";
static constexpr AZStd::string_view HelpMenuIdentifier = "o3de.menu.editor.help";
static constexpr AZStd::string_view HelpDocumentationMenuIdentifier = "o3de.menu.editor.help.documentation";
static constexpr AZStd::string_view HelpGameDevResourcesMenuIdentifier = "o3de.menu.editor.help.gamedevresources";

static constexpr AZStd::string_view EditorMainWindowTopToolBarAreaIdentifier = "o3de.toolbararea.editor.mainwindow.top";

static constexpr AZStd::string_view ToolsToolBarIdentifier = "o3de.toolbar.editor.tools";
static constexpr AZStd::string_view PlayControlsToolBarIdentifier = "o3de.toolbar.editor.playcontrols";

static const int maxRecentFiles = 10;

class EditorViewportDisplayInfoHandler
    : private AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler
{
public:
    EditorViewportDisplayInfoHandler()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(
            m_actionManagerInterface, "EditorViewportDisplayInfoHandler - could not get ActionManagerInterface on EditorViewportDisplayInfoHandler construction.");

        if (m_actionManagerInterface)
        {
            AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler::BusConnect();
        }
    }

    ~EditorViewportDisplayInfoHandler()
    {
        if (m_actionManagerInterface)
        {
            AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler::BusDisconnect();
        }
    }

    void OnViewportInfoDisplayStateChanged([[maybe_unused]] AZ::AtomBridge::ViewportInfoDisplayState state) override
    {
        m_actionManagerInterface->TriggerActionUpdater(ViewportDisplayInfoStateChangedUpdaterIdentifier);
    }

private:
    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
};

bool IsLevelLoaded()
{
    auto cryEdit = CCryEditApp::instance();
    return !cryEdit->IsExportingLegacyData() && GetIEditor()->IsLevelLoaded();
}

bool AreEntitiesSelected()
{
    bool result = false;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        result, &AzToolsFramework::ToolsApplicationRequestBus::Handler::AreAnyEntitiesSelected);
    return result;
}

static bool CompareLayoutNames(const QString& name1, const QString& name2)
{
    return name1.compare(name2, Qt::CaseInsensitive) < 0;
}

void EditorActionsHandler::Initialize(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    m_cryEditApp = CCryEditApp::instance();
    m_qtViewPaneManager = QtViewPaneManager::instance();

    m_levelExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

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

    // Retrieve the bookmark count from the loader.
    m_defaultBookmarkCount = AzToolsFramework::LocalViewBookmarkLoader::DefaultViewBookmarkCount;

    const int DefaultViewportId = 0;
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    AzToolsFramework::EditorEventsBus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(DefaultViewportId);

    m_editorViewportDisplayInfoHandler = new EditorViewportDisplayInfoHandler();

    m_initialized = true;
}

EditorActionsHandler::~EditorActionsHandler()
{
    if (m_initialized)
    {
        AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

        if (m_editorViewportDisplayInfoHandler)
        {
            delete m_editorViewportDisplayInfoHandler;
        }
    }
}

void EditorActionsHandler::OnActionContextRegistrationHook()
{
    AzToolsFramework::ActionContextProperties contextProperties;
    contextProperties.m_name = "O3DE Editor";

    m_actionManagerInterface->RegisterActionContext("", EditorMainWindowActionContextIdentifier, contextProperties, m_mainWindow);
}

void EditorActionsHandler::OnActionUpdaterRegistrationHook()
{
    m_actionManagerInterface->RegisterActionUpdater(AngleSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(DrawHelpersStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(OnlyShowHelpersForSelectedEntitiesIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EntitySelectionChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(GameModeStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(GridSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(IconsStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(RecentFilesChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(UndoRedoUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(ViewportDisplayInfoStateChangedUpdaterIdentifier);

    // If the Prefab system is not enabled, have a backup to update actions based on level loading.
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        m_isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (!m_isPrefabSystemEnabled)
    {
        m_actionManagerInterface->RegisterActionUpdater(LevelLoadedUpdaterIdentifier);
    }
}

void EditorActionsHandler::OnActionRegistrationHook()
{
    // --- File Actions

    // New Level
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.new";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "New Level";
        actionProperties.m_description = "Create a new level";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnCreateLevel();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.file.new", "Ctrl+N");
    }

    // Open Level
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.open";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open Level...";
        actionProperties.m_description = "Open an existing level";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnOpenLevel();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

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
            actionProperties.m_hideFromMenusWhenDisabled = false;

            AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);

            m_actionManagerInterface->RegisterAction(
                EditorMainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [&, index]
                {
                    OpenLevelByRecentFileEntryIndex(index);
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

            // This action is only accessible outside of Component Modes
            m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
        }
    }

    // Clear Recent Files
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.recent.clearAll";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Clear All";
        actionProperties.m_description = "Clear the recent files list.";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [&]
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

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
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
            [cryEdit = m_cryEditApp]
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
            []
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
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.saveLevelStatistics";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save Level Statistics";
        actionProperties.m_description = "Logs Editor memory usage.";
        actionProperties.m_category = "Level";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnToolsLogMemoryUsage();
            }
        );

        // This action is required by python tests, but is always disabled.
        m_actionManagerInterface->InstallEnabledStateCallback(
            actionIdentifier,
            []() -> bool
            {
                return false;
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Edit Project Settings
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.project.editSettings";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Edit Project Settings...";
        actionProperties.m_description = "Open the Project Settings panel.";
        actionProperties.m_category = "Project";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnOpenProjectManagerSettings();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Edit Platform Settings
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.platform.editSettings";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Edit Platform Settings...";
        actionProperties.m_description = "Open the Platform Settings panel.";
        actionProperties.m_category = "Platform";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [qtViewPaneManager = m_qtViewPaneManager]
            {
                qtViewPaneManager->OpenPane(LyViewPane::ProjectSettingsTool);
            }
        );
    }

    // New Project
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.project.new";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "New Project...";
        actionProperties.m_description = "Create a new project in the Project Manager.";
        actionProperties.m_category = "Project";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnOpenProjectManagerNew();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Open Project
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.project.open";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Open Project...";
        actionProperties.m_description = "Open a different project in the Project Manager.";
        actionProperties.m_category = "Project";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnOpenProjectManager();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Show Log File
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.showLog";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Log File";
        actionProperties.m_category = "Project";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnFileEditLogFile();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
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
            [=]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.snap.toggleAngleSnapping";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Angle snapping";
        actionProperties.m_description = "Toggle angle snapping";
        actionProperties.m_category = "Edit";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Angle.svg";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                SandboxEditor::SetAngleSnapping(!SandboxEditor::AngleSnappingEnabled());
            },
            []() -> bool
            {
                return SandboxEditor::AngleSnappingEnabled();
            }
        );

        // Trigger update when the angle snapping setting changes
        m_actionManagerInterface->AddActionToUpdater(AngleSnappingStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Grid Snapping
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.snap.toggleGridSnapping";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Grid snapping";
        actionProperties.m_description = "Toggle grid snapping";
        actionProperties.m_category = "Edit";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Grid.svg";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                SandboxEditor::SetGridSnapping(!SandboxEditor::GridSnappingEnabled());
            },
            []() -> bool
            {
                return SandboxEditor::GridSnappingEnabled();
            }
        );

        // Trigger update when the grid snapping setting changes
        m_actionManagerInterface->AddActionToUpdater(GridSnappingStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Global Preferences
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.globalPreferences";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Global Preferences...";
        actionProperties.m_category = "Editor";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnToolsPreferences();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Editor Settings Manager
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.editorSettingsManager";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Editor Settings Manager";
        actionProperties.m_category = "Editor";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                QtViewPaneManager::instance()->OpenPane(LyViewPane::EditorSettingsManager);
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // --- Game Actions

    // Play Game
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.play";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Play Game";
        actionProperties.m_description = "Activate the game input mode.";
        actionProperties.m_category = "Game";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Play.svg";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnViewSwitchToGame();
            },
            []
            {
                return GetIEditor()->IsInGameMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Ctrl+G");
    }

    // Play Game (Maximized)
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.playMaximized";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Play Game (Maximized)";
        actionProperties.m_description = "Activate the game input mode (maximized).";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnViewSwitchToGameFullScreen();
            },
            []
            {
                return GetIEditor()->IsInGameMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Simulate
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.simulate";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Simulate";
        actionProperties.m_description = "Enable processing of Physics and AI.";
        actionProperties.m_category = "Game";
        actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Simulate_Physics.svg";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnSwitchPhysics();
            },
            [cryEdit = m_cryEditApp]
            {
                return !cryEdit->IsExportingLegacyData() && GetIEditor()->GetGameEngine()->GetSimulationMode();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(GameModeStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Export Selected Objects
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.exportSelectedObjects";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Export Selected Objects";
        actionProperties.m_description = "Export Selected Objects.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnExportSelectedObjects();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, AreEntitiesSelected);
        m_actionManagerInterface->AddActionToUpdater(EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Export Occlusion Mesh
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.exportOcclusionMesh";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Export Occlusion Mesh";
        actionProperties.m_description = "Export Occlusion Mesh.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnFileExportOcclusionMesh();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Move Player and Camera Separately
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.movePlayerAndCameraSeparately";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Move Player and Camera Separately";
        actionProperties.m_description = "Move Player and Camera Separately.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                GetIEditor()->GetGameEngine()->SyncPlayerPosition(!GetIEditor()->GetGameEngine()->IsSyncPlayerPosition());
            },
            []
            {
                return !GetIEditor()->GetGameEngine()->IsSyncPlayerPosition();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Stop All Sounds
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.audio.stopAllSounds";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Stop All Sounds";
        actionProperties.m_description = "Stop All Sounds.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
                    &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Refresh Audio System
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.audio.refresh";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Refresh";
        actionProperties.m_description = "Refresh Audio System.";
        actionProperties.m_category = "Game";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
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

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Error Report
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.debugging.errorDialog";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Error Report";
        actionProperties.m_description = "Open the Error Report dialog.";
        actionProperties.m_category = "Debugging";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [qtViewPaneManager = m_qtViewPaneManager]
            {
                qtViewPaneManager->OpenPane(LyViewPane::ErrorReport);
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Configure Toolbox Macros
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.game.debugging.toolboxMacros";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Configure Toolbox Macros...";
        actionProperties.m_description = "Open the Toolbox Macros dialog.";
        actionProperties.m_category = "Debugging";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [&]
            {
                ToolsConfigDialog dlg;
                if (dlg.exec() == QDialog::Accepted)
                {
                    RefreshToolboxMacroActions();
                }
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // --- View Actions

    // Component Entity Layout
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.layout.componentEntityLayout";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Component Entity Layout (Default)";
        actionProperties.m_category = "Layout";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                m_mainWindow->m_viewPaneManager->RestoreDefaultLayout();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Save Layout...
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.layout.save";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save Layout...";
        actionProperties.m_description = "Save the current layout.";
        actionProperties.m_category = "Layout";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                m_mainWindow->SaveLayout();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Restore Default Layout
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.layout.restoreDefault";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Restore Default Layout";
        actionProperties.m_description = "Restored the default layout for the Editor.";
        actionProperties.m_category = "Layout";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                m_mainWindow->m_viewPaneManager->RestoreDefaultLayout(true);
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Go to Position...
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.goToPosition";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Go to Position...";
        actionProperties.m_description = "Move the editor camera to the position and rotation provided.";
        actionProperties.m_category = "View";
        actionProperties.m_iconPath = ":/Menu/camera.svg";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnDisplayGotoPosition();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);
    }

    // Center on Selection
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.centerOnSelection";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Center on Selection";
        actionProperties.m_description = "Center the viewport to show selected entities.";
        actionProperties.m_category = "View";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::GoToSelectedEntitiesInViewports);
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, AreEntitiesSelected);
        m_actionManagerInterface->AddActionToUpdater(EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Z");
    }

    // View Bookmarks
    InitializeViewBookmarkActions();

    // Show Helpers
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.toggleHelpers";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Helpers";
        actionProperties.m_description = "Show/Hide Helpers.";
        actionProperties.m_category = "View";
        actionProperties.m_iconPath = ":/Menu/helpers.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::SetHelpersVisible(!AzToolsFramework::HelpersVisible());
                AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Broadcast(
                    &AzToolsFramework::ViewportInteraction::ViewportSettingNotifications::OnDrawHelpersChanged,
                    AzToolsFramework::HelpersVisible());
            },
            []
            {
                return AzToolsFramework::HelpersVisible();
            }
        );

        m_actionManagerInterface->AddActionToUpdater(DrawHelpersStateChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Shift+Space");
    }

    // Show Icons
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.toggleIcons";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Icons";
        actionProperties.m_description = "Show/Hide Icons.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::SetIconsVisible(!AzToolsFramework::IconsVisible());
                AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Broadcast(
                    &AzToolsFramework::ViewportInteraction::ViewportSettingNotifications::OnIconsVisibilityChanged,
                    AzToolsFramework::IconsVisible());
            },
            []
            {
                return AzToolsFramework::IconsVisible();
            }
        );

        m_actionManagerInterface->AddActionToUpdater(IconsStateChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Ctrl+Space");
    }

    // Only Show Helpers for Selected Entities
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.toggleSelectedEntityHelpers";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Helpers for Selected Entities Only";
        actionProperties.m_description = "If enabled, shows Helpers for selected entities only. By default, shows Helpers for all entities.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::SetOnlyShowHelpersForSelectedEntities(!AzToolsFramework::OnlyShowHelpersForSelectedEntities());
                AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Broadcast(
                    &AzToolsFramework::ViewportInteraction::ViewportSettingNotifications::OnOnlyShowHelpersForSelectedEntitiesChanged,
                    AzToolsFramework::OnlyShowHelpersForSelectedEntities());
            },
            []
            {
                return AzToolsFramework::OnlyShowHelpersForSelectedEntities();
            });

        m_actionManagerInterface->AddActionToUpdater(OnlyShowHelpersForSelectedEntitiesIdentifier, actionIdentifier);
    }

    // Refresh Style
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.refreshEditorStyle";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Refresh Style";
        actionProperties.m_description = "Refreshes the editor stylesheet.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                GetIEditor()->Notify(eNotify_OnStyleChanged);
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
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
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnAppAbout();
            }
        );
    }

    // Welcome
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.help.welcome";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&Welcome";
        actionProperties.m_category = "Help";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnAppShowWelcomeScreen();
            }
        );

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }
}

void EditorActionsHandler::OnWidgetActionRegistrationHook()
{
    // Help - Search Documentation Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Search Documentation";
        widgetActionProperties.m_category = "Help";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.help.searchDocumentation",
            widgetActionProperties,
            [&]
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
            [&]
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
            [&]
            {
                return CreatePlayControlsLabel();
            }
        );
    }

    // Prefab Edit Visual Mode Selection Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Prefab Edit Visual Mode Selection";
        widgetActionProperties.m_category = "Prefabs";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.prefab.editVisualMode",
            widgetActionProperties,
            []() -> QWidget*
            {
                return new PrefabEditVisualModeWidget();
            }
        );
    }
}     

void EditorActionsHandler::OnMenuBarRegistrationHook()
{
    // Register MenuBar
    m_menuManagerInterface->RegisterMenuBar(EditorMainWindowMenuBarIdentifier, m_mainWindow);
}

void EditorActionsHandler::OnMenuRegistrationHook()
{
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
                [&]
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
                menuProperties.m_name = "ToolBox Macros";
                m_menuManagerInterface->RegisterMenu(ToolBoxMacrosMenuIdentifier, menuProperties);
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
            menuProperties.m_name = "Layouts";
            m_menuManagerInterface->RegisterMenu(LayoutsMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Viewport";
            m_menuManagerInterface->RegisterMenu(ViewportMenuIdentifier, menuProperties);
        }
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Go to Location";
                m_menuManagerInterface->RegisterMenu(GoToLocationMenuIdentifier, menuProperties);
            }
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Save Location";
                m_menuManagerInterface->RegisterMenu(SaveLocationMenuIdentifier, menuProperties);
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
}

void EditorActionsHandler::OnMenuBindingHook()
{
    // Add Menus to MenuBar
    // We space the sortkeys by 100 to allow external systems to add menus in-between.
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, FileMenuIdentifier, 100);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, EditMenuIdentifier, 200);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, GameMenuIdentifier, 300);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, ToolsMenuIdentifier, 400);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, ViewMenuIdentifier, 500);
    m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, HelpMenuIdentifier, 600);

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
                m_menuManagerInterface->AddActionToMenu(EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleGridSnapping", 100);
                m_menuManagerInterface->AddActionToMenu(EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleAngleSnapping", 200);
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
            m_menuManagerInterface->AddActionToMenu(GameDebuggingMenuIdentifier, "o3de.action.game.debugging.errorDialog", 100);
            m_menuManagerInterface->AddSeparatorToMenu(GameDebuggingMenuIdentifier, 200);
            m_menuManagerInterface->AddSubMenuToMenu(GameDebuggingMenuIdentifier, ToolBoxMacrosMenuIdentifier, 300);
            {
                // Some of the contents of the ToolBox Macros menu are initialized in RefreshToolboxMacrosActions.

                m_menuManagerInterface->AddSeparatorToMenu(ToolBoxMacrosMenuIdentifier, 200);
                m_menuManagerInterface->AddActionToMenu(ToolBoxMacrosMenuIdentifier, "o3de.action.game.debugging.toolboxMacros", 300);
            }
        }
    }

    // View
    {
        m_menuManagerInterface->AddSubMenuToMenu(ViewMenuIdentifier, LayoutsMenuIdentifier, 100);
        {
            m_menuManagerInterface->AddActionToMenu(LayoutsMenuIdentifier, "o3de.action.layout.componentEntityLayout", 100);
            m_menuManagerInterface->AddSeparatorToMenu(LayoutsMenuIdentifier, 200);

            // Some of the contents of the Layouts menu are initialized in RefreshLayoutActions.

            m_menuManagerInterface->AddSeparatorToMenu(LayoutsMenuIdentifier, 400);
            m_menuManagerInterface->AddActionToMenu(LayoutsMenuIdentifier, "o3de.action.layout.save", 500);
            m_menuManagerInterface->AddActionToMenu(LayoutsMenuIdentifier, "o3de.action.layout.restoreDefault", 600);
        }
        m_menuManagerInterface->AddSubMenuToMenu(ViewMenuIdentifier, ViewportMenuIdentifier, 200);
        {
            m_menuManagerInterface->AddActionToMenu(ViewportMenuIdentifier, "o3de.action.view.goToPosition", 100);
            m_menuManagerInterface->AddActionToMenu(ViewportMenuIdentifier, "o3de.action.view.centerOnSelection", 200);
            m_menuManagerInterface->AddSubMenuToMenu(ViewportMenuIdentifier, GoToLocationMenuIdentifier, 300);
            {
                for (int index = 0; index < m_defaultBookmarkCount; ++index)
                {
                    const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].goTo", index);
                    m_menuManagerInterface->AddActionToMenu(GoToLocationMenuIdentifier, actionIdentifier, 0);
                }
            }
            m_menuManagerInterface->AddSubMenuToMenu(ViewportMenuIdentifier, SaveLocationMenuIdentifier, 400);
            {
                for (int index = 0; index < m_defaultBookmarkCount; ++index)
                {
                    const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].save", index);
                    m_menuManagerInterface->AddActionToMenu(SaveLocationMenuIdentifier, actionIdentifier, 0);
                }
            }
            m_menuManagerInterface->AddSeparatorToMenu(ViewportMenuIdentifier, 500);
            m_menuManagerInterface->AddActionToMenu(ViewportMenuIdentifier, "o3de.action.view.toggleHelpers", 600);
            m_menuManagerInterface->AddActionToMenu(ViewportMenuIdentifier, "o3de.action.view.toggleIcons", 700);
            m_menuManagerInterface->AddActionToMenu(ViewportMenuIdentifier, "o3de.action.view.toggleSelectedEntityHelpers", 800);
        }
        m_menuManagerInterface->AddActionToMenu(ViewMenuIdentifier, "o3de.action.view.refreshEditorStyle", 300);
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

void EditorActionsHandler::OnToolBarAreaRegistrationHook()
{
    m_toolBarManagerInterface->RegisterToolBarArea(
        EditorMainWindowTopToolBarAreaIdentifier, m_mainWindow, Qt::ToolBarArea::TopToolBarArea);
}

void EditorActionsHandler::OnToolBarRegistrationHook()
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
}

void EditorActionsHandler::OnToolBarBindingHook()
{
    // Add ToolBars to ToolBar Areas
    // We space the sortkeys by 100 to allow external systems to add toolbars in-between.
    m_toolBarManagerInterface->AddToolBarToToolBarArea(EditorMainWindowTopToolBarAreaIdentifier, ToolsToolBarIdentifier, 100);
    m_toolBarManagerInterface->AddToolBarToToolBarArea(EditorMainWindowTopToolBarAreaIdentifier, PlayControlsToolBarIdentifier, 200);

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

void EditorActionsHandler::OnPostActionManagerRegistrationHook()
{
    // Ensure the layouts menu is refreshed when the layouts list changes.
    QObject::connect(
        m_mainWindow->m_viewPaneManager, &QtViewPaneManager::savedLayoutsChanged, m_mainWindow,
        [&]
        {
            RefreshLayoutActions();
        }
    );

    RefreshLayoutActions();

    // Ensure the tools menu and toolbar are refreshed when the viewpanes change.
    QObject::connect(
        m_qtViewPaneManager, &QtViewPaneManager::registeredPanesChanged, m_mainWindow,
        [&]
        {
            RefreshToolActions();
        }
    );
    
    RefreshToolActions();

    // Initialize the Toolbox Macro actions
    RefreshToolboxMacroActions();
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

    auto searchAction = [lineEdit]
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
        [actionManagerInterface = m_actionManagerInterface]
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
        [actionManagerInterface = m_actionManagerInterface]
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
        [actionManagerInterface = m_actionManagerInterface]
        {
            actionManagerInterface->TriggerActionUpdater(UndoRedoUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::OnAngleSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(AngleSnappingStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnDrawHelpersChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(DrawHelpersStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnGridSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(GridSnappingStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnIconsVisibilityChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(IconsStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnOnlyShowHelpersForSelectedEntitiesChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(OnlyShowHelpersForSelectedEntitiesIdentifier);
}

bool EditorActionsHandler::IsRecentFileActionActive(int index)
{
    return (index < m_recentFileActionsCount);
}

bool EditorActionsHandler::IsRecentFileEntryValid(const QString& entry, const QString& gameFolderPath)
{
    if (entry.isEmpty())
    {
        return false;
    }

    QFileInfo info(entry);
    if (!info.exists())
    {
        return false;
    }

    if (!entry.endsWith(m_levelExtension))
    {
        return false;
    }

    const QDir gameDir(gameFolderPath);
    QDir dir(entry); // actually pointing at file, first cdUp() gets us the parent dir
    while (dir.cdUp())
    {
        if (dir == gameDir)
        {
            return true;
        }
    }

    return false;
}

void EditorActionsHandler::OpenLevelByRecentFileEntryIndex(int index)
{
    // Out of bounds, do nothing
    if (index >= m_recentFileActionsCount)
    {
        return;
    }

    RecentFileList* recentFiles = m_cryEditApp->GetRecentFileList();
    const int recentFilesSize = recentFiles->GetSize();

    QString sCurDir = QString(Path::GetEditingGameDataFolder().c_str()) + QDir::separator();
    QFileInfo gameDir(sCurDir); // Pass it through QFileInfo so it comes out normalized
    const QString gameDirPath = gameDir.absolutePath();

    // Index is the index of the action in the menu, but in generating that list we skipped invalid files from other projects.
    // As such, we need to actually go through the list again to find the correct index for the recentFiles array.

    int counter = 0;
    int fileIndex = 0;
    for (; fileIndex < recentFilesSize; ++fileIndex)
    {
        if (!IsRecentFileEntryValid((*recentFiles)[fileIndex], gameDirPath))
        {
            continue;
        }

        if (counter == index)
        {
            break;
        }

        ++counter;
    }

    m_cryEditApp->OpenDocumentFile((*recentFiles)[fileIndex].toUtf8().data(), true, COpenSameLevelOptions::ReopenLevelIfSame);
    
}

void EditorActionsHandler::UpdateRecentFileActions()
{
    RecentFileList* recentFiles = m_cryEditApp->GetRecentFileList();
    const int recentFilesSize = recentFiles->GetSize();

    QString sCurDir = QString(Path::GetEditingGameDataFolder().c_str()) + QDir::separator();
    QFileInfo gameDir(sCurDir); // Pass it through QFileInfo so it comes out normalized
    const QString gameDirPath = gameDir.absolutePath();

    m_recentFileActionsCount = 0;
    int counter = 0;

    // Update all names
    for (int index = 0; (index < maxRecentFiles) || (index < recentFilesSize); ++index)
    {
        if (!IsRecentFileEntryValid((*recentFiles)[index], gameDirPath))
        {
            continue;
        }

        AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", counter + 1);

        if (index < recentFilesSize)
        {
            QString displayName;
            recentFiles->GetDisplayName(displayName, index, sCurDir);

            m_actionManagerInterface->SetActionName(
                actionIdentifier, AZStd::string::format("%i | %s", counter + 1, displayName.toUtf8().data()));

            ++m_recentFileActionsCount;
        }
        else
        {
            m_actionManagerInterface->SetActionName(actionIdentifier, AZStd::string::format("Recent File #%i", counter + 1));
        }

        ++counter;
    }

    // Trigger the updater
    m_actionManagerInterface->TriggerActionUpdater(RecentFilesChangedUpdaterIdentifier);
}

void EditorActionsHandler::RefreshLayoutActions()
{
    m_menuManagerInterface->RemoveSubMenusFromMenu(LayoutsMenuIdentifier, m_layoutMenuIdentifiers);
    m_layoutMenuIdentifiers.clear();

    // Place all sub-menus in the same sort index in the menu.
    // This will display them in order of addition (alphabetical) and ensure no external tool can add items in-between
    const int sortKey = 300;

    QStringList layoutNames = m_mainWindow->m_viewPaneManager->LayoutNames();
    std::sort(layoutNames.begin(), layoutNames.end(), CompareLayoutNames);

    for (const auto& layoutName : layoutNames)
    {
        AZStd::string layoutMenuIdentifier = AZStd::string::format("o3de.menu.layout[%s]", layoutName.toUtf8().data());

        // Create the menu and related actions for the layout if they do not already exist.
        if (!m_menuManagerInterface->IsMenuRegistered(layoutMenuIdentifier))
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = layoutName.toUtf8().data();
            m_menuManagerInterface->RegisterMenu(layoutMenuIdentifier, menuProperties);

            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.layout[%s].load", layoutName.toUtf8().data());
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Load";
                actionProperties.m_description = AZStd::string::format("Load the \"%s\" layout.", layoutName.toUtf8().data());
                actionProperties.m_category = "Layout";
                actionProperties.m_hideFromMenusWhenDisabled = false;

                m_actionManagerInterface->RegisterAction(
                    EditorMainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [layout = layoutName, this]
                    {
                        m_mainWindow->ViewLoadPaneLayout(layout);
                    }
                );

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

                m_menuManagerInterface->AddActionToMenu(layoutMenuIdentifier, actionIdentifier, 0);
            }

            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.layout[%s].save", layoutName.toUtf8().data());
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Save";
                actionProperties.m_description = AZStd::string::format("Save the \"%s\" layout.", layoutName.toUtf8().data());
                actionProperties.m_category = "Layout";
                actionProperties.m_hideFromMenusWhenDisabled = false;

                m_actionManagerInterface->RegisterAction(
                    EditorMainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [layout = layoutName, this]
                    {
                        m_mainWindow->ViewSavePaneLayout(layout);
                    }
                );

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

                m_menuManagerInterface->AddActionToMenu(layoutMenuIdentifier, actionIdentifier, 100);
            }

            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.layout[%s].rename", layoutName.toUtf8().data());
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Rename...";
                actionProperties.m_description = AZStd::string::format("Rename the \"%s\" layout.", layoutName.toUtf8().data());
                actionProperties.m_category = "Layout";
                actionProperties.m_hideFromMenusWhenDisabled = false;

                m_actionManagerInterface->RegisterAction(
                    EditorMainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [layout = layoutName, this]
                    {
                        m_mainWindow->ViewRenamePaneLayout(layout);
                    }
                );

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

                m_menuManagerInterface->AddActionToMenu(layoutMenuIdentifier, actionIdentifier, 200);
            }

            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.layout[%s].delete", layoutName.toUtf8().data());
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = "Delete";
                actionProperties.m_description = AZStd::string::format("Delete the \"%s\" layout.", layoutName.toUtf8().data());
                actionProperties.m_category = "Layout";
                actionProperties.m_hideFromMenusWhenDisabled = false;

                m_actionManagerInterface->RegisterAction(
                    EditorMainWindowActionContextIdentifier,
                    actionIdentifier,
                    actionProperties,
                    [layout = layoutName, this]
                    {
                        m_mainWindow->ViewDeletePaneLayout(layout);
                    }
                );

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);

                m_menuManagerInterface->AddActionToMenu(layoutMenuIdentifier, actionIdentifier, 300);
            }
        }

        m_layoutMenuIdentifiers.push_back(layoutMenuIdentifier);
        m_menuManagerInterface->AddSubMenuToMenu(LayoutsMenuIdentifier, layoutMenuIdentifier, sortKey);
    }
}

void EditorActionsHandler::RefreshToolboxMacroActions()
{
    // If the toolbox macros are being displayed in the menu already, remove them.
    m_menuManagerInterface->RemoveActionsFromMenu(ToolBoxMacrosMenuIdentifier, m_toolboxMacroActionIdentifiers);
    m_toolboxMacroActionIdentifiers.clear();

    // Place all actions in the same sort index in the menu .
    // This will display them in order of addition (alphabetical).
    const int sortKey = 0;

    auto tools = GetIEditor()->GetToolBoxManager();
    const int macroCount = tools->GetMacroCount(true);

    for (int macroIndex = 0; macroIndex < macroCount; ++macroIndex)
    {
        auto macro = tools->GetMacro(macroIndex, true);
        const int toolbarId = macro->GetToolbarId();
        if (toolbarId == -1 || toolbarId == ID_TOOLS_TOOL1)
        {
            AZStd::string toolboxMacroActionIdentifier = AZStd::string::format("o3de.action.toolboxMacro[%s]", macro->GetTitle().toStdString().c_str());

            // Create the action if it does not already exist.
            if (!m_actionManagerInterface->IsActionRegistered(toolboxMacroActionIdentifier))
            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = macro->GetTitle().toStdString().c_str();
                actionProperties.m_category = "Toolbox Macro";
                actionProperties.m_iconPath = macro->GetIconPath().toStdString().c_str();
                actionProperties.m_hideFromMenusWhenDisabled = false;

                m_actionManagerInterface->RegisterAction(
                    EditorMainWindowActionContextIdentifier,
                    toolboxMacroActionIdentifier,
                    actionProperties,
                    [macro]
                    {
                        macro->Execute();
                    }
                );

                // This action is only accessible outside of Component Modes
                m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, toolboxMacroActionIdentifier);
            }

            m_menuManagerInterface->AddActionToMenu(ToolBoxMacrosMenuIdentifier, toolboxMacroActionIdentifier, sortKey);
            m_toolboxMacroActionIdentifiers.push_back(AZStd::move(toolboxMacroActionIdentifier));
        }
    }
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
        if (!m_actionManagerInterface->IsActionRegistered(toolActionIdentifier))
        {
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = viewpane.m_options.optionalMenuText.length() > 0 ? viewpane.m_options.optionalMenuText.toUtf8().data()
                                                                                       : viewpane.m_name.toUtf8().data();
            actionProperties.m_category = "Tool";
            actionProperties.m_iconPath = viewpane.m_options.toolbarIcon;
            actionProperties.m_hideFromMenusWhenDisabled = false;

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

            // This action is only accessible outside of Component Modes
            m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, toolActionIdentifier);
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

void EditorActionsHandler::InitializeViewBookmarkActions()
{
    // --- Go to Location
    for (int index = 0; index < m_defaultBookmarkCount; ++index)
    {
        const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].goTo", index);

        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AZStd::string::format("Go to Location %i", index+1);
        actionProperties.m_description = AZStd::string::format("Go to Location %i.", index+1);
        actionProperties.m_category = "View Bookmark";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        auto outcome = m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [index]
            {
                AzToolsFramework::ViewBookmarkInterface* viewBookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();
                if (!viewBookmarkInterface)
                {
                    AZ_Warning("Main Window", false, "Couldn't find View Bookmark Loader");
                    return false;
                }

                const AZStd::optional<AzToolsFramework::ViewBookmark> bookmark = viewBookmarkInterface->LoadBookmarkAtIndex(index);
                if (!bookmark.has_value())
                {
                    return false;
                }

                // Check the bookmark we want to load is not exactly 0.
                if (bookmark.value().IsZero())
                {
                    QString tagConsoleText = QObject::tr("View Bookmark %1 has not been set yet").arg(index + 1);
                    AZ_Warning("Main Window", false, tagConsoleText.toUtf8().data());
                    return false;
                }

                SandboxEditor::InterpolateDefaultViewportCameraToTransform(
                    bookmark->m_position, AZ::DegToRad(bookmark->m_rotation.GetX()), AZ::DegToRad(bookmark->m_rotation.GetZ()));

                QString tagConsoleText = QObject::tr("View Bookmark %1 loaded position: x=%2, y=%3, z=%4")
                                             .arg(index + 1)
                                             .arg(bookmark->m_position.GetX(), 0, 'f', 2)
                                             .arg(bookmark->m_position.GetY(), 0, 'f', 2)
                                             .arg(bookmark->m_position.GetZ(), 0, 'f', 2);

                AZ_Printf("MainWindow", tagConsoleText.toUtf8().data());
                return true;
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, AZStd::string::format("Shift+F%i", index+1));
    }

    // --- Save Location
    for (int index = 0; index < m_defaultBookmarkCount; ++index)
    {
        const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].save", index);

        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AZStd::string::format("Save Location %i", index+1);
        actionProperties.m_description = AZStd::string::format("Save Location %i.", index+1);
        actionProperties.m_category = "View Bookmark";
        actionProperties.m_hideFromMenusWhenDisabled = false;

        m_actionManagerInterface->RegisterAction(
            EditorMainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [index]
            {
                if (auto viewBookmark = AzToolsFramework::StoreViewBookmarkFromActiveCameraAtIndex(index); viewBookmark.has_value())
                {
                    const QString tagConsoleText = QObject::tr("View Bookmark %1 set to the position: x=%2, y=%3, z=%4")
                                                       .arg(index + 1)
                                                       .arg(viewBookmark->m_position.GetX(), 0, 'f', 2)
                                                       .arg(viewBookmark->m_position.GetY(), 0, 'f', 2)
                                                       .arg(viewBookmark->m_position.GetZ(), 0, 'f', 2);

                    AZ_Printf("MainWindow", tagConsoleText.toUtf8().data());
                }
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(LevelLoadedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, AZStd::string::format("Ctrl+F%i", index+1));
    }
}
