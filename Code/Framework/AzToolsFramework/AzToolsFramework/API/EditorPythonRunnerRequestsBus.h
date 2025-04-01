/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    /**
      * Provides a bus to run Python scripts
      */
    class EditorPythonRunnerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! executes a Python script using a string, prints the result if printResult is true and script is an expression
        virtual void ExecuteByString([[maybe_unused]] AZStd::string_view script, [[maybe_unused]] bool printResult) {}

        //! executes a Python script using a filename
        virtual bool ExecuteByFilename([[maybe_unused]] AZStd::string_view filename)
        {
            return false;
        }

        //! executes a Python script using a filename and args
        virtual bool ExecuteByFilenameWithArgs(
            [[maybe_unused]] AZStd::string_view filename, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args)
        {
            return false;
        }

        //! executes a Python script as a test
        virtual bool ExecuteByFilenameAsTest(
            [[maybe_unused]] AZStd::string_view filename,
            [[maybe_unused]] AZStd::string_view testCase,
            [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args)
        {
            return false;
        }
    };
    using EditorPythonRunnerRequestBus = AZ::EBus<EditorPythonRunnerRequests>;

} // namespace AzToolsFramework

