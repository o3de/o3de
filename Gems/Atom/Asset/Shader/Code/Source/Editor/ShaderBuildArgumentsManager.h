/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>

#include "CommonFiles/CommonTypes.h"
#include <Atom/RHI.Edit/ShaderBuildArguments.h> 


namespace UnitTest
{
    class ShaderBuildArgumentsTests;
}

namespace AZ
{
    namespace ShaderBuilder
    {
        //! This class manages a stack of ShaderBuildArguments.
        //! It simplifies command line argument definition at each level
        //! of the shader build hierarchy:
        //! 
        //! {
        //! push(global arguments)
        //!     {
        //!     push(PlatformInfo arguments)
        //!         {
        //!         push(RHI arguments)
        //!             {
        //!             push(.shader arguments)
        //!                 {
        //!                 push(shader.supervariant arguments)
        //!                 ...
        //!                 -- build shader with current arguments. --
        //!                 ...
        //!                 pop()
        //!                 }
        //!             pop() 
        //!             }
        //!         pop()
        //!         }
        //!     pop()
        //!     }
        //! pop()
        //! }
        //!
        //! At each push(), two set of arguments are necessary, the "remove" set and the "add" set.
        //! The idea is that it allows deep customization of all the shader build arguments by removing
        //! or adding arguments at each level.
        class ShaderBuildArgumentsManager final
        {
        public:
            AZ_CLASS_ALLOCATOR(ShaderBuildArgumentsManager, AZ::SystemAllocator);

            static constexpr char LogName[] = "ShaderBuildArgumentsManager";

            // The value of this registry key is customizable by the user.
            static constexpr char ConfigPathRegistryKey[] = "/O3DE/Atom/Shaders/Build/ConfigPath";

            static constexpr char DefaultConfigPathDirectory[] = "@gemroot:AtomShader@/Assets/Config/Shader";
            static constexpr char ShaderBuildOptionsJson[] = "shader_build_options.settings";
            static constexpr char PlatformsDir[] = "Platform";

            //! Always loads all the factory arguments provided by the Atom Gem. In addition
            //! it checks if the user customized all or some of the arguments with the registry key: @ConfigPathRegistryKey (see above)
            void Init();

            //! Pushes a new scope of arguments into the stack . The arguments to push are searched internally by the given @name,
            //! but if such arguments are not found, which is a common situation, then the current set of arguments at the top of the stack
            //! are pushed again on top of the stack, so subsequent calls to PopArgumentScope() work seamlessly.
            //! @param name Substring of the internally owned set of arguments.
            //! For example if the user wants to use the arguments for dx12 on Windows ("Windows.dx12"),
            //! then it is expected that this function should be called twice, as follows:
            //!     PushArguments("Windows")
            //!     PushArguments("dx12")
            //! The names come from the directory structure under the Platform/ folder:
            //! - Platform/
            //!   - Windows/  (Platform name)
            //!     - dx12/   (RHI name)
            //!     - vulkan/ (RHI name)
            //! @returns The resulting (combined, with - and +) set of arguments at the top of the stack.
            const AZ::RHI::ShaderBuildArguments& PushArgumentScope(const AZStd::string& name);

            //! Similar to above, but the arguments being pushed are anonymous.
            //! @param removeArguments List of arguments to remove from the top of the stack.
            //! @param addArguments List of arguments to add to the top of the stack.
            //! @param definitions Additional arguments, specialized for the C-preprocessor, of the form "MACRO", or "MACRO=VALUE".
            //! @returns The resulting (combined, with - and +) set of arguments at the top of the stack.
            const AZ::RHI::ShaderBuildArguments& PushArgumentScope(const AZ::RHI::ShaderBuildArguments& removeArguments,
                                                               const AZ::RHI::ShaderBuildArguments& addArguments,
                                                               const AZStd::vector<AZStd::string>& definitions);

            //! @returns The resulting (combined, with - and +) set of arguments at the top of the stack.
            const AZ::RHI::ShaderBuildArguments& GetCurrentArguments() const;

            //! Your typical stack popping function.
            //! @remark: The "" (global) arguments are never popped, regardless of how many times this function is called.
            void PopArgumentScope();

            //! Finds the shader build config files from the default locations. Returns a map where the key is the name of the scope,
            //! and the value is a fully qualified file path.
            //! Remarks: Posible scope names are:
            //!     "global"
            //!     "<platform>". Example "Android", "Windows", etc
            //!     "<platform>.<rhi>". Example "Windows.dx12" or "Windows.vulkan".
            static AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath> DiscoverConfigurationFiles();

        private:
            friend class ::UnitTest::ShaderBuildArgumentsTests;
            void Init(AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && removeBuildArgumentsMap
                    , AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && addBuildArgumentsMap);

            //! @returns A fully qualified path where the factory settings, as provided by Atom, are found.
            static AZ::IO::FixedMaxPath GetDefaultConfigDirectoryPath();

            //! @returns A fully qualified path where the user customized command line arguments are found.
            //!     The returned path will be empty if the user did not customize the path in the registry.
            static AZ::IO::FixedMaxPath GetUserConfigDirectoryPath();

            //! @param dirPath Starting directory for the search of  shader_build_options.json files.
            //! @returns A map where the key is the name of the scope, and the value is a fully qualified file path.
            //! Remarks: Posible scope names are:
            //!     "global"
            //!     "<platform>". Example "Android", "Windows", etc
            //!     "<platform>.<rhi>". Example "Windows.dx12" or "Windows.vulkan".
            static AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath> DiscoverConfigurationFilesInDirectory(const AZ::IO::FixedMaxPath& dirPath);

            const AZ::RHI::ShaderBuildArguments& PushArgumentsInternal(const AZStd::string& name, const AZ::RHI::ShaderBuildArguments& arguments);

            //! In this map we store which arguments should be removed for a fully qualified scope of arguments.
            //! A fully qualified scope name can be:
            //!     "Windows" or "Windows.dx12" or "Windows.vulkan".
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> m_removeBuildArgumentsMap;

            //! In this map we store which arguments should be added for a fully qualified scope of arguments.
            //! A fully qualified scope name can be:
            //!     "" (The global scope) or "Windows" or "Windows.dx12" or "Windows.vulkan".
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> m_addBuildArgumentsMap;

            AZStd::stack<AZ::RHI::ShaderBuildArguments> m_argumentsStack;
            AZStd::stack<AZStd::string> m_argumentsNameStack;
        };
    } // ShaderBuilder namespace
} // AZ
