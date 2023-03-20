/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TextureAssetTypeInfo.h"

#include <LmbrCentral/Rendering/TextureAsset.h>

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
