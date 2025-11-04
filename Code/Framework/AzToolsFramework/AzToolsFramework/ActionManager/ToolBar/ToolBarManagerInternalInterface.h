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

namespace AzToolsFramework
{
    using ToolBarManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using ToolBarManagerStringResult = AZ::Outcome<AZStd::string, AZStd::string>;

    //! ToolBarManagerInternalInterface
    //! Internal Interface to query implementation details for toolBars.
    class ToolBarManagerInternalInterface
    {
    public:
        AZ_RTTI(ToolBarManagerInternalInterface, "{55B9CA70-5277-4B8A-8F76-8C1F2A75D558}");

        //! Queues up a toolBar for a refresh at the end of this tick.
        //! @param toolBarIdentifier The identifier for the toolbar to refresh.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult QueueToolBarRefresh(const AZStd::string& toolBarIdentifier) = 0;

        //! Queues up all toolbars containing the action provided for a refresh at the end of this tick.
        //! @param actionIdentifier The identifier for the action triggering the refresh for toolbars containing it.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult QueueRefreshForToolBarsContainingAction(const AZStd::string& actionIdentifier) = 0;

        //! Refreshes all toolbars that were queued up for refresh.
        virtual void RefreshToolBars() = 0;

        //! Refreshes all toolbar areas that were queued up for refresh.
        virtual void RefreshToolBarAreas() = 0;

        //! Serialize a toolbar by its identifier.
        virtual ToolBarManagerStringResult SerializeToolBar(const AZStd::string& toolBarIdentifier) = 0;

        //! Completely reset the ToolBar Manager from all items registered after initialization.
        //! Clears all ToolBars and ToolBar Areas.
        //! Used in Unit tests to allow clearing the environment between runs.
        virtual void Reset() = 0;
    };

} // namespace AzToolsFramework
