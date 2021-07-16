/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
