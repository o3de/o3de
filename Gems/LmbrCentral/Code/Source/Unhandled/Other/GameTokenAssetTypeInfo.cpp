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
