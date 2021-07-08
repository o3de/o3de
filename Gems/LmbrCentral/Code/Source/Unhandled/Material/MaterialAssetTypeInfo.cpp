/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        return "Icons/Components/Decal.svg";
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
        return "Icons/Components/Decal.svg";
    }
} // namespace LmbrCentral
