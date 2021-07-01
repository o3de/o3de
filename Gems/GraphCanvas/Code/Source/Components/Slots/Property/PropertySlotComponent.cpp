/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    AZ::Entity* PropertySlotComponent::ConstructConnectionEntity(const Endpoint&, const Endpoint&, bool)
    {
        AZ_Error("Graph Canvas", false, "Property slots cannot have connections.");
        return nullptr;
    }
}
