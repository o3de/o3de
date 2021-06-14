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

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace UnitTest
{

    //! Utility function for creating the simplest possible ShaderAsset
    AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateTestShaderAsset(
        const AZ::Data::AssetId& shaderAssetId,
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> optionalSrgLayout = nullptr,
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> optionalShaderOptions = nullptr,
        const AZ::Name& shaderName = AZ::Name{ "TestShader" },
        const AZ::Name& drawListName = AZ::Name{ "depth" } );

} //namespace UnitTest
