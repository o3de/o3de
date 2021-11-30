/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/Slots/Data/DataSlotComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Slots/Data/DataSlotConnectionPin.h>
#include <Components/Slots/Data/DataSlotLayoutComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/StylingComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/DataConnectionFilters.h>
#include <GraphCanvas/Components/StyleBus.h>

namespace GraphCanvas
{
    //////////////////////
    // DataSlotComponent
    //////////////////////

    void DataSlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<DataSlotComponent, SlotComponent>()
                ->Version(6)
                ->Field("TypeId", &DataSlotComponent::m_dataTypeId)
                ->Field("DataSlotType", &DataSlotComponent::m_dataSlotType)
                ->Field("CanConvertSlotTypes", &DataSlotComponent::m_canConvertSlotTypes)
                ->Field("ContainedTypeIds", &DataSlotComponent::m_containedTypeIds)
                ->Field("DataValueType", &DataSlotComponent::m_valueType)
                ->Field("IsUserSlot", &DataSlotComponent::m_isUserSlot)
            ;
        }
    }

    AZ::Entity* DataSlotComponent::CreateDataSlot(const AZ::EntityId& nodeId, const DataSlotConfiguration& dataSlotConfiguration)
    {
        AZ::Entity* entity = SlotComponent::CreateCoreSlotEntity();

        DataSlotComponent* dataSlot = aznew DataSlotComponent(dataSlotConfiguration);

        if (!entity->AddComponent(dataSlot))
        {
            delete dataSlot;
            delete entity;
            return nullptr;
        }

        entity->CreateComponent<DataSlotLayoutComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::DataSlot, nodeId, "");

        SlotConnectionFilterComponent* connectionFilter = entity->CreateComponent<SlotConnectionFilterComponent>();

        SlotTypeFilter* slotTypeFilter = aznew SlotTypeFilter(ConnectionFilterType::Include);
        slotTypeFilter->AddSlotType(SlotTypes::DataSlot);

        connectionFilter->AddFilter(slotTypeFilter);

        ConnectionTypeFilter* connectionTypeFilter = aznew ConnectionTypeFilter(ConnectionFilterType::Include);

        switch (dataSlot->GetConnectionType())
        {
            case ConnectionType::CT_Input:
                connectionTypeFilter->AddConnectionType(CT_Output);
                break;
            case ConnectionType::CT_Output:
                connectionTypeFilter->AddConnectionType(CT_Input);
                break;
            default:
                break;
        };

        connectionFilter->AddFilter(connectionTypeFilter);

        connectionFilter->AddFilter(aznew DataSlotTypeFilter());

        return entity;
    }
    
    DataSlotComponent::DataSlotComponent()
        : SlotComponent(SlotTypes::DataSlot)
        , m_canConvertSlotTypes(false)
        , m_dataSlotType(DataSlotType::Value)
        , m_valueType(DataValueType::Primitive)
        , m_dataTypeId(AZ::Uuid::CreateNull())
        , m_previousDataSlotType(DataSlotType::Unknown)
        , m_isUserSlot(false)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::DataGroup;
        }
    }

    DataSlotComponent::DataSlotComponent(const DataSlotConfiguration& dataSlotConfiguration)
        : SlotComponent(SlotTypes::DataSlot, dataSlotConfiguration)
        , m_canConvertSlotTypes(dataSlotConfiguration.m_canConvertTypes)
        , m_dataSlotType(dataSlotConfiguration.m_dataSlotType)
        , m_valueType(dataSlotConfiguration.m_dataValueType)
        , m_dataTypeId(dataSlotConfiguration.m_typeId)
        , m_containedTypeIds(dataSlotConfiguration.m_containerTypeIds)
        , m_previousDataSlotType(DataSlotType::Unknown)
        , m_isUserSlot(dataSlotConfiguration.m_isUserAdded)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::DataGroup;
        }
    }

    DataSlotComponent::~DataSlotComponent()
    {
    }
    
    void DataSlotComponent::Init()
    {
        SlotComponent::Init();
    }
    
    void DataSlotComponent::Activate()
    {
        SlotComponent::Activate();
        
        DataSlotRequestBus::Handler::BusConnect(GetEntityId());

        // Will usually happen in a copy/paste scenario
        if (m_valueType == DataValueType::Container)
        {
            SetDataAndContainedTypeIds(m_dataTypeId, m_containedTypeIds, m_valueType);
        }        
    }
    
    void DataSlotComponent::Deactivate()
    {
        SlotComponent::Deactivate();
        
        DataSlotRequestBus::Handler::BusDisconnect();
    }

    void DataSlotComponent::DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        const bool updateDisplay = false;
        RestoreDisplay(updateDisplay);

        DataSlotType slotType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(slotType, endpoint.GetSlotId(), &DataSlotRequests::GetDataSlotType);

        m_displayedConnection = connectionId;
        m_connections.emplace_back(m_displayedConnection);
        m_previousDataSlotType = m_dataSlotType;

        bool isDisabled = false;

        if (slotType == DataSlotType::Value)
        {
            m_dataSlotType = DataSlotType::Value;
            isDisabled = HasConnections();
        }
        else if (slotType == DataSlotType::Reference)
        {
            if (DataSlotUtils::IsValueDataReferenceType(m_dataSlotType) || CanConvertToReference())
            {
                m_dataSlotType = DataSlotType::Reference;
            }
        }

        if (m_previousDataSlotType != m_dataSlotType)
        {
            DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
        }

        NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, isDisabled);

        UpdateDisplay();
    }

    void DataSlotComponent::RemoveProposedConnection(const AZ::EntityId& /*connectionId*/, const Endpoint& /*endpoint*/)
    {
        RestoreDisplay(true);
    }

    void DataSlotComponent::AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        SlotComponent::AddConnectionId(connectionId, endpoint);

        if (m_dataSlotType == DataSlotType::Value)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, true);
        }
        else if (m_dataSlotType == DataSlotType::Reference)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
        }
        else
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }
    }

    void DataSlotComponent::RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        SlotComponent::RemoveConnectionId(connectionId, endpoint);

        NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
    }

    void DataSlotComponent::SetNode(const AZ::EntityId& nodeId)
    {
        SlotComponent::SetNode(nodeId);
    }

    SlotConfiguration* DataSlotComponent::CloneSlotConfiguration() const
    {
        DataSlotConfiguration* slotConfiguration = aznew DataSlotConfiguration();

        slotConfiguration->m_dataSlotType = GetDataSlotType();

        slotConfiguration->m_typeId = GetDataTypeId();

        slotConfiguration->m_containerTypeIds = m_containedTypeIds;

        slotConfiguration->m_isUserAdded = m_isUserSlot;

        PopulateSlotConfiguration((*slotConfiguration));

        return slotConfiguration;
    }

    void DataSlotComponent::UpdateDisplay()
    {
        DataSlotLayoutRequestBus::Event(GetEntityId(), &DataSlotLayoutRequests::UpdateDisplay);
    }

    void DataSlotComponent::RestoreDisplay(bool updateDisplay)
    {
        if (m_previousDataSlotType != DataSlotType::Unknown)
        {
            bool typeChanged = m_dataSlotType != m_previousDataSlotType;

            m_dataSlotType = m_previousDataSlotType;

            if (m_displayedConnection.IsValid())
            {
                for (auto connectionIter = m_connections.begin(); connectionIter != m_connections.end(); ++connectionIter)
                {
                    if (*connectionIter == m_displayedConnection)
                    {
                        m_connections.erase(connectionIter);
                        break;
                    }
                }
            }

            if (updateDisplay)
            {
                UpdatePropertyDisplayState();

                if (typeChanged)
                {
                    DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
                }

                UpdateDisplay();
            }

            m_previousDataSlotType = DataSlotType::Unknown;
            m_displayedConnection.SetInvalid();
        }
    }

    void DataSlotComponent::OnFinalizeDisplay()
    {
        UpdatePropertyDisplayState();
    }

    void DataSlotComponent::UpdatePropertyDisplayState()
    {
        if (m_dataSlotType == DataSlotType::Reference)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
        }
        else if (DataSlotUtils::IsValueDataSlotType(m_dataSlotType))
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }
    }

    bool DataSlotComponent::ConvertToReference()
    {
        if (CanConvertToReference())
        {
            AZ::EntityId nodeId = GetNode();
            GraphId graphId;

            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            {
                ScopedGraphUndoBlocker undoBlocker(graphId);

                bool convertedToReference = false;
                GraphModelRequestBus::EventResult(convertedToReference, graphId, &GraphModelRequests::ConvertSlotToReference, Endpoint(nodeId, GetEntityId()));

                if (convertedToReference)
                {
                    m_dataSlotType = DataSlotType::Reference;
                    DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
                    NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
                }
            }

            if (m_dataSlotType == DataSlotType::Reference)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
            }
        }

        return m_dataSlotType == DataSlotType::Reference;
    }

    bool DataSlotComponent::CanConvertToReference() const
    {
        bool canToggleReference = false;        

        if (m_canConvertSlotTypes && DataSlotUtils::IsValueDataSlotType(m_dataSlotType) && !HasConnections())
        {
            AZ::EntityId nodeId = GetNode();
            GraphId graphId;

            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);            
            GraphModelRequestBus::EventResult(canToggleReference, graphId, &GraphModelRequests::CanConvertSlotToReference, Endpoint(nodeId, GetEntityId()));
        }

        return canToggleReference;
    }

    bool DataSlotComponent::ConvertToValue()
    {
        if (CanConvertToValue())
        {
            AZ::EntityId nodeId = GetNode();
            GraphId graphId;

            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            {
                ScopedGraphUndoBlocker undoBlocker(graphId);
                
                bool converted = false;
                GraphModelRequestBus::EventResult(converted, graphId, &GraphModelRequests::ConvertSlotToValue, Endpoint(nodeId, GetEntityId()));

                if (converted)
                {
                    m_dataSlotType = DataSlotType::Value;

                    DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
                    NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
                }
            }

            if (DataSlotUtils::IsValueDataSlotType(m_dataSlotType))
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
            }
        }

        return DataSlotUtils::IsValueDataSlotType(m_dataSlotType);
    }

    bool DataSlotComponent::CanConvertToValue() const
    {
        bool canConvertToValue = false;

        if (m_canConvertSlotTypes && m_dataSlotType == DataSlotType::Reference)
        {
            AZ::EntityId nodeId = GetNode();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            GraphModelRequestBus::EventResult(canConvertToValue, graphId, &GraphModelRequests::CanConvertSlotToValue, Endpoint(nodeId, GetEntityId()));
        }

        return canConvertToValue;
    }

    DataSlotType DataSlotComponent::GetDataSlotType() const
    {
        return m_dataSlotType;
    }

    DataValueType DataSlotComponent::GetDataValueType() const
    {
        return m_valueType;
    }

    AZ::Uuid DataSlotComponent::GetDataTypeId() const
    {
        return m_dataTypeId;
    }

    void DataSlotComponent::SetDataTypeId(AZ::Uuid typeId)
    {
        if (m_dataTypeId != typeId)
        {
            m_dataTypeId = typeId;
            UpdateDisplay();
        }
    }

    bool DataSlotComponent::IsUserSlot() const
    {
        return m_isUserSlot;
    }

    const Styling::StyleHelper* DataSlotComponent::GetDataColorPalette() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

        const Styling::StyleHelper* stylingHelper = nullptr;
        StyleManagerRequestBus::EventResult(stylingHelper, editorId, &StyleManagerRequests::FindDataColorPalette, m_dataTypeId);

        return stylingHelper;
    }

    size_t DataSlotComponent::GetContainedTypesCount() const
    {
        return m_containedTypeIds.size();
    }

    AZ::Uuid DataSlotComponent::GetContainedTypeId(size_t index) const
    {
        return m_containedTypeIds[index];
    }

    const GraphCanvas::Styling::StyleHelper* DataSlotComponent::GetContainedTypeColorPalette(size_t index) const
    {
        AZ::Uuid dataTypeId = m_containedTypeIds[index];

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

        const Styling::StyleHelper* stylingHelper = nullptr;
        StyleManagerRequestBus::EventResult(stylingHelper, editorId, &StyleManagerRequests::FindDataColorPalette, dataTypeId);

        return stylingHelper;
    }

    void DataSlotComponent::SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds, DataValueType valueType)
    {
        if (m_dataTypeId != typeId)
        {
            m_dataTypeId = typeId;

            // Handles a weird case with string.
            // Since it's a primitive, but is a container of chars.
            if (valueType == DataValueType::Primitive)
            {
                m_containedTypeIds.clear();
            }
            else
            {
                m_containedTypeIds = typeIds;
            }

            m_valueType = valueType;

            DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDisplayTypeChanged, m_dataTypeId, m_containedTypeIds);
            
            UpdatePropertyDisplayState();
        }
    }

    AZ::Entity* DataSlotComponent::ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
    {
        const AZStd::string k_valueConnectionSubStyle = ".varFlow";
        const AZStd::string k_referenceConnectionSubStyle = ".referenceFlow";

        bool isReference = GetDataSlotType() == DataSlotType::Reference;

        // Create this Connection's entity.
        return DataConnectionComponent::CreateDataConnection(sourceEndpoint, targetEndpoint, createModelConnection, isReference ? k_referenceConnectionSubStyle : k_valueConnectionSubStyle);
    }
}
