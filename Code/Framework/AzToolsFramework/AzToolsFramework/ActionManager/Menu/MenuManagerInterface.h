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
    //! MenuManagerInterface
    //! Interface to register and manage menus in the Editor.
    class MenuManagerInterface
    {
    public:
        AZ_RTTI(MenuManagerInterface, "{D70B7989-62BD-447E-ADF6-0971EC4B7DEE}");

        //! Bind an action to a menu.
        virtual void AddActionToMenu(
            AZStd::string_view actionIdentifier, AZStd::string_view menuIdentifier, int sortIndex) = 0;

        // Retrieve a QMenu from its identifier.
        virtual QMenu* GetMenu(AZStd::string_view menuIdentifier) = 0;
    };

} // namespace AzToolsFramework
