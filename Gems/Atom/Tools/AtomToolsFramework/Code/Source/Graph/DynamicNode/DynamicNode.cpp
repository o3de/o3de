/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Slot.h>

namespace AtomToolsFramework
{
    void DynamicNode::Reflect(AZ::ReflectContext* context)
    {
        DynamicNodeSlotConfig::Reflect(context);
        DynamicNodeConfig::Reflect(context);
        DynamicNodeManager::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNode, GraphModel::Node>()
                ->Version(0)
                ->Field("toolId", &DynamicNode::m_toolId)
                ->Field("configId", &DynamicNode::m_configId)
                ;


            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicNode>("DynamicNode", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ_CRC_CE("TitlePaletteOverride"), &DynamicNode::GetTitlePaletteName);
            }
        }
    }

    DynamicNode::DynamicNode(GraphModel::GraphPtr ownerGraph, const AZ::Crc32& toolId, const AZ::Uuid& configId)
        : GraphModel::Node(ownerGraph)
        , m_toolId(toolId)
        , m_configId(configId)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* DynamicNode::GetTitle() const
    {
        return m_config.m_title.c_str();
    }

    const char* DynamicNode::GetSubTitle() const
    {
        return m_config.m_subTitle.c_str();
    }

    const AZ::Uuid& DynamicNode::GetConfigId() const
    {
        return m_configId;
    }

    const DynamicNodeConfig& DynamicNode::GetConfig() const
    {
        return m_config;
    }

    AZStd::string DynamicNode::GetTitlePaletteName() const
    {
        return !m_config.m_titlePaletteName.empty() ? m_config.m_titlePaletteName : "DefaultNodeTitlePalette";
    }

    void DynamicNode::RegisterSlots()
    {
        m_config = {};
        AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
            m_config, m_toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::GetConfigById, m_configId);

        const bool enablePropertyEditingOnNodeUI =
            GetSettingsValue("/O3DE/AtomToolsFramework/DynamicNode/EnablePropertyEditingOnNodeUI", true);

        for (const auto& slotConfig : m_config.m_propertySlots)
        {
            // Property slots only support one data type. Search for the first valid supported data type.
            if (!slotConfig.GetDefaultDataType())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register property slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.ToFixedString().c_str());
                continue;
            }

            // Assigning the default value from the slot configuration or the first data type
            if (slotConfig.GetDefaultValue().empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register property slot \"%s\" with invalid default value, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.ToFixedString().c_str());
                continue;
            }

            RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
                GraphModel::SlotDirection::Input,
                GraphModel::SlotType::Property,
                slotConfig.m_name,
                slotConfig.m_displayName,
                slotConfig.m_description,
                slotConfig.GetSupportedDataTypes(),
                slotConfig.GetDefaultValue(),
                1, 1, "", "",
                slotConfig.m_enumValues,
                slotConfig.m_visibleOnNode,
                slotConfig.m_editableOnNode && enablePropertyEditingOnNodeUI));
        }

        // Register all of the input data slots with the dynamic node
        for (const auto& slotConfig : m_config.m_inputSlots)
        {
            // Input slots support incoming connections from multiple data types. We must build a container of all of the data type objects
            // for all of the supported types to create the input slot.
            if (slotConfig.GetSupportedDataTypes().empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register input slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.ToFixedString().c_str());
                continue;
            }

            // Assigning the default value from the slot configuration or the first data type
            if (slotConfig.GetDefaultValue().empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register input slot \"%s\" with invalid default value, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.ToFixedString().c_str());
                continue;
            }

            RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
                GraphModel::SlotDirection::Input,
                GraphModel::SlotType::Data,
                slotConfig.m_name,
                slotConfig.m_displayName,
                slotConfig.m_description,
                slotConfig.GetSupportedDataTypes(),
                slotConfig.GetDefaultValue(),
                1, 1, "", "",
                slotConfig.m_enumValues,
                slotConfig.m_visibleOnNode,
                slotConfig.m_editableOnNode && enablePropertyEditingOnNodeUI));
        }

        for (const auto& slotConfig : m_config.m_outputSlots)
        {
            // Output slots only support one data type. Search for the first valid supported data type.
            if (!slotConfig.GetDefaultDataType())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register output slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.ToFixedString().c_str());
                continue;
            }

            RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
                GraphModel::SlotDirection::Output,
                GraphModel::SlotType::Data,
                slotConfig.m_name,
                slotConfig.m_displayName,
                slotConfig.m_description,
                slotConfig.GetSupportedDataTypes(),
                slotConfig.GetDefaultValue(),
                1, 1, "", "",
                slotConfig.m_enumValues,
                slotConfig.m_visibleOnNode,
                slotConfig.m_editableOnNode && enablePropertyEditingOnNodeUI));
        }
    }
} // namespace AtomToolsFramework
