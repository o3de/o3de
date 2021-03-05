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

#include "SystemComponentFixture.h"
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <AzCore/Serialization/Utils.h>

namespace EMotionFX
{

TEST_F(SystemComponentFixture, DISABLED_EventDataFactoryMakesUniqueData)
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

    track->Destroy();
    loadedTrack->Destroy();
}

} // end namespace EMotionFX
