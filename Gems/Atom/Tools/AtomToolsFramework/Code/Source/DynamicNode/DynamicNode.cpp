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
        m_config = {};
        AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
            m_config, m_toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::GetConfig, m_configId);

        RegisterSlots();
        CreateSlotData();
    }

    void DynamicNode::PostLoadSetup(GraphModel::GraphPtr ownerGraph, GraphModel::NodeId id)
    {
        m_config = {};
        AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
            m_config, m_toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::GetConfig, m_configId);

        Node::PostLoadSetup(ownerGraph, id);
    }

    const char* DynamicNode::GetTitle() const
    {
        return m_config.m_title.c_str();
    }

    const char* DynamicNode::GetSubTitle() const
    {
        return m_config.m_subTitle.c_str();
    }

    void DynamicNode::RegisterSlots()
    {
        for (const auto& slotConfig : m_config.m_inputSlots)
        {
            GraphModel::DataTypeList dataTypes;
            for (const auto& supportedDataType : slotConfig.m_supportedDataTypes)
            {
                if (auto dataType = GetGraphContext()->GetDataType(supportedDataType))
                {
                    dataTypes.push_back(dataType);
                }
            }

            if (!dataTypes.empty())
            {
                const AZStd::any& defaultValue =
                    !slotConfig.m_defaultValue.empty() ? slotConfig.m_defaultValue : dataTypes.front()->GetDefaultValue();
                RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
                    slotConfig.m_name, slotConfig.m_displayName, dataTypes, defaultValue, slotConfig.m_description));
            }
        }

        for (const auto& slotConfig : m_config.m_outputSlots)
        {
            for (const auto& supportedDataType : slotConfig.m_supportedDataTypes)
            {
                if (auto dataType = GetGraphContext()->GetDataType(supportedDataType))
                {
                    RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
                        slotConfig.m_name, slotConfig.m_displayName, dataType, slotConfig.m_description));
                    break;
                }
            }
        }

        for (const auto& slotConfig : m_config.m_propertySlots)
        {
            for (const auto& supportedDataType : slotConfig.m_supportedDataTypes)
            {
                if (auto dataType = GetGraphContext()->GetDataType(supportedDataType))
                {
                    const AZStd::any& defaultValue =
                        !slotConfig.m_defaultValue.empty() ? slotConfig.m_defaultValue : dataType->GetDefaultValue();
                    RegisterSlot(GraphModel::SlotDefinition::CreateProperty(
                        slotConfig.m_name, slotConfig.m_displayName, dataType, defaultValue, slotConfig.m_description));
                    break;
                }
            }
        }
    }
} // namespace AtomToolsFramework
