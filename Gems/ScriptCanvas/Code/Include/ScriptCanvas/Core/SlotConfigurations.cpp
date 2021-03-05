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

#include <ScriptCanvas/Core/SlotConfigurations.h>

#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvas
{
    //////////////////
    // SlotTypeUtils
    //////////////////

    AZStd::pair<ConnectionType, SlotTypeDescriptor> SlotTypeUtils::BreakApartSlotType(CombinedSlotType slotType)
    {
        AZStd::pair<ConnectionType, SlotTypeDescriptor> brokenDescription;

        switch (slotType)
        {
        case CombinedSlotType::ExecutionIn:
            brokenDescription.first = ConnectionType::Input;
            brokenDescription.second = SlotTypeDescriptor::Execution;
            break;
        case CombinedSlotType::ExecutionOut:
            brokenDescription.first = ConnectionType::Output;
            brokenDescription.second = SlotTypeDescriptor::Execution;
            break;
        case CombinedSlotType::LatentOut:
            brokenDescription.first = ConnectionType::Output;
            brokenDescription.second = SlotTypeDescriptor::Execution;
            break;
        case CombinedSlotType::DataIn:
            brokenDescription.first = ConnectionType::Input;
            brokenDescription.second = SlotTypeDescriptor::Data;
            break;
        case CombinedSlotType::DataOut:
            brokenDescription.first = ConnectionType::Output;
            brokenDescription.second = SlotTypeDescriptor::Data;
            break;
        default:
            brokenDescription.first = ConnectionType::Unknown;
            brokenDescription.second = SlotTypeDescriptor::Unknown;
            break;
        }

        return brokenDescription;
    }

    //////////////////////
    // SlotConfiguration
    //////////////////////

    SlotConfiguration::SlotConfiguration(SlotTypeDescriptor slotType)
        : m_slotId(AZ::Uuid::CreateRandom())
    {
        m_slotDescriptor.m_slotType = slotType;
    }

    void SlotConfiguration::SetConnectionType(ConnectionType connectionType)
    {
        m_slotDescriptor.m_connectionType = connectionType;
    }

    //////////////////////////
    // DataSlotConfiguration
    //////////////////////////

    DataSlotConfiguration::DataSlotConfiguration(Datum&& datum)
        : SlotConfiguration(SlotTypeDescriptor::Data)
        , m_datum(AZStd::move(datum))
    {
    }

    DataSlotConfiguration::DataSlotConfiguration(Data::Type dataType)
        : SlotConfiguration(SlotTypeDescriptor::Data)
        , m_datum(dataType, Datum::eOriginality::Original, nullptr, AZ::Uuid::CreateNull())
    {
    }

    DataSlotConfiguration::DataSlotConfiguration(Data::Type dataType, AZStd::string name, ConnectionType connectionType)
        : DataSlotConfiguration(dataType)
    {
        m_name = name;
        SetConnectionType(connectionType);
    }

    void DataSlotConfiguration::SetType(Data::Type dataType)
    {
        m_datum.SetType(dataType);
    }

    void DataSlotConfiguration::SetType(const AZ::BehaviorParameter& typeDesc)
    {
        auto dataRegistry = GetDataRegistry();
        Data::Type scType = !AZ::BehaviorContextHelper::IsStringParameter(typeDesc) ? Data::FromAZType(typeDesc.m_typeId) : Data::Type::String();
        auto typeIter = dataRegistry->m_creatableTypes.find(scType);

        if (typeIter != dataRegistry->m_creatableTypes.end())
        {
            m_datum.SetType(scType);
        }
    }

    /////////////////////////////////
    // DynamicDataSlotConfiguration
    /////////////////////////////////
}