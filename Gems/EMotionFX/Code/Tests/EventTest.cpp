/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFX/Source/MotionEvent.h"
#include "SystemComponentFixture.h"
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/TwoStringEventData.h>

namespace EMotionFX
{

    TEST_F(SystemComponentFixture, EventMoveTestEventDataChangeEvent)
    {
        AZ::u32 eventCount = 0;
        Event::EventDataChangeEvent::Handler handler = Event::EventDataChangeEvent::Handler(
            [&eventCount]()
            {
                eventCount++;
            });

        MotionEvent event;
        event.RegisterEventDataChangeEvent(handler);
        event.AppendEventData(EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
        EXPECT_EQ(event.GetEventDatas().size(), 1);

        MotionEvent event2;
        event2 = AZStd::move(event);
        event2.AppendEventData(EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
        EXPECT_EQ(event2.GetEventDatas().size(), 2);

        MotionEvent event3(AZStd::move(event2));
        event3.AppendEventData(EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
        EXPECT_EQ(event3.GetEventDatas().size(), 3);

        MotionEvent event4(event3);
        event4.AppendEventData(EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
        EXPECT_EQ(event4.GetEventDatas().size(), 4);

        MotionEvent event5 = event3;
        event5.AppendEventData(EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
        EXPECT_EQ(event5.GetEventDatas().size(), 4);

        EXPECT_EQ(eventCount, 3);
    }
} // namespace EMotionFX
