/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "GameTokenAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType gameTokenAssetType("{1D4B56F8-366A-4040-B645-AE87E3A00DAB}");

    GameTokenAssetTypeInfo::~GameTokenAssetTypeInfo()
    {
        Unregister();
    }

    void GameTokenAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(gameTokenAssetType);
    }

    void GameTokenAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(gameTokenAssetType);
    }

    AZ::Data::AssetType GameTokenAssetTypeInfo::GetAssetType() const
    {
        return gameTokenAssetType;
    }

    const char* GameTokenAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Game Tokens Library";
    }

    const char* GameTokenAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
