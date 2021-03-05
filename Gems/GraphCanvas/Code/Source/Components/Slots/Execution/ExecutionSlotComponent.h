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