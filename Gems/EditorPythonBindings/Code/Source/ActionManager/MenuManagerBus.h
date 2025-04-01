/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

namespace EditorPythonBindings
{
    //! MenuManagerRequestBus
    //! Bus to register menus and add menu items to them in the Editor via Python.
    //! If writing C++ code, use the MenuManagerInterface instead.
    class MenuManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Register a new Menu to the Menu Manager.
        virtual AzToolsFramework::MenuManagerOperationResult RegisterMenu(
            const AZStd::string& identifier, const AzToolsFramework::MenuProperties& properties) = 0;

        //! Bind an Action to a Menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add a Separator to a Menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Add a Sub-Menu to a Menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;

        //! Add a Widget to a Menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddWidgetToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex) = 0;

        //! Add a Menu to a Menu Bar.
        virtual AzToolsFramework::MenuManagerOperationResult AddMenuToMenuBar(
            const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex) = 0;
    };

    using MenuManagerRequestBus = AZ::EBus<MenuManagerRequests>;

} // namespace EditorPythonBindings
