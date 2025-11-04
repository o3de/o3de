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
    void SlotId::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SlotId>()
                ->Version(0)
                ->Field("m_name", &SlotId::m_name)
                ->Field("m_subId", &SlotId::m_subId)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SlotId>("GraphModelSlotId")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Constructor<const SlotName&>()
                ->Constructor<const SlotName&, SlotSubId>()
                ->Method("__repr__", &SlotId::ToString)
                ->Method("ToString", &SlotId::ToString)
                ->Method("IsValid", &SlotId::IsValid)
                ->Method("GetHash", &SlotId::GetHash)
                ->Property("name", BehaviorValueProperty(&SlotId::m_name))
                ->Property("subId", BehaviorValueProperty(&SlotId::m_subId))
                ;
        }
    }

    SlotId::SlotId(const SlotName& name)
        : m_name(name)
    {
    }

    SlotId::SlotId(const SlotName& name, SlotSubId subId)
        : m_name(name)
        , m_subId(subId)
    {
    }

    bool SlotId::IsValid() const
    {
        return !m_name.empty() && (m_subId >= 0);
    }

    bool SlotId::operator==(const SlotId& rhs) const
    {
        return (m_name == rhs.m_name) && (m_subId == rhs.m_subId);
    }

    bool SlotId::operator!=(const SlotId& rhs) const
    {
        return (m_name != rhs.m_name) || (m_subId != rhs.m_subId);
    }

    bool SlotId::operator<(const SlotId& rhs) const
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

    bool SlotId::operator>(const SlotId& rhs) const
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

    AZStd::size_t SlotId::GetHash() const
    {
        AZStd::size_t result = 0;
        AZStd::hash_combine(result, m_name);
        AZStd::hash_combine(result, m_subId);
        return result;
    }

    AZStd::string SlotId::ToString() const
    {
        return AZStd::string::format("GraphModelSlotId(%s,%d)", m_name.c_str(), m_subId);
    }

    /////////////////////////////////////////////////////////
    // SlotDefinition

    SlotDefinition::SlotDefinition(
        SlotDirection slotDirection,
        SlotType slotType,
        const AZStd::string& name,
        const AZStd::string& displayName,
        const AZStd::string& description,
        const DataTypeList& supportedDataTypes,
        const AZStd::any& defaultValue,
        int minimumSlots,
        int maximumSlots,
        const AZStd::string& addButtonLabel,
        const AZStd::string& addButtonTooltip,
        const AZStd::vector<AZStd::string>& enumValues,
        bool visibleOnNode,
        bool editableOnNode)
        : m_slotDirection(slotDirection)
        , m_slotType(slotType)
        , m_name(name)
        , m_displayName(displayName)
        , m_description(description)
        , m_supportedDataTypes(supportedDataTypes)
        , m_defaultValue(defaultValue)
        , m_minimumSlots(AZStd::max(0, AZStd::min(minimumSlots, maximumSlots)))
        , m_maximumSlots(AZStd::max(0, AZStd::max(minimumSlots, maximumSlots)))
        , m_addButtonLabel(addButtonLabel)
        , m_addButtonTooltip(addButtonTooltip)
        , m_enumValues(enumValues)
        , m_visibleOnNode(visibleOnNode)
        , m_editableOnNode(editableOnNode)
    {
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
        return m_minimumSlots >= 0 && m_maximumSlots >= 1 && m_minimumSlots < m_maximumSlots;
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

    const AZStd::vector<AZStd::string>& SlotDefinition::GetEnumValues() const
    {
        return m_enumValues;
    }

    const int SlotDefinition::GetMinimumSlots() const
    {
        return m_minimumSlots;
    }

    const int SlotDefinition::GetMaximumSlots() const
    {
        return m_maximumSlots;
    }

    const AZStd::string& SlotDefinition::GetExtensionLabel() const
    {
        return m_addButtonLabel;
    }

    const AZStd::string& SlotDefinition::GetExtensionTooltip() const
    {
        return m_addButtonTooltip;
    }

    /////////////////////////////////////////////////////////
    // Slot

    void Slot::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Slot, GraphElement>()
                ->Version(1)
                ->Field("m_value", &Slot::m_value)
                ->Field("m_subId", &Slot::m_subId)
                ;

            serializeContext->RegisterGenericType<SlotPtr>();
            serializeContext->RegisterGenericType<SlotPtrList>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Slot>("GraphModelSlot")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Method("Is", &Slot::Is)
                ->Method("GetSlotDirection", &Slot::GetSlotDirection)
                ->Method("GetSlotType", &Slot::GetSlotType)
                ->Method("SupportsValues", &Slot::SupportsValues)
                ->Method("SupportsDataTypes", &Slot::SupportsDataTypes)
                ->Method("SupportsConnections", &Slot::SupportsConnections)
                ->Method("SupportsExtendability", &Slot::SupportsExtendability)
                ->Method("IsVisibleOnNode", &Slot::IsVisibleOnNode)
                ->Method("IsEditableOnNode", &Slot::IsEditableOnNode)
                ->Method("GetName", &Slot::GetName)
                ->Method("GetDisplayName", &Slot::GetDisplayName)
                ->Method("GetDescription", &Slot::GetDescription)
                ->Method("GetEnumValues", &Slot::GetEnumValues)
                ->Method("GetDataType", &Slot::GetDataType)
                ->Method("GetDefaultDataType", &Slot::GetDefaultDataType)
                ->Method("GetDefaultValue", &Slot::GetDefaultValue)
                ->Method("GetSupportedDataTypes", &Slot::GetSupportedDataTypes)
                ->Method("IsSupportedDataType", &Slot::IsSupportedDataType)
                ->Method("GetMinimumSlots", &Slot::GetMinimumSlots)
                ->Method("GetMaximumSlots", &Slot::GetMaximumSlots)
                ->Method("GetSlotId", &Slot::GetSlotId)
                ->Method("GetSlotSubId", &Slot::GetSlotSubId)
                ->Method("GetParentNode", &Slot::GetParentNode)
                ->Method("GetValue", static_cast<AZStd::any(Slot::*)()const>(&Slot::GetValue))
                ->Method("SetValue", static_cast<void(Slot::*)(const AZStd::any&)>(&Slot::SetValue))
                ->Method("GetConnections", &Slot::GetConnections)
                ->Method("ClearCachedData", &Slot::ClearCachedData)
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
        ClearCachedData();

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
        AZStd::scoped_lock lock(m_parentNodeMutex);
        if (m_parentNodeDirty)
        {
            m_parentNodeDirty = false;
            for (auto nodePair : GetGraph()->GetNodes())
            {
                if (nodePair.second->Contains(shared_from_this()))
                {
                    m_parentNode = nodePair.second;
                    return m_parentNode;
                }
            }
        }
        return m_parentNode;
    }

    AZStd::any Slot::GetValue() const
    {
        return !m_value.empty() ? m_value : GetDefaultValue();
    }

    const Slot::ConnectionList& Slot::GetConnections() const
    {
        AZStd::scoped_lock lock(m_connectionsMutex);
        if (m_connectionsDirty)
        {
            m_connectionsDirty = false;
            m_connections.clear();
            m_connections.reserve(GetGraph()->GetConnectionCount());
            for (const auto& connection : GetGraph()->GetConnections())
            {
                const auto& sourceSlot = connection->GetSourceSlot();
                const auto& targetSlot = connection->GetTargetSlot();
                if (targetSlot.get() == this || sourceSlot.get() == this)
                {
                    m_connections.emplace_back(connection);
                }
            }
        }
        return m_connections;
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

    const AZStd::vector<AZStd::string>& Slot::GetEnumValues() const
    {
        return m_slotDefinition->GetEnumValues();
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

    void Slot::ClearCachedData()
    {
        {
            AZStd::scoped_lock lock(m_parentNodeMutex);
            m_parentNodeDirty = true;
            m_parentNode = {};
        }

        {
            AZStd::scoped_lock lock(m_connectionsMutex);
            m_connectionsDirty = true;
            m_connections.clear();
        }
    }
} // namespace GraphModel
