/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/EditorActionsHandler.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorToolBarIdentifiers.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerRequestBus.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkLoader.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzQtComponents/Components/Style.h>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

#include <Core/Widgets/PrefabEditVisualModeWidget.h>
#include <Core/Widgets/ViewportSettingsWidgets.h>
#include <CryEdit.h>
#include <EditorCoreAPI.h>
#include <Editor/EditorViewportCamera.h>
#include <Editor/EditorViewportSettings.h>
#include <Editor/Undo/Undo.h>
#include <GameEngine.h>
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>
#include <MainWindow.h>
#include <QtViewPaneManager.h>
#include <ToolBox.h>
#include <ToolsConfigPage.h>
#include <Util/PathUtil.h>

#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWidget>

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
        m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier);
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

    // Get EditorEntityContextId
    AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
        editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    AzToolsFramework::EditorEventsBus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(DefaultViewportId);
    AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(editorEntityContextId);
    AzToolsFramework::ContainerEntityNotificationBus::Handler::BusConnect(editorEntityContextId);

    m_editorViewportDisplayInfoHandler = new EditorViewportDisplayInfoHandler();

    m_initialized = true;
}

EditorActionsHandler::~EditorActionsHandler()
{
    if (m_initialized)
    {
        AzToolsFramework::ContainerEntityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
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
    // Main Window
    {
        AzToolsFramework::ActionContextProperties contextProperties;
        contextProperties.m_name = "O3DE Editor";

        m_actionManagerInterface->RegisterActionContext(
            EditorIdentifiers::MainWindowActionContextIdentifier, contextProperties);

        m_hotKeyManagerInterface->AssignWidgetToActionContext(EditorIdentifiers::MainWindowActionContextIdentifier, m_mainWindow);
    }

    // Editor Asset Browser
    {
        AzToolsFramework::ActionContextProperties contextProperties;
        contextProperties.m_name = "O3DE Editor - Asset Browser";

        m_actionManagerInterface->RegisterActionContext(
            EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, contextProperties);
    }

    // Editor Console
    {
        AzToolsFramework::ActionContextProperties contextProperties;
        contextProperties.m_name = "O3DE Editor - Console";

        m_actionManagerInterface->RegisterActionContext(
            EditorIdentifiers::EditorConsoleActionContextIdentifier, contextProperties);
    }

    // Editor Entity Property Editor (Entity Inspector)
    {
        AzToolsFramework::ActionContextProperties contextProperties;
        contextProperties.m_name = "O3DE Editor - Entity Inspector";

        m_actionManagerInterface->RegisterActionContext(
            EditorIdentifiers::EditorEntityPropertyEditorActionContextIdentifier, contextProperties);
    }
}

void EditorActionsHandler::OnActionUpdaterRegistrationHook()
{
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::AngleSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::ContainerEntityStatesChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::EntityPickingModeChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::GridSnappingStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::IconsStateChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::RecentFilesChangedUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::UndoRedoUpdaterIdentifier);
    m_actionManagerInterface->RegisterActionUpdater(EditorIdentifiers::ViewportDisplayInfoStateChangedUpdaterIdentifier);
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
            actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::HideWhenDisabled;

            AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);

            m_actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
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

            m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::RecentFilesChangedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;
        
        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.file.save", actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnFileSave();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.file.save", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, "o3de.action.file.save");
        
        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.file.save", "Ctrl+S");
    }

    // Save As...
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save As...";
        actionProperties.m_description = "Save the current level";
        actionProperties.m_category = "Level";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.file.saveAs", actionProperties,
            []
            {
                CCryEditDoc* pDoc = GetIEditor()->GetDocument();
                pDoc->OnFileSaveAs();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.file.saveAs", IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, "o3de.action.file.saveAs");
    }

    // Save Level Statistics
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.file.saveLevelStatistics";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Save Level Statistics";
        actionProperties.m_description = "Logs Editor memory usage.";
        actionProperties.m_category = "Level";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_description = "Exit the Editor";
        actionProperties.m_category = "Editor";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::UndoRedoUpdaterIdentifier, "o3de.action.edit.undo");

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.edit.undo", "Ctrl+Z");
    }

    // Redo
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&Redo";
        actionProperties.m_description = "Redo last undo operation";
        actionProperties.m_category = "Edit";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            "o3de.action.edit.redo",
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnRedo();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.edit.redo",
            []() -> bool
            {
                return GetIEditor()->GetUndoManager()->IsHaveRedo();
            }
        );

        // Trigger update after every undo or redo operation
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::UndoRedoUpdaterIdentifier, "o3de.action.edit.redo");

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::AngleSnappingStateChangedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::GridSnappingStateChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Show Grid
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.snap.toggleShowingGrid";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Grid";
        actionProperties.m_description = "Show Grid for entity snapping.";
        actionProperties.m_category = "Edit";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                SandboxEditor::SetShowingGrid(!SandboxEditor::ShowingGrid());
            },
            []()
            {
                return SandboxEditor::ShowingGrid();
            }
        );

        // Trigger update when the grid snapping setting changes
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::GridShowingChangedUpdaterIdentifier, actionIdentifier);

        // This action is only accessible outside of Component Modes
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
    }

    // Global Preferences
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.edit.globalPreferences";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Global Preferences...";
        actionProperties.m_category = "Editor";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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

    // Rename Entity (in the Entity Outliner)
    {
        const AZStd::string_view actionIdentifier = "o3de.action.entity.rename";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Rename";
        actionProperties.m_description = "Rename the current selection.";
        actionProperties.m_category = "Entity";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []()
            {
                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                // Can only rename one entity at a time
                if (selectedEntities.size() == 1)
                {
                    AzToolsFramework::EntityOutlinerRequestBus::Broadcast(
                        &AzToolsFramework::EntityOutlinerRequests::TriggerRenameEntityUi, selectedEntities.front());
                }
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(
            actionIdentifier,
            []() -> bool
            {
                int selectedEntitiesCount;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntitiesCount, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntitiesCount);

                // Can only rename one entity at a time
                return selectedEntitiesCount == 1;
            }
        );

        // Trigger update whenever entity selection changes.
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "F2");
    }

    // Find Entity (in the Entity Outliner)
    {
        const AZStd::string_view actionIdentifier = "o3de.action.entityOutliner.findEntity";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Find in Entity Outliner";
        actionProperties.m_description = "Ensure the current entity is visible in the Entity Outliner.";
        actionProperties.m_category = "Entity";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []()
            {
                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                if (!selectedEntities.empty())
                {
                    AzToolsFramework::EditorEntityContextNotificationBus::Broadcast(
                        &EditorEntityContextNotification::OnFocusInEntityOutliner, selectedEntities);
                }
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(
            actionIdentifier,
            []() -> bool
            {
                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                return !selectedEntities.empty();
            }
        );

        // Trigger update whenever entity selection changes.
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        // Trigger update whenever entity selection changes.
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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

    // -- Tools Actions

    // Lua Editor
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.tools.luaEditor";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Lua Editor";
        actionProperties.m_category = "Tools";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::LaunchLuaEditor, nullptr);
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnDisplayGotoPosition();
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, IsLevelLoaded);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);
    }

    // Center on Selection
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.centerOnSelection";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Find Selected Entities in Viewport";
        actionProperties.m_description = "Center the viewport to show selected entities.";
        actionProperties.m_category = "View";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::GoToSelectedEntitiesInViewports);
            }
        );

        m_actionManagerInterface->InstallEnabledStateCallback(actionIdentifier, AreEntitiesSelected);
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Z");
    }

    // View Bookmarks
    InitializeViewBookmarkActions();

    // Show Icons
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.toggleIcons";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Icons";
        actionProperties.m_description = "Show/Hide Icons.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            []
            {
                AzToolsFramework::SetIconsVisible(!AzToolsFramework::IconsVisible());
                AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                    &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            },
            []
            {
                return AzToolsFramework::IconsVisible();
            });

        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::IconsStateChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Ctrl+Space");
    }

    // Show Helpers
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.showHelpers";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Helpers for all entities";
        actionProperties.m_description = "Show Helpers.";
        actionProperties.m_category = "View";
        actionProperties.m_iconPath = ":/Menu/helpers.svg";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                AzToolsFramework::SetHelpersVisible(true);
                AzToolsFramework::SetOnlyShowHelpersForSelectedEntities(false);

                m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier);

                AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                    &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            },
            []
            {
                return AzToolsFramework::HelpersVisible();
            }
        );

        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Shift+Space");
    }

    // Only Show Helpers for Selected Entities
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.showSelectedEntityHelpers";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Show Helpers for selected entities";
        actionProperties.m_description = "If enabled, shows Helpers for selected entities only.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                AzToolsFramework::SetOnlyShowHelpersForSelectedEntities(true);
                AzToolsFramework::SetHelpersVisible(false);

                m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier);

                AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                    &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            },
            []
            {
                return AzToolsFramework::OnlyShowHelpersForSelectedEntities();
            });

        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier, actionIdentifier);
    }

    // Hide Helpers
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.hideHelpers";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Hide Helpers";
        actionProperties.m_description = "Hide all helpers";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterCheckableAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]
            {
                AzToolsFramework::SetOnlyShowHelpersForSelectedEntities(false);
                AzToolsFramework::SetHelpersVisible(false);

                m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier);

                AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                    &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            },
            []
            {
                return !AzToolsFramework::HelpersVisible() && !AzToolsFramework::OnlyShowHelpersForSelectedEntities();
            });

        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier, actionIdentifier);
    }

    // Refresh Style
    {
        constexpr AZStd::string_view actionIdentifier = "o3de.action.view.refreshEditorStyle";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Refresh Style";
        actionProperties.m_description = "Refreshes the editor stylesheet.";
        actionProperties.m_category = "View";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.tutorials", actionProperties,
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
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.documentation.o3de", actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnDocumentationO3DE();
            }
        );
    }

    // Release Notes
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Release Notes";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.documentation.releasenotes", actionProperties,
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
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.resources.gamedevblog", actionProperties,
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
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.resources.forums", actionProperties,
            [cryEdit = m_cryEditApp]
            {
                cryEdit->OnDocumentationForums();
            }
        );
    }

    // About O3DE
    {
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "&About O3DE";
        actionProperties.m_category = "Help";

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier, "o3de.action.help.abouto3de", actionProperties,
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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

    // Viewport - Field of View Property Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Viewport Field of View";
        widgetActionProperties.m_category = "Viewport";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.viewport.fieldOfView",
            widgetActionProperties,
            []() -> QWidget*
            {
                return new ViewportFieldOfViewPropertyWidget();
            }
        );
    }

    // Viewport - Camera Speed Scale Property Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Viewport Camera Speed Scale";
        widgetActionProperties.m_category = "Viewport";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.viewport.cameraSpeedScale",
            widgetActionProperties,
            []() -> QWidget*
            {
                return new ViewportCameraSpeedScalePropertyWidget();
            }
        );
    }

    // Viewport - Grid Size Property Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Viewport Grid Snapping Size";
        widgetActionProperties.m_category = "Viewport";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.viewport.gridSnappingSize",
            widgetActionProperties,
            []() -> QWidget*
            {
                return new ViewportGridSnappingSizePropertyWidget();
            }
        );
    }

    // Viewport - Angle Size Property Widget
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Viewport Angle Snapping Size";
        widgetActionProperties.m_category = "Viewport";

        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.viewport.angleSnappingSize",
            widgetActionProperties,
            []() -> QWidget*
            {
                return new ViewportAngleSnappingSizePropertyWidget();
            }
        );
    }
}     

