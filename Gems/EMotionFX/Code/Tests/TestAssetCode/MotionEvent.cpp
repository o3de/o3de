/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tests/TestAssetCode/MotionEvent.h"
#include <EMotionFX/Source/TwoStringEventData.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace EMotionFX
{
    // These functions make various configurations of events
    void MakeNoEvents(MotionEventTrack* /*track*/)
    {
    }

    void MakeOneRangedEvent(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("subject", "params");
        track->AddEvent(0.25f, 0.75f, data);
    }

    void MakeOneEvent(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("subject", "params");
        track->AddEvent(0.25f, data);
    }

    void MakeTwoEvents(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("subject", "params");
        track->AddEvent(0.25f, data);
        track->AddEvent(0.75f, data);
    }

    void MakeThreeEvents(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("subject", "params");
        track->AddEvent(0.25f, data);
        track->AddEvent(0.75f, data);
        track->AddEvent(1.25f, data);
    }

    void MakeThreeRangedEvents(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("subject", "params");
        track->AddEvent(0.25f, 0.5f, data);
        track->AddEvent(0.75f, 1.0f, data);
        track->AddEvent(1.25f, 1.5f, data);
    }

    void MakeTwoLeftRightEvents(MotionEventTrack* track)
    {
        AZStd::shared_ptr<const TwoStringEventData> leftEventData = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("Left", "params", "Right");
        AZStd::shared_ptr<const TwoStringEventData> rightEventData = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("Right", "params", "Left");
        track->AddEvent(0.0f, leftEventData);
        track->AddEvent(1.0f, rightEventData);
        track->AddEvent(2.0f, leftEventData);
        track->AddEvent(3.0f, rightEventData);
    }
} // end namespace EMotionFX
