/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

class QMenu;
class QMenuBar;
class QWidget;

namespace AzToolsFramework
{
    using MenuManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using MenuManagerIntegerResult = AZ::Outcome<int, AZStd::string>;

    //! Provides additional properties to initialize a Menu upon registration.
    struct MenuProperties
    {
        AZ_RTTI(MenuProperties, "{E46CF861-2A19-43EC-8CD7-42E4C03AD6CF}");

        MenuProperties() = default;
        virtual ~MenuProperties() = default;

        AZStd::string m_name = "";
    };

    //! MenuManagerInterface
    //! Interface to register and manage menus in the Editor.
    class MenuManagerInterface
    {
    public:
        AZ_RTTI(MenuManagerInterface, "{D70B7989-62BD-447E-ADF6-0971EC4B7DEE}");

        //! Register a new Menu to the Menu Manager.
        //! @param menuIdentifier The identifier for the menu that is being registered.
        //! @param properties The properties object for the newly registered menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult RegisterMenu(const AZStd::string& menuIdentifier, const MenuProperties& properties) = 0;

        //! Register a new Menu Bar to the Menu Manager.
        //! @param menuBarIdentifier The identifier for the menu bar that is being registered.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult RegisterMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Add an Action to a Menu. Will prompt an update of the menu.
        //! @param menuIdentifier The identifier for the menu the action is being added to.
        //! @param actionIdentifier The identifier for the action to add to the menu.
        //! @param sortIndex An integer defining the position the action should appear in the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add multiple Actions to a Menu.
        //! @param menuIdentifier The identifier for the menu the actions are being added to.
        //! @param actions A vector of pairs of identifiers for the actions to add to the menu and their sort position.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddActionsToMenu(
            const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions) = 0;

        //! Removes an Action from a Menu.
        //! @param menuIdentifier The identifier for the menu the action is being removed from.
        //! @param actionIdentifier The identifier for the action to remove from the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult RemoveActionFromMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) = 0;

        //! Removes multiple Actions from a Menu.
        //! @param menuIdentifier The identifier for the menu the actions are being removed from.
        //! @param actionIdentifiers A vector of identifiers for the actions to remove from the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult RemoveActionsFromMenu(
            const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers) = 0;

        //! Add a Separator to a Menu.
        //! @param menuIdentifier The identifier for the menu the separator is being added to.
        //! @param sortIndex An integer defining the position the separator should appear in the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Add a Sub-Menu to a Menu.
        //! @param menuIdentifier The identifier for the menu the sub-menu is being added to.
        //! @param subMenuIdentifier The identifier for the sub-menu to add to the menu.
        //! @param sortIndex An integer defining the position the sub-menu should appear in the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;

        //! Add a Widget to a Menu.
        //! @param menuIdentifier The identifier for the menu the sub-menu is being added to.
        //! @param widget A pointer to the widget to add to the menu.
        //! @param sortIndex An integer defining the position the widget should appear in the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddWidgetToMenu(
            const AZStd::string& menuIdentifier, QWidget* widget, int sortIndex) = 0;

        //! Add a Menu to a Menu Bar.
        //! @param menuBarIdentifier The identifier for the menu bar the menu is being added to.
        //! @param menuIdentifier The identifier for the menu to add to the menu bar.
        //! @param sortIndex An integer defining the position the menu should appear in the menu bar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddMenuToMenuBar(
            const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Retrieve the sort key of an action in a menu from its identifier.
        //! @param menuIdentifier The identifier for the menu to query.
        //! @param actionIdentifier The identifier for the action whose sort key to get in the menu.
        //! @return A successful outcome object containing the sort key, or a string with a message detailing the error in case of failure.
        virtual MenuManagerIntegerResult GetSortKeyOfActionInMenu(const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve the sort key of a sub-menu in a menu from its identifier.
        //! @param menuIdentifier The identifier for the menu to query.
        //! @param subMenuIdentifier The identifier for the sub-menu whose sort key to get in the menu.
        //! @return A successful outcome object containing the sort key, or a string with a message detailing the error in case of failure.
        virtual MenuManagerIntegerResult GetSortKeyOfSubMenuInMenu(const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier) const = 0;

        //! Retrieve the sort key of a sub-menu in a menu from its identifier.
        //! @param menuBarIdentifier The identifier for the menu bar to query.
        //! @param menuIdentifier The identifier for the menu whose sort key to get in the menu bar.
        //! @return A successful outcome object containing the sort key, or a string with a message detailing the error in case of failure.
        virtual MenuManagerIntegerResult GetSortKeyOfMenuInMenuBar(const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier) const = 0;
    };

    //! MenuManagerInternalInterface
    //! Internal Interface to query implementation details for menus.
    class MenuManagerInternalInterface
    {
    public:
        AZ_RTTI(MenuManagerInternalInterface, "{59ED06E9-0F68-4CF4-9C2A-4FEFE534AD02}");

        //! Retrieve a QMenu from its identifier.
        //! @param menuIdentifier The identifier for the menu to retrieve.
        //! @return A raw pointer to the QMenu object.
        virtual QMenu* GetMenu(const AZStd::string& menuIdentifier) = 0;

        //! Retrieve a QMenuBar from its identifier.
        //! @param menuBarIdentifier The identifier for the menu bar to retrieve.
        //! @return A raw pointer to the QMenuBar object.
        virtual QMenuBar* GetMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Queues up a menu for a refresh at the end of this tick.
        //! @param menuIdentifier The identifier for the menu to refresh.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenu(const AZStd::string& menuIdentifier) = 0;

        //! Queues up all menus containing the action provided for a refresh at the end of this tick.
        //! @param actionIdentifier The identifier for the action triggering the refresh for menus containing it.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenusContainingAction(const AZStd::string& actionIdentifier) = 0;

        //! Queues up a menuBar for a refresh at the end of this tick.
        //! @param menuIdentifier The identifier for the menuBar to refresh.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Refreshes all menus that were queued up by QueueMenuRefresh.
        virtual void RefreshMenus() = 0;

        //! Refreshes all menuBars that were queued up by QueueMenuBarRefresh.
        virtual void RefreshMenuBars() = 0;
    };

} // namespace AzToolsFramework
