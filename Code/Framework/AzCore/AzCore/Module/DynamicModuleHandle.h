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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Module/Environment.h>

namespace AZ
{
    class Module;

    /**
     * Handles platform-specific interaction with dynamic modules
     * (aka DLLs, aka dynamic link libraries, aka shared libraries).
     */
    class DynamicModuleHandle
    {
    public:
        /// Platform-specific implementation should call Unload().
        virtual ~DynamicModuleHandle() = default;

        // no copy
        DynamicModuleHandle(const DynamicModuleHandle&) = delete;
        DynamicModuleHandle& operator=(const DynamicModuleHandle&) = delete;

        /// Creates a platform-specific DynamicModuleHandle.
        /// Note that the specified module is not loaded until \ref Load is called.
        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName);

        /// Loads the module.
        /// Invokes the \ref InitializeDynamicModuleFunction if it is found in the module and this is the first time loading the module.
        /// \param isInitializeFunctionRequired Whether a missing \ref InitializeDynamicModuleFunction
        ///                                     causes the Load to fail.
        ///
        /// \return True if the module loaded successfully.
        bool Load(bool isInitializeFunctionRequired);

        /// Unload the module.
        /// Invokes the \ref UninitializeDynamicModuleFunction if it is found in the module and
        /// this was the first handle to load the module.
        /// \return true if the module unloaded successfully.
        bool Unload();

        /// \return whether the module has been successfully loaded.
        virtual bool IsLoaded() const = 0;

        /// \return the module's name on disk.
        const OSString& GetFilename() const { return m_fileName; }

        /// Returns a function from the module.
        /// nullptr is returned if the function is inaccessible
        /// or the module is not loaded.
        template<typename Function>
        Function GetFunction(const char* functionName) const
        {
            if (IsLoaded())
            {
                return reinterpret_cast<Function>(GetFunctionAddress(functionName));
            }
            else
            {
                return nullptr;
            }
        }

    protected:
        DynamicModuleHandle(const char* fileFullName);

        enum class LoadStatus
        {
            LoadSuccess,
            LoadFailure,
            AlreadyLoaded
        };

        // Attempt to load a module.
        virtual LoadStatus LoadModule() = 0;
        virtual bool       UnloadModule() = 0;
        virtual void*      GetFunctionAddress(const char* functionName) const = 0;

        /// Store the module name for future loads.
        OSString        m_fileName;

        // whoever first creates this variable is the one who called initialize and will 
        // be the one that calls uninitialize.  Note that we don't actually care what the bool value is
        // just the refcount and ownership.
        AZ::EnvironmentVariable<bool> m_initialized;  
    };

    /**
     * Functions that the \ref AZ::ComponentApplication expects
     * to find in a dynamic module.
     */

    /// \code
    /// extern "C" AZ_DLL_EXPORT
    /// void InitializeDynamicModule(void* sharedEnvironment)
    /// \endcode
    /// The very first function invoked in a dynamic module.
    /// Implementations should attach to the environment.
    /// \param sharedEnvironment is an \ref AZ::EnvironmentInstance.
    using InitializeDynamicModuleFunction = void(*)(void* sharedEnvironment);
    const char InitializeDynamicModuleFunctionName[] = "InitializeDynamicModule";

    /// \code
    /// extern "C" AZ_DLL_EXPORT
    /// void UninitializeDynamicModule()
    /// \endcode
    /// The very last function invoked in a dynamic module.
    /// Implementations should detach from the AZ::Environment.
    using UninitializeDynamicModuleFunction = void(*)();
    const char UninitializeDynamicModuleFunctionName[] = "UninitializeDynamicModule";

    /// \code
    /// extern "C" AZ_DLL_EXPORT
    /// AZ::Module* CreateModuleClass()
    /// \endcode
    /// Function by which a dynamic module creates its AZ::Module class.
    /// This will be called after InitializeDynamicModule().
    using CreateModuleClassFunction = AZ::Module * (*)();
    const char CreateModuleClassFunctionName[] = "CreateModuleClass";

    /// \code
    /// extern "C" AZ_DLL_EXPORT
    /// void DestroyModuleClass(AZ::Module* module)
    /// \endcode
    /// Function by which a dynamic module destroys its AZ::Module class.
    /// This will be callled before UninitializeDynamicModule().
    using DestroyModuleClassFunction = void(*)(AZ::Module* module);
    const char DestroyModuleClassFunctionName[] = "DestroyModuleClass";
} // namespace AZ
