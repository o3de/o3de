/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "EntityPrototypeLibraryAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType entityPrototypeLibraryAssetType("{B034F8AB-D881-4A35-A408-184E3FDEB2FE}");

    EntityPrototypeLibraryAssetTypeInfo::~EntityPrototypeLibraryAssetTypeInfo()
    {
        Unregister();
    }

    void EntityPrototypeLibraryAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(entityPrototypeLibraryAssetType);
    }

    void EntityPrototypeLibraryAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(entityPrototypeLibraryAssetType);
    }

    AZ::Data::AssetType EntityPrototypeLibraryAssetTypeInfo::GetAssetType() const
    {
        return entityPrototypeLibraryAssetType;
    }

    const char* EntityPrototypeLibraryAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Entity Prototype Library";
    }

    const char* EntityPrototypeLibraryAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
