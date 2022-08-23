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

#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

namespace EditorPythonBindings
{
    //! ToolBarManagerRequestBus
    //! Bus to register and manage ToolBars in the Editor via Python.
    //! If writing C++ code, use the ToolBarManagerInterface instead.
    class ToolBarManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Register a new ToolBar to the ToolBar Manager.
        virtual AzToolsFramework::ToolBarManagerOperationResult RegisterToolBar(
            const AZStd::string& toolBarIdentifier, const AzToolsFramework::ToolBarProperties& properties) = 0;

        //! Add an Action to a ToolBar.
        virtual AzToolsFramework::ToolBarManagerOperationResult AddActionToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add an Action with a submenu to a ToolBar.
        virtual AzToolsFramework::ToolBarManagerOperationResult AddActionWithSubMenuToToolBar(
            const AZStd::string& toolBarIdentifier,
            const AZStd::string& actionIdentifier,
            const AZStd::string& subMenuIdentifier,
            int sortIndex) = 0;

        //! Add multiple Actions to a ToolBar. Saves time as it only updates the toolbar once at the end.
        virtual AzToolsFramework::ToolBarManagerOperationResult AddActionsToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions) = 0;

        //! Removes an Action from a ToolBar.
        virtual AzToolsFramework::ToolBarManagerOperationResult RemoveActionFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) = 0;

        //! Removes multiple Actions from a Menu.
        virtual AzToolsFramework::ToolBarManagerOperationResult RemoveActionsFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers) = 0;

        //! Add a Separator to a ToolBar.
        virtual AzToolsFramework::ToolBarManagerOperationResult AddSeparatorToToolBar(
            const AZStd::string& toolBarIdentifier, int sortIndex) = 0;

        //! Add a Widget to a ToolBar.
        virtual AzToolsFramework::ToolBarManagerOperationResult AddWidgetToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex) = 0;

        //! Retrieve the sort key of an action in a toolbar from its identifier.
        virtual AzToolsFramework::ToolBarManagerIntegerResult GetSortKeyOfActionInToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve the sort key of a widget action in a toolbar from its identifier.
        virtual AzToolsFramework::ToolBarManagerIntegerResult GetSortKeyOfWidgetInToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier) const = 0;
    };

    using ToolBarManagerRequestBus = AZ::EBus<ToolBarManagerRequests>;

} // namespace EditorPythonBindings
