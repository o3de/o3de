/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/any.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <Editor/GraphCanvas/Components/DynamicSlotComponent.h>
#include <Editor/Nodes/NodeDisplayUtils.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

namespace ScriptCanvasEditor
{
    /////////////////////////
    // DynamicSlotComponent
    /////////////////////////

    void DynamicSlotComponent::Reflect(AZ::ReflectContext* reflectionContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectionContext);

        if (serializeContext)
        {
            serializeContext->Class<DynamicSlotComponent, AZ::Component>()
                ->Version(1)
                ->Field("SlotGroup", &DynamicSlotComponent::m_slotGroup)
            ;
        }
    }

    DynamicSlotComponent::DynamicSlotComponent()
        : m_slotGroup(GraphCanvas::SlotGroups::Invalid)
        , m_queueUpdates(false)
    {
    }

    DynamicSlotComponent::DynamicSlotComponent(GraphCanvas::SlotGroup slotGroup)
        : m_slotGroup(slotGroup)
        , m_queueUpdates(false)
    {
    }

    void DynamicSlotComponent::Init()
    {
    }

    void DynamicSlotComponent::Activate()
    {
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        DynamicSlotRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DynamicSlotComponent::Deactivate()
    {
        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void DynamicSlotComponent::OnSceneSet(const AZ::EntityId&)
    {
        OnUserDataChanged();
        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void DynamicSlotComponent::OnSlotAdded(const ScriptCanvas::SlotId& slotId)
    {
        // For EBuses we need to be a little tricky. Since we are going to be pointing at a single node from
        // multiple graph canvas nodes. So we want to convert the script canvas id into a graph canvas id
        //
        // Then check to see if we are the correct node for that particular event.
        const AZ::EntityId* scriptCanvasNodeId = ScriptCanvas::NodeNotificationsBus::GetCurrentBusId();

        if (scriptCanvasNodeId == nullptr)
        {
            return;
        }

        ScriptCanvas::Endpoint endpoint((*scriptCanvasNodeId), slotId);

        if (m_queueUpdates)
        {
            m_queuedEndpoints.insert(endpoint);
            return;
        }

        HandleSlotAdded(endpoint);
    }

    void DynamicSlotComponent::OnSlotRemoved(const ScriptCanvas::SlotId& slotId)
    {
        if (m_queueUpdates)
        {
            const AZ::EntityId* scriptCanvasNodeId = ScriptCanvas::NodeNotificationsBus::GetCurrentBusId();

            if (scriptCanvasNodeId)
            {
                ScriptCanvas::Endpoint endpoint((*scriptCanvasNodeId), slotId);
                m_queuedEndpoints.erase(endpoint);
            }
        }

        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        for (AZ::EntityId entityId : slotIds)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::SlotRequestBus::EventResult(userData, entityId, &GraphCanvas::SlotRequests::GetUserData);

            if (userData)
            {
                ScriptCanvas::SlotId* testId = AZStd::any_cast<ScriptCanvas::SlotId>(userData);

                if (testId && (*testId) == slotId)
                {
                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, entityId);
                }
            }
        }
    }

    void DynamicSlotComponent::OnUserDataChanged()
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData)
        {
            AZ::EntityId* entityId = AZStd::any_cast<AZ::EntityId>(userData);

            if (entityId)
            {
                m_scriptCanvasNodeId = (*entityId);
                ScriptCanvas::NodeNotificationsBus::Handler::BusDisconnect();
                ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(m_scriptCanvasNodeId);
            }
        }
    }

    void DynamicSlotComponent::StartQueueSlotUpdates()
    {
        if (!m_queueUpdates)
        {
            m_queueUpdates = true;

            m_queuedEndpoints.clear();
        }
    }

    void DynamicSlotComponent::StopQueueSlotUpdates()
    {
        if (m_queueUpdates)
        {
            m_queueUpdates = false;

            for (auto endpoint : m_queuedEndpoints)
            {
                HandleSlotAdded(endpoint);
            }

            m_queuedEndpoints.clear();
        }
    }

    void DynamicSlotComponent::ConfigureGraphCanvasSlot(const ScriptCanvas::Slot* slot, const GraphCanvas::SlotId& graphCanvasSlotId)
    {
        // By default no further configuration is necessary
        AZ_UNUSED(slot);
        AZ_UNUSED(graphCanvasSlotId);
    }

    void DynamicSlotComponent::HandleSlotAdded(const ScriptCanvas::Endpoint& endpoint)
    {
        AZ::EntityId graphCanvasNodeId;
        SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, endpoint.GetNodeId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        bool isEBusNode = false;
        NodeDescriptorRequestBus::EventResult(isEBusNode, graphCanvasNodeId, &NodeDescriptorRequests::IsType, NodeDescriptorType::EBusHandler);

        if (isEBusNode)
        {
            AZ::EntityId targetEventReceiverNode;
            EBusHandlerNodeDescriptorRequestBus::EventResult(targetEventReceiverNode, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::FindGraphCanvasNodeIdForSlot, endpoint.GetSlotId());

            if (targetEventReceiverNode != GetEntityId())
            {
                return;
            }
        }

        const ScriptCanvas::Slot* slot = nullptr;
        ScriptCanvas::NodeRequestBus::EventResult(slot, endpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, endpoint.GetSlotId());

        if (slot && slot->IsVisible())
        {
            AZ::EntityId graphCanvasSlotId = Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*slot), m_slotGroup);

            ConfigureGraphCanvasSlot(slot, graphCanvasSlotId);
        }
    }
}
