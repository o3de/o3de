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
#include "SkeletonAssetTypeInfo.h"

namespace LmbrCentral
{       
    static AZ::Data::AssetType skeletonType("{60161B46-21F0-4396-A4F0-F2CCF0664CDE}");

    SkeletonAssetTypeInfo::~SkeletonAssetTypeInfo()
    {
        Unregister();
    }

    void SkeletonAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(skeletonType);
    }

    void SkeletonAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(skeletonType);
    }

    AZ::Data::AssetType SkeletonAssetTypeInfo::GetAssetType() const
    {
        return skeletonType;
    }

    const char* SkeletonAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Skeleton";
    }

    const char* SkeletonAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