void EditorActionsHandler::OnMenuBarRegistrationHook()
{
    // Register MenuBar
    m_menuManagerInterface->RegisterMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, m_mainWindow);
}

void EditorActionsHandler::OnMenuRegistrationHook()
{
    // Initialize Menus
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&File";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::FileMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Open Recent";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::RecentFilesMenuIdentifier, menuProperties);

            // Legacy - the menu should update when the files list is changed.
            QMenu* menu = m_menuManagerInternalInterface->GetMenu(EditorIdentifiers::FileMenuIdentifier);
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
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EditMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Modify";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EditModifyMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Snap";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EditModifySnapMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Transform Mode";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EditModifyModesMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Editor Settings";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EditSettingsMenuIdentifier, menuProperties);
        }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Game";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::GameMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Play Game";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::PlayGameMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Audio";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::GameAudioMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Debugging";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::GameDebuggingMenuIdentifier, menuProperties);
        }
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "ToolBox Macros";
                m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ToolBoxMacrosMenuIdentifier, menuProperties);
            }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Tools";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ToolsMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&View";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Layouts";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::LayoutsMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Viewport";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportMenuIdentifier, menuProperties);
        }
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Go to Location";
                m_menuManagerInterface->RegisterMenu(EditorIdentifiers::GoToLocationMenuIdentifier, menuProperties);
            }
            {
                AzToolsFramework::MenuProperties menuProperties;
                menuProperties.m_name = "Save Location";
                m_menuManagerInterface->RegisterMenu(EditorIdentifiers::SaveLocationMenuIdentifier, menuProperties);
            }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "&Help";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::HelpMenuIdentifier, menuProperties);
    }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Documentation";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::HelpDocumentationMenuIdentifier, menuProperties);
        }
        {
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "GameDev Resources";
            m_menuManagerInterface->RegisterMenu(EditorIdentifiers::HelpGameDevResourcesMenuIdentifier, menuProperties);
        }

    // Editor Menus
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Entity Outliner Context Menu";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Viewport Context Menu";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::ViewportContextMenuIdentifier, menuProperties);
    }
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = "Create";
        m_menuManagerInterface->RegisterMenu(EditorIdentifiers::EntityCreationMenuIdentifier, menuProperties);
    }

}

