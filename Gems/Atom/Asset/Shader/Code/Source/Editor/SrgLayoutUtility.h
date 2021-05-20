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

#include <AzCore/base.h>

#include "CommonFiles/CommonTypes.h"
#include <Atom/RPI.Reflect/Shader/ShaderAsset2.h>
#include "ShaderBuilderUtility.h" 

namespace AZ
{
    namespace ShaderBuilder
    {
        namespace SrgLayoutUtility
        {

            bool LoadShaderResourceGroupLayouts(
                [[maybe_unused]] const char* builderName, const SrgDataContainer& resourceGroups, const bool platformUsesRegisterSpaces,
                RPI::ShaderResourceGroupLayoutList& srgLayoutList);

        }  // SrgLayoutUtility namespace
    } // ShaderBuilder namespace
} // AZ
