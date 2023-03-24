/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Module/InputOutputNodes.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/Slot.h>
#include <GraphModel/Model/DataType.h>

namespace GraphModel
{
    //////////////////////////////////////////////////////////////////////////////
    // BaseInputOutputNode

    void BaseInputOutputNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseInputOutputNode, Node>()
                ->Version(0)
                ->Field("m_dataType", &BaseInputOutputNode::m_dataType)
                ;
        }
    }
        
    BaseInputOutputNode::BaseInputOutputNode(GraphPtr graph, DataTypePtr dataType)
        : Node(graph)
    {
        // Copy because m_dataType has to be non-const for use with SerializeContext, and dataType is const
        m_dataType = AZStd::make_shared<DataType>(*dataType);
    }

    const char* BaseInputOutputNode::GetTitle() const 
    {
        return m_title.c_str();
    }

    GraphModel::DataTypePtr BaseInputOutputNode::GetNodeDataType() const 
    { 
        return m_dataType; 
    }

    AZStd::string BaseInputOutputNode::GetName() const 
    { 
        return GetSlot("name")->GetValue<AZStd::string>();
    }

    AZStd::string BaseInputOutputNode::GetDisplayName() const 
    { 
        return GetSlot("displayName")->GetValue<AZStd::string>();
    }

    AZStd::string BaseInputOutputNode::GetDescription() const
    { 
        return GetSlot("description")->GetValue<AZStd::string>(); 
    }
                
    void BaseInputOutputNode::RegisterCommonSlots(AZStd::string_view directionName)
    {
        GraphModel::DataTypePtr stringDataType = GetGraphContext()->GetDataType<AZStd::string>();

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Property,
            "name",
            "Name",
            AZStd::string::format("The official name for this %s", directionName.data()),
            GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue()));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Property,
            "displayName",
            "Display Name",
            AZStd::string::format("The name for this %s, displayed to the user. Will use the above Name if left blank.", directionName.data()),
            GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue()));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Property,
            "description",
            "Description",
            AZStd::string::format("A description of this %s, used for tooltips", directionName.data()),
            GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue()));
    }


    //////////////////////////////////////////////////////////////////////////////
    // GraphInputNode

    void GraphInputNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GraphInputNode, BaseInputOutputNode>()
                ->Version(0)
                ;
        }
    }
        
    GraphInputNode::GraphInputNode(GraphModel::GraphPtr graph, DataTypePtr dataType)
        : BaseInputOutputNode(graph, dataType)
    {
        m_title = m_dataType->GetDisplayName() + " Input";

        RegisterSlots();

        CreateSlotData();
    }

    void GraphInputNode::PostLoadSetup(GraphPtr graph, NodeId id)
    {
        m_title = m_dataType->GetDisplayName() + " Input";

        Node::PostLoadSetup(graph, id);
    }

    AZStd::any GraphInputNode::GetDefaultValue() const 
    {
        return GetSlot("defaultValue")->GetValue(); 
    }

    void GraphInputNode::RegisterSlots()
    {
        // Register just a single output slot for the data that is input through this node
        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Output,
            GraphModel::SlotType::Data,
            "value",
            "Value",
            "An external value provided as input to this graph",
            GraphModel::DataTypeList{ m_dataType }));

        // Register meta-data properties
        RegisterCommonSlots("input");

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Property,
            "defaultValue",
            "Default Value",
            "The default value for this input when no data is provided externally",
            GraphModel::DataTypeList{ m_dataType },
            m_dataType->GetDefaultValue()));
    }

    //////////////////////////////////////////////////////////////////////////////
    // GraphOutputNode

    void GraphOutputNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GraphOutputNode, BaseInputOutputNode>()
                ->Version(0)
                ;
        }
    }
        
    GraphOutputNode::GraphOutputNode(GraphModel::GraphPtr graph, DataTypePtr dataType)
        : BaseInputOutputNode(graph, dataType)
    {
        m_title = m_dataType->GetDisplayName() + " Output";

        RegisterSlots();

        CreateSlotData();
    }

    void GraphOutputNode::PostLoadSetup(GraphPtr graph, NodeId id)
    {
        m_title = m_dataType->GetDisplayName() + " Output";

        Node::PostLoadSetup(graph, id);
    }

    void GraphOutputNode::RegisterSlots()
    {
        // Register just a single input slot for the data that is output through this node
        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            "value",
            "Value",
            "A value output by this graph for external use",
            GraphModel::DataTypeList{ m_dataType },
            m_dataType->GetDefaultValue()));

        // Register meta-data properties
        RegisterCommonSlots("output");
    }


}
