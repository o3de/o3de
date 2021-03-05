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
