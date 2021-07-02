/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