void EditorActionsHandler::OnMenuBindingHook()
{
    // Add Menus to MenuBar
    // We space the sortkeys by 100 to allow external systems to add menus in-between.
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::FileMenuIdentifier, 100);
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::EditMenuIdentifier, 200);
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::GameMenuIdentifier, 300);
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::ToolsMenuIdentifier, 400);
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::ViewMenuIdentifier, 500);
    m_menuManagerInterface->AddMenuToMenuBar(EditorIdentifiers::EditorMainWindowMenuBarIdentifier, EditorIdentifiers::HelpMenuIdentifier, 600);

    // Add actions to each menu

    // File
    {
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.new", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.open", 200);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::FileMenuIdentifier, EditorIdentifiers::RecentFilesMenuIdentifier, 300);
        {
            for (int index = 0; index < maxRecentFiles; ++index)
            {
                AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", index + 1);
                m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::RecentFilesMenuIdentifier, actionIdentifier, 100);
            }
            m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::RecentFilesMenuIdentifier, 200);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::RecentFilesMenuIdentifier, "o3de.action.file.recent.clearAll", 300);
        }
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::FileMenuIdentifier, 400);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.save", 500);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.saveAs", 600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.saveLevelStatistics", 700);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::FileMenuIdentifier, 800);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.project.editSettings", 900);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.platform.editSettings", 1000);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::FileMenuIdentifier, 1100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.project.new", 1200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.project.open", 1300);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::FileMenuIdentifier, 1400);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.file.showLog", 1500);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::FileMenuIdentifier, 1600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::FileMenuIdentifier, "o3de.action.editor.exit", 1700);
    }

    // Edit
    {
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.undo", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.redo", 200);

        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::EditMenuIdentifier, EditorIdentifiers::EditModifyMenuIdentifier, 1800);
        {
            m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::EditModifyMenuIdentifier, EditorIdentifiers::EditModifySnapMenuIdentifier, 100);
            {
                m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleGridSnapping", 100);
                m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditModifySnapMenuIdentifier, "o3de.action.edit.snap.toggleAngleSnapping", 200);
            }
            m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::EditModifyMenuIdentifier, EditorIdentifiers::EditModifyModesMenuIdentifier, 200);
        }
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EditMenuIdentifier, 1900);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::EditMenuIdentifier, EditorIdentifiers::EditSettingsMenuIdentifier, 2000);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditSettingsMenuIdentifier, "o3de.action.edit.globalPreferences", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditSettingsMenuIdentifier, "o3de.action.edit.editorSettingsManager", 200);
        }
    }

    // Game
    {
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::GameMenuIdentifier, EditorIdentifiers::PlayGameMenuIdentifier, 100);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::PlayGameMenuIdentifier, "o3de.action.game.play", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::PlayGameMenuIdentifier, "o3de.action.game.playMaximized", 200);
        }
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GameMenuIdentifier, "o3de.action.game.simulate", 200);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::GameMenuIdentifier, 300);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::GameMenuIdentifier, 600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GameMenuIdentifier, "o3de.action.game.movePlayerAndCameraSeparately", 700);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::GameMenuIdentifier, 800);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::GameMenuIdentifier, EditorIdentifiers::GameAudioMenuIdentifier, 900);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GameAudioMenuIdentifier, "o3de.action.game.audio.stopAllSounds", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GameAudioMenuIdentifier, "o3de.action.game.audio.refresh", 200);
        }
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::GameMenuIdentifier, 1000);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::GameMenuIdentifier, EditorIdentifiers::GameDebuggingMenuIdentifier, 1100);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GameDebuggingMenuIdentifier, "o3de.action.game.debugging.errorDialog", 100);
            m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::GameDebuggingMenuIdentifier, 200);
            m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::GameDebuggingMenuIdentifier, EditorIdentifiers::ToolBoxMacrosMenuIdentifier, 300);
            {
                // Some of the contents of the ToolBox Macros menu are initialized in RefreshToolboxMacrosActions.

                m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ToolBoxMacrosMenuIdentifier, 200);
                m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ToolBoxMacrosMenuIdentifier, "o3de.action.game.debugging.toolboxMacros", 300);
            }
        }
    }

    // Tools
    {
        m_menuManagerInterface->AddActionToMenu(
            EditorIdentifiers::ToolsMenuIdentifier,
            "o3de.action.tools.luaEditor",
            m_actionManagerInterface->GenerateActionAlphabeticalSortKey("o3de.action.tools.luaEditor")
        );
    }

    // View
    {
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewMenuIdentifier, EditorIdentifiers::LayoutsMenuIdentifier, 100);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::LayoutsMenuIdentifier, "o3de.action.layout.componentEntityLayout", 100);
            m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::LayoutsMenuIdentifier, 200);

            // Some of the contents of the Layouts menu are initialized in RefreshLayoutActions.

            m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::LayoutsMenuIdentifier, 400);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::LayoutsMenuIdentifier, "o3de.action.layout.save", 500);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::LayoutsMenuIdentifier, "o3de.action.layout.restoreDefault", 600);
        }
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewMenuIdentifier, EditorIdentifiers::ViewportMenuIdentifier, 200);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportMenuIdentifier, "o3de.action.view.goToPosition", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportMenuIdentifier, "o3de.action.view.centerOnSelection", 200);
            m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewportMenuIdentifier, EditorIdentifiers::GoToLocationMenuIdentifier, 300);
            {
                for (int index = 0; index < m_defaultBookmarkCount; ++index)
                {
                    const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].goTo", index);
                    m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::GoToLocationMenuIdentifier, actionIdentifier, 0);
                }
            }
            m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::ViewportMenuIdentifier, EditorIdentifiers::SaveLocationMenuIdentifier, 400);
            {
                for (int index = 0; index < m_defaultBookmarkCount; ++index)
                {
                    const AZStd::string actionIdentifier = AZStd::string::format("o3de.action.view.bookmark[%i].save", index);
                    m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::SaveLocationMenuIdentifier, actionIdentifier, 0);
                }
            }
            m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportMenuIdentifier, 500);
            m_menuManagerInterface->AddSubMenuToMenu(
                EditorIdentifiers::ViewportMenuIdentifier, EditorIdentifiers::ViewportHelpersMenuIdentifier, 600);
        }
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewMenuIdentifier, "o3de.action.view.refreshEditorStyle", 300);
    }

    // Help
    {
        m_menuManagerInterface->AddWidgetToMenu(EditorIdentifiers::HelpMenuIdentifier, "o3de.widgetAction.help.searchDocumentation", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpMenuIdentifier, "o3de.action.help.tutorials", 200);
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::HelpMenuIdentifier, EditorIdentifiers::HelpDocumentationMenuIdentifier, 300);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.o3de", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.gamelift", 200);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpDocumentationMenuIdentifier, "o3de.action.help.documentation.releasenotes", 300);
        }
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::HelpMenuIdentifier, EditorIdentifiers::HelpGameDevResourcesMenuIdentifier, 400);
        {
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpGameDevResourcesMenuIdentifier, "o3de.action.help.resources.gamedevblog", 100);
            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpGameDevResourcesMenuIdentifier, "o3de.action.help.resources.forums", 200);
        }
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::HelpMenuIdentifier, 500);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpMenuIdentifier, "o3de.action.help.abouto3de", 600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::HelpMenuIdentifier, "o3de.action.help.welcome", 700);
    }

    // Entity Outliner Context Menu
    m_menuManagerInterface->AddSubMenuToMenu(
        EditorIdentifiers::EntityOutlinerContextMenuIdentifier, EditorIdentifiers::EntityCreationMenuIdentifier, 200);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 10000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 20000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 30000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 40000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 50000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 60000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 70000);
    m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.entity.rename", 70100);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, 80000);
    m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.view.centerOnSelection", 80100);

    // Viewport Context Menu
    m_menuManagerInterface->AddSubMenuToMenu(
        EditorIdentifiers::ViewportContextMenuIdentifier, EditorIdentifiers::EntityCreationMenuIdentifier, 200);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 10000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 20000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 30000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 40000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 50000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 60000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 70000);
    m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, 80000);
    m_menuManagerInterface->AddActionToMenu(
        EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.entityOutliner.findEntity", 80100);
}

