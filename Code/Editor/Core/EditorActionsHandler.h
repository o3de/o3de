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

class CCryEditApp;
class QMainWindow;
class QtViewPaneManager;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class ToolBarManagerInterface;
} // namespace AzToolsFramework

class EditorActionsHandler
    : private AzToolsFramework::EditorEventsBus::Handler
{
public:
    void Initialize(QMainWindow* mainWindow);
    ~EditorActionsHandler();

private:
    void InitializeActionContext();
    void InitializeActions();
    void InitializeMenus();
    void InitializeToolBars();

    QWidget* CreateDocsSearchWidget();
    
    // EditorEventsBus overrides ...
    void OnViewPaneOpened(const char* viewPaneName) override;
    void OnViewPaneClosed(const char* viewPaneName) override;

    void RefreshToolActions();

    bool m_initialized = false;

    // Editor Action Manager initialization functions
    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
    AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;

    CCryEditApp* m_cryEditApp;
    QMainWindow* m_mainWindow;
    QtViewPaneManager* m_qtViewPaneManager;

    AZStd::vector<AZStd::string> m_toolActionIdentifiers;
};
