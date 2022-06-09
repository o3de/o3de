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

        //! Bind an Action to a Menu.
        //! @param menuIdentifier The identifier for the menu the action is being added to.
        //! @param actionIdentifier The identifier for the action to add to the menu.
        //! @param sortIndex An integer defining the position the action should appear in the menu.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

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

        //! Retrieve a QMenu from its identifier.
        //! @param menuIdentifier The identifier for the menu to retrieve.
        //! @return A raw pointer to the QMenu object.
        virtual QMenu* GetMenu(const AZStd::string& menuIdentifier) = 0;

        //! Retrieve a QMenuBar from its identifier.
        //! @param menuBarIdentifier The identifier for the menu bar to retrieve.
        //! @return A raw pointer to the QMenuBar object.
        virtual QMenuBar* GetMenuBar(const AZStd::string& menuBarIdentifier) = 0;
    };

} // namespace AzToolsFramework
