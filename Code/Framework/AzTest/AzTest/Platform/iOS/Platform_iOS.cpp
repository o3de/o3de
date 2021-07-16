/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <dlfcn.h>
#include <iostream>
#include <AzTest/Platform.h>

#include <unistd.h>
#include <sys/resource.h>
#include <fstream>

class ModuleHandle
    : public AZ::Test::IModuleHandle
{
public:
    explicit ModuleHandle(const std::string& lib)
        : m_libHandle(nullptr)
    {
        std::string libPath = lib + ".framework/" + lib;

        std::cout << "Calling dlopen " << libPath << std::endl;
        m_libHandle = dlopen(libPath.c_str(), RTLD_NOW);

        const char* err = dlerror();
        if (err)
        {
            std::cerr << "error: " << err << std::endl;
        }
    }

    ModuleHandle(const ModuleHandle&) = delete;
    ModuleHandle& operator=(const ModuleHandle&) = delete;

    ~ModuleHandle() override
    {
        if (m_libHandle)
        {
            dlclose(m_libHandle);
        }
    }

    bool IsValid() override { return m_libHandle != nullptr; }

    std::shared_ptr<AZ::Test::IFunctionHandle> GetFunction(const std::string& name) override;

private:
    friend class FunctionHandle;

    void* m_libHandle;
};

//==============================================================================
class FunctionHandle
    : public AZ::Test::IFunctionHandle
{
public:
    FunctionHandle(ModuleHandle& module, const std::string& symbol)
        : m_fn(nullptr)
    {
        m_fn = dlsym(module.m_libHandle, symbol.c_str());
    }

    FunctionHandle(const FunctionHandle&) = delete;
    FunctionHandle& operator=(const FunctionHandle&) = delete;

    ~FunctionHandle() override = default;

    int operator()(int argc, char** argv) override
    {
        using Fn = int(int, char**);

        Fn* fn = reinterpret_cast<Fn*>(m_fn);
        return (*fn)(argc, argv);
    }

    int operator()() override
    {
        using Fn = int();

        Fn* fn = reinterpret_cast<Fn*>(m_fn);
        return (*fn)();
    }

    bool IsValid() override { return m_fn != nullptr; }

private:
    void* m_fn;
};

//==============================================================================
std::shared_ptr<AZ::Test::IFunctionHandle> ModuleHandle::GetFunction(const std::string& name)
{
    return std::make_shared<FunctionHandle>(*this, name);
}

//==============================================================================
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
            return std::make_shared<ModuleHandle>(lib);
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
            size_t start = path.rfind('/');
            if (start == std::string::npos)
            {
                start = 0;
            }
            size_t end = path.size();

            return path.substr(start, end);
        }

        void Platform::Printf([[maybe_unused]] const char* format, ...)
        {
            // Not currently supported
        }

        AZ::EnvironmentInstance Platform::GetTestRunnerEnvironment()
        {
            return nullptr;
        }
    } // Test
} // AZ
