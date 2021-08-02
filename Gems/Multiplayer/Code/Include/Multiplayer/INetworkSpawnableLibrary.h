/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/string/string.h>

namespace Multiplayer
{
    //! @class INetworkSpawnableLibrary
    //! @brief The interface for managing network spawnables.
    class INetworkSpawnableLibrary
    {
    public:
        AZ_RTTI(INetworkSpawnableLibrary, "{A3CF809C-6C1D-4B43-B2C4-3901B5DE1ABE}");

        virtual ~INetworkSpawnableLibrary() = default;
        virtual void BuildSpawnablesList() = 0;
        virtual void ProcessSpawnableAsset(const AZStd::string& relativePath, AZ::Data::AssetId id) = 0;
        virtual AZ::Name GetSpawnableNameFromAssetId(AZ::Data::AssetId assetId) = 0;
        virtual AZ::Data::AssetId GetAssetIdByName(AZ::Name name) = 0;
    };
}
