/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

class CCryEditApp;
class MainWindow;
class QMainWindow;
class QtViewPaneManager;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ActionManagerInternalInterface;
    class HotKeyManagerInterface;
    class MenuManagerInterface;
    class MenuManagerInternalInterface;
    class ToolBarManagerInterface;
} // namespace AzToolsFramework

class EditorViewportDisplayInfoHandler;

class EditorActionsHandler
    : private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    , private AzToolsFramework::EditorEventsBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    , private AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler
    , private AzToolsFramework::EditorPickModeNotificationBus::Handler
    , private AzToolsFramework::ContainerEntityNotificationBus::Handler
{
public:
    void Initialize(MainWindow* mainWindow);
    ~EditorActionsHandler();

private:
    QWidget* CreateDocsSearchWidget();
    QWidget* CreateExpander();
    QWidget* CreatePlayControlsLabel();

    // ActionManagerRegistrationNotificationBus overrides ...
    void OnActionContextRegistrationHook() override;
    void OnActionUpdaterRegistrationHook() override;
    void OnMenuBarRegistrationHook() override;
    void OnMenuRegistrationHook() override;
    void OnToolBarAreaRegistrationHook() override;
    void OnToolBarRegistrationHook() override;
    void OnActionRegistrationHook() override;
    void OnWidgetActionRegistrationHook() override;
    void OnMenuBindingHook() override;
    void OnToolBarBindingHook() override;
    void OnPostActionManagerRegistrationHook() override;
    
    // EditorEventsBus overrides ...
    void OnViewPaneOpened(const char* viewPaneName) override;
    void OnViewPaneClosed(const char* viewPaneName) override;

    // EditorEntityContextNotificationBus overrides ...
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;

    // ToolsApplicationNotificationBus overrides ...
    void AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;
    void AfterUndoRedo() override;
    void OnEndUndo(const char* label, bool changed) override;

    // ViewportSettingsNotificationBus overrides ...
    void OnAngleSnappingChanged(bool enabled) override;
    void OnDrawHelpersChanged(bool enabled) override;
    void OnGridShowingChanged(bool showing) override;
    void OnGridSnappingChanged(bool enabled) override;
    void OnIconsVisibilityChanged(bool enabled) override;

    // EditorPickModeNotificationBus overrides ...
    void OnEntityPickModeStarted() override;
    void OnEntityPickModeStopped() override;

    // ContainerEntityNotificationBus overrides ...
    void OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open);

    // Layouts
    void RefreshLayoutActions();

    // Recent Files
    const char* m_levelExtension = nullptr;
    int m_recentFileActionsCount = 0;
    bool IsRecentFileActionActive(int index);
    bool IsRecentFileEntryValid(const QString& entry, const QString& gameFolderPath);
    void UpdateRecentFileActions();
    void OpenLevelByRecentFileEntryIndex(int index);

    // Toolbox Macros
    void RefreshToolboxMacroActions();

    // Tools
    void RefreshToolActions();

    // View Bookmarks
    int m_defaultBookmarkCount = 12;
    void InitializeViewBookmarkActions();

    bool m_initialized = false;

    // Editor Action Manager initialization functions
    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    AzToolsFramework::ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    AzToolsFramework::HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
    AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
    AzToolsFramework::MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
    AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;

    CCryEditApp* m_cryEditApp;
    MainWindow* m_mainWindow;
    QtViewPaneManager* m_qtViewPaneManager;

    EditorViewportDisplayInfoHandler* m_editorViewportDisplayInfoHandler = nullptr;

    AZStd::vector<AZStd::string> m_layoutMenuIdentifiers;
    AZStd::vector<AZStd::string> m_toolActionIdentifiers;
    AZStd::vector<AZStd::string> m_toolboxMacroActionIdentifiers;
};
