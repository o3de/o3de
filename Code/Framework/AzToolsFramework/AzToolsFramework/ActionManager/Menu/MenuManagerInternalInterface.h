/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/string/string.h>

class QMenu;
class QMenuBar;
class QWidget;

namespace AzToolsFramework
{
    using MenuManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using MenuManagerStringResult = AZ::Outcome<AZStd::string, AZStd::string>;

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

        //! Queues up a menu for a refresh at the end of this tick.
        //! @param menuIdentifier The identifier for the menu to refresh.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenu(const AZStd::string& menuIdentifier) = 0;

        //! Queues up all menus containing the action provided for a refresh at the end of this tick.
        //! @param actionIdentifier The identifier for the action triggering the refresh for menus containing it.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenusContainingAction(const AZStd::string& actionIdentifier) = 0;

        //! Queues up all menus containing the sub-menu for a refresh at the end of this tick.
        //! @param subMenuIdentifier The identifier for the sub-menu triggering the refresh for menus containing it.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenusContainingSubMenu(const AZStd::string& subMenuIdentifier) = 0;

        //! Queues up a menuBar for a refresh at the end of this tick.
        //! @param menuIdentifier The identifier for the menuBar to refresh.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual MenuManagerOperationResult QueueRefreshForMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Refreshes all menus that were queued up by QueueMenuRefresh.
        virtual void RefreshMenus() = 0;

        //! Refreshes all menuBars that were queued up by QueueMenuBarRefresh.
        virtual void RefreshMenuBars() = 0;

        //! Serialize a menu by its identifier.
        virtual MenuManagerStringResult SerializeMenu(const AZStd::string& menuIdentifier) = 0;

        //! Serialize a menu bar by its identifier.
        virtual MenuManagerStringResult SerializeMenuBar(const AZStd::string& menuBarIdentifier) = 0;

        //! Completely reset the Menu Manager from all items registered after initialization.
        //! Clears all Menus and Menu Bars.
        //! Used in Unit tests to allow clearing the environment between runs.
        virtual void Reset() = 0;
    };

} // namespace AzToolsFramework
