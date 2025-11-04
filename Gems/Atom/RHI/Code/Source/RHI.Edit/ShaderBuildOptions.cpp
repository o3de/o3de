/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/ShaderBuildOptions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/regex.h>

#include <Atom/RHI.Edit/Utils.h>

namespace AZ::RHI
{
    void ShaderBuildOptions::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderBuildOptions>()
                ->Version(1)
                ->Field("RemoveBuildArguments", &ShaderBuildOptions::m_removeBuildArguments)
                ->Field("AddBuildArguments", &ShaderBuildOptions::m_addBuildArguments)
                ->Field("Definitions", &ShaderBuildOptions::m_definitions)
                ;
        }
    }
}
