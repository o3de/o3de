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

#include "EntityIconAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType entityIconAssetType("{3436C30E-E2C5-4C3B-A7B9-66C94A28701B}");

    EntityIconAssetTypeInfo::~EntityIconAssetTypeInfo()
    {
        Unregister();
    }

    void EntityIconAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(entityIconAssetType);
    }

    void EntityIconAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(entityIconAssetType);
    }

    AZ::Data::AssetType EntityIconAssetTypeInfo::GetAssetType() const
    {
        return entityIconAssetType;
    }

    const char* EntityIconAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Entity Icon";
    }

    const char* EntityIconAssetTypeInfo::GetGroup() const
    {
        return "UI";
    }

    const char * EntityIconAssetTypeInfo::GetBrowserIcon() const
    {
        return "Icons/PropertyEditor/image_icon.svg";
    }
} // namespace LmbrCentral
