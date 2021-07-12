/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace Multiplayer
{
    /// Implementation of the network prefab library interface.
    class NetworkSpawnableLibrary final
        : public INetworkSpawnableLibrary
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        AZ_RTTI(NetworkSpawnableLibrary, "{65E15F33-E893-49C2-A8E2-B6A8A6EF31E0}", INetworkSpawnableLibrary);

        NetworkSpawnableLibrary();
        ~NetworkSpawnableLibrary();

        /// INetworkSpawnableLibrary overrides.
        void BuildSpawnablesList() override;
        void ProcessSpawnableAsset(const AZStd::string& relativePath, AZ::Data::AssetId id) override;
        AZ::Name GetSpawnableNameFromAssetId(AZ::Data::AssetId assetId) override;
        AZ::Data::AssetId GetAssetIdByName(AZ::Name name) override;

        /// AssetCatalogEventBus overrides.
        void OnCatalogLoaded(const char* catalogFile) override;

    private:
        AZStd::unordered_map<AZ::Name, AZ::Data::AssetId> m_spawnables;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Name> m_spawnablesReverseLookup;
    };
}
