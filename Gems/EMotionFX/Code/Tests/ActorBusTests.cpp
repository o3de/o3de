/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorBus.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    class ActorNotificationTestBus
        : public ActorNotificationBus::Handler
    {
    public:
        ActorNotificationTestBus()
        {
            ActorNotificationBus::Handler::BusConnect();
        }

        ~ActorNotificationTestBus()
        {
            ActorNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnActorCreated, void(Actor*));
        MOCK_METHOD1(OnActorDestroyed, void(Actor*));
    };

    TEST_F(SystemComponentFixture, ActorNotificationBusTest)
    {
        ActorNotificationTestBus testBus;

        EXPECT_CALL(testBus, OnActorCreated(testing::_));
        Actor* actor = aznew Actor("TestActor");

        EXPECT_CALL(testBus, OnActorDestroyed(actor));
        delete actor;
    }
} // end namespace EMotionFX
