/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

#include "EBusSenderNodeDescriptorComponent.h"

#include "Editor/Translation/TranslationHelper.h"
#include "ScriptCanvas/Libraries/Core/Method.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // EBusSenderNodeDescriptorComponent
    //////////////////////////////////////
    
    void EBusSenderNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<EBusSenderNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2)
            ;
        }
    }
    
    EBusSenderNodeDescriptorComponent::EBusSenderNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusSender)
    {
    }
    
    void EBusSenderNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const GraphCanvas::GraphId& sceneId, const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasNodeId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::Method* method = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::Method>(entity);

            if (method)
            {
                if (method->HasBusID())
                {
                    ScriptCanvas::SlotId slotId = method->GetBusSlotId();

                    AZStd::vector< AZ::EntityId > graphCanvasSlots;
                    GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlots, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                    for (const AZ::EntityId& graphCanvasId : graphCanvasSlots)
                    {
                        AZStd::any* slotData = nullptr;
                        GraphCanvas::SlotRequestBus::EventResult(slotData, graphCanvasId, &GraphCanvas::SlotRequests::GetUserData);

                        if (slotData
                            && slotData->is< ScriptCanvas::SlotId >())
                        {
                            ScriptCanvas::SlotId currentSlotId = (*AZStd::any_cast<ScriptCanvas::SlotId>(slotData));

                            if (currentSlotId == slotId)
                            {
                                GraphCanvas::SlotRequestBus::Event(graphCanvasId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusSenderBusIdNameKey());
                                GraphCanvas::SlotRequestBus::Event(graphCanvasId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusSenderBusIdTooltipKey());

                                break;
                            }
                        }
                    }
                }
            }
        }        
    }
}
