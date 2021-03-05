/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        RegisterSlot(GraphModel::SlotDefinition::CreateProperty("name", "Name", stringDataType, stringDataType->GetDefaultValue(), 
            AZStd::string::format("The official name for this %s", directionName.data())));
        RegisterSlot(GraphModel::SlotDefinition::CreateProperty("displayName", "Display Name", stringDataType, stringDataType->GetDefaultValue(), 
            AZStd::string::format("The name for this %s, displayed to the user. Will use the above Name if left blank.", directionName.data())));
        RegisterSlot(GraphModel::SlotDefinition::CreateProperty("description", "Description", stringDataType, stringDataType->GetDefaultValue(), 
            AZStd::string::format("A description of this %s, used for tooltips", directionName.data())));
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
        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData("value", "Value", m_dataType, "An external value provided as input to this graph"));

        // Register meta-data properties
        RegisterCommonSlots("input");
        RegisterSlot(GraphModel::SlotDefinition::CreateProperty("defaultValue", "Default Value", m_dataType, m_dataType->GetDefaultValue(),
            "The default value for this input when no data is provided externally"));
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
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData("value", "Value", m_dataType, m_dataType->GetDefaultValue(), "A value output by this graph for external use"));

        // Register meta-data properties
        RegisterCommonSlots("output");
    }


}
