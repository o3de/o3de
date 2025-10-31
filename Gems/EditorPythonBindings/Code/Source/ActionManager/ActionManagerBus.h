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

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <Source/ActionManager/PythonEditorAction.h>

namespace EditorPythonBindings
{
    //! ActionManagerRequestBus
    //! Bus to register and trigger actions in the Editor via Python.
    //! If writing C++ code, use the ActionManagerInterface instead.
    class ActionManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Register a new Action to the Action Manager.
        virtual AzToolsFramework::ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& identifier,
            const AzToolsFramework::ActionProperties& properties,
            PythonEditorAction handler
        ) = 0;

        //! Register a new Checkable Action to the Action Manager.
        virtual AzToolsFramework::ActionManagerOperationResult RegisterCheckableAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const AzToolsFramework::ActionProperties& properties,
            PythonEditorAction handler,
            PythonEditorAction updateCallback
        ) = 0;

        //! Trigger an Action via its identifier.
        virtual AzToolsFramework::ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) = 0;

        //! Update the state of a Checkable Action via its identifier.
        virtual AzToolsFramework::ActionManagerOperationResult UpdateAction(const AZStd::string& actionIdentifier) = 0;
    };

    using ActionManagerRequestBus = AZ::EBus<ActionManagerRequests>;

} // namespace EditorPythonBindings