void EditorActionsHandler::OnToolBarAreaRegistrationHook()
{
    m_toolBarManagerInterface->RegisterToolBarArea(
        EditorIdentifiers::MainWindowTopToolBarAreaIdentifier, m_mainWindow, Qt::ToolBarArea::TopToolBarArea);
}

void EditorActionsHandler::OnToolBarRegistrationHook()
{
    // Initialize ToolBars
    {
        AzToolsFramework::ToolBarProperties toolBarProperties;
        toolBarProperties.m_name = "Tools";
        m_toolBarManagerInterface->RegisterToolBar(EditorIdentifiers::ToolsToolBarIdentifier, toolBarProperties);
    }
    {
        AzToolsFramework::ToolBarProperties toolBarProperties;
        toolBarProperties.m_name = "Play Controls";
        m_toolBarManagerInterface->RegisterToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, toolBarProperties);
    }
}

void EditorActionsHandler::OnToolBarBindingHook()
{
    // Add ToolBars to ToolBar Areas
    // We space the sortkeys by 100 to allow external systems to add toolbars in-between.
    m_toolBarManagerInterface->AddToolBarToToolBarArea(
        EditorIdentifiers::MainWindowTopToolBarAreaIdentifier, EditorIdentifiers::ToolsToolBarIdentifier, 100);
    m_toolBarManagerInterface->AddToolBarToToolBarArea(
        EditorIdentifiers::MainWindowTopToolBarAreaIdentifier, EditorIdentifiers::PlayControlsToolBarIdentifier, 200);

    // Add actions to each toolbar

    // Play Controls
    {
        m_toolBarManagerInterface->AddWidgetToToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, "o3de.widgetAction.expander", 100);
        m_toolBarManagerInterface->AddSeparatorToToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, 200);
        m_toolBarManagerInterface->AddWidgetToToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, "o3de.widgetAction.game.playControlsLabel", 300);
        m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(
            EditorIdentifiers::PlayControlsToolBarIdentifier, "o3de.action.game.play", EditorIdentifiers::PlayGameMenuIdentifier, 400);
        m_toolBarManagerInterface->AddSeparatorToToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, 500);
        m_toolBarManagerInterface->AddActionToToolBar(EditorIdentifiers::PlayControlsToolBarIdentifier, "o3de.action.game.simulate", 600);
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

    QMenu* helpMenu = m_menuManagerInternalInterface->GetMenu(EditorIdentifiers::HelpMenuIdentifier);

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
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnStopPlayInEditor()
{
    // Wait one frame for the game mode to actually be shut off.
    QTimer::singleShot(
        0,
        nullptr,
        [actionManagerInterface = m_actionManagerInterface]
        {
            actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::GameModeStateChangedUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::AfterEntitySelectionChanged(
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlySelectedEntities,
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier);
}

void EditorActionsHandler::AfterUndoRedo()
{
    // Wait one frame for the undo stack to actually be updated.
    QTimer::singleShot(
        0,
        nullptr,
        [actionManagerInterface = m_actionManagerInterface]
        {
            actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::UndoRedoUpdaterIdentifier);
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
            actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::UndoRedoUpdaterIdentifier);
        }
    );
}

