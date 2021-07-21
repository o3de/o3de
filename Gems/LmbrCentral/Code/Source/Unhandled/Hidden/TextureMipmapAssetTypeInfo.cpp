/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "TextureMipmapAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType textureMipmapType("{3918728C-D3CA-4D9E-813E-A5ED20C6821E}");

    TextureMipmapAssetTypeInfo::~TextureMipmapAssetTypeInfo()
    {
        Unregister();
    }

    void TextureMipmapAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(textureMipmapType);
    }

    void TextureMipmapAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(textureMipmapType);
    }

    AZ::Data::AssetType TextureMipmapAssetTypeInfo::GetAssetType() const
    {
        return textureMipmapType;
    }

    const char* TextureMipmapAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Texture Mipmap";
    }

    const char* TextureMipmapAssetTypeInfo::GetGroup() const
    {
        return "Hidden";
    }
} // namespace LmbrCentral
