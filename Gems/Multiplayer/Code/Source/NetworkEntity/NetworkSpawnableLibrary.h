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

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzCore/Name/Name.h>

namespace Multiplayer
{
    /// Implementation of the network prefab library interface.
    class NetworkSpawnableLibrary final
        : private AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        NetworkSpawnableLibrary();
        ~NetworkSpawnableLibrary();

        void BuildPrefabsList();
        void ProcessSpawnableAsset(const AZStd::string& relativePath, AZ::Data::AssetId id);

        /// AssetCatalogEventBus overrides.
        void OnCatalogLoaded(const char* catalogFile) override;

        AZ::Name GetPrefabNameFromAssetId(AZ::Data::AssetId assetId);
        AZ::Data::AssetId GetAssetIdByName(AZ::Name name);

    private:
        AZStd::unordered_map<AZ::Name, AZ::Data::AssetId> m_spawnables;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Name> m_spawnablesReverseLookup;
    };
}
