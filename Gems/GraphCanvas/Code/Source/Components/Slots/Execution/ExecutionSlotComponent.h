/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Components/Slots/SlotComponent.h>

namespace GraphCanvas
{
    class ExecutionSlotComponent
        : public SlotComponent
    {
    public:
        AZ_COMPONENT(ExecutionSlotComponent, "{36A31585-F202-4D83-9491-6178C8B94F03}", SlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreateExecutionSlot(const AZ::EntityId& nodeId, const SlotConfiguration& slotConfiguration);
        
        ExecutionSlotComponent();
        explicit ExecutionSlotComponent(const SlotConfiguration& slotConfiguration);

        ~ExecutionSlotComponent();

        // SlotRequestBus
        SlotConfiguration* CloneSlotConfiguration() const override;
        ////

    protected:
        ExecutionSlotComponent(const ExecutionSlotComponent&) = delete;
        ExecutionSlotComponent& operator=(const ExecutionSlotComponent&) = delete;
        AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection) override;
    };
}
