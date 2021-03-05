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
#include "GroupAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType groupAssetType("{7629EDD3-A361-49A2-B271-252127097D81}");

    GroupAssetTypeInfo::~GroupAssetTypeInfo()
    {
        Unregister();
    }

    void GroupAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(groupAssetType);
    }

    void GroupAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(groupAssetType);
    }

    AZ::Data::AssetType GroupAssetTypeInfo::GetAssetType() const
    {
        return groupAssetType;
    }

    const char* GroupAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Group";
    }

    const char* GroupAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
