/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        virtual void ExecuteByString(AZStd::string_view script, bool printResult) { AZ_UNUSED(script); AZ_UNUSED(printResult); }

        //! executes a Python script using a filename
        virtual void ExecuteByFilename(AZStd::string_view filename) { AZ_UNUSED(filename); }

        //! executes a Python script using a filename and args
        virtual void ExecuteByFilenameWithArgs(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) { AZ_UNUSED(filename); AZ_UNUSED(args);  }

        //! executes a Python script as a test
        virtual void ExecuteByFilenameAsTest(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) { AZ_UNUSED(filename); AZ_UNUSED(args); }
    };
    using EditorPythonRunnerRequestBus = AZ::EBus<EditorPythonRunnerRequests>;

} // namespace AzToolsFramework

