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

#include <AzToolsFramework/ActionManager/Action/EditorActionUtils.h>

class QWidget;

namespace AzToolsFramework
{
    using ActionManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using ActionManagerBooleanResult = AZ::Outcome<bool, AZStd::string>;
    using ActionManagerGetterResult = AZ::Outcome<AZStd::string, AZStd::string>;

    //! Action Context Properties object.
    //! Used to streamline registration of an Action Context.
    struct ActionContextProperties
    {
        AZ_RTTI(ActionContextProperties, "{74694A62-E3FF-43EE-98DF-D66731DC2286}");

        ActionContextProperties() = default;
        virtual ~ActionContextProperties() = default;

        AZStd::string m_name; //!< The friendly name for the Action Context.
    };

    //! Action Properties object.
    //! Used to streamline registration of an Action.
    struct ActionProperties
    {
        AZ_RTTI(ActionProperties, "{B84A0BDD-4D15-4078-B6AE-240F825358F7}");

        ActionProperties() = default;
        virtual ~ActionProperties() = default;

        AZStd::string m_name; //!< The friendly name for the Action. Used in menu items and tooltips.
        AZStd::string m_description; //!< The description for the Action.
        AZStd::string m_category; //!< The category for the Action to be used in UI.
        AZStd::string m_iconPath; //!< The qrc path to the icon to be used in UI.
        //! Determines in which mode this action should be accessible.
        //! Empty means action will always appear regardless of the mode.
        AZStd::vector<AZStd::string> m_modes;

        //! Determines visibility for this action in Menus.
        ActionVisibility m_menuVisibility = ActionVisibility::HideWhenDisabled;
        //! Determines visibility for this action in ToolBars.
        ActionVisibility m_toolBarVisibility = ActionVisibility::OnlyInActiveMode;
    };

    //! Widget Action Properties object.
    //! Used to streamline registration of a Widget Action.
    struct WidgetActionProperties
    {
        AZ_RTTI(WidgetActionProperties, "{9A72C602-ABAA-4010-8806-4EEA32D5F716}");

        WidgetActionProperties() = default;
        virtual ~WidgetActionProperties() = default;

        AZStd::string m_name; //!< The friendly name for the Widget Action.
        AZStd::string m_category; //!< The category for the Widget Action to be used in UI.
    };

    //! ActionManagerInterface
    //! Interface to register and trigger actions in the Editor.
    class ActionManagerInterface
    {
    public:
        AZ_RTTI(ActionManagerInterface, "{2E2A421E-0842-4F90-9F5C-DDE0C4F820DE}");

        //! Register a new Action Context to the Action Manager.
        //! @param contextIdentifier The identifier for the newly registered action context.
        //! @param properties The properties object for the newly registered action context.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterActionContext(
            const AZStd::string& contextIdentifier,
            const ActionContextProperties& properties
        ) = 0;

        //! Returns whether an action context with the identifier queried is registered to the Action Manager.
        //! @param contextIdentifier The identifier for the action context to query.
        //! @return True if an Action Context with the identifier provided was found, false otherwise.
        virtual bool IsActionContextRegistered(const AZStd::string& contextIdentifier) const = 0;

