/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace Multiplayer
{
    /// Implementation of the network prefab library interface.
    class NetworkSpawnableLibrary final
        : public INetworkSpawnableLibrary
    {
    public:
        AZ_RTTI(NetworkSpawnableLibrary, "{65E15F33-E893-49C2-A8E2-B6A8A6EF31E0}", INetworkSpawnableLibrary);

        NetworkSpawnableLibrary();
        ~NetworkSpawnableLibrary();

        //! INetworkSpawnableLibrary overrides.
        //! @{
        // Iterates over all assets (on-disk and in-memory) and stores any spawnables that are "network.spawnables"
        // This allows users to look up network spawnable assets by name or id if needed later
        void BuildSpawnablesList() override;
        void ProcessSpawnableAsset(const AZStd::string& relativePath, AZ::Data::AssetId id) override;
        AZ::Name GetSpawnableNameFromAssetId(AZ::Data::AssetId assetId) override;
        AZ::Data::AssetId GetAssetIdByName(AZ::Name name) override;
        //! @}

    private:
        AZStd::unordered_map<AZ::Name, AZ::Data::AssetId> m_spawnables;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Name> m_spawnablesReverseLookup;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_criticalAssetsHandler;
    };
}
