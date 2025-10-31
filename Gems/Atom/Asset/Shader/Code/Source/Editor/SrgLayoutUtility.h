/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
                [[maybe_unused]] const char* builderName,
                const SrgDataContainer& resourceGroups,
                RPI::ShaderResourceGroupLayoutList& srgLayoutList);

        }  // SrgLayoutUtility namespace
    } // ShaderBuilder namespace
} // AZ
