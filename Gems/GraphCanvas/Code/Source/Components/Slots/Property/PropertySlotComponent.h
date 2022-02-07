/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Components/Slots/SlotComponent.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotBus.h>

namespace GraphCanvas
{
    class PropertySlotComponent
        : public SlotComponent
        , public PropertySlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PropertySlotComponent, "{72D2C614-0E1C-4048-9382-4BBA4B25C66F}", SlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration);
        
        PropertySlotComponent();
        PropertySlotComponent(const AZ::Crc32& propertyId, const SlotConfiguration& slotConfiguration);
        ~PropertySlotComponent();
        
        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // Slot RequestBus
        int GetLayoutPriority() const override;
        void SetLayoutPriority(int priority) override;
        ////

        // PropertySlotBus
        const AZ::Crc32& GetPropertyId() const override;
        ////

    private:
        PropertySlotComponent(const PropertySlotComponent&) = delete;
        PropertySlotComponent& operator=(const PropertySlotComponent&) = delete;
        AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection) override;
        
        AZ::Crc32 m_propertyId;
    };
}
