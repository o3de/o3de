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
#include "precompiled.h"

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>

#include <Editor/GraphCanvas/Components/MappingComponent.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Variable/GraphVariable.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>


namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // SceneMemberMappingComponent
    ////////////////////////////////

    void SceneMemberMappingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SceneMemberMappingComponent, AZ::Component>()
                ->Field("SourceId", &SceneMemberMappingComponent::m_sourceId)
                ->Version(1)
                ;
        }
    }

    SceneMemberMappingComponent::SceneMemberMappingComponent(const AZ::EntityId& sourceId)
        : m_sourceId(sourceId)
    {
    }

    void SceneMemberMappingComponent::Activate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());        
        SceneMemberMappingConfigurationRequestBus::Handler::BusConnect(GetEntityId());
        ConfigureMapping(m_sourceId);
    }

    void SceneMemberMappingComponent::Deactivate()
    {
        SceneMemberMappingRequestBus::Handler::BusDisconnect();
        SceneMemberMappingConfigurationRequestBus::Handler::BusDisconnect();        
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
    }

    void SceneMemberMappingComponent::ConfigureMapping(const AZ::EntityId& scriptCanvasMemberId)
    {
        if (m_sourceId.IsValid())
        {
            SceneMemberMappingRequestBus::Handler::BusDisconnect();
        }

        m_sourceId = scriptCanvasMemberId;

        if (m_sourceId.IsValid())
        {
            SceneMemberMappingRequestBus::Handler::BusConnect(m_sourceId);
        }
    }

    AZ::EntityId SceneMemberMappingComponent::GetGraphCanvasEntityId() const
    {
        return GetEntityId();
    }

    void SceneMemberMappingComponent::OnBatchedConnectionManipulationBegin()
    {
        ScriptCanvas::NodeRequestBus::Event(m_sourceId, &ScriptCanvas::NodeRequests::SignalBatchedConnectionManipulationBegin);
    }

    void SceneMemberMappingComponent::OnBatchedConnectionManipulationEnd()
    {
        ScriptCanvas::NodeRequestBus::Event(m_sourceId, &ScriptCanvas::NodeRequests::SignalBatchedConnectionManipulationEnd);
    }

    /////////////////////////
    // SlotMappingComponent
    /////////////////////////
    
    void SlotMappingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SlotMappingComponent, AZ::Component>()
                ->Version(1)
                ->Field("SourceId", &SlotMappingComponent::m_sourceId)
            ;
        }
    }   

    SlotMappingComponent::SlotMappingComponent(const AZ::EntityId& sourceId)
        : m_sourceId(sourceId)
    {
    }

    void SlotMappingComponent::Activate()
    {
        m_slotMapping.clear();

        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());        
        SceneMemberMappingConfigurationRequestBus::Handler::BusConnect(GetEntityId());

        ConfigureMapping(m_sourceId);
    }
    
    void SlotMappingComponent::Deactivate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();

        SlotMappingRequestBus::MultiHandler::BusDisconnect();
    }
    
    void SlotMappingComponent::OnAddedToScene(const AZ::EntityId&)
    {
        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);
        
        for (const AZ::EntityId& slotId : slotIds)
        {
            OnSlotAddedToNode(slotId);
        }

        SlotMappingRequestBus::MultiHandler::BusConnect(GetEntityId());
        SlotMappingRequestBus::MultiHandler::BusConnect(m_sourceId);
    }
    
    void SlotMappingComponent::OnSlotAddedToNode(const AZ::EntityId& slotId)
    {
        AZStd::any* userData = nullptr;
        
        GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);

        AZStd::string displayTitle;
        GraphCanvas::NodeTitleRequestBus::EventResult(displayTitle, GetEntityId(), &GraphCanvas::NodeTitleRequests::GetTitle);
        
        if (userData && userData->is<ScriptCanvas::SlotId>())
        {
            m_slotMapping[(*AZStd::any_cast<ScriptCanvas::SlotId>(userData))] = slotId;
        }
    }
    
    void SlotMappingComponent::OnSlotRemovedFromNode(const AZ::EntityId& slotId)
    {
        AZStd::any* userData = nullptr;

        GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);

        if (userData && userData->is<ScriptCanvas::SlotId>())
        {
            m_slotMapping.erase((*AZStd::any_cast<ScriptCanvas::SlotId>(userData)));
        }
    }

    AZ::EntityId SlotMappingComponent::MapToGraphCanvasId(const ScriptCanvas::SlotId& slotId)
    {
        AZ::EntityId mappedId;

        auto mapIter = m_slotMapping.find(slotId);

        if (mapIter != m_slotMapping.end())
        {
            mappedId = mapIter->second;
        }
        else
        {
            if (GraphCanvas::GraphUtils::IsWrapperNode(GetEntityId()))
            {
                AZStd::vector< AZ::EntityId > wrappedNodes;
                GraphCanvas::WrapperNodeRequestBus::EventResult(wrappedNodes, GetEntityId(), &GraphCanvas::WrapperNodeRequests::GetWrappedNodeIds);

                for (const auto& wrappedId : wrappedNodes)
                {
                    SlotMappingRequestBus::EventResult(mappedId, wrappedId, &SlotMappingRequests::MapToGraphCanvasId, slotId);

                    if (mappedId.IsValid())
                    {
                        break;
                    }
                }
            }
        }

        return mappedId;
    }

    void SlotMappingComponent::OnSlotRenamed(const ScriptCanvas::SlotId& slotId, AZStd::string_view newName)
    {
        AZ::EntityId graphCanvasSlotId = MapToGraphCanvasId(slotId);

        if (graphCanvasSlotId.IsValid())
        {
            GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetName, newName);
        }
    }

    void SlotMappingComponent::OnSlotDisplayTypeChanged(const ScriptCanvas::SlotId& slotId, const ScriptCanvas::Data::Type& slotType)
    {
        AZ::EntityId graphCanvasSlotId = MapToGraphCanvasId(slotId);

        if (graphCanvasSlotId.IsValid())
        {
            GraphCanvas::DataValueType valueType = GraphCanvas::DataValueType::Primitive;

            AZ::Uuid typeId = ScriptCanvas::Data::ToAZType(slotType);

            if (ScriptCanvas::Data::IsContainerType(typeId))
            {
                valueType = GraphCanvas::DataValueType::Container;
            }
            else if (typeId.IsNull())
            {
                ScriptCanvas::Slot* slot = nullptr;
                ScriptCanvas::NodeRequestBus::EventResult(slot, m_sourceId, &ScriptCanvas::NodeRequests::GetSlot, slotId);

                if (slot && slot->IsDynamicSlot())
                {
                    auto dynamicSlotType = slot->GetDynamicDataType();

                    if (dynamicSlotType == ScriptCanvas::DynamicDataType::Container)
                    {
                        valueType = GraphCanvas::DataValueType::Container;
                    }
                }
            }

            GraphCanvas::DataSlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::DataSlotRequests::SetDataAndContainedTypeIds, typeId, ScriptCanvas::Data::GetContainedTypes(typeId), valueType);
        }
    }

    void SlotMappingComponent::ConfigureMapping(const AZ::EntityId& scriptCanvasMemberId)
    {
        if (m_sourceId.IsValid())
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusDisconnect();
            SlotMappingRequestBus::MultiHandler::BusDisconnect(m_sourceId);
        }

        m_sourceId = scriptCanvasMemberId;

        if (m_sourceId.IsValid())
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(m_sourceId);
            SlotMappingRequestBus::MultiHandler::BusConnect(m_sourceId);
        }
    }
}
