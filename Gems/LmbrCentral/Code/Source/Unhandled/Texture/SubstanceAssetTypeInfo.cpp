/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "SubstanceAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType substanceType("{71F9D30E-13F7-40D6-A3C9-E5358004B31F}");

    SubstanceAssetTypeInfo::~SubstanceAssetTypeInfo()
    {
        Unregister();
    }

    void SubstanceAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(substanceType);
    }

    void SubstanceAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(substanceType);
    }

    AZ::Data::AssetType SubstanceAssetTypeInfo::GetAssetType() const
    {
        return substanceType;
    }

    const char* SubstanceAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Substance";
    }

    const char* SubstanceAssetTypeInfo::GetGroup() const
    {
        return "Texture";
    }
} // namespace LmbrCentral
