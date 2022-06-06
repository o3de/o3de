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
        //! @return An void outcome if successful, or a string with a message detailing the error otherwise.
        virtual MenuManagerOperationResult RegisterMenu(const AZStd::string& menuIdentifier, const MenuProperties& properties) = 0;

        //! Register a new Menu Bar to the Menu Manager.
        //! @param menuBarIdentifier The identifier for the menu bar that is being registered.
        //! @return An void outcome if successful, or a string with a message detailing the error otherwise.
        virtual MenuManagerOperationResult RegisterMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Bind an Action to a Menu.
        //! @param menuIdentifier The identifier for the menu the action is being added to.
        //! @param actionIdentifier The identifier for the action to add to the menu.
        //! @param sortIndex A positive integer describing the position the action should appear in in the menu.
        //! @return An void outcome if successful, or a string with a message detailing the error otherwise.
        virtual MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add a Separator to a Menu.
        virtual MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Add a Sub-Menu to a Menu.
        virtual MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;

        //! Add a Menu to a Menu Bar.
        virtual MenuManagerOperationResult AddMenuToMenuBar(
            const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Retrieve a QMenu from its identifier.
        virtual QMenu* GetMenu(const AZStd::string& menuIdentifier) = 0;

        //! Retrieve a QMenuBar from its identifier.
        virtual QMenuBar* GetMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Get the sort key for an action in a menu.
        virtual int GetSortKey(const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) = 0;
    };

} // namespace AzToolsFramework
