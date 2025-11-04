/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/span.h>

namespace AZ::RHI
{
    //! This class is used to hold the commmand line arguments that should be passed
    //! to all the applications used during shader compilation.
    //! It supports + and - operators as a way to combine or remove arguments.
    struct ShaderBuildArguments final
    {
        AZ_TYPE_INFO(ShaderBuildArguments, "{3AD6EE90-2BAC-4F8F-822A-F4E1315F6B1B}");

        ShaderBuildArguments() = default;
        ~ShaderBuildArguments() = default;

        // Useful for unit testing
        ShaderBuildArguments(
            bool generateDebugInfo,
            const AZStd::vector<AZStd::string>& preprocessorArguments,
            const AZStd::vector<AZStd::string>& azslcArguments,
            const AZStd::vector<AZStd::string>& dxcArguments,
            const AZStd::vector<AZStd::string>& spirvCrossArguments,
            const AZStd::vector<AZStd::string>& metalAirArguments,
            const AZStd::vector<AZStd::string>& metalLibArguments);

        static void Reflect(ReflectContext* context);

        ShaderBuildArguments operator+(const ShaderBuildArguments& rhs) const;
        ShaderBuildArguments& operator+=(const ShaderBuildArguments& rhs);
        ShaderBuildArguments operator-(const ShaderBuildArguments& rhs) const;
        ShaderBuildArguments& operator-=(const ShaderBuildArguments& rhs);

        // @returns true, if @arg is in @argList
        static bool HasArgument(const AZStd::vector<AZStd::string>& argList, const AZStd::string& arg);

        // Appends to @out all the strings (arguments) from @in, only if, the string is not in @out already.
        static void AppendArguments(AZStd::vector<AZStd::string>& out, const AZStd::vector<AZStd::string>& in);

        // Removes from @out, all the strings (arguments) in @in. It's ok if any of the strings is not found in @out.
        static void RemoveArguments(AZStd::vector<AZStd::string>& out, const AZStd::vector<AZStd::string>& in);

        // Appends definitions as command line arguments to m_preprocessorArguments.
        // @param definitions A list of strings, where each string looks like "MACRO" or "MACRO=VALUE".
        // @returns Returns a negative number if there's an error, for example a string like "-Dmacro" is an error.
        //    If successful it returns the amount of arguments added to m_preprocessorArguments.
        //    Redundant definitions are skipped.
        int AppendDefinitions(const AZStd::vector<AZStd::string>& definitions);

        // @returns A ' ' (space) separated string of all the strings in @argList. It is the typical
        // Array.Join(' ').
        static AZStd::string ListAsString(const AZStd::vector<AZStd::string>& argList);

        //! Convenience flag to enable/disable generation of debugging info
        //! during shader compilation. If true, the appropriate command line arguments will be inserted
        //! to each of the applications used for shader compilation so the expected debug information
        //! is generated at each stage.
        //! Also when this flag is true, the Temp folder will be preserved, even if the shader compilation
        //! is successful. This makes it easier to debug shaders with tools like
        //! RenderDoc or Pix. 
        bool m_generateDebugInfo = false;

        //! Command line arguments for the C Pre-Processor.
        AZStd::vector<AZStd::string> m_preprocessorArguments;

        //! Command line arguments for AZSLc.
        AZStd::vector<AZStd::string> m_azslcArguments;

        //! Command line arguments for DirectxShaderCompiler.
        AZStd::vector<AZStd::string> m_dxcArguments;

        //! Command line arguments for spirv-cross.
        AZStd::vector<AZStd::string> m_spirvCrossArguments;

        //! Command line arguments for '/usr/bin/xcrun metal'.
        AZStd::vector<AZStd::string> m_metalAirArguments;

        //! Command line arguments for '/usr/bin/xcrun metallib'.
        AZStd::vector<AZStd::string> m_metalLibArguments;

    };
}
