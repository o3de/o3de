/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        //! @builderName: A string with the name of the builder calling this API. Used for trace debugging.
        //! @optionalIncludeFolder: An additional directory to add to the list of include folders for the C-preprocessor.
        GlobalBuildOptions ReadBuildOptions(const char* builderName, const char* optionalIncludeFolder = nullptr);
    }
}
