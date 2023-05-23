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

#include <QToolBar>

namespace AzToolsFramework
{
    using ToolBarManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using ToolBarManagerIntegerResult = AZ::Outcome<int, AZStd::string>;
    using ToolBarManagerStringResult = AZ::Outcome<AZStd::string, AZStd::string>;

    //! Provides additional properties to initialize a ToolBar upon registration.
    struct ToolBarProperties
    {
        AZ_RTTI(ToolBarProperties, "{49B21EA2-5449-47EC-AEFC-3DBC0288CED0}");

        ToolBarProperties() = default;
        virtual ~ToolBarProperties() = default;

        AZStd::string m_name = "";
    };

    //! ToolBarManagerInterface
    //! Interface to register and manage ToolBars in the Editor.
    class ToolBarManagerInterface
    {
    public:
        AZ_RTTI(ToolBarManagerInterface, "{2736A3CA-B260-4355-B61D-E287A3DB2A6F}");

        //! Register a new ToolBar to the ToolBar Manager.
        //! @param toolBarIdentifier The identifier for the ToolBar that is being registered.
        //! @param properties The properties object for the newly registered ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult RegisterToolBar(const AZStd::string& toolBarIdentifier, const ToolBarProperties& properties) = 0;

        //! Register a new ToolBar Area to the ToolBar Manager.
        //! @param toolBarAreaIdentifier The identifier for the toolbar area that is being registered.
        //! @param mainWindow Pointer to the QMainWindow to associate the toolbar area with.
        //! @param toolBarArea Enum of which part of the QMainWindow the toolbar area will cover.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult RegisterToolBarArea(
            const AZStd::string& toolBarAreaIdentifier, QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea) = 0;

        //! Add an Action to a ToolBar.
        //! @param toolBarIdentifier The identifier for the ToolBar the action is being added to.
        //! @param actionIdentifier The identifier for the action to add to the ToolBar.
        //! @param sortIndex An integer defining the position the action should appear in the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddActionToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, int sortIndex) = 0;

        //! Add an Action with a submenu to a ToolBar.
        //! @param toolBarIdentifier The identifier for the ToolBar the action is being added to.
        //! @param actionIdentifier The identifier for the action to add to the ToolBar.
        //! @param subMenuIdentifier The identifier for the menu to add to the ToolBar next to the action.
        //! @param sortIndex An integer defining the position the action should appear in the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddActionWithSubMenuToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) = 0;

        //! Add multiple Actions to a ToolBar. Saves time as it only updates the toolbar once at the end.
        //! @param toolBarIdentifier The identifier for the ToolBar the actions are being added to.
        //! @param actions A vector of pairs of identifiers for the actions to add to the toolbar and their sort position.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddActionsToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions) = 0;

        //! Removes an Action from a ToolBar.
        //! @param toolBarIdentifier The identifier for the ToolBar the action is being removed from.
        //! @param actionIdentifier The identifier for the action to remove from the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult RemoveActionFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) = 0;

        //! Removes multiple Actions from a Menu.
        //! @param toolBarIdentifier The identifier for the ToolBar the actions are being removed from.
        //! @param actionIdentifiers A vector of identifiers for the actions to remove from the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult RemoveActionsFromToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers) = 0;

        //! Add a Separator to a ToolBar.
        //! @param toolBarIdentifier The identifier for the ToolBar the separator is being added to.
        //! @param sortIndex An integer defining the position the separator should appear in the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddSeparatorToToolBar(
            const AZStd::string& toolBarIdentifier, int sortIndex) = 0;

        //! Add a Widget to a ToolBar.
        //! @param toolBarIdentifier The identifier for the ToolBar the widget is being added to.
        //! @param widgetActionIdentifier The identifier for the widget to add to the ToolBar.
        //! @param sortIndex An integer defining the position the widget should appear in the ToolBar.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddWidgetToToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex) = 0;

        //! Add a ToolBar to a ToolBar Area.
        //! @param toolBarAreaIdentifier The identifier for the toolbar area the toolbar is being added to.
        //! @param toolBarIdentifier The identifier for the toolbar to add to the toolbar area.
        //! @param sortIndex An integer defining the position the toolbar should appear in the toolbar area.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerOperationResult AddToolBarToToolBarArea(
            const AZStd::string& toolBarAreaIdentifier, const AZStd::string& toolBarIdentifier, int sortIndex) = 0;

        //! Generate an instance of a ToolBar from its identifier.
        //! The requester should take care of correctly parenting and deleting the ToolBar once it is no longer needed.
        //! Note that the ToolBar Manager system will still retain control over the contents of the ToolBar
        //! and clear and re-populate it when necessary.
        //! @param toolBarIdentifier The identifier for the ToolBar to generate.
        //! @return A raw pointer to the new QToolBar object.
        virtual QToolBar* GenerateToolBar(const AZStd::string& toolBarIdentifier) = 0;

        //! Retrieve the sort key of an action in a toolbar from its identifier.
        //! @param toolBarIdentifier The identifier for the toolbar to query.
        //! @param actionIdentifier The identifier for the action whose sort key to get in the toolbar.
        //! @return A successful outcome object containing the sort key, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerIntegerResult GetSortKeyOfActionInToolBar(const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve the sort key of a widget action in a toolbar from its identifier.
        //! @param toolBarIdentifier The identifier for the toolbar to query.
        //! @param widgetActionIdentifier The identifier for the widget whose sort key to get in the toolbar.
        //! @return A successful outcome object containing the sort key, or a string with a message detailing the error in case of failure.
        virtual ToolBarManagerIntegerResult GetSortKeyOfWidgetInToolBar(
            const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier) const = 0;
    };

} // namespace AzToolsFramework
