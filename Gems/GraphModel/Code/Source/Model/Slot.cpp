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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Model/Slot.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/IGraphContext.h>

namespace GraphModel
{
    void SlotIdData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SlotIdData>()
                ->Version(0)
                ->Field("m_name", &SlotIdData::m_name)
                ->Field("m_subId", &SlotIdData::m_subId)
                ;
        }
    }

    SlotIdData::SlotIdData(const SlotName& name)
        : m_name(name)
    {
    }

    SlotIdData::SlotIdData(const SlotName& name, SlotSubId subId)
        : m_name(name)
        , m_subId(subId)
    {
    }

    bool SlotIdData::IsValid() const
    {
        return !m_name.empty() && (m_subId >= 0);
    }

    bool SlotIdData::operator==(const SlotIdData& rhs) const
    {
        return (m_name == rhs.m_name) && (m_subId == rhs.m_subId);
    }

    bool SlotIdData::operator!=(const SlotIdData& rhs) const
    {
        return (m_name != rhs.m_name) || (m_subId != rhs.m_subId);
    }

    bool SlotIdData::operator<(const SlotIdData& rhs) const
    {
        if (m_name < rhs.m_name)
        {
            return true;
        }
        else if (m_name == rhs.m_name)
        {
            return m_subId < rhs.m_subId;
        }

        return false;
    }

    bool SlotIdData::operator>(const SlotIdData& rhs) const
    {
        if (m_name > rhs.m_name)
        {
            return true;
        }
        else if (m_name == rhs.m_name)
        {
            return m_subId > rhs.m_subId;
        }

        return false;
    }

    AZStd::size_t SlotIdData::GetHash() const
    {
        AZStd::size_t result = 0;
        AZStd::hash_combine(result, m_name);
        AZStd::hash_combine(result, m_subId);

        return result;
    }

    /////////////////////////////////////////////////////////
    // SlotDefinition

    SlotDefinitionPtr SlotDefinition::CreateInputData(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Data;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = { dataType };
        slotDefinition->m_defaultValue = defaultValue;
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateInputData(AZStd::string_view name, AZStd::string_view displayName, DataTypeList supportedDataTypes, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Data;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = supportedDataTypes;
        slotDefinition->m_defaultValue = defaultValue;
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateOutputData(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Output;
        slotDefinition->m_slotType = SlotType::Data;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = { dataType };
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateInputEvent(AZStd::string_view name, AZStd::string_view displayName, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Event;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateOutputEvent(AZStd::string_view name, AZStd::string_view displayName, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Output;
        slotDefinition->m_slotType = SlotType::Event;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateProperty(AZStd::string_view name, AZStd::string_view displayName, DataTypePtr dataType, AZStd::any defaultValue, AZStd::string_view description, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Property;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = { dataType };
        slotDefinition->m_defaultValue = defaultValue;
        slotDefinition->m_description = description;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    void SlotDefinition::HandleExtendableSlotRegistration(AZStd::shared_ptr<SlotDefinition> slotDefinition, ExtendableSlotConfiguration* extendableSlotConfiguration)
    {
        if (extendableSlotConfiguration)
        {
            slotDefinition->m_extendableSlotConfiguration = *extendableSlotConfiguration;

            if (slotDefinition->m_extendableSlotConfiguration.m_minimumSlots > slotDefinition->m_extendableSlotConfiguration.m_maximumSlots)
            {
                AZ_Assert(false, "Invalid extendable slot configuration for %s, minimum slots greater than maximum slots", slotDefinition->GetName().c_str());
                return;
            }

            slotDefinition->m_extendableSlotConfiguration.m_isValid = true;
        }
    }

    SlotDirection SlotDefinition::GetSlotDirection() const
    {
        return m_slotDirection;
    }

    SlotType SlotDefinition::GetSlotType() const
    {
        return m_slotType;
    }

    bool SlotDefinition::SupportsValue() const
    {
        return (GetSlotType() == SlotType::Data && GetSlotDirection() == SlotDirection::Input) || 
               (GetSlotType() == SlotType::Property);
    }

    bool SlotDefinition::SupportsDataType() const
    {
        return GetSlotType() == SlotType::Data || GetSlotType() == SlotType::Property;
    }

    bool SlotDefinition::SupportsConnections() const
    {
        return GetSlotType() == SlotType::Data || GetSlotType() == SlotType::Event;
    }

    bool SlotDefinition::Is(SlotDirection slotDirection, SlotType slotType) const
    {
        return GetSlotDirection() == slotDirection && GetSlotType() == slotType;
    }

    bool SlotDefinition::SupportsExtendability() const
    {
        return m_extendableSlotConfiguration.m_isValid;
    }

    const DataTypeList& SlotDefinition::GetSupportedDataTypes() const
    {
        return m_supportedDataTypes;
    }

    const SlotName& SlotDefinition::GetName() const
    {
        return m_name;
    }

    const AZStd::string& SlotDefinition::GetDisplayName() const
    {
        return m_displayName;
    }

    const AZStd::string& SlotDefinition::GetDescription() const
    {
        return m_description;
    }

    AZStd::any SlotDefinition::GetDefaultValue() const
    {
        return m_defaultValue;
    }

    const int SlotDefinition::GetMinimumSlots() const
    {
        return m_extendableSlotConfiguration.m_minimumSlots;
    }

    const int SlotDefinition::GetMaximumSlots() const
    {
        return m_extendableSlotConfiguration.m_maximumSlots;
    }

    const AZStd::string& SlotDefinition::GetExtensionLabel() const
    {
        return m_extendableSlotConfiguration.m_addButtonLabel;
    }

    const AZStd::string& SlotDefinition::GetExtensionTooltip() const
    {
        return m_extendableSlotConfiguration.m_addButtonTooltip;
    }

    /////////////////////////////////////////////////////////
    // Slot

    void Slot::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Slot>()
                ->Version(0)
                ->Field("m_value", &Slot::m_value)
                ->Field("m_subId", &Slot::m_subId)
                // m_slotDescription is not reflected because that data is populated procedurally by each node
                // m_connections is not reflected because they are actually owned by the Graph and reflected there
                ;
        }
    }

    Slot::Slot(GraphPtr graph, SlotDefinitionPtr slotDefinition, SlotSubId subId)
        : GraphElement(graph)
        , m_slotDefinition(slotDefinition)
        , m_subId(subId)
    {
        if (SupportsValue())
        {
            // The m_value must be initialized with an object of the appropriate type, or 
            // GetValue() will fail the first time its called.
            SetValue(m_slotDefinition->GetDefaultValue());
        }
    }

    void Slot::PostLoadSetup(GraphPtr graph, SlotDefinitionPtr slotDefinition)
    {
        AZ_Assert(nullptr == GetGraph(), "This slot is not freshly loaded");
        AZ_Assert(m_parentNode._empty(), "This slot is not freshly loaded");

        m_graph = graph;
        m_slotDefinition = slotDefinition;

        if (SupportsValue())
        {
            // CJS TODO: Consider using AZ::Outcome for better error reporting

            // Check the serialized value type against the supported types for this slot
            // instead of just using Slot::GetDataType(), because for slots with
            // multiple supported types, Slot::GetDataType() will call GetParentNode()
            // to try and resolve its type, which will be a nullptr at this point
            // because the parent won't be valid yet
            bool valueTypeSupported = false;
            DataTypePtr valueDataType = GetGraphContext()->GetDataTypeForValue(m_value);
            for (DataTypePtr dataType : GetSupportedDataTypes())
            {
                if (valueDataType == dataType)
                {
                    valueTypeSupported = true;
                    break;
                }
            }

            AZ_Error(GetGraph()->GetSystemName(),
                valueTypeSupported,
                "Possible data corruption. Slot [%s] data type [%s] does not match any supported data type.",
                GetDisplayName().c_str(),
                valueDataType->GetDisplayName().c_str());
        }
    }

    NodePtr Slot::GetParentNode() const
    {
        // Originally the parent node was passed to the Slot constructor, but this was before
        // using shared_ptr for Nodes. Because the Node constructor is what creates the Slots,
        // and shared_from_this() doesn't work in constructors, we can't initialize m_parentNode
        // until after the Node is created. So we search for and cache the pointer here the first
        // time it is requested.
        if (m_parentNode._empty())
        {
            for (auto nodeIter : GetGraph()->GetNodes())
            {
                if (nodeIter.second->Contains(shared_from_this()))
                {
                    m_parentNode = nodeIter.second;
                    break;
                }
            }
        }

        return m_parentNode.lock();
    }

    AZStd::any Slot::GetValue() const
    {
        return m_value;
    }

    Slot::ConnectionList Slot::GetConnections() const
    {
        ConnectionList connections;

        for (auto iter : m_connections)
        {
            if (ConnectionPtr connection = iter.lock())
            {
                connections.insert(connection);
            }
            else
            {
                AZ_Assert(false , "Slot's connection cache is out of date");
            }
        }

        return connections;
    }

    SlotDefinitionPtr Slot::GetDefinition() const
    {
        return m_slotDefinition;
    }

    bool Slot::Is(SlotDirection slotDirection, SlotType slotType) const 
    { 
        return m_slotDefinition->Is(slotDirection, slotType); 
    }

    SlotDirection        Slot::GetSlotDirection()      const { return m_slotDefinition->GetSlotDirection(); }
    SlotType             Slot::GetSlotType()           const { return m_slotDefinition->GetSlotType(); }
    bool                 Slot::SupportsValue()         const { return m_slotDefinition->SupportsValue(); }
    bool                 Slot::SupportsDataType()      const { return m_slotDefinition->SupportsDataType(); }
    bool                 Slot::SupportsConnections()   const { return m_slotDefinition->SupportsConnections(); }
    bool                 Slot::SupportsExtendability() const { return m_slotDefinition->SupportsExtendability(); }
    const SlotName&      Slot::GetName()               const { return m_slotDefinition->GetName(); }
    const AZStd::string& Slot::GetDisplayName()        const { return m_slotDefinition->GetDisplayName(); }
    const AZStd::string& Slot::GetDescription()        const { return m_slotDefinition->GetDescription(); }
    AZStd::any           Slot::GetDefaultValue()       const { return m_slotDefinition->GetDefaultValue(); }
    const DataTypeList&  Slot::GetSupportedDataTypes() const { return m_slotDefinition->GetSupportedDataTypes(); }

    const int Slot::GetMinimumSlots() const
    {
        return m_slotDefinition->GetMinimumSlots();
    }

    const int Slot::GetMaximumSlots() const
    {
        return m_slotDefinition->GetMaximumSlots();
    }

    SlotId Slot::GetSlotId() const
    {
        return SlotId(GetName(), m_subId);
    }

    SlotSubId Slot::GetSlotSubId() const
    {
        return m_subId;
    }

    const DataTypeList& Slot::GetPossibleDataTypes() const
    {
        // TODO: For now this will just return all the supported types, but eventually
        // it return the subset of possible data types given the current configuration
        // of the node.
        return GetSupportedDataTypes();
    }

    DataTypePtr Slot::GetDataType() const
    {
        // If the slot definition only has a single data type, then that is returned.
        // Otherwise, we can ask our parent node to find out what the active type is.
        DataTypeList possibleDataTypes = GetPossibleDataTypes();
        size_t numPossibleDataTypes = possibleDataTypes.size();
        if (numPossibleDataTypes == 1)
        {
            return possibleDataTypes[0];
        }
        else if (numPossibleDataTypes > 1)
        {
            return GetParentNode()->GetDataType(shared_from_this());
        }

        return nullptr;
    }

    void Slot::SetValue(const AZStd::any& value)
    {
        if (SupportsValue())
        {
#if defined(AZ_ENABLE_TRACING)
            DataTypePtr dataType = GetGraphContext()->GetDataTypeForValue(value);
            AssertTypeMatch(dataType, "Slot::SetValue used with the wrong type");
#endif

            m_value = value;
        }
    }

#if defined(AZ_ENABLE_TRACING)
    void Slot::AssertWithTypeInfo(bool expression, DataTypePtr dataTypeUsed, const char* message) const
    {
        AZ_Assert(expression, "%s (Slot DataType=['%s', '%s', %s]. Used DataType=['%s', '%s', %s]). m_value TypeId=%s.",
            message,
            GetDataType()->GetDisplayName().c_str(),
            GetDataType()->GetCppName().c_str(),
            GetDataType()->GetTypeUuidString().c_str(),
            dataTypeUsed->GetDisplayName().c_str(),
            dataTypeUsed->GetCppName().c_str(),
            dataTypeUsed->GetTypeUuidString().c_str(),
            m_value.type().ToString<AZStd::string>().c_str()
        );
    }

    void Slot::AssertTypeMatch(DataTypePtr dataTypeUsed, const char* message) const
    {
        // Check if any of the possible data types for this slot match
        bool expression = false;
        for (auto iter : GetPossibleDataTypes())
        {
            expression |= (*dataTypeUsed == *iter);
        }
        AssertWithTypeInfo(expression, dataTypeUsed, message);
    }
#endif // AZ_ENABLE_TRACING


} // namespace GraphModel
