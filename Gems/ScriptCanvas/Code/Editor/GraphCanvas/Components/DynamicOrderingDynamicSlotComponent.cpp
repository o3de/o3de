/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/Slot.h>

#include <Editor/GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

namespace ScriptCanvasEditor
{    
    ////////////////////////////////////////
    // DynamicOrderingDynamicSlotComponent
    ////////////////////////////////////////

    void DynamicOrderingDynamicSlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<DynamicOrderingDynamicSlotComponent, DynamicSlotComponent>()
                ->Version(0)
                ;
        }
    }

    DynamicOrderingDynamicSlotComponent::DynamicOrderingDynamicSlotComponent()
        : DynamicSlotComponent()
        , m_deserialized(false)
    {
    }

    DynamicOrderingDynamicSlotComponent::DynamicOrderingDynamicSlotComponent(GraphCanvas::SlotGroup slotGroup)
        : DynamicSlotComponent(slotGroup)
        , m_deserialized(false)
    {
    }

    void DynamicOrderingDynamicSlotComponent::OnSystemTick()
    {
        if (GetScriptCanvasNodeId().IsValid())
        {
            SlotMappingRequests* requestInterface = SlotMappingRequestBus::FindFirstHandler(GetEntityId());

            if (requestInterface)
            {
                AZStd::vector<ScriptCanvas::Slot*> scriptSlots;
                ScriptCanvas::NodeRequestBus::EventResult(scriptSlots, GetScriptCanvasNodeId(), &ScriptCanvas::NodeRequests::ModAllSlots);

                for (int i = 0; i < scriptSlots.size(); ++i)
                {
                    // Higher means it goes first. So we need to inverse our number.
                    int priority = static_cast<int>(scriptSlots.size()) - i;

                    ScriptCanvas::SlotId scriptCanvasSlotId = scriptSlots[i]->GetId();
                    GraphCanvas::SlotId graphCanvasSlotId = requestInterface->MapToGraphCanvasId(scriptCanvasSlotId);

                    if (graphCanvasSlotId.IsValid())
                    {
                        GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetLayoutPriority, priority);
                    }
                }
            }
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void DynamicOrderingDynamicSlotComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        DynamicSlotComponent::OnSceneSet(sceneId);

        if (m_deserialized)
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void DynamicOrderingDynamicSlotComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& serializationTarget)
    {
        DynamicSlotComponent::OnSceneMemberDeserialized(graphId, serializationTarget);
        m_deserialized = true;
    }

    void DynamicOrderingDynamicSlotComponent::OnSlotsReordered()
    {
        DynamicSlotComponent::OnSlotsReordered();

        AZ::SystemTickBus::Handler::BusConnect();
    }

    void DynamicOrderingDynamicSlotComponent::StopQueueSlotUpdates()
    {
        DynamicSlotComponent::StopQueueSlotUpdates();

        if (GetScriptCanvasNodeId().IsValid())
        {
            OnSystemTick();
        }
    }

    void DynamicOrderingDynamicSlotComponent::ConfigureGraphCanvasSlot([[maybe_unused]] const ScriptCanvas::Slot* slot, [[maybe_unused]] const GraphCanvas::SlotId& graphCanvasSlotId)
    {
        AZ::SystemTickBus::Handler::BusConnect();
    }
}
