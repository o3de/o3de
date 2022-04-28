/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
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
                ->Field("config", &DynamicNode::m_config);
        }
    }

    DynamicNode::DynamicNode(GraphModel::GraphPtr ownerGraph, const DynamicNodeConfig& config)
        : GraphModel::Node(ownerGraph)
        , m_config(config)
    {
        RegisterSlots();
        CreateSlotData();
    }

    void DynamicNode::PostLoadSetup(GraphModel::GraphPtr ownerGraph, GraphModel::NodeId id)
    {
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
                RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
                    slotConfig.m_name, slotConfig.m_displayName, dataTypes, dataTypes.front()->GetDefaultValue(),
                    slotConfig.m_description));
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
    }
} // namespace AtomToolsFramework
