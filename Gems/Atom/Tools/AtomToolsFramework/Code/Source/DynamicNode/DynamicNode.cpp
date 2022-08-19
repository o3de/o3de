/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Slot.h>

namespace AtomToolsFramework
{
    void DynamicNode::Reflect(AZ::ReflectContext* context)
    {
        DynamicNodeSlotConfig::Reflect(context);
        DynamicNodeConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicNode, GraphModel::Node>()
                ->Version(0)
                ->Field("toolId", &DynamicNode::m_toolId)
                ->Field("configId", &DynamicNode::m_configId);
        }
    }

    DynamicNode::DynamicNode(GraphModel::GraphPtr ownerGraph, const AZ::Crc32& toolId, const AZStd::string& configId)
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

    const AZStd::string& DynamicNode::GetConfigId() const
    {
        return m_configId;
    }

    const DynamicNodeConfig& DynamicNode::GetConfig() const
    {
        return m_config;
    }

    void DynamicNode::RegisterSlots()
    {
        m_config = {};
        AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
            m_config, m_toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::GetConfig, m_configId);

        // Register all of the input data slots with the dynamic node
        for (const auto& slotConfig : m_config.m_inputSlots)
        {
            // Input slots support incoming connections from multiple data types. We must build a container of all of the data type objects
            // for all of the supported types to create the input slot.
            GraphModel::DataTypeList dataTypes;
            dataTypes.reserve(slotConfig.m_supportedDataTypes.size());
            for (const AZStd::string& dataTypeName : slotConfig.m_supportedDataTypes)
            {
                GraphModel::DataTypePtr dataType = GetGraphContext()->GetDataType(dataTypeName);
                if (!dataType)
                {
                    AZ_Error(
                        "DynamicNode",
                        false,
                        "Unable to register input slot \"%s\" with unsupported data type \"%s\", from DynamicNodeConfig \"%s\"",
                        slotConfig.m_displayName.c_str(),
                        dataTypeName.c_str(),
                        m_configId.c_str());
                    continue;
                }

                dataTypes.push_back(dataType);
            }

            if (dataTypes.empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register input slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.c_str());
                continue;
            }

            // Assigning the default value from the slot configuration or the first data type
            const AZStd::any& defaultValue =
                !slotConfig.m_defaultValue.empty() ? slotConfig.m_defaultValue : dataTypes.front()->GetDefaultValue();
            if (defaultValue.empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register input slot \"%s\" with invalid default value, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.c_str());
                continue;
            }

            RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
                slotConfig.m_name, slotConfig.m_displayName, dataTypes, defaultValue, slotConfig.m_description));
        }

        for (const auto& slotConfig : m_config.m_outputSlots)
        {
            // Output slots only support one data type. Search for the first valid supported data type.
            GraphModel::DataTypePtr dataType;
            for (const AZStd::string& dataTypeName : slotConfig.m_supportedDataTypes)
            {
                dataType = GetGraphContext()->GetDataType(dataTypeName);
                if (dataType)
                {
                    break;
                }
            }

            if (!dataType)
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register output slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.c_str());
                continue;
            }

            RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
                slotConfig.m_name, slotConfig.m_displayName, dataType, slotConfig.m_description));
        }

        for (const auto& slotConfig : m_config.m_propertySlots)
        {
            // Property slots only support one data type. Search for the first valid supported data type.
            GraphModel::DataTypePtr dataType;
            for (const AZStd::string& dataTypeName : slotConfig.m_supportedDataTypes)
            {
                dataType = GetGraphContext()->GetDataType(dataTypeName);
                if (dataType)
                {
                    break;
                }
            }

            if (!dataType)
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register property slot \"%s\" with no supported data types, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.c_str());
                continue;
            }

            // Assigning the default value from the slot configuration or the first data type
            const AZStd::any& defaultValue = !slotConfig.m_defaultValue.empty() ? slotConfig.m_defaultValue : dataType->GetDefaultValue();
            if (defaultValue.empty())
            {
                AZ_Error(
                    "DynamicNode",
                    false,
                    "Unable to register property slot \"%s\" with invalid default value, from DynamicNodeConfig \"%s\"",
                    slotConfig.m_displayName.c_str(),
                    m_configId.c_str());
                continue;
            }

            RegisterSlot(GraphModel::SlotDefinition::CreateProperty(
                slotConfig.m_name, slotConfig.m_displayName, dataType, defaultValue, slotConfig.m_description));
        }
    }
} // namespace AtomToolsFramework
