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

        //! Bind an action to a menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add a separator to a menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Add a sub-menu to a menu.
        virtual AzToolsFramework::MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;
    };

    using MenuManagerRequestBus = AZ::EBus<MenuManagerRequests>;

} // namespace EditorPythonBindings
