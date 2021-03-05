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
#include "FontAssetTypeInfo.h"

namespace LmbrCentral
{       
    static AZ::Data::AssetType fontAssetType("{57767D37-0EBE-43BE-8F60-AB36D2056EF8}");

    FontAssetTypeInfo::~FontAssetTypeInfo()
    {
        Unregister();
    }

    void FontAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(fontAssetType);
    }

    void FontAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(fontAssetType);
    }

    AZ::Data::AssetType FontAssetTypeInfo::GetAssetType() const
    {
        return fontAssetType;
    }

    const char* FontAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Font";
    }
    
    const char * FontAssetTypeInfo::GetGroup() const
    {
        return "UI";
    }
} // namespace LmbrCentral
