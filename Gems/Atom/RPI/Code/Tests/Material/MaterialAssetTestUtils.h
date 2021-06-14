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
        AZ::RPI::MaterialPropertyValue value = asset->GetDefaultPropertyValues()[propertyIndex.GetIndex()];

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
