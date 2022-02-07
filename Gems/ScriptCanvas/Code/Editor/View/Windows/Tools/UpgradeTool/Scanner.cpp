/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <Editor/View/Windows/Tools/UpgradeTool/LogTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Scanner.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>

namespace ScannerCpp
{
    void TraverseTree
        ( QModelIndex index
        , AzToolsFramework::AssetBrowser::AssetBrowserFilterModel& model
        , ScriptCanvasEditor::VersionExplorer::ScanResult& result)
    {
        QModelIndex sourceIndex = model.mapToSource(index);
        AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry =
            reinterpret_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

        if (entry
            && entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source
            && azrtti_istypeof<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry)
            && entry->GetFullPath().ends_with(".scriptcanvas"))
        {
            auto sourceEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry);

            AZStd::string fullPath = sourceEntry->GetFullPath();
            AzFramework::StringFunc::Path::Normalize(fullPath);

            result.m_catalogAssets.push_back(
                ScriptCanvasEditor::SourceHandle(nullptr, sourceEntry->GetSourceUuid(), fullPath));
        }

        const int rowCount = model.rowCount(index);

        for (int i = 0; i < rowCount; ++i)
        {
            TraverseTree(model.index(i, 0, index), model, result);
        }
    }
}


namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Scanner::Scanner(const ScanConfiguration& config, AZStd::function<void()> onComplete)
            : m_config(config)
            , m_onComplete(onComplete)
        {
            AzToolsFramework::AssetBrowser::AssetBrowserModel* assetBrowserModel = nullptr;
            AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult
                ( assetBrowserModel, &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);

            if (assetBrowserModel)
            {
                auto stringFilter = new AzToolsFramework::AssetBrowser::StringFilter();
                stringFilter->SetName("ScriptCanvas");
                stringFilter->SetFilterString(".scriptcanvas");
                stringFilter->SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);

                AzToolsFramework::AssetBrowser::AssetBrowserFilterModel assetFilterModel;
                assetFilterModel.SetFilter(AzToolsFramework::AssetBrowser::FilterConstType(stringFilter));
                assetFilterModel.setSourceModel(assetBrowserModel);

                ScannerCpp::TraverseTree(QModelIndex(), assetFilterModel, m_result);
            }

            AZ::SystemTickBus::Handler::BusConnect();
        }

        void Scanner::FilterAsset(SourceHandle asset)
        {
            if (m_config.filter && m_config.filter(asset) == ScanConfiguration::Filter::Exclude)
            {
                VE_LOG("Scanner: Excluded: %s ", ModCurrentAsset().Path().c_str());
                m_result.m_filteredAssets.push_back(ModCurrentAsset().Describe());
                ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanFilteredGraph, ModCurrentAsset());
            }
            else
            {
                VE_LOG("Scanner: Included: %s ", ModCurrentAsset().Path().c_str());
                m_result.m_unfiltered.push_back(ModCurrentAsset().Describe());
                ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanUnFilteredGraph, ModCurrentAsset());
            }
        }

        const ScanResult& Scanner::GetResult() const
        {
            return m_result;
        }

        SourceHandle Scanner::LoadAsset()
        {
            auto fileOutcome = LoadFromFile(ModCurrentAsset().Path().c_str());
            if (fileOutcome.IsSuccess())
            {
                return fileOutcome.GetValue();
            }
            else
            {
                return {};
            }
        }

        SourceHandle& Scanner::ModCurrentAsset()
        {
            return m_result.m_catalogAssets[m_catalogAssetIndex];
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
                if (auto asset = LoadAsset(); asset.IsGraphValid())
                {
                    VE_LOG("Scanner: Loaded: %s ", ModCurrentAsset().Path().c_str());
                    FilterAsset(asset);
                }
                else
                {
                    VE_LOG("Scanner: Failed to load: %s ", ModCurrentAsset().Path().c_str());
                    m_result.m_loadErrors.push_back(ModCurrentAsset().Describe());
                    ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanLoadFailure, ModCurrentAsset());
                }

                VE_LOG("Scanner: scan of %s complete", ModCurrentAsset().Path().c_str());
                ++m_catalogAssetIndex;
            }
        }

        ScanResult&& Scanner::TakeResult()
        {
            return AZStd::move(m_result);
        }
    }
}
