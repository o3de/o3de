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

#include "LmbrCentral_precompiled.h"
#include "MaterialAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    // MaterialAssetTypeInfo

    MaterialAssetTypeInfo::~MaterialAssetTypeInfo()
    {
        Unregister();
    }

    void MaterialAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<MaterialAsset>::Uuid());
    }

    void MaterialAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<MaterialAsset>::Uuid());
    }

    AZ::Data::AssetType MaterialAssetTypeInfo::GetAssetType() const
    {
        return AZ::AzTypeInfo<MaterialAsset>::Uuid();
    }

    const char* MaterialAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Material";
    }

    const char* MaterialAssetTypeInfo::GetGroup() const
    {
        return "Material";
    }

    const char* MaterialAssetTypeInfo::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Decal.svg";
    }

    AZ::Uuid MaterialAssetTypeInfo::GetComponentTypeId() const
    {
        return AZ::Uuid("{BA3890BD-D2E7-4DB6-95CD-7E7D5525567A}");
    }

    // DccMaterialAssetTypeInfo

    DccMaterialAssetTypeInfo::~DccMaterialAssetTypeInfo()
    {
        Unregister();
    }

    void DccMaterialAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<DccMaterialAsset>::Uuid());
    }

    void DccMaterialAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<DccMaterialAsset>::Uuid());
    }

    AZ::Data::AssetType DccMaterialAssetTypeInfo::GetAssetType() const
    {
        return AZ::AzTypeInfo<DccMaterialAsset>::Uuid();
    }

    const char* DccMaterialAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "DccMaterial";
    }

    const char* DccMaterialAssetTypeInfo::GetGroup() const
    {
        return "DccMaterial";
    }

    const char* DccMaterialAssetTypeInfo::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/Decal.svg";
    }
} // namespace LmbrCentral
