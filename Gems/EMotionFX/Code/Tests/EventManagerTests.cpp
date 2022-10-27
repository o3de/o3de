/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemComponentFixture.h"
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <AzCore/Serialization/Utils.h>

namespace EMotionFX
{

TEST_F(SystemComponentFixture, EventDataFactoryMakesUniqueData)
{
    MotionEventTrack* track = aznew MotionEventTrack();
    track->SetName("My name");
    {
        const auto data = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter");
        track->AddEvent(0.5, data);
    }
    EXPECT_EQ(track->GetEvent(0).GetEventDatas()[0].use_count(), 1);

    AZStd::vector<char> charBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char> > charStream(&charBuffer);
    AZ::Utils::SaveObjectToStream(charStream, AZ::ObjectStream::ST_XML, track, GetSerializeContext());

    MotionEventTrack* loadedTrack = AZ::Utils::LoadObjectFromBuffer<MotionEventTrack>(&charBuffer[0], charBuffer.size(), GetSerializeContext());
    EXPECT_EQ(loadedTrack->GetEvent(0).GetEventDatas()[0], track->GetEvent(0).GetEventDatas()[0]);
    EXPECT_EQ(loadedTrack->GetEvent(0).GetEventDatas()[0].use_count(), 2);

    delete track;
    delete loadedTrack;
}


TEST_F(SystemComponentFixture, EventDataMotionEventChangeEvent)
{
    AZ::u32 eventCount = 0;
    Event::EventDataChangeEvent::Handler handler = Event::EventDataChangeEvent::Handler(
        [&eventCount]()
        {
            eventCount++;
        });
    MotionEventTrack* track = aznew MotionEventTrack();
    track->SetName("My name");
    track->AddEvent(0.5, EventDataSet({}));
    track->GetEvent(0).RegisterEventDataChangeEvent(handler);

    track->GetEvent(0).AppendEventData(
        EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));

    // event handler shouldn't be carried over to new track
    MotionEventTrack* track2 = aznew MotionEventTrack();
    track->CopyTo(track2);
    track2->GetEvent(0).RemoveEventData(0);

    track->GetEvent(0).AppendEventData(
        EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
    track->GetEvent(0).InsertEventData(
        0, EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("My subject", "My parameter"));
    track->GetEvent(0).RemoveEventData(0);

    EXPECT_EQ(track->GetEvent(0).GetEventDatas().size(), 2);
    EXPECT_EQ(track2->GetEvent(0).GetEventDatas().size(), 0);
    EXPECT_EQ(eventCount, 4);

    delete track;
    delete track2;
}

} // end namespace EMotionFX
