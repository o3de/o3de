/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/Component/TickBus.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerNotificationBus.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBar.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ActionManagerInternalInterface;
    
    //! ToolBar Manager class definition.
    //! Handles Editor ToolBars and allows registration and access across tools.
    class ToolBarManager
        : private ToolBarManagerInterface
        , private ToolBarManagerInternalInterface
        , private AZ::SystemTickBus::Handler
        , private ActionManagerNotificationBus::Handler
    {
    public:
        ToolBarManager();
        virtual ~ToolBarManager();

    private:
        // ToolBarManagerInterface overrides ...
        ToolBarManagerOperationResult RegisterToolBar(const AZStd::string& toolBarIdentifier, const ToolBarProperties& properties) override;
        ToolBarManagerOperationResult AddActionToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, int sortIndex) override;
        ToolBarManagerOperationResult AddActionWithSubMenuToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) override;
        ToolBarManagerOperationResult AddActionsToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions) override;
        ToolBarManagerOperationResult RemoveActionFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) override;
        ToolBarManagerOperationResult RemoveActionsFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers) override;
        ToolBarManagerOperationResult AddSeparatorToToolBar(const AZStd::string& toolBarIdentifier, int sortIndex) override;
        ToolBarManagerOperationResult AddWidgetToToolBar(const AZStd::string& toolBarIdentifier, QWidget* widget, int sortIndex) override;
        QToolBar* GetToolBar(const AZStd::string& toolBarIdentifier) override;
        ToolBarManagerIntegerResult GetSortKeyOfActionInToolBar(const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const override;

        // ToolBarManagerInternalInterface overrides ...
        ToolBarManagerOperationResult QueueToolBarRefresh(const AZStd::string& toolBarIdentifier) override;
        ToolBarManagerOperationResult QueueRefreshForToolBarsContainingAction(const AZStd::string& actionIdentifier) override;
        void RefreshToolBars() override;

        // SystemTickBus overrides ...
        void OnSystemTick() override;

        // ActionManagerNotificationBus overrides ...
        void OnActionStateChanged(AZStd::string actionIdentifier) override;

        AZStd::unordered_map<AZStd::string, EditorToolBar> m_toolBars;

        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> m_actionsToToolBarsMap;

        AZStd::unordered_set<AZStd::string> m_toolBarsToRefresh;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
