/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>

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

        if (m_name == rhs.m_name)
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

        if (m_name == rhs.m_name)
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

    SlotDefinitionPtr SlotDefinition::CreateInputData(
        AZStd::string_view name,
        AZStd::string_view displayName,
        DataTypePtr dataType,
        AZStd::any defaultValue,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        return CreateInputData(
            name,
            displayName,
            DataTypeList{ dataType },
            defaultValue,
            description,
            extendableSlotConfiguration,
            visibleOnNode,
            editableOnNode);
    }

    SlotDefinitionPtr SlotDefinition::CreateInputData(
        AZStd::string_view name,
        AZStd::string_view displayName,
        DataTypeList supportedDataTypes,
        AZStd::any defaultValue,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Data;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = supportedDataTypes;
        slotDefinition->m_defaultValue = defaultValue;
        slotDefinition->m_description = description;
        slotDefinition->m_visibleOnNode = visibleOnNode;
        slotDefinition->m_editableOnNode = editableOnNode;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateOutputData(
        AZStd::string_view name,
        AZStd::string_view displayName,
        DataTypePtr dataType,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Output;
        slotDefinition->m_slotType = SlotType::Data;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = { dataType };
        slotDefinition->m_defaultValue = dataType->GetDefaultValue();
        slotDefinition->m_description = description;
        slotDefinition->m_visibleOnNode = visibleOnNode;
        slotDefinition->m_editableOnNode = editableOnNode;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateInputEvent(
        AZStd::string_view name,
        AZStd::string_view displayName,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Event;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_description = description;
        slotDefinition->m_visibleOnNode = visibleOnNode;
        slotDefinition->m_editableOnNode = editableOnNode;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateOutputEvent(
        AZStd::string_view name,
        AZStd::string_view displayName,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Output;
        slotDefinition->m_slotType = SlotType::Event;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_description = description;
        slotDefinition->m_visibleOnNode = visibleOnNode;
        slotDefinition->m_editableOnNode = editableOnNode;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    SlotDefinitionPtr SlotDefinition::CreateProperty(
        AZStd::string_view name,
        AZStd::string_view displayName,
        DataTypePtr dataType,
        AZStd::any defaultValue,
        AZStd::string_view description,
        ExtendableSlotConfiguration* extendableSlotConfiguration,
        bool visibleOnNode,
        bool editableOnNode)
    {
        AZStd::shared_ptr<SlotDefinition> slotDefinition = AZStd::make_shared<SlotDefinition>();
        slotDefinition->m_slotDirection = SlotDirection::Input;
        slotDefinition->m_slotType = SlotType::Property;
        slotDefinition->m_name = name;
        slotDefinition->m_displayName = displayName;
        slotDefinition->m_supportedDataTypes = { dataType };
        slotDefinition->m_defaultValue = defaultValue;
        slotDefinition->m_description = description;
        slotDefinition->m_visibleOnNode = visibleOnNode;
        slotDefinition->m_editableOnNode = editableOnNode;

        HandleExtendableSlotRegistration(slotDefinition, extendableSlotConfiguration);

        return slotDefinition;
    }

    void SlotDefinition::HandleExtendableSlotRegistration(
        AZStd::shared_ptr<SlotDefinition> slotDefinition, ExtendableSlotConfiguration* extendableSlotConfiguration)
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

    bool SlotDefinition::SupportsValues() const
    {
        return (GetSlotType() == SlotType::Data && GetSlotDirection() == SlotDirection::Input) || 
               (GetSlotType() == SlotType::Property);
    }

    bool SlotDefinition::SupportsDataTypes() const
    {
        return GetSlotType() == SlotType::Data || GetSlotType() == SlotType::Property;
    }

    bool SlotDefinition::SupportsConnections() const
    {
        return GetSlotType() == SlotType::Data || GetSlotType() == SlotType::Event;
    }

    bool SlotDefinition::IsVisibleOnNode() const
    {
        return m_visibleOnNode;
    }

    bool SlotDefinition::IsEditableOnNode() const
    {
        return m_editableOnNode;
    }

    bool SlotDefinition::SupportsExtendability() const
    {
        return m_extendableSlotConfiguration.m_isValid;
    }

    bool SlotDefinition::Is(SlotDirection slotDirection, SlotType slotType) const
    {
        return GetSlotDirection() == slotDirection && GetSlotType() == slotType;
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
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Slot>()
                ->Version(1)
                ->Field("m_value", &Slot::m_value)
                ->Field("m_subId", &Slot::m_subId)
                ;
        }
    }

    Slot::Slot(GraphPtr graph, SlotDefinitionPtr slotDefinition, SlotSubId subId)
        : GraphElement(graph)
        , m_slotDefinition(slotDefinition)
        , m_subId(subId)
    {
        // The m_value must be initialized with an object of the appropriate type, or GetValue() will fail the first time its called.
        SetValue(m_slotDefinition->GetDefaultValue());
    }

    void Slot::PostLoadSetup(GraphPtr graph, SlotDefinitionPtr slotDefinition)
    {
        AZ_Assert(nullptr == GetGraph(), "This slot is not freshly loaded");

        m_graph = graph;
        m_slotDefinition = slotDefinition;

        if (SupportsValues())
        {
            AZ_Error(
                GetGraph()->GetSystemName(),
                GetDataType(),
                "Possible data corruption. Slot [%s] does not match any supported data type.",
                GetDisplayName().c_str());
        }
    }

    NodePtr Slot::GetParentNode() const
    {
        for (auto nodePair : GetGraph()->GetNodes())
        {
            if (nodePair.second->Contains(shared_from_this()))
            {
                return nodePair.second;
            }
        }
        return {};
    }

    AZStd::any Slot::GetValue() const
    {
        return !m_value.empty() ? m_value : GetDefaultValue();
    }

    Slot::ConnectionList Slot::GetConnections() const
    {
        ConnectionList connections;
        for (const auto& connection : GetGraph()->GetConnections())
        {
            const auto& sourceSlot = connection->GetSourceSlot();
            const auto& targetSlot = connection->GetTargetSlot();
            if (targetSlot && sourceSlot && targetSlot != sourceSlot && (targetSlot.get() == this || sourceSlot.get() == this))
            {
                connections.insert(connection);
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

    SlotDirection Slot::GetSlotDirection() const
    {
        return m_slotDefinition->GetSlotDirection();
    }

    SlotType Slot::GetSlotType() const
    {
        return m_slotDefinition->GetSlotType();
    }

    bool Slot::SupportsValues() const
    {
        return m_slotDefinition->SupportsValues();
    }

    bool Slot::SupportsDataTypes() const
    {
        return m_slotDefinition->SupportsDataTypes();
    }

    bool Slot::SupportsConnections() const
    {
        return m_slotDefinition->SupportsConnections();
    }

    bool Slot::SupportsExtendability() const
    {
        return m_slotDefinition->SupportsExtendability();
    }

    bool Slot::IsVisibleOnNode() const
    {
        return m_slotDefinition->IsVisibleOnNode();
    }

    bool Slot::IsEditableOnNode() const
    {
        return m_slotDefinition->IsEditableOnNode();
    }

    const SlotName& Slot::GetName() const
    {
        return m_slotDefinition->GetName();
    }

    const AZStd::string& Slot::GetDisplayName() const
    {
        return m_slotDefinition->GetDisplayName();
    }

    const AZStd::string& Slot::GetDescription() const
    {
        return m_slotDefinition->GetDescription();
    }

    AZStd::any Slot::GetDefaultValue() const
    {
        return m_slotDefinition->GetDefaultValue();
    }

    const DataTypeList& Slot::GetSupportedDataTypes() const
    {
        return m_slotDefinition->GetSupportedDataTypes();
    }

    bool Slot::IsSupportedDataType(DataTypePtr dataType) const
    {
        for (DataTypePtr supportedDataType : GetSupportedDataTypes())
        {
            if (supportedDataType == dataType)
            {
                return true;
            }
        }

        return false;
    }

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

    DataTypePtr Slot::GetDataType() const
    {
        // Because some slots support multiple data types search for the one corresponding to the value
        return GetDataTypeForValue(GetValue());
    }

    DataTypePtr Slot::GetDefaultDataType() const
    {
        return GetDataTypeForValue(GetDefaultValue());
    }

    void Slot::SetValue(const AZStd::any& value)
    {
        if (SupportsValues())
        {
#if defined(AZ_ENABLE_TRACING)
            DataTypePtr dataTypeRequested = GetDataTypeForValue(value);
            AssertWithTypeInfo(IsSupportedDataType(dataTypeRequested), dataTypeRequested, "Slot::SetValue Requested with the wrong type");
#endif
            m_value = value;
        }
    }

#if defined(AZ_ENABLE_TRACING)
    void Slot::AssertWithTypeInfo(bool expression, DataTypePtr dataTypeRequested, const char* message) const
    {
        DataTypePtr dataType = GetDataType();
        AZ_Assert(expression, "%s Slot %s (Current DataType=['%s', '%s', %s]. Requested DataType=['%s', '%s', %s]). Current Value TypeId=%s.",
            message,
            GetDisplayName().c_str(),
            !dataType ? "nullptr" : dataType->GetDisplayName().c_str(),
            !dataType ? "nullptr" : dataType->GetCppName().c_str(),
            !dataType ? "nullptr" : dataType->GetTypeUuidString().c_str(),
            !dataTypeRequested ? "nullptr" : dataTypeRequested->GetDisplayName().c_str(),
            !dataTypeRequested ? "nullptr" : dataTypeRequested->GetCppName().c_str(),
            !dataTypeRequested ? "nullptr" : dataTypeRequested->GetTypeUuidString().c_str(),
            GetValue().type().ToString<AZStd::string>().c_str()
        );
    }
#endif // AZ_ENABLE_TRACING

    DataTypePtr Slot::GetDataTypeForTypeId(const AZ::Uuid& typeId) const
    {
        // Search for and return the first data type that supports the input type ID
        for (DataTypePtr dataType : GetSupportedDataTypes())
        {
            // If this slot does not support input values but has registered data types then the first data type will be returned.
            if (!SupportsDataTypes() || dataType->IsSupportedType(typeId))
            {
                return dataType;
            }
        }

        return {};
    }

    DataTypePtr Slot::GetDataTypeForValue(const AZStd::any& value) const
    {
        // Search for and return the first data type that supports the input value
        for (DataTypePtr dataType : GetSupportedDataTypes())
        {
            // If this slot does not support input values but has registered data types then the first data type will be returned.
            if (!SupportsValues() || dataType->IsSupportedValue(value))
            {
                return dataType;
            }
        }

        return {};
    }
} // namespace GraphModel
