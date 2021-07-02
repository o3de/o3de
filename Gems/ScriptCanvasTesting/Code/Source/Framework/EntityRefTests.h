/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>

namespace ScriptCanvasTests
{
    class EntityRefTestEvents : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void TestEvent(AZ::EntityId, bool) = 0;
    };
    using EntityRefTestEventBus = AZ::EBus<EntityRefTestEvents>;


    class TestComponent : public AZ::Component
        , EntityRefTestEventBus::Handler
    {
    public:
        AZ_COMPONENT(TestComponent, "{527680AE-BF46-4BC8-A923-A39B458A3B53}", AZ::Component);

        void Init() override {}
        void Activate() override
        {
            EntityRefTestEventBus::Handler::BusConnect(GetEntityId());
        }
        void Deactivate() override
        {
            EntityRefTestEventBus::Handler::BusDisconnect();
        }

        void TestEvent(AZ::EntityId referencedEntity, bool shouldBeSelf) override
        {
            // validate entity
            if (shouldBeSelf)
            {
                EXPECT_TRUE(GetEntity()->GetId() == referencedEntity);
            }

            AZ_TracePrintf("Script Canvas", "TestEvent handled by: %s\n", GetEntity()->GetName().c_str());
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
            if (behaviorContext)
            {
                behaviorContext->EBus<EntityRefTestEventBus>("EntityRefTestEventBus")
                    ->Event("TestEvent", &EntityRefTestEvents::TestEvent)
                    ;
            }
        }
    };
}
