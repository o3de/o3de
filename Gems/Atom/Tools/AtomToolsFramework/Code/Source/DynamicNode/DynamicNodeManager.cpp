/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodePaletteItem.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>

namespace AtomToolsFramework
{
    void DynamicNodeManager::LoadMatchingConfigFiles(const AZStd::unordered_set<AZStd::string>& extensions)
    {
        // Enumerate all of the dynamic node configuration files in the project. This is currently using the asset system to enumerate the
        // files as a proof of concept. If these remain editor only files then they don't need to go through the asset processor and should
        // instead traverse source file directories using the file system.
        AZStd::vector<AZ::Data::Asset<AZ::RPI::AnyAsset>> dynamicNodeConfigAssets;
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB =
            [&]([[maybe_unused]] const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type())
            {
                for (const auto& extension : extensions)
                {
                    if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), extension))
                    {
                        dynamicNodeConfigAssets.emplace_back(assetInfo.m_assetId, AZ::AzTypeInfo<AZ::RPI::AnyAsset>::Uuid());
                        break;
                    }
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        // Initiate loading of all of the configuration assets
        for (auto& dynamicNodeConfigAsset : dynamicNodeConfigAssets)
        {
            dynamicNodeConfigAsset.QueueLoad();
        }

        for (auto& dynamicNodeConfigAsset : dynamicNodeConfigAssets)
        {
            // Block until each of the assets is ready so configuration data can be registered immediately
            dynamicNodeConfigAsset.BlockUntilLoadComplete();
            if (dynamicNodeConfigAsset.IsReady())
            {
                if (auto config = dynamicNodeConfigAsset->GetDataAs<DynamicNodeConfig>())
                {
                    Register(*config);
                }
            }
        }

        // Sort all of the configurations by category and title so that they are presorted when generating the node palette tree items.
        Sort();
    }

    void DynamicNodeManager::Register(const DynamicNodeConfig& config)
    {
        m_dynamicNodeConfigs.push_back(config);
    }

    void DynamicNodeManager::Clear()
    {
        m_dynamicNodeConfigs.clear();
    }

    void DynamicNodeManager::Sort()
    {
        AZStd::sort(
            m_dynamicNodeConfigs.begin(), m_dynamicNodeConfigs.end(),
            [](const DynamicNodeConfig& config1, const DynamicNodeConfig& config2)
            {
                return AZStd::make_pair(config1.m_category, config1.m_title) < AZStd::make_pair(config2.m_category, config2.m_title);
            });
    }

    GraphCanvas::GraphCanvasTreeItem* DynamicNodeManager::CreateNodePaletteRootTreeItem(const AZ::Crc32& toolId) const
    {
        auto rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", toolId);

        AZStd::unordered_map<AZStd::string, GraphCanvas::GraphCanvasTreeItem*> categryMap;
        categryMap[""] = rootItem;

        for (auto& config : m_dynamicNodeConfigs)
        {
            auto categoryItr = categryMap.find(config.m_category);
            if (categoryItr == categryMap.end())
            {
                categoryItr = categryMap.emplace(config.m_category,
                    rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>(config.m_category, toolId)).first;
            }

            categoryItr->second->CreateChildNode<DynamicNodePaletteItem>(toolId, config);
        }

        GraphModelIntegration::AddCommonNodePaletteUtilities(rootItem, toolId);
        return rootItem;
    }
} // namespace AtomToolsFramework
