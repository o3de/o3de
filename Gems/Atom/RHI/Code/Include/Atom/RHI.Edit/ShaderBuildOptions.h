/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <Atom/RHI.Edit/ShaderBuildArguments.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Represents the content of the files located at:
    //! "@ENGINEROOT@/Gems/Atom/Config/shader_build_options.json"
    struct ShaderBuildOptions
    {
        AZ_TYPE_INFO(ShaderBuildOptions, "{1FC1C54D-8B53-4212-9113-F682ACC4D782}");

        static void Reflect(ReflectContext* context);

        RHI::ShaderBuildArguments m_removeBuildArguments;  // These arguments are removed first.
        RHI::ShaderBuildArguments m_addBuildArguments;     // Later, these arguments are added.

        // This field exists as a convenience/alias of the preprocessor definitions.
        // When building shaders, this is what developers customize most of the time.
        // The values in the list look like this:
        //     "MACRO" or "MACRO=VALUE".
        // These values will be automatically appended to m_addBuildArguments.m_preprocessorArguments as:
        //     "-DMACRO" or "-DMACRO=VALUE".
        AZStd::vector<AZStd::string> m_definitions;

    };
}
