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
