/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    //! Provides a bus to notify when Python scripts are about to run.
    class EditorPythonScriptNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notifies the start of execution of a Python script using a string.
        virtual void OnStartExecuteByString([[maybe_unused]] AZStd::string_view script) {}

        //! Notifies the start of execution of a Python script using a filename.
        virtual void OnStartExecuteByFilename([[maybe_unused]] AZStd::string_view filename) {}

        //! Notifies the start of execution of a Python script using a filename and args.
        virtual void OnStartExecuteByFilenameWithArgs(
            [[maybe_unused]] AZStd::string_view filename, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args) {}

        //! Notifies the start of execution of a Python script as a test.
        virtual void OnStartExecuteByFilenameAsTest(
            [[maybe_unused]] AZStd::string_view filename, [[maybe_unused]] AZStd::string_view testCase, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args) {}
    };
    using EditorPythonScriptNotificationsBus = AZ::EBus<EditorPythonScriptNotifications>;
}
