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

#include <Source/ActionManager/PythonEditorAction.h>

namespace EditorPythonBindings
{
    using ActionManagerOperationResult = AZ::Outcome<void, AZStd::string>;

    //! ActionManagerRequestBus
    //! Bus to register and trigger actions in the Editor via Python.
    //! If writing C++ code, use the ActionManagerInterface instead.
    class ActionManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Register a new Action to the Action Manager.
        virtual ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& identifier,
            const AZStd::string& name,
            const AZStd::string& description,
            const AZStd::string& category,
            const AZStd::string& iconPath,
            PythonEditorAction handler) = 0;

        //! Trigger an Action via its identifier.
        virtual ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) = 0;
    };

    using ActionManagerRequestBus = AZ::EBus<ActionManagerRequests>;

} // namespace EditorPythonBindings
