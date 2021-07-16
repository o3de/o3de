/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CommonFiles/Preprocessor.h>
#include <Atom/RHI.Edit/ShaderCompilerArguments.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        //! represents the json config file contents of project-wide shader_global_build_options's file.
        struct GlobalBuildOptions final
        {
            AZ_RTTI(GlobalBuildOptions, "{F7F1247D-A417-43E1-9B52-84DD226A9E1A}");

            static void Reflect(AZ::ReflectContext* context);

            //! include paths and defines
            PreprocessorOptions m_preprocessorSettings;

            //! command line arguments related to warnings, optimizations, matrices order and others.
            RHI::ShaderCompilerArguments m_compilerArguments;
        };

        //! Reads the global options used when compiling shaders. The options are defined in <GameProject>/Config/shader_global_build_options.json
        //! @param builderName: A string with the name of the builder calling this API. Used for trace debugging.
        //! @param optionalIncludeFolder: An additional directory to add to the list of include folders for the C-preprocessor.
        GlobalBuildOptions ReadBuildOptions(const char* builderName, const char* optionalIncludeFolder = nullptr);
    }
}
