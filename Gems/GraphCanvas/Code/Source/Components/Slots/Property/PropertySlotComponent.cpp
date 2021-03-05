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

#include <Components/Slots/Property/PropertySlotComponent.h>

#include <Components/Slots/Property/PropertySlotLayoutComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>

namespace GraphCanvas
{
    //////////////////////////
    // PropertySlotComponent
    //////////////////////////

    void PropertySlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<PropertySlotComponent, SlotComponent>()
                ->Version(1)
                ->Field("PropertyId", &PropertySlotComponent::m_propertyId)
            ;
        }
    }

    AZ::Entity* PropertySlotComponent::CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration)
    {
        AZ::Entity* entity = SlotComponent::CreateCoreSlotEntity();

        entity->CreateComponent<PropertySlotComponent>(propertyId, slotConfiguration);
        entity->CreateComponent<PropertySlotLayoutComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::PropertySlot, nodeId);

        SlotConnectionFilterComponent* connectionFilter = entity->CreateComponent<SlotConnectionFilterComponent>();

        // We don't want to accept any connections.
        connectionFilter->AddFilter(aznew SlotTypeFilter(ConnectionFilterType::Include));

        return entity;
    }
    
    PropertySlotComponent::PropertySlotComponent()
        : SlotComponent(SlotTypes::PropertySlot)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::PropertyGroup;
        }
    }
    
    PropertySlotComponent::PropertySlotComponent(const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration)
        : SlotComponent(SlotTypes::PropertySlot, slotConfiguration)
        , m_propertyId(propertyId)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::PropertyGroup;
        }
    }

    PropertySlotComponent::~PropertySlotComponent()
    {
    }
    
    void PropertySlotComponent::Init()
    {
        SlotComponent::Init();
    }
    
    void PropertySlotComponent::Activate()
    {
        SlotComponent::Activate();
        
        PropertySlotRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void PropertySlotComponent::Deactivate()
    {
        SlotComponent::Deactivate();
        
        PropertySlotRequestBus::Handler::BusDisconnect();
    }

    int PropertySlotComponent::GetLayoutPriority() const
    {
        // Going to want property slots to always be at the top of a display group.
        return std::numeric_limits<int>().max();
    }

    void PropertySlotComponent::SetLayoutPriority(int priority)
    {
        AZ_UNUSED(priority);
    }

    const AZ::Crc32& PropertySlotComponent::GetPropertyId() const
    {
        return m_propertyId;
    }

    AZ::Entity* PropertySlotComponent::ConstructConnectionEntity([[maybe_unused]] const Endpoint& sourceEndpoint, [[maybe_unused]] const Endpoint& targetEndpoint, [[maybe_unused]] bool createModelConnection)
    {
        AZ_Error("Graph Canvas", false, "Property slots cannot have connections.");
        return nullptr;
    }
}
