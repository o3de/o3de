/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzTest/AzTest.h>
#include <memory>
#include <string>
#include "Utils.h"

namespace AZ
{
    namespace Test
    {
        const int INCORRECT_USAGE = 101;
        const int LIB_NOT_FOUND = 102;
        const int SYMBOL_NOT_FOUND = 103;

        const int MAX_PRINT_MSG = 4096;

        struct IFunctionHandle;

        //! Handle to the a module/shared library
        struct IModuleHandle
        {
            virtual ~IModuleHandle() = default;

            //! Is the module handle valid (was it loaded successfully)?
            virtual bool IsValid() = 0;

            //! Retrieve a function from within the module
            virtual std::shared_ptr<IFunctionHandle> GetFunction(const std::string& name) = 0;
        };

        //! Handle to a function within the module/shared library
        struct IFunctionHandle
        {
            virtual ~IFunctionHandle() = default;

            //! Is the function handle valid (was it found inside the module correctly)?
            virtual bool IsValid() = 0;

            //! call as a "main" function
            virtual int operator()(int argc, char** argv) = 0;

            //! call as simple function
            virtual int operator()() = 0;
        };

        //! Platform implementation of AzTest scanner
        class Platform
        {
        public:
            bool SupportsWaitForDebugger();
            void WaitForDebugger();
            void SuppressPopupWindows();
            std::shared_ptr<IModuleHandle> GetModule(const std::string& lib);
            std::string GetModuleNameFromPath(const std::string& path);
            void Printf(const char* format, ...);
            AZ::EnvironmentInstance GetTestRunnerEnvironment();
        };

        Platform& GetPlatform();
    } // Test
} // AZ
