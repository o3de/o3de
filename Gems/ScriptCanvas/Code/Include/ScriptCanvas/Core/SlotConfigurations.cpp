/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    void DataSlotConfiguration::DeepCopyFrom(const Datum& source)
    {
        m_datum.DeepCopyDatum(source);
    }

    void DataSlotConfiguration::SetType(Data::Type dataType)
    {
        m_datum.SetType(dataType);
    }

    void DataSlotConfiguration::SetType(const AZ::BehaviorParameter& typeDesc)
    {
        Data::Type scType = !AZ::BehaviorContextHelper::IsStringParameter(typeDesc) ? Data::FromAZType(typeDesc.m_typeId) : Data::Type::String();
        auto dataRegistry = GetDataRegistry();
        if (dataRegistry->IsUseableInSlot(scType))
        {
            m_datum.SetType(scType);
        }
    }

    /////////////////////////////////
    // DynamicDataSlotConfiguration
    /////////////////////////////////
}