void EditorActionsHandler::OnAngleSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::AngleSnappingStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnDrawHelpersChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::DrawHelpersStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnGridShowingChanged([[maybe_unused]] bool showing)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::GridShowingChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnGridSnappingChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::GridSnappingStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnIconsVisibilityChanged([[maybe_unused]] bool enabled)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::IconsStateChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnEntityPickModeStarted()
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::EntityPickingModeChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnEntityPickModeStopped()
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::EntityPickingModeChangedUpdaterIdentifier);
}

void EditorActionsHandler::OnContainerEntityStatusChanged([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] bool open)
{
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::ContainerEntityStatesChangedUpdaterIdentifier);
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

    int index = 0;

    // Update all names
    for (int counter = 0; counter < maxRecentFiles; ++counter)
    {
        // Loop through all Recent Files Menu entries, even the hidden ones.
        AZStd::string actionIdentifier = AZStd::string::format("o3de.action.file.recent.file%i", counter + 1);

        // Check if the recent file at index is valid. If not, increment index until you find one, or the list ends.
        while (index < recentFilesSize)
        {
            if (IsRecentFileEntryValid((*recentFiles)[index], gameDirPath))
            {
                break;
            }

            ++index;
        }

        if (index < recentFilesSize)
        {
            // If the index is valid, use it to populate the action's name and then increment for the next menu item.
            QString displayName;
            recentFiles->GetDisplayName(displayName, index, sCurDir);

            m_actionManagerInterface->SetActionName(
                actionIdentifier, AZStd::string::format("%i | %s", counter + 1, displayName.toUtf8().data()));

            ++m_recentFileActionsCount;
            ++index;
        }
        else
        {
            // If the index is invalid, give the default name for consistency.
            // The menu item will not show as it will be disabled by its enabled state callback.
            m_actionManagerInterface->SetActionName(actionIdentifier, AZStd::string::format("Recent File #%i", counter + 1));
        }
    }

    // Trigger the updater
    m_actionManagerInterface->TriggerActionUpdater(EditorIdentifiers::RecentFilesChangedUpdaterIdentifier);
}

