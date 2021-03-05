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
        void Init();
        void Activate();
        void Deactivate();
        ////

        // Slot RequestBus
        int GetLayoutPriority() const override;
        void SetLayoutPriority(int priority) override;
        ////

        // PropertySlotBus
        const AZ::Crc32& GetPropertyId() const;
        ////

    private:
        PropertySlotComponent(const PropertySlotComponent&) = delete;
        PropertySlotComponent& operator=(const PropertySlotComponent&) = delete;
        AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection) override;
        
        AZ::Crc32 m_propertyId;
    };
}
