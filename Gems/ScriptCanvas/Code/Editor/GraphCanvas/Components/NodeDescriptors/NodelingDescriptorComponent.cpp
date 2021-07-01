/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include "NodelingDescriptorComponent.h"
#include "ScriptCanvas/Bus/EditorScriptCanvasBus.h"
#include "ScriptCanvas/GraphCanvas/MappingBus.h"
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Libraries/Core/Nodeling.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // NodelingDescriptorComponent
    ////////////////////////////////

    void NodelingDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<NodelingDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }
    
    NodelingDescriptorComponent::NodelingDescriptorComponent(NodeDescriptorType descriptorType)
        : NodeDescriptorComponent(descriptorType)
    {
    }

    void NodelingDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        ScriptCanvas::GraphNotificationBus::Handler::BusDisconnect();
    }

    void NodelingDescriptorComponent::OnNameChanged(const AZStd::string& displayName)
    {
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, displayName);
    }
    
    void NodelingDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);
        
        ScriptCanvas::GraphScopedNodeId scopedNodeId(scriptCanvasId, scriptCanvasNodeId);

        ScriptCanvas::NodelingNotificationBus::Handler::BusConnect(scopedNodeId);
        
        AZStd::string displayName;
        ScriptCanvas::NodelingRequestBus::EventResult(displayName, scopedNodeId, &ScriptCanvas::NodelingRequests::GetDisplayName);

        OnNameChanged(displayName);

        ScriptCanvas::GraphNotificationBus::Handler::BusConnect(scriptCanvasId);

    }

    ScriptCanvas::Slot* NodelingDescriptorComponent::GetSlotFromNodeling(const AZ::EntityId& connectionId)
    {
        GraphCanvas::Endpoint sourceEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

        GraphCanvas::Endpoint targetEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        AZ::EntityId scriptCanvasNodeId = FindScriptCanvasNodeId();
        ScriptCanvas::NodeRequestBus::EventResult(scriptCanvasId, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetOwningScriptCanvasId);

        AZStd::vector<GraphCanvas::Endpoint> endpoints = { sourceEndpoint, targetEndpoint };

        for (auto& endpoint : endpoints)
        {
            ScriptCanvas::Endpoint scEndpoint;
            EditorGraphRequestBus::EventResult(scEndpoint, scriptCanvasId, &EditorGraphRequests::ConvertToScriptCanvasEndpoint, endpoint);

            ScriptCanvas::Slot* slot = nullptr;
            ScriptCanvas::NodeRequestBus::EventResult(slot, scEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, scEndpoint.GetSlotId());

            if (slot)
            {
                if (azrtti_istypeof<ScriptCanvas::Nodes::Core::Internal::Nodeling>(slot->GetNode()))
                {
                    return slot;
                }
            }
        }

        return nullptr;
    }

    void NodelingDescriptorComponent::OnConnectionComplete(const AZ::EntityId& connectionId)
    {

        AZStd::vector< AZ::EntityId > graphCanvasSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);


        ScriptCanvas::Slot* slot = GetSlotFromNodeling(connectionId);
        if (slot)
        {
            const auto& descriptor = slot->GetDescriptor();
            if (descriptor.IsExecution())
            {
                AZStd::vector<ScriptCanvas::Slot*> scriptSlots;
                ScriptCanvas::NodeRequestBus::EventResult(scriptSlots, slot->GetNodeId(), &ScriptCanvas::NodeRequests::ModAllSlots);

                if (descriptor.IsInput())
                {
                    // Hide the output slot
                    for (ScriptCanvas::Slot* s : scriptSlots)
                    {
                        if (s->IsExecution() && s->IsOutput())
                        {
                            // Need the GC slot Id

                            AZ::EntityId graphCanvasGraphId;
                            ScriptCanvasEditor::SlotMappingRequestBus::EventResult(graphCanvasGraphId, s->GetNodeId(), &ScriptCanvasEditor::SlotMappingRequests::MapToGraphCanvasId, s->GetId());
                            if (graphCanvasGraphId.IsValid())
                            {
                                m_removedSlotId = graphCanvasGraphId;
                                //m_removedSlot = *s; // copy the slot?
                                GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, graphCanvasGraphId);
                            }

                            s->SetVisible(false);
                        }
                    }
                }
                else
                {
                    // Hide the input slot
                    for (ScriptCanvas::Slot* s : scriptSlots)
                    {
                        if (s->IsExecution() && s->IsInput())
                        {
                            AZ::EntityId graphCanvasGraphId;
                            ScriptCanvasEditor::SlotMappingRequestBus::EventResult(graphCanvasGraphId, s->GetNodeId(), &ScriptCanvasEditor::SlotMappingRequests::MapToGraphCanvasId, s->GetId());
                            if (graphCanvasGraphId.IsValid())
                            {
                                GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, graphCanvasGraphId);
                            }

                            s->SetVisible(false);
                        }
    }
                }
            }
        }
    }

    void NodelingDescriptorComponent::OnDisonnectionComplete(const AZ::EntityId& connectionId)
    {

        ScriptCanvas::Slot* slot = GetSlotFromNodeling(connectionId);
        if (slot)
        {
            const auto& descriptor = slot->GetDescriptor();
            if (descriptor.IsExecution())
            {
                AZStd::vector<ScriptCanvas::Slot*> scriptSlots;
                ScriptCanvas::NodeRequestBus::EventResult(scriptSlots, slot->GetNodeId(), &ScriptCanvas::NodeRequests::ModAllSlots);

                if (descriptor.IsInput())
                {
                    // Hide the output slot
                    for (ScriptCanvas::Slot* s : scriptSlots)
                    {
                        if (s->IsExecution() && s->IsOutput())
                        {

                            AZ::EntityId graphCanvasGraphId;
                            ScriptCanvasEditor::SlotMappingRequestBus::EventResult(graphCanvasGraphId, s->GetNodeId(), &ScriptCanvasEditor::SlotMappingRequests::MapToGraphCanvasId, s->GetId());
                            if (graphCanvasGraphId.IsValid())
                            {
                                GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::AddSlot, m_removedSlotId);
                            }

                            s->SetVisible(true);
                        }
                    }
                }
                else
                {
                    // Hide the input slot
                    for (ScriptCanvas::Slot* s : scriptSlots)
                    {
                        if (s->IsExecution() && s->IsInput())
                        {

                            AZ::EntityId graphCanvasGraphId;
                            ScriptCanvasEditor::SlotMappingRequestBus::EventResult(graphCanvasGraphId, s->GetNodeId(), &ScriptCanvasEditor::SlotMappingRequests::MapToGraphCanvasId, s->GetId());
                            if (graphCanvasGraphId.IsValid())
                            {
                                GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::AddSlot, graphCanvasGraphId);
                            }

                            s->SetVisible(true);
                        }
                    }
                }
            }
        }
    }

    void NodelingDescriptorComponent::OnPreConnectionRemoved(const AZ::EntityId& connectionId)
    {
        ScriptCanvas::Slot* slot = GetSlotFromNodeling(connectionId);
        if (slot)
        {
            const auto& descriptor = slot->GetDescriptor();
            if (descriptor.IsExecution())
            {
                AZStd::vector<ScriptCanvas::Slot*> scriptSlots;
                ScriptCanvas::NodeRequestBus::EventResult(scriptSlots, slot->GetNodeId(), &ScriptCanvas::NodeRequests::ModAllSlots);

                // Show all the slots
                for (ScriptCanvas::Slot* s : scriptSlots)
                {
                    s->SetVisible(true);
                }
            }
        }
    }


}
