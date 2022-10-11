/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Common/TestUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <AzCore/Name/Name.h>

namespace UnitTest
{
    template<typename T>
    void CheckPropertyValue(const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& asset, const AZ::Name& propertyName, T expectedValue)
    {
        const AZ::RPI::MaterialPropertyIndex propertyIndex = asset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName);

        EXPECT_TRUE(propertyIndex.IsValid());

        AZ::RPI::MaterialPropertyValue value = asset->GetDefaultPropertyValues()[propertyIndex.GetIndex()];

        EXPECT_TRUE(value == expectedValue);
    }

    template<typename T>
    void CheckMaterialPropertyValue(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& asset, const AZ::Name& propertyName, T expectedValue)
    {
        const AZ::RPI::MaterialPropertyIndex propertyIndex = asset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName);

        EXPECT_TRUE(propertyIndex.IsValid());

        AZ::RPI::MaterialPropertyValue value = asset->GetPropertyValues()[propertyIndex.GetIndex()];

        EXPECT_TRUE(value == expectedValue);
    }

    void AddMaterialPropertyForSrg(
        AZ::RPI::MaterialTypeAssetCreator& materialTypeCreator,
        const AZ::Name& propertyName,
        AZ::RPI::MaterialPropertyDataType dataType,
        const AZ::Name& shaderInputName);

    void AddCommonTestMaterialProperties(AZ::RPI::MaterialTypeAssetCreator& materialTypeCreator, AZStd::string propertyGroupPrefix = "");

    AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> CreateCommonTestMaterialSrgLayout();

} //namespace UnitTest