        //! Register a new Mode for an Action Context to the Action Manager.
        //! @param actionContextIdentifier The identifier for the action context the newly registered mode should be added to.
        //! @param modeIdentifier The new value for the category property of the widget action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterActionContextMode(
            const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier) = 0;

        //! Register a new Action to the Action Manager.
        //! @param contextIdentifier The identifier for the action context the newly registered action should be added to.
        //! @param actionIdentifier The identifier for the newly registered action.
        //! @param properties The properties object for the newly registered action.
        //! @param handler The handler function that should be called when the action is triggered.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const ActionProperties& properties,
            AZStd::function<void()> handler
        ) = 0;

        //! Register a new Checkable Action to the Action Manager.
        //! @param contextIdentifier The identifier for the action context the newly registered action should be added to.
        //! @param actionIdentifier The identifier for the newly registered action.
        //! @param properties The properties object for the newly registered action.
        //! @param handler The handler function that should be called when the action is triggered.
        //! @param checkStateCallback The handler function that will be called when the action's checked state needs to be updated.
        //!                           Returns a boolean that will be used to set the checked value of the action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterCheckableAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const ActionProperties& properties,
            AZStd::function<void()> handler,
            AZStd::function<bool()> checkStateCallback) = 0;

        //! Returns whether an action with the identifier queried is registered to the Action Manager.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if an Action with the identifier provided was found, false otherwise.
        virtual bool IsActionRegistered(const AZStd::string& actionIdentifier) const = 0;

        //! Get an Action's name via its identifier.
        //! @param actionIdentifier The action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetActionName(const AZStd::string& actionIdentifier) = 0;

        //! Set an Action's name via its identifier.
        //! @param actionIdentifier The action identifier to set the value to.
        //! @param name The new value for the name property of the action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetActionName(const AZStd::string& actionIdentifier, const AZStd::string& name) = 0;

        //! Get an Action's description via its identifier.
        //! @param actionIdentifier The action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetActionDescription(const AZStd::string& actionIdentifier) = 0;

        //! Set an Action's description via its identifier.
        //! @param actionIdentifier The action identifier to set the value to.
        //! @param description The new value for the description property of the action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetActionDescription(const AZStd::string& actionIdentifier, const AZStd::string& description) = 0;

        //! Get an Action's category via its identifier.
        //! @param actionIdentifier The action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetActionCategory(const AZStd::string& actionIdentifier) = 0;

        //! Set an Action's category via its identifier.
        //! @param actionIdentifier The action identifier to set the value to.
        //! @param category The new value for the category property of the action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetActionCategory(const AZStd::string& actionIdentifier, const AZStd::string& category) = 0;

        //! Get an Action's icon path via its identifier.
        //! @param actionIdentifier The action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetActionIconPath(const AZStd::string& actionIdentifier) = 0;

        //! Set an Action's icon path via its identifier.
        //! @param actionIdentifier The action identifier to set the value to.
        //! @param iconPath The new value for the icon path property of the action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetActionIconPath(const AZStd::string& actionIdentifier, const AZStd::string& iconPath) = 0;

        //! Generates a sort key from the action's name that can be used to sort alphabetically.
        //! @param actionIdentifier The action identifier to query.
        //! @return A sortKey if the action was found, or the max integer otherwise.
        virtual int GenerateActionAlphabeticalSortKey(const AZStd::string& actionIdentifier) = 0;

        //! Returns the enabled state for the action.
        //! @param actionIdentifier The action identifier to query.
        //! @return A successful outcome object with the enabled state, or a string with a message detailing the error in case of failure.
        virtual ActionManagerBooleanResult IsActionEnabled(const AZStd::string& actionIdentifier) const = 0;

        //! Trigger an Action via its identifier.
        //! @param actionIdentifier The identifier for the action to trigger.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) = 0;

        //! Installs an enabled state callback to an action that will set its enabled state when the action is updated.
        //! An action can only have a single enabled state callback. The function will fail if called multiple times.
        //! @param actionIdentifier The identifier for the action to install the callback to.
        virtual ActionManagerOperationResult InstallEnabledStateCallback(
            const AZStd::string& actionIdentifier,
            AZStd::function<bool()> enabledStateCallback
        ) = 0;

        //! Update the state of an Action via its identifier.
        //! This will update both the enabled and checked state.
        //! @param actionIdentifier The identifier for the action to update.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult UpdateAction(const AZStd::string& actionIdentifier) = 0;

        //! Register a new Action Updater to the Action Manager.
        //! An Action Updater stores a list of Action identifiers that will be updated when the Updater condition is met.
        //! The system that registers the Action Updater is expected to trigger the TriggerActionUpdater function appropriately.
        //! @param actionUpdaterIdentifier The identifier for the newly registered action updater.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterActionUpdater(const AZStd::string& actionUpdaterIdentifier) = 0;

        //! Adds an action identifier to the updater's list.
        //! @param actionUpdaterIdentifier The identifier for the updater to add the action to.
        //! @param actionIdentifier The identifier for the action to add the updater's list.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult AddActionToUpdater(const AZStd::string& actionUpdaterIdentifier, const AZStd::string& actionIdentifier) = 0;

        //! Trigger an update on all actions registered to the Action Updater.
        //! @param actionUpdaterIdentifier The identifier for the action updater to trigger an update on.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult TriggerActionUpdater(const AZStd::string& actionUpdaterIdentifier) = 0;

        //! Register a new Widget Action to the Action Manager.
        //! @param widgetActionIdentifier The identifier for the newly registered widget action.
        //! @param properties The properties object for the newly registered widget action.
        //! @param generator The generator function that will be called to create the widget when invoked.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterWidgetAction(
            const AZStd::string& widgetActionIdentifier,
            const WidgetActionProperties& properties,
            AZStd::function<QWidget*()> generator
        ) = 0;

        //! Returns whether a widget action with the identifier queried is registered to the Action Manager.
        //! @param widgetActionIdentifier The identifier for the widget action to query.
        //! @return True if a WidgetAction with the identifier provided was found, false otherwise.
        virtual bool IsWidgetActionRegistered(const AZStd::string& widgetActionIdentifier) const = 0;

        //! Get a Widget Action's name via its identifier.
        //! @param widgetActionIdentifier The widget action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetWidgetActionName(const AZStd::string& widgetActionIdentifier) = 0;

        //! Set a Widget Action's name via its identifier.
        //! @param widgetActionIdentifier The widget action identifier to set the value to.
        //! @param name The new value for the name property of the widget action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetWidgetActionName(
            const AZStd::string& widgetActionIdentifier, const AZStd::string& name) = 0;

        //! Get a Widget Action's category via its identifier.
        //! @param widgetActionIdentifier The widget action identifier to get the value from.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetWidgetActionCategory(const AZStd::string& widgetActionIdentifier) = 0;

        //! Set a Widget Action's category via its identifier.
        //! @param widgetActionIdentifier The widget action identifier to set the value to.
        //! @param category The new value for the category property of the widget action.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetWidgetActionCategory(
            const AZStd::string& widgetActionIdentifier, const AZStd::string& category) = 0;

        //! Sets an additional mode for an action via its identifier.
        //! @param modeIdentifier The action context mode identifier to add to the action.
        //! @param actionIdentifier The action to set the mode to.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult AssignModeToAction(
            const AZStd::string& modeIdentifier, const AZStd::string& actionIdentifier) = 0;

        //! Returns whether the Action is active in the Mode its Action Context is currently in.
        //! @param actionIdentifier The action to query.
        //! @return A successful outcome object with the result, or a string with a message detailing the error in case of failure.
        virtual ActionManagerBooleanResult IsActionActiveInCurrentMode(const AZStd::string& actionIdentifier) const = 0;

        //! Sets the active mode for an action context via its identifier.
        //! @param actionContextIdentifier The action context to set the active mode to.
        //! @param modeIdentifier The action context mode identifier to set as active.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult SetActiveActionContextMode(
            const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier) = 0;

        //! Gets the active mode for an action context via its identifier.
        //! @param actionContextIdentifier The action context to get the active mode of.
        //! @return A successful outcome object containing the value, or a string with a message detailing the error in case of failure.
        virtual ActionManagerGetterResult GetActiveActionContextMode(const AZStd::string& actionContextIdentifier) const = 0;
    };

} // namespace AzToolsFramework
