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
#include "SkeletonParamsAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType skeletonParamsType("{4BBB785A-6824-4803-A607-F9323E7BEEF1}");

    SkeletonParamsAssetTypeInfo::~SkeletonParamsAssetTypeInfo()
    {
        Unregister();
    }

    void SkeletonParamsAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(skeletonParamsType);
    }

    void SkeletonParamsAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(skeletonParamsType);
    }

    AZ::Data::AssetType SkeletonParamsAssetTypeInfo::GetAssetType() const
    {
        return skeletonParamsType;
    }

    const char* SkeletonParamsAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Skeleton Params";
    }

    const char* SkeletonParamsAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
