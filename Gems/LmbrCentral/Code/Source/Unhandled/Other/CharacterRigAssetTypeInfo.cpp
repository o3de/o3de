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
#include "CharacterRigAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType characterRigType("{B8C75662-D585-4B4A-B1A4-F9DFE3E174F0}");

    CharacterRigAssetTypeInfo::~CharacterRigAssetTypeInfo()
    {
        Unregister();
    }

    void CharacterRigAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(characterRigType);
    }

    void CharacterRigAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(characterRigType);
    }

    AZ::Data::AssetType CharacterRigAssetTypeInfo::GetAssetType() const
    {
        return characterRigType;
    }

    const char* CharacterRigAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Character Rig";
    }

    const char* CharacterRigAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
