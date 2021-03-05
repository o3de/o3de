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
#include "PrefabsLibraryAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType prefabsLibraryAssetType("{2DC3C556-9461-4729-8313-2BA0CB64EF52}");

    PrefabsLibraryAssetTypeInfo::~PrefabsLibraryAssetTypeInfo()
    {
        Unregister();
    }

    void PrefabsLibraryAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(prefabsLibraryAssetType);
    }

    void PrefabsLibraryAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(prefabsLibraryAssetType);
    }

    AZ::Data::AssetType PrefabsLibraryAssetTypeInfo::GetAssetType() const
    {
        return prefabsLibraryAssetType;
    }

    const char* PrefabsLibraryAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Prefabs Library";
    }

    const char* PrefabsLibraryAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
