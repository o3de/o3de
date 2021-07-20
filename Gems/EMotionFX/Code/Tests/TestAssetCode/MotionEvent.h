/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/MotionEventTrack.h>

namespace EMotionFX
{
    // These functions make various configurations of events
    void MakeNoEvents(MotionEventTrack* /*track*/);

    void MakeOneRangedEvent(MotionEventTrack* track);

    void MakeOneEvent(MotionEventTrack* track);

    void MakeTwoEvents(MotionEventTrack* track);

    void MakeThreeEvents(MotionEventTrack* track);

    void MakeThreeRangedEvents(MotionEventTrack* track);

    void MakeTwoLeftRightEvents(MotionEventTrack* track);

} // end namespace EMotionFX
