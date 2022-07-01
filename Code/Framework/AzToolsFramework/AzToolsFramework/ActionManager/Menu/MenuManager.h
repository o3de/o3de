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
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>
#include <AzToolsFramework/ActionManager/Menu/EditorMenuBar.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    
    //! Menu Manager class definition.
    //! Handles Editor Menus and allows registration and access across tools.
    class MenuManager
        : private MenuManagerInterface
        , private MenuManagerInternalInterface
        , private AZ::SystemTickBus::Handler
        , private ActionManagerNotificationBus::Handler
    {
    public:
        MenuManager();
        virtual ~MenuManager();

    private:
        // MenuManagerInterface overrides ...
        MenuManagerOperationResult RegisterMenu(const AZStd::string& menuIdentifier, const MenuProperties& properties) override;
        MenuManagerOperationResult RegisterMenuBar(const AZStd::string& menuBarIdentifier) override;
        MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddActionsToMenu(
            const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions) override;
        MenuManagerOperationResult RemoveActionFromMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) override;
        MenuManagerOperationResult RemoveActionsFromMenu(
            const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers) override;
        MenuManagerOperationResult AddSeparatorToMenu(const AZStd::string& menuIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddWidgetToMenu(
            const AZStd::string& menuIdentifier, QWidget* widget, int sortIndex) override;
        MenuManagerOperationResult AddMenuToMenuBar(
            const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex) override;
        MenuManagerIntegerResult GetSortKeyOfActionInMenu(const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) const override;
        MenuManagerIntegerResult GetSortKeyOfSubMenuInMenu(const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier) const override;
        MenuManagerIntegerResult GetSortKeyOfMenuInMenuBar(const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier) const override;

        // MenuManagerInterface overrides ...
        QMenu* GetMenu(const AZStd::string& menuIdentifier) override;
        QMenuBar* GetMenuBar(const AZStd::string& menuBarIdentifier) override;
        MenuManagerOperationResult QueueRefreshForMenu(const AZStd::string& menuIdentifier) override;
        MenuManagerOperationResult QueueRefreshForMenusContainingAction(const AZStd::string& actionIdentifier) override;
        MenuManagerOperationResult QueueRefreshForMenuBar(const AZStd::string& menuBarIdentifier) override;
        void RefreshMenus() override;
        void RefreshMenuBars() override;

        // SystemTickBus overrides ...
        void OnSystemTick() override;

        // ActionManagerNotificationBus overrides ...
        void OnActionStateChanged(AZStd::string actionIdentifier) override;

        AZStd::unordered_map<AZStd::string, EditorMenu> m_menus;
        AZStd::unordered_map<AZStd::string, EditorMenuBar> m_menuBars;

        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> m_actionsToMenusMap;

        AZStd::unordered_set<AZStd::string> m_menusToRefresh;
        AZStd::unordered_set<AZStd::string> m_menuBarsToRefresh;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
