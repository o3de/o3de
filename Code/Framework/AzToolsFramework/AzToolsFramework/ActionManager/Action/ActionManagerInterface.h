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
#include <AzCore/std/function/function_base.h>

#include <QWidget>

class QAction;

namespace AzToolsFramework
{
    using ActionManagerOperationResult = AZ::Outcome<void, AZStd::string>;
    using ActionManagerBooleanResult = AZ::Outcome<bool, AZStd::string>;
    using ActionManagerGetterResult = AZ::Outcome<AZStd::string, AZStd::string>;

    struct ActionContextProperties
    {
        AZ_RTTI(ActionContextProperties, "{74694A62-E3FF-43EE-98DF-D66731DC2286}");

        ActionContextProperties() = default;
        virtual ~ActionContextProperties() = default;

        AZStd::string m_name = "";
    };

    struct ActionProperties
    {
        AZ_RTTI(ActionProperties, "{B84A0BDD-4D15-4078-B6AE-240F825358F7}");

        ActionProperties() = default;
        virtual ~ActionProperties() = default;

        AZStd::string m_name = "";
        AZStd::string m_description = "";
        AZStd::string m_category = "";
        AZStd::string m_iconPath = "";
        bool m_hideFromMenusWhenDisabled = true;
        bool m_hideFromToolBarsWhenDisabled = false;
    };

    //! ActionManagerInterface
    //! Interface to register and trigger actions in the Editor.
    class ActionManagerInterface
    {
    public:
        AZ_RTTI(ActionManagerInterface, "{2E2A421E-0842-4F90-9F5C-DDE0C4F820DE}");

        //! Register a new Action Context to the Action Manager.
        //! @param parentContextIdentifier The identifier for the action context the newly registered context should be parented to.
        //! @param contextIdentifier The identifier for the newly registered action context.
        //! @param properties The properties object for the newly registered action context.
        //! @param widget The owning widget for the newly registered action context.
        //! @return A successful outcome object, or a string with a message detailing the error in case of failure.
        virtual ActionManagerOperationResult RegisterActionContext(
            const AZStd::string& parentContextIdentifier,
            const AZStd::string& contextIdentifier,
            const ActionContextProperties& properties,
            QWidget* widget
        ) = 0;

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
            AZStd::function<bool()> checkStateCallback
        ) = 0;

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
    };

    //! ActionManagerInternalInterface
    //! Internal Interface to query implementation details for actions.
    class ActionManagerInternalInterface
    {
    public:
        AZ_RTTI(ActionManagerInternalInterface, "{2DCEB7AB-B07A-4085-B5AF-6EEB37439ED6}");

        //! Retrieve a QAction via its identifier.
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw pointer to the QAction, or nullptr if the action could not be found.
        virtual QAction* GetAction(const AZStd::string& actionIdentifier) = 0;

        //! Retrieve a QAction via its identifier (const version).
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw const pointer to the QAction, or nullptr if the action could not be found.
        virtual const QAction* GetActionConst(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve whether an Action should be hidden from Menus when disabled.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual bool GetHideFromMenusWhenDisabled(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve whether an Action should be hidden from ToolBars when disabled.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual bool GetHideFromToolBarsWhenDisabled(const AZStd::string& actionIdentifier) const = 0;
    };

} // namespace AzToolsFramework
