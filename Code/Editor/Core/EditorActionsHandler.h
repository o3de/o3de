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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
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

class EditorActionsHandler
    : private AzToolsFramework::EditorEventsBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    , private AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler
{
public:
    void Initialize(MainWindow* mainWindow);
    ~EditorActionsHandler();

private:
    void InitializeActionContext();
    void InitializeActionUpdaters();
    void InitializeActions();
    void InitializeWidgetActions();
    void InitializeMenus();
    void InitializeToolBars();

    QWidget* CreateDocsSearchWidget();
    QWidget* CreateExpander();
    QWidget* CreatePlayControlsLabel();
    
    // EditorEventsBus overrides ...
    void OnViewPaneOpened(const char* viewPaneName) override;
    void OnViewPaneClosed(const char* viewPaneName) override;

    // EditorEntityContextNotificationBus overrides ...
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    void OnEntityStreamLoadSuccess() override;

    // ToolsApplicationNotificationBus overrides ...
    void AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;
    void AfterUndoRedo() override;
    void OnEndUndo(const char* label, bool changed) override;

    // ViewportSettingsNotificationBus overrides ...
    void OnAngleSnappingChanged(bool enabled) override;
    void OnDrawHelpersChanged(bool enabled) override;
    void OnGridSnappingChanged(bool enabled) override;
    void OnIconsVisibilityChanged(bool enabled) override;

    // Layouts
    void RefreshLayoutActions();

    // Recent Files
    bool IsRecentFileActionActive(int index);
    void UpdateRecentFileActions();

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

    AZStd::vector<AZStd::string> m_layoutMenuIdentifiers;
    AZStd::vector<AZStd::string> m_toolActionIdentifiers;
    AZStd::vector<AZStd::string> m_toolboxMacroActionIdentifiers;

    bool m_isPrefabSystemEnabled = false;
};