void EditorActionsHandler::RefreshLayoutActions()
{
    m_menuManagerInterface->RemoveSubMenusFromMenu(EditorIdentifiers::LayoutsMenuIdentifier, m_layoutMenuIdentifiers);
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
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
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
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
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
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
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
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_menuManagerInterface->AddSubMenuToMenu(EditorIdentifiers::LayoutsMenuIdentifier, layoutMenuIdentifier, sortKey);
    }
}

void EditorActionsHandler::RefreshToolboxMacroActions()
{
    // If the toolbox macros are being displayed in the menu already, remove them.
    m_menuManagerInterface->RemoveActionsFromMenu(EditorIdentifiers::ToolBoxMacrosMenuIdentifier, m_toolboxMacroActionIdentifiers);
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
                actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

                m_actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
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

            m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ToolBoxMacrosMenuIdentifier, toolboxMacroActionIdentifier, sortKey);
            m_toolboxMacroActionIdentifiers.push_back(AZStd::move(toolboxMacroActionIdentifier));
        }
    }
}

void EditorActionsHandler::RefreshToolActions()
{
    // If the tools are being displayed in the menu or toolbar already, remove them.
    m_menuManagerInterface->RemoveActionsFromMenu(EditorIdentifiers::ToolsMenuIdentifier, m_toolActionIdentifiers);
    m_toolBarManagerInterface->RemoveActionsFromToolBar(EditorIdentifiers::ToolsToolBarIdentifier, m_toolActionIdentifiers);
    m_toolActionIdentifiers.clear();

    AZStd::vector<AZStd::pair<AZStd::string, int>> toolsMenuItems;
    AZStd::vector<AZStd::pair<AZStd::string, int>> toolsToolBarItems;

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
            actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;
            actionProperties.m_toolBarVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

            m_actionManagerInterface->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                toolActionIdentifier,
                actionProperties,
                [viewpaneManager = m_qtViewPaneManager, viewpaneName = viewpane.m_name]
                {
                    viewpaneManager->TogglePane(viewpaneName);
                },
                [viewpaneManager = m_qtViewPaneManager, viewpaneName = viewpane.m_name]() -> bool
                {
                    return viewpaneManager->IsEnumeratedInstanceVisible(viewpaneName);
                }
            );

            // This action is only accessible outside of Component Modes
            m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, toolActionIdentifier);
        }

        m_toolActionIdentifiers.push_back(toolActionIdentifier);

        // Set the sortKey as the ASCII of the first character in the toolName.
        // This will allow Tool actions to always be sorted alphabetically even if they are
        // plugged in by Gems, as long as they use this same logic.
        int sortKey = m_actionManagerInterface->GenerateActionAlphabeticalSortKey(toolActionIdentifier);

        if (viewpane.m_options.showInMenu)
        {
            toolsMenuItems.push_back({ toolActionIdentifier, sortKey });
        }

        if (viewpane.m_options.showOnToolsToolbar)
        {
            toolsToolBarItems.push_back({ toolActionIdentifier, sortKey });
        }
    }

    m_menuManagerInterface->AddActionsToMenu(EditorIdentifiers::ToolsMenuIdentifier, toolsMenuItems);
    m_toolBarManagerInterface->AddActionsToToolBar(EditorIdentifiers::ToolsToolBarIdentifier, toolsToolBarItems);
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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        auto outcome = m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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

                SandboxEditor::HandleDefaultViewportCameraTransitionFromSetting(
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);

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
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
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
        m_actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);

        m_hotKeyManagerInterface->SetActionHotKey(actionIdentifier, AZStd::string::format("Ctrl+F%i", index+1));
    }
}
