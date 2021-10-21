/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <dlfcn.h>
#include <iostream>
#include <AzCore/IO/Path/Path.h>
#include <AzTest/Platform.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

class ModuleHandle
    : public AZ::Test::IModuleHandle
{
public:
    explicit ModuleHandle(const std::string& lib)
        : m_libHandle(nullptr)
    {
        AZ::IO::FixedMaxPath libext = AZStd::string_view{ lib.c_str(), lib.size() };
        if (!libext.Stem().Native().starts_with(AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX))
        {
           libext = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX + libext.Native();
        }
        if (libext.Extension() != AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION)
        {
            libext.Native() += AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
        }

        m_libHandle = dlopen(libext.c_str(), RTLD_NOW);
        const char* error = dlerror();
        if (error)
        {
            std::cout << "Error from dlopen(): " << error << std::endl;
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

class FunctionHandle
    : public AZ::Test::IFunctionHandle
{
public:
    explicit FunctionHandle(ModuleHandle& module, const std::string& symbol)
        : m_fn(nullptr)
    {
        m_fn = dlsym(module.m_libHandle, symbol.c_str());
        const char* error = dlerror();
        if (error)
        {
            std::cout << "Error from dlsym(): " << error << std::endl;
        }
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

std::shared_ptr<AZ::Test::IFunctionHandle> ModuleHandle::GetFunction(const std::string& name)
{
    return std::make_shared<FunctionHandle>(*this, name);
}

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
            return true;
        }

        std::shared_ptr<IModuleHandle> Platform::GetModule(const std::string& lib)
        {
            return std::make_shared<ModuleHandle>(lib);
        }

        void Platform::WaitForDebugger()
        {
            bool debuggerAttached = false;
            // Apple Technical Q&A QA1361
            // https://developer.apple.com/library/content/qa/qa1361/_index.html
            while (!debuggerAttached)
            {
                const int managementInformationBaseNameLength = 4;
                int managementInformationBaseName[managementInformationBaseNameLength];
                struct kinfo_proc kernelInfo;
                size_t kernelInfoSize = sizeof(kernelInfo);

                // Initialize the flags as they are only set if sysctl succeeds.
                kernelInfo.kp_proc.p_flag = 0;

                // Initialize managementInformationBaseName, which tells sysctl the info we want,
                // in this case we're looking for information about a specific process ID.
                managementInformationBaseName[0] = CTL_KERN;
                managementInformationBaseName[1] = KERN_PROC;
                managementInformationBaseName[2] = KERN_PROC_PID;
                managementInformationBaseName[3] = getpid();

                sysctl(managementInformationBaseName, managementInformationBaseNameLength, &kernelInfo, &kernelInfoSize, nullptr, 0);

                // We're being debugged if the P_TRACED flag is set.
                debuggerAttached = ((kernelInfo.kp_proc.p_flag & P_TRACED) != 0);
            }
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
            size_t end = path.rfind('.');

            return path.substr(start, end);
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
