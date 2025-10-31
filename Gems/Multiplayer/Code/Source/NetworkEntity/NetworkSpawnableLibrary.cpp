/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/NetworkSpawnableLibrary.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzCore/Interface/Interface.h>
#include <Multiplayer/MultiplayerConstants.h>

namespace Multiplayer
{
    NetworkSpawnableLibrary::NetworkSpawnableLibrary()
    {
        AZ::Interface<INetworkSpawnableLibrary>::Register(this);
        if (auto settingsRegistry{ AZ::SettingsRegistry::Get() }; settingsRegistry != nullptr)
        {
            auto LifecycleCallback = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
            {
                BuildSpawnablesList();
            };
            AZ::ComponentApplicationLifecycle::RegisterHandler(*settingsRegistry, m_criticalAssetsHandler,
                AZStd::move(LifecycleCallback), "CriticalAssetsCompiled");
        }
    }

    NetworkSpawnableLibrary::~NetworkSpawnableLibrary()
    {
        m_criticalAssetsHandler = {};
        AZ::Interface<INetworkSpawnableLibrary>::Unregister(this);
    }

    void NetworkSpawnableLibrary::BuildSpawnablesList()
    {
        m_spawnables.clear();
        m_spawnablesReverseLookup.clear();

        auto enumerateCallback = [this](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (info.m_assetType == AZ::AzTypeInfo<AzFramework::Spawnable>::Uuid() &&
                info.m_relativePath.ends_with(NetworkSpawnableFileExtension))
            {
                ProcessSpawnableAsset(info.m_relativePath, id);
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnumerateAssets, nullptr,
            enumerateCallback, nullptr);
    }

    void NetworkSpawnableLibrary::ProcessSpawnableAsset(const AZStd::string& relativePath, const AZ::Data::AssetId id)
    {
        const AZ::Name name = AZ::Name(relativePath);
        m_spawnables[name] = id;
        m_spawnablesReverseLookup[id] = name;
    }

    AZ::Name NetworkSpawnableLibrary::GetSpawnableNameFromAssetId(AZ::Data::AssetId assetId)
    {
        if (assetId.IsValid())
        {
            auto it = m_spawnablesReverseLookup.find(assetId);
            if (it != m_spawnablesReverseLookup.end())
            {
                return it->second;
            }
        }

        return {};
    }

    AZ::Data::AssetId NetworkSpawnableLibrary::GetAssetIdByName(AZ::Name name)
    {
        auto it = m_spawnables.find(name);
        if (it != m_spawnables.end())
        {
            return it->second;
        }

        return {};
    }
}
