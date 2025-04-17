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
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBar.h>
#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBarArea.h>

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
        explicit ToolBarManager(QWidget* defaultParentWidget);
        virtual ~ToolBarManager();

        static void Reflect(AZ::ReflectContext* context);

    private:
        // ToolBarManagerInterface overrides ...
        ToolBarManagerOperationResult RegisterToolBar(const AZStd::string& toolBarIdentifier, const ToolBarProperties& properties) override;
        ToolBarManagerOperationResult RegisterToolBarArea(
            const AZStd::string& toolBarAreaIdentifier, QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea) override;
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
        ToolBarManagerOperationResult AddWidgetToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex) override;
        ToolBarManagerOperationResult AddToolBarToToolBarArea(
            const AZStd::string& toolBarAreaIdentifier, const AZStd::string& toolBarIdentifier, int sortIndex) override;
        QToolBar* GenerateToolBar(const AZStd::string& toolBarIdentifier) override;
        ToolBarManagerIntegerResult GetSortKeyOfActionInToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const override;
        ToolBarManagerIntegerResult GetSortKeyOfWidgetInToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier) const override;

        // ToolBarManagerInternalInterface overrides ...
        ToolBarManagerOperationResult QueueToolBarRefresh(const AZStd::string& toolBarIdentifier) override;
        ToolBarManagerOperationResult QueueRefreshForToolBarsContainingAction(const AZStd::string& actionIdentifier) override;
        void RefreshToolBars() override;
        void RefreshToolBarAreas() override;
        ToolBarManagerStringResult SerializeToolBar(const AZStd::string& toolBarIdentifier) override;
        void Reset() override;

        // SystemTickBus overrides ...
        void OnSystemTick() override;

        // ActionManagerNotificationBus overrides ...
        void OnActionStateChanged(AZStd::string actionIdentifier) override;

        AZStd::unordered_map<AZStd::string, EditorToolBar> m_toolBars;
        AZStd::unordered_map<AZStd::string, EditorToolBarArea> m_toolBarAreas;

        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> m_actionsToToolBarsMap;
        AZStd::unordered_set<AZStd::string> m_toolBarsToRefresh;
        AZStd::unordered_set<AZStd::string> m_toolBarAreasToRefresh;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
