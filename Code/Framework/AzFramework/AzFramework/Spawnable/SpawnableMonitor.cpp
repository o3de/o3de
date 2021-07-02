/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/SpawnableMonitor.h>

namespace AzFramework
{
    SpawnableMonitor::SpawnableMonitor(AZ::Data::AssetId spawnableAssetId)
    {
        AZ::Data::AssetBus::MultiHandler::BusConnect(spawnableAssetId);
        m_isConnected = true;
    }

    SpawnableMonitor::~SpawnableMonitor()
    {
        Disconnect();
    }

    bool SpawnableMonitor::Connect(AZ::Data::AssetId spawnableAssetId)
    {
        if (!m_isConnected)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(spawnableAssetId);
            m_isConnected = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableMonitor::Disconnect()
    {
        if (m_isConnected)
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_isLoaded = false;
            m_isConnected = false;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SpawnableMonitor::IsConnected() const
    {
        return m_isConnected;
    }

    bool SpawnableMonitor::IsLoaded() const
    {
        return m_isLoaded;
    }

    void SpawnableMonitor::OnSpawnableLoaded()
    {
    }

    void SpawnableMonitor::OnSpawnableUnloaded()
    {
    }

    void SpawnableMonitor::OnSpawnableReloaded([[maybe_unused]] AZ::Data::Asset<Spawnable>&& replacementAsset)
    {
        OnSpawnableUnloaded();
        OnSpawnableLoaded();
    }

    void SpawnableMonitor::OnSpawnableIssue([[maybe_unused]] IssueType issueType, [[maybe_unused]] AZStd::string_view message)
    {
        switch (issueType)
        {
        case IssueType::Error:
            AZ_Error("Spawnables", false, "%.*s", AZ_STRING_ARG(message));
            break;
        case IssueType::Cancel:
            AZ_TracePrintf("Spawnables", "%.*s", AZ_STRING_ARG(message));
            break;
        case IssueType::ReloadError:
            AZ_TracePrintf("Spawnables", "%.*s", AZ_STRING_ARG(message));
            break;
        }
    }

    void SpawnableMonitor::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Assert(!m_isLoaded, "Trying to load spawnable %s (%s) for a second time.",
            asset.GetHint().c_str(), asset.GetId().ToString<AZStd::string>().c_str());
        m_isLoaded = true;
        OnSpawnableLoaded();
    }

    void SpawnableMonitor::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Assert(m_isLoaded, "Trying to reload a spawnable %s (%s) that wasn't loaded.",
            asset.GetHint().c_str(), asset.GetId().ToString<AZStd::string>().c_str());
        OnSpawnableReloaded(AZStd::move(asset));
    }

    void SpawnableMonitor::OnAssetUnloaded([[maybe_unused]] const AZ::Data::AssetId assetId,
        [[maybe_unused]] const AZ::Data::AssetType assetType)
    {
        AZ_Assert(m_isLoaded, "Trying to unload a spawnable %s that was never loaded.", assetId.ToString<AZStd::string>().c_str());
        m_isLoaded = false;
        OnSpawnableUnloaded();
    }

    void SpawnableMonitor::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_isLoaded)
        {
            m_isLoaded = false;
            OnSpawnableUnloaded();
        }

        AZStd::string message = AZStd::string::format("An error occurred during the loading of spawnable '%s' (%s).",
            asset.GetHint().c_str(), asset.GetId().ToString<AZStd::string>().c_str());
        OnSpawnableIssue(IssueType::Error, message);
    }

    void SpawnableMonitor::OnAssetCanceled(AZ::Data::AssetId assetId)
    {
        if (m_isLoaded)
        {
            m_isLoaded = false;
            OnSpawnableUnloaded();
        }

        AZStd::string message = AZStd::string::format("The loading of spawnable '%s' was canceled.",
            assetId.ToString<AZStd::string>().c_str());
        OnSpawnableIssue(IssueType::Cancel, message);
    }

    void SpawnableMonitor::OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_isLoaded)
        {
            m_isLoaded = false;
            OnSpawnableUnloaded();
        }

        AZStd::string message = AZStd::string::format("An error occurred while trying to reload spawnable '%s' (%s).",
            asset.GetHint().c_str(), asset.GetId().ToString<AZStd::string>().c_str());
        OnSpawnableIssue(IssueType::ReloadError, message);
    }
} // namespace AzFramework
