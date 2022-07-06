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

class CCryEditApp;
class QMainWindow;
class QtViewPaneManager;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ActionManagerInternalInterface;
    class MenuManagerInterface;
    class MenuManagerInternalInterface;
    class ToolBarManagerInterface;
} // namespace AzToolsFramework

class EditorActionsHandler
    : private AzToolsFramework::EditorEventsBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
{
public:
    void Initialize(QMainWindow* mainWindow);
    ~EditorActionsHandler();

private:
    void InitializeActionContext();
    void InitializeActionUpdaters();
    void InitializeActions();
    void InitializeMenus();
    void InitializeToolBars();

    QWidget* CreateExpander();
    QWidget* CreateLabel(const AZStd::string& text);
    QWidget* CreateDocsSearchWidget();
    
    // EditorEventsBus overrides ...
    void OnViewPaneOpened(const char* viewPaneName) override;
    void OnViewPaneClosed(const char* viewPaneName) override;

    // EditorEntityContextNotificationBus overrides ...
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    void OnEntityStreamLoadSuccess() override;

    // ToolsApplicationNotificationBus overrides ...
    void AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList& newlyDeselectedEntities);

    // Recent Files
    bool IsRecentFileActionActive(int index);
    void UpdateRecentFileActions();

    // Tools
    void RefreshToolActions();

    bool m_initialized = false;

    // Editor Action Manager initialization functions
    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    AzToolsFramework::ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
    AzToolsFramework::MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
    AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;

    CCryEditApp* m_cryEditApp;
    QMainWindow* m_mainWindow;
    QtViewPaneManager* m_qtViewPaneManager;

    AZStd::vector<AZStd::string> m_toolActionIdentifiers;

    bool m_isPrefabSystemEnabled = false;
};
