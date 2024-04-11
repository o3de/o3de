/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/PlatformDef.h>

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
        /// Flags used for loading a dynamic module.
        enum class LoadFlags : uint32_t
        {
            None                = 0,
            InitFuncRequired    = 1 << 0, /// Whether a missing \ref InitializeDynamicModuleFunction causes the Load to fail.
            GlobalSymbols       = 1 << 1, /// On platforms that support it, make the module's symbols global and available for
                                          /// the relocation processing of other modules. Otherwise, the symbols need to be queried manually.
            NoLoad              = 1 << 2  /// Don't load the library (only get a handle if it's already loaded).
                                          /// This can be used to test if the library is already resident.
        };

        /// Platform-specific implementation should call Unload().
        virtual ~DynamicModuleHandle() = default;

        // no copy
        DynamicModuleHandle(const DynamicModuleHandle&) = delete;
        DynamicModuleHandle& operator=(const DynamicModuleHandle&) = delete;

        /// Creates a platform-specific DynamicModuleHandle.
        /// \param fullFileName         The file name of the dynamic module to load
        /// \param correctModuleName    Option to correct the filename to conform to the current platform's dynamic module naming convention. 
        ///                             (i.e. lib<ModuleName>.so on unix-like platforms)
        ///
        /// Note that the specified module is not loaded until \ref Load is called.
        /// \return Unique ptr to the newly created dynamic module handler.
        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName, bool correctModuleName = true);

        /// Loads the module.
        /// Invokes the \ref InitializeDynamicModuleFunction if it is found in the module and this is the first time loading the module.
        /// \param isInitializeFunctionRequired Whether a missing \ref InitializeDynamicModuleFunction
        ///                                     causes the Load to fail.
        /// \param globalSymbols                On platforms that support it, make the module's symbols global and available for the relocation processing of other modules. Otherwise, the symbols 
        ///                                     need to be queried manually.
        ///
        /// \return True if the module loaded successfully.
        AZ_DEPRECATED_MESSAGE("This method has been deprecated. Please use DynamicModuleHandle::Load(LoadFlags flags) function instead")
        bool Load(bool isInitializeFunctionRequired, bool globalSymbols = false);

        /// Loads the module.
        /// Invokes the \ref InitializeDynamicModuleFunction if it is found in the module and this is the first time loading the module.
        /// \param flags Flags to control the loading of the module. \ref LoadFlags
        ///
        /// \return True if the module loaded successfully.
        bool Load(LoadFlags flags = LoadFlags::None);

        /// Unload the module.
        /// Invokes the \ref UninitializeDynamicModuleFunction if it is found in the module and
        /// this was the first handle to load the module.
        /// \return true if the module unloaded successfully.
        bool Unload();

        /// \return whether the module has been successfully loaded.
        virtual bool IsLoaded() const = 0;

        /// \return the module's name on disk.
        const char* GetFilename() const { return m_fileName.c_str(); }

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

        template<typename T>
        constexpr bool CheckBitsAny(T v, T bits)
        {
            return (v & bits) != T{};
        }

        // Attempt to load a module.
        virtual LoadStatus LoadModule(LoadFlags flags) = 0;
        virtual bool       UnloadModule() = 0;
        virtual void*      GetFunctionAddress(const char* functionName) const = 0;

        /// Store the module name for future loads.
        AZ::IO::FixedMaxPathString m_fileName;

        // whoever first creates this variable is the one who called initialize and will
        // be the one that calls uninitialize.  Note that we don't actually care what the bool value is
        // just the refcount and ownership.
        AZ::EnvironmentVariable<bool> m_initialized;
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(DynamicModuleHandle::LoadFlags);

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
    using InitializeDynamicModuleFunction = void(*)();
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
