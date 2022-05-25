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
        virtual MenuManagerOperationResult RegisterMenu(const AZStd::string& identifier, const MenuProperties& properties) = 0;

        //! Bind an action to a menu.
        virtual MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add a separator to a menu.
        virtual MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) = 0;

        //! Add a sub-menu to a menu.
        virtual MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;

        //! Retrieve a QMenu from its identifier.
        virtual QMenu* GetMenu(const AZStd::string& menuIdentifier) = 0;
    };

} // namespace AzToolsFramework
