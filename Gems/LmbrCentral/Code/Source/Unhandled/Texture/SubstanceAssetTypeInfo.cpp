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
