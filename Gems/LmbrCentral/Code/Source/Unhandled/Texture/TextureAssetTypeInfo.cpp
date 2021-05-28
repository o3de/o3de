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
#include "TextureAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    TextureAssetTypeInfo::~TextureAssetTypeInfo()
    {
        Unregister();
    }

    void TextureAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<TextureAsset>::Uuid());
    }

    void TextureAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(AZ::AzTypeInfo<TextureAsset>::Uuid());
    }

    AZ::Data::AssetType TextureAssetTypeInfo::GetAssetType() const
    {
        return AZ::AzTypeInfo<TextureAsset>::Uuid();
    }

    const char* TextureAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Texture";
    }

    const char* TextureAssetTypeInfo::GetGroup() const
    {
        return "Texture";
    }
    const char* TextureAssetTypeInfo::GetBrowserIcon() const
    {
        return "Icons/PropertyEditor/image_icon.svg";
    }
} // namespace LmbrCentral
