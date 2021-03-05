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
#include "CharacterPhysicsAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    static AZ::Data::AssetType characterPhysicsType("{29D64D95-E46D-4D66-9BD4-552C139A73DC}");

    CharacterPhysicsAssetTypeInfo::~CharacterPhysicsAssetTypeInfo()
    {
        Unregister();
    }

    void CharacterPhysicsAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(characterPhysicsType);
    }

    void CharacterPhysicsAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(characterPhysicsType);
    }

    AZ::Data::AssetType CharacterPhysicsAssetTypeInfo::GetAssetType() const
    {
        return characterPhysicsType;
    }

    const char* CharacterPhysicsAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Character Physics";
    }

    const char* CharacterPhysicsAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
