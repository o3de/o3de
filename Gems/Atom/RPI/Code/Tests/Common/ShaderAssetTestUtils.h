/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
