/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        //! Notifies the execution of a Python script using a string.
        virtual void OnExecuteByString([[maybe_unused]] AZStd::string_view script) {}

        //! Notifies the execution of a Python script using a filename.
        virtual void OnExecuteByFilename([[maybe_unused]] AZStd::string_view filename) {}

        //! Notifies the execution of a Python script using a filename and args.
        virtual void OnExecuteByFilenameWithArgs(
            [[maybe_unused]] AZStd::string_view filename, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args) {}

        //! Notifies the execution of a Python script as a test.
        virtual void OnExecuteByFilenameAsTest(
            [[maybe_unused]] AZStd::string_view filename, [[maybe_unused]] AZStd::string_view testCase, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args) {}
    };
    using EditorPythonScriptNotificationsBus = AZ::EBus<EditorPythonScriptNotifications>;
}
