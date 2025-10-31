/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CharacterPhysicsAssetTypeInfo.h"

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
