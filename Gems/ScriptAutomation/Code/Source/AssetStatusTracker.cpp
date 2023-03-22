/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetStatusTracker.h"
#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptAutomation
{
    AssetStatusTracker::~AssetStatusTracker()
    {
        AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
    }

    void AssetStatusTracker::StartTracking()
    {
        if (!m_isTracking)
        {
            AzFramework::AssetSystemInfoBus::Handler::BusConnect();
        }

        m_isTracking = true;

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allAssetStatusData.clear();
    }

    void AssetStatusTracker::StopTracking()
    {
        if (m_isTracking)
        {
            m_isTracking = false;

            AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            m_allAssetStatusData.clear();
        }
    }

    void AssetStatusTracker::ExpectAsset(AZStd::string sourceAssetPath, uint32_t expectedCount)
    {
        AzFramework::StringFunc::Path::Normalize(sourceAssetPath);
        AZStd::to_lower(sourceAssetPath.begin(), sourceAssetPath.end());

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allAssetStatusData[sourceAssetPath].m_expectedCount += expectedCount;
    }

    bool AssetStatusTracker::DidExpectedAssetsFinish() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        for (auto& assetData : m_allAssetStatusData)
        {
            const AssetStatusEvents& status = assetData.second;

            if (status.m_expectedCount > (status.m_succeeded + status.m_failed))
            {
                return false;
            }
        }

        return true;
    }
    
    AZStd::vector<AZStd::string> AssetStatusTracker::GetIncompleteAssetList() const
    {
        AZStd::vector<AZStd::string> incomplete;
        
        for (auto& assetData : m_allAssetStatusData)
        {
            const AssetStatusEvents& status = assetData.second;

            if (status.m_expectedCount > (status.m_succeeded + status.m_failed))
            {
                incomplete.push_back(assetData.first);
            }
        }

        return incomplete;
    }


    void AssetStatusTracker::AssetCompilationStarted(const AZStd::string& assetPath)
    {
        AZ_TracePrintf("Automation", "AssetCompilationStarted(%s)\n", assetPath.c_str());

        AZStd::string normalizedAssetPath = assetPath;
        AzFramework::StringFunc::Path::Normalize(normalizedAssetPath);
        AZStd::to_lower(normalizedAssetPath.begin(), normalizedAssetPath.end());

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allAssetStatusData[normalizedAssetPath].m_started++;
    }

    void AssetStatusTracker::AssetCompilationSuccess(const AZStd::string& assetPath)
    {
        AZ_TracePrintf("Automation", "AssetCompilationSuccess(%s)\n", assetPath.c_str());

        AZStd::string normalizedAssetPath = assetPath;
        AzFramework::StringFunc::Path::Normalize(normalizedAssetPath);
        AZStd::to_lower(normalizedAssetPath.begin(), normalizedAssetPath.end());

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allAssetStatusData[normalizedAssetPath].m_succeeded++;
    }

    void AssetStatusTracker::AssetCompilationFailed(const AZStd::string& assetPath)
    {
        AZ_TracePrintf("Automation", "AssetCompilationFailed(%s)\n", assetPath.c_str());

        AZStd::string normalizedAssetPath = assetPath;
        AzFramework::StringFunc::Path::Normalize(normalizedAssetPath);
        AZStd::to_lower(normalizedAssetPath.begin(), normalizedAssetPath.end());

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allAssetStatusData[normalizedAssetPath].m_failed++;
    }


} // namespace ScriptAutomation
