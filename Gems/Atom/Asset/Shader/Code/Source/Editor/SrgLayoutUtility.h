/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#include "CommonFiles/CommonTypes.h"
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
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
