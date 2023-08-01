/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "ScriptCanvasTestNodes.h"

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/SlotConfigurationDefaults.h>

#include <gtest/gtest.h>

namespace TestNodes
{
    //////////////////////////////////////////////////////////////////////////////
    void TestResult::OnInit()
    {        
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());

        ScriptCanvas::DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "Value";
        slotConfiguration.m_dynamicDataType = ScriptCanvas::DynamicDataType::Any;
        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);

        AddSlot(slotConfiguration);
    }

    void TestResult::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestResult, Node>()
                ->Version(5)
                ->Field("m_string", &TestResult::m_string)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<TestResult>("TestResult", "Development node, will be replaced by a Log node")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/TestResult.png")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestResult::m_string, "String", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    void ContractNode::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ContractNode, Node>()
                ->Version(1)
                ;
        }
    }

    void ContractNode::OnInit()
    {
        using namespace ScriptCanvas;

        SlotId inSlotId = Node::AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());

        auto func = []() { return aznew DisallowReentrantExecutionContract{}; };
        ContractDescriptor descriptor{ AZStd::move(func) };
        GetSlot(inSlotId)->AddContract(descriptor);

        AddSlot(ScriptCanvas::DataSlotConfiguration(Data::Type::String(), "Set String", ScriptCanvas::ConnectionType::Input));
        AddSlot(ScriptCanvas::DataSlotConfiguration(Data::Type::String(), "Get String", ScriptCanvas::ConnectionType::Output));

        AddSlot(ScriptCanvas::DataSlotConfiguration(Data::Type::Number(), "Set Number", ScriptCanvas::ConnectionType::Input));
        AddSlot(ScriptCanvas::DataSlotConfiguration(Data::Type::Number(), "Get Number", ScriptCanvas::ConnectionType::Output));
    }

    //////////////////////////////////////////////////////////////////////////////
    void InfiniteLoopNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<InfiniteLoopNode, Node>()
                ->Version(0)
                ;
        }
    }

    void InfiniteLoopNode::OnInit()
    {
        AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        AddSlot(ScriptCanvas::ExecutionSlotConfiguration("Before Infinity", ScriptCanvas::ConnectionType::Output));
        AddSlot(ScriptCanvas::ExecutionSlotConfiguration("After Infinity", ScriptCanvas::ConnectionType::Output));
    }

    //////////////////////////////////////////////////////////////////////////////
    void UnitTestErrorNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<UnitTestErrorNode, Node>()
                ->Version(0)
                ;

        }
    }

    void UnitTestErrorNode::OnInit()
    {
        AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());

        ScriptCanvas::DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
        slotConfiguration.m_name = "This";
        slotConfiguration.m_dynamicDataType = ScriptCanvas::DynamicDataType::Any;

        AddSlot(slotConfiguration);
    }

    //////////////////////////////////////////////////////////////////////////////

    void AddNodeWithRemoveSlot::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<AddNodeWithRemoveSlot, ScriptCanvas::Node>()
                ->Version(0)
                ->Field("m_dynamicSlotIds", &AddNodeWithRemoveSlot::m_dynamicSlotIds)
                ;

        }
    }

    ScriptCanvas::SlotId AddNodeWithRemoveSlot::AddSlot(AZStd::string_view slotName)
    {
        ScriptCanvas::SlotId addedSlotId = FindSlotIdForDescriptor(slotName, ScriptCanvas::SlotDescriptors::DataIn());
        if (!addedSlotId.IsValid())
        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = slotName;
            slotConfiguration.SetDefaultValue(0.0);
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);

            addedSlotId = Node::AddSlot(slotConfiguration);

            m_dynamicSlotIds.push_back(addedSlotId);
        }

        return addedSlotId;
    }

    bool AddNodeWithRemoveSlot::RemoveSlot(const ScriptCanvas::SlotId& slotId, bool emitWarning /*= true*/)
    {
        AZStd::erase_if(m_dynamicSlotIds, [slotId](const ScriptCanvas::SlotId& sId) { return (sId == slotId); });
        return Node::RemoveSlot(slotId, true, emitWarning);
    }

    void AddNodeWithRemoveSlot::OnInit()
    {
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());

        for (AZStd::string slotName : {"A", "B", "C"})
        {
            ScriptCanvas::SlotId slotId = FindSlotIdForDescriptor(slotName, ScriptCanvas::SlotDescriptors::DataIn());

            if (!slotId.IsValid())
            {
                ScriptCanvas::DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = slotName;
                slotConfiguration.SetDefaultValue(0);
                slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);

                m_dynamicSlotIds.push_back(Node::AddSlot(slotConfiguration));
            }
        }

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = "Result";
            slotConfiguration.SetType(ScriptCanvas::Data::Type::Number());
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            m_resultSlotId = Node::AddSlot(slotConfiguration);
        }
    }

    void StringView::OnInit()
    {
        AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = "View";
            slotConfiguration.m_toolTip = "Input string_view object";
            slotConfiguration.ConfigureDatum(AZStd::move(ScriptCanvas::Datum(ScriptCanvas::Data::Type::String(), ScriptCanvas::Datum::eOriginality::Copy)));
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);

            AddSlot(slotConfiguration);
        }

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = "Result";
            slotConfiguration.m_toolTip = "Output string object";
            slotConfiguration.SetAZType<AZStd::string>();
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            m_resultSlotId = AddSlot(slotConfiguration);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    void StringView::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<StringView, Node>()
                ->Version(0)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    void InsertSlotConcatNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<InsertSlotConcatNode, ScriptCanvas::Node>()
                ->Version(0)
                ;

        }
    }

    ScriptCanvas::SlotId InsertSlotConcatNode::InsertSlot(AZ::s64 index, AZStd::string_view slotName)
    {
        using namespace ScriptCanvas;
        SlotId addedSlotId = FindSlotIdForDescriptor(slotName, ScriptCanvas::SlotDescriptors::DataIn());
        if (!addedSlotId.IsValid())
        {
            DataSlotConfiguration dataConfig;
            dataConfig.m_name = slotName;
            dataConfig.m_toolTip = "";
            dataConfig.SetConnectionType(ScriptCanvas::ConnectionType::Input);
            dataConfig.SetDefaultValue(Data::StringType());

            addedSlotId = Node::InsertSlot(index, dataConfig);
        }

        return addedSlotId;
    }

    void InsertSlotConcatNode::OnInit()
    {
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        Node::AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());
        Node::AddSlot(ScriptCanvas::DataSlotConfiguration(ScriptCanvas::Data::Type::String(), "Result", ScriptCanvas::ConnectionType::Output));
    }
    
    /////////////////////////////
    // ConfigurableUnitTestNode
    /////////////////////////////

    void ConfigurableUnitTestNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<ConfigurableUnitTestNode, ScriptCanvas::Node>()
                ->Version(0)
                ;
        }
    }

    ScriptCanvas::Slot* ConfigurableUnitTestNode::AddTestingSlot(const ScriptCanvas::SlotConfiguration& slotConfiguration)
    {
        ScriptCanvas::SlotId slotId = AddSlot(slotConfiguration);

        return GetSlot(slotId);
    }

    ScriptCanvas::Slot* ConfigurableUnitTestNode::InsertTestingSlot(int index, const ScriptCanvas::SlotConfiguration& slotConfiguration)
    {
        ScriptCanvas::SlotId slotId = InsertSlot(index, slotConfiguration);

        return GetSlot(slotId);
    }

    AZStd::vector< const ScriptCanvas::Slot* > ConfigurableUnitTestNode::FindSlotsByDescriptor(const ScriptCanvas::SlotDescriptor& slotDescriptor) const
    {
        return GetAllSlotsByDescriptor(slotDescriptor);
    }

    void ConfigurableUnitTestNode::TestClearDisplayType(const AZ::Crc32& dynamicGroup)
    {
        ClearDisplayType(dynamicGroup);
    }

    void ConfigurableUnitTestNode::TestSetDisplayType(const AZ::Crc32& dynamicGroup, const ScriptCanvas::Data::Type& dataType)
    {
        SetDisplayType(dynamicGroup, dataType);
    }

    bool ConfigurableUnitTestNode::TestHasConcreteDisplayType(const AZ::Crc32& dynamicGroup) const
    {
        return FindConcreteDisplayType(dynamicGroup).IsValid();
    }

    bool ConfigurableUnitTestNode::TestIsSlotConnectedToConcreteDisplayType(const ScriptCanvas::Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const
    {
        return FindConnectedConcreteDisplayType(slot, exploredGroupCache).IsValid();
    }

    void ConfigurableUnitTestNode::SetSlotExecutionMap(ScriptCanvas::SlotExecution::Map* executionMap)
    {
        m_slotExecutionMap = executionMap;
    }

    const ScriptCanvas::SlotExecution::Map* ConfigurableUnitTestNode::GetSlotExecutionMap() const
    {
        return m_slotExecutionMap;
    }
}

