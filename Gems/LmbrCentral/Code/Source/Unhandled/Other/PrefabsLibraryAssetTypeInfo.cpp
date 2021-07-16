/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
