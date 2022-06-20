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
#include <AtomToolsFramework/Util/Util.h>
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
    DynamicNodeManager::DynamicNodeManager(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        DynamicNodeManagerRequestBus::Handler::BusConnect(m_toolId);
    }

    DynamicNodeManager::~DynamicNodeManager()
    {
        DynamicNodeManagerRequestBus::Handler::BusDisconnect();
    }

    void DynamicNodeManager::LoadConfigFiles(const AZStd::unordered_set<AZStd::string>& extensions)
    {
        // Enumerate all of the dynamic node configuration files in the project. This is currently using the asset system to enumerate the
        // files as a proof of concept. If these remain editor only files then they don't need to go through the asset processor and should
        // instead traverse source file directories using the file system.
        AZStd::unordered_set<AZStd::string> configPaths;
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB =
            [&]([[maybe_unused]] const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type())
            {
                for (const auto& extension : extensions)
                {
                    if (AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), extension))
                    {
                        const AZStd::string& configPath =
                            ConvertPathToAlias(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId));
                        configPaths.insert(configPath);
                        AZ_TracePrintf(__FUNCTION__, "Discovered dynamic node config: %s\n", configPath.c_str());
                        break;
                    }
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        // Load and register all discovered dynamic node configuration
        for (const AZStd::string& configPath : configPaths)
        {
            DynamicNodeConfig config;
            if (config.Load(configPath))
            {
                RegisterConfig(configPath, config);
            }
        }
    }

    void DynamicNodeManager::RegisterConfig(const AZStd::string& configId, const DynamicNodeConfig& config)
    {
        m_nodeConfigMap[configId] = config;
    }

    DynamicNodeConfig DynamicNodeManager::GetConfig(const AZStd::string& configId) const
    {
        auto configItr = m_nodeConfigMap.find(configId);
        return configItr != m_nodeConfigMap.end() ? configItr->second : DynamicNodeConfig();
    }

    void DynamicNodeManager::Clear()
    {
        m_nodeConfigMap.clear();
    }

    GraphCanvas::GraphCanvasTreeItem* DynamicNodeManager::CreateNodePaletteTree() const
    {
        auto rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", m_toolId);

        AZStd::unordered_map<AZStd::string, GraphCanvas::GraphCanvasTreeItem*> categoryMap;
        categoryMap[""] = rootItem;

        // Create a container of all of the node configs, sorted by category and title to generate the node palette
        AZStd::vector<AZStd::pair<AZStd::string, DynamicNodeConfig>> sortedConfigVec(
            m_nodeConfigMap.begin(), m_nodeConfigMap.end());

        AZStd::sort(
            sortedConfigVec.begin(), sortedConfigVec.end(),
            [](const auto& config1, const auto& config2)
            {
                return
                    AZStd::make_pair(config1.second.m_category, config1.second.m_title) <
                    AZStd::make_pair(config2.second.m_category, config2.second.m_title);
            });

        // Create the node palette tree by traversing this sorted configuration container, registering any unique categories and child items
        for (const auto& [configId, config] : sortedConfigVec)
        {
            auto categoryItr = categoryMap.find(config.m_category);
            if (categoryItr == categoryMap.end())
            {
                categoryItr = categoryMap.emplace(config.m_category,
                    rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>(config.m_category, m_toolId)).first;
            }

            categoryItr->second->CreateChildNode<DynamicNodePaletteItem>(m_toolId, configId, config);
        }

        GraphModelIntegration::AddCommonNodePaletteUtilities(rootItem, m_toolId);
        return rootItem;
    }
} // namespace AtomToolsFramework
