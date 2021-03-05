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
