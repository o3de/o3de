/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/Platform.h>
#include <iostream>

class ModuleHandle
    : public AZ::Test::IModuleHandle
{
public:
    explicit ModuleHandle(const std::string&)
    {
    }

    ModuleHandle(const ModuleHandle&) = delete;
    ModuleHandle& operator=(const ModuleHandle&) = delete;

    ~ModuleHandle() override = default;

    bool IsValid() override { return false; }

    std::shared_ptr<AZ::Test::IFunctionHandle> GetFunction(const std::string& name) override
    {
        return {};
    }
};

class FunctionHandle
    : public AZ::Test::IFunctionHandle
{
public:
    FunctionHandle(ModuleHandle&, const std::string&)
    {
    }

    FunctionHandle(const FunctionHandle&) = delete;
    FunctionHandle& operator=(const FunctionHandle&) = delete;

    ~FunctionHandle() override = default;

    int operator()(int, char**) override
    {
        return -1;
    }

    int operator()() override
    {
        return -1;
    }

    bool IsValid() override { return false; }
};

namespace AZ
{
    namespace Test
    {
        Platform& GetPlatform()
        {
            static Platform s_platform;
            return s_platform;
        }

        bool Platform::SupportsWaitForDebugger()
        {
            return false;
        }

        std::shared_ptr<IModuleHandle> Platform::GetModule(const std::string& lib)
        {
            return {};
        }

        void Platform::WaitForDebugger()
        {
            std::cerr << "Platform does not support waiting for debugger." << std::endl;
        }

        void Platform::SuppressPopupWindows()
        {
        }

        std::string Platform::GetModuleNameFromPath(const std::string& path)
        {
            return {};
        }

        void Platform::Printf(const char* format, ...)
        {
            // Not currently supported
        }

        AZ::EnvironmentInstance Platform::GetTestRunnerEnvironment()
        {
            // Not currently supported
            return nullptr;
        }
    } // Test
} // AZ
