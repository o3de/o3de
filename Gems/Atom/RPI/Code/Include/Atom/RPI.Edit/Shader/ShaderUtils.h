/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    namespace RPI
    {
        namespace ShaderUtils
        {
            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile);
        }
    }
}
