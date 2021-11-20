/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/Scanner.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <Editor/View/Windows/Tools/UpgradeTool/LogTraits.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Scanner::Scanner(const ScanConfiguration& config, AZStd::function<void()> onComplete)
            : m_config(config)
            , m_onComplete(onComplete)
        {
            AZ::Data::AssetCatalogRequestBus::Broadcast
                ( &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets
                , nullptr
                , [this](const AZ::Data::AssetId, const AZ::Data::AssetInfo& assetInfo)
                    {
                        if (assetInfo.m_assetType == azrtti_typeid<ScriptCanvasAsset>())
                        {
                            m_result.m_catalogAssets.push_back(assetInfo);
                        }
                    }
                , nullptr);

            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanBegin, m_result.m_catalogAssets.size());
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void Scanner::FilterAsset(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (m_config.filter && m_config.filter(asset))
            {
                VE_LOG("Scanner: Excluded: %s ", GetCurrentAsset().m_relativePath.c_str());
                m_result.m_filteredAssets.push_back(GetCurrentAsset());
                ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanFilteredGraph, GetCurrentAsset());
            }
            else
            {
                VE_LOG("Scanner: Included: %s ", GetCurrentAsset().m_relativePath.c_str());
                m_result.m_unfiltered.push_back({ asset, GetCurrentAsset() });
                ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanUnFilteredGraph, GetCurrentAsset());
            }
        }

        const AZ::Data::AssetInfo& Scanner::GetCurrentAsset() const
        {
            return m_result.m_catalogAssets[m_catalogAssetIndex];
        }

        const ScanResult& Scanner::GetResult() const
        {
            return m_result;
        }

        AZ::Data::Asset<AZ::Data::AssetData> Scanner::LoadAsset()
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset
                ( GetCurrentAsset().m_assetId
                , azrtti_typeid<ScriptCanvasAsset>()
                , AZ::Data::AssetLoadBehavior::PreLoad);

            asset.BlockUntilLoadComplete();

            if (asset.IsReady())
            {
                return asset;
            }
            else
            {
                return {};
            }
        }

        void Scanner::OnSystemTick()
        {
            if (m_catalogAssetIndex == m_result.m_catalogAssets.size())
            {
                VE_LOG("Scanner: Complete.");
                AZ::SystemTickBus::Handler::BusDisconnect();

                if (m_onComplete)
                {
                    m_onComplete();
                }
            }
            else
            {
                if (auto asset = LoadAsset())
                {
                    VE_LOG("Scanner: Loaded: %s ", GetCurrentAsset().m_relativePath.c_str());
                    FilterAsset(asset);
                }
                else
                {
                    VE_LOG("Scanner: Failed to load: %s ", GetCurrentAsset().m_relativePath.c_str());
                    m_result.m_loadErrors.push_back(GetCurrentAsset());
                    ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanLoadFailure, GetCurrentAsset());
                }

                VE_LOG("Scanner: scan of %s complete", GetCurrentAsset().m_relativePath.c_str());
                ++m_catalogAssetIndex;
            }
        }

        ScanResult&& Scanner::TakeResult()
        {
            return AZStd::move(m_result);
        }
    }
}
