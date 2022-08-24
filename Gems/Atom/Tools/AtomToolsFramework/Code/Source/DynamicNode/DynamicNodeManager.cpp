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

    void DynamicNodeManager::RegisterDataTypes(const GraphModel::DataTypeList& dataTypes)
    {
        m_registeredDataTypes.insert(m_registeredDataTypes.end(), dataTypes.begin(), dataTypes.end());
    }

    GraphModel::DataTypeList DynamicNodeManager::GetRegisteredDataTypes()
    {
        return m_registeredDataTypes;
    }

    void DynamicNodeManager::LoadConfigFiles(const AZStd::string& extension)
    {
        // Load and register all discovered dynamic node configuration
        for (AZStd::string configPath : GetPathsInSourceFoldersMatchingWildcard(AZStd::string::format("*.%s", extension.c_str())))
        {
            DynamicNodeConfig config;
            if (config.Load(configPath))
            {
                AZ_TracePrintf("DynamicNodeManager", "DynamicNodeConfig \"%s\" loaded.\n", configPath.c_str());
                RegisterConfig(config);
            }
        }
    }

    bool DynamicNodeManager::RegisterConfig(const DynamicNodeConfig& config)
    {
        AZ_TracePrintf("DynamicNodeManager", "DynamicNodeConfig \"%s\" registering.\n", config.m_id.ToFixedString().c_str());

        if (!ValidateSlotConfigVec(config.m_id, config.m_inputSlots) ||
            !ValidateSlotConfigVec(config.m_id, config.m_outputSlots) ||
            !ValidateSlotConfigVec(config.m_id, config.m_propertySlots))
        {
            AZ_Error("DynamicNodeManager", false, "DynamicNodeConfig \"%s\" could not be registered.", config.m_id.ToFixedString().c_str());
            return false;
        }

        m_nodeConfigMap[config.m_id] = config;
        AZ_TracePrintf("DynamicNodeManager", "DynamicNodeConfig \"%s\" registered.\n", config.m_id.ToFixedString().c_str());
        return true;
    }

    DynamicNodeConfig DynamicNodeManager::GetConfigById(const AZ::Uuid& configId) const
    {
        auto configItr = m_nodeConfigMap.find(configId);
        if (configItr != m_nodeConfigMap.end())
        {
            return configItr->second;
        }

        AZ_Error("DynamicNodeManager", false, "DynamicNodeConfig \"%s\" could not be found.", configId.ToFixedString().c_str());
        return DynamicNodeConfig();
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

        // Create the node palette tree by traversing the configuration container, registering any unique categories and child items
        for (const auto& configPair : m_nodeConfigMap)
        {
            const auto& config = configPair.second;
            auto categoryItr = categoryMap.find(config.m_category);
            if (categoryItr == categoryMap.end())
            {
                categoryItr = categoryMap.emplace(config.m_category,
                    rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>(config.m_category, m_toolId)).first;
            }

            categoryItr->second->CreateChildNode<DynamicNodePaletteItem>(m_toolId, config);
        }

        GraphModelIntegration::AddCommonNodePaletteUtilities(rootItem, m_toolId);
        return rootItem;
    }

    bool DynamicNodeManager::ValidateSlotConfig(
        [[maybe_unused]] const AZ::Uuid& configId, const DynamicNodeSlotConfig& slotConfig) const
    {
        if (slotConfig.m_supportedDataTypes.empty())
        {
            AZ_Error(
                "DynamicNodeManager",
                false,
                "DynamicNodeConfig \"%s\" could not be validated because DynamicNodeSlotConfig \"%s\" has no supported data types.",
                configId.ToFixedString().c_str(),
                slotConfig.m_displayName.c_str());
            return false;
        }

        for (const AZStd::string& dataTypeName : slotConfig.m_supportedDataTypes)
        {
            if (!AZStd::any_of(
                    m_registeredDataTypes.begin(),
                    m_registeredDataTypes.end(),
                    [&dataTypeName](const auto& dataType)
                    {
                        return dataTypeName == dataType->GetCppName() || dataTypeName == dataType->GetDisplayName();
                    }))
            {
                AZ_Error(
                    "DynamicNodeManager",
                    false,
                    "DynamicNodeConfig \"%s\" could not be validated because DynamicNodeSlotConfig \"%s\" references unregistered data type \"%s\".",
                    configId.ToFixedString().c_str(),
                    slotConfig.m_displayName.c_str(),
                    dataTypeName.c_str());
                return false;
            }
        }

        return true;
    }

    bool DynamicNodeManager::ValidateSlotConfigVec(
        [[maybe_unused]] const AZ::Uuid& configId, const AZStd::vector<DynamicNodeSlotConfig>& slotConfigVec) const
    {
        for (const auto& slotConfig : slotConfigVec)
        {
            if (!ValidateSlotConfig(configId, slotConfig))
            {
                AZ_Error(
                    "DynamicNodeManager",
                    false,
                    "DynamicNodeConfig \"%s\" could not be validated because DynamicNodeSlotConfig \"%s\" could not be validated.",
                    configId.ToFixedString().c_str(),
                    slotConfig.m_displayName.c_str());
                return false;
            }
        }

        return true;
    }
} // namespace AtomToolsFramework
