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

// include the required headers
#include "EMotionFXConfig.h"
#include "MotionEventTrack.h"
#include <AzCore/std/containers/vector.h>


namespace AZ
{
    class ReflectContext;
}


namespace EMotionFX
{
    /**
     *
     *
     */
    class EMFX_API AnimGraphSyncTrack
        : public MotionEventTrack
    {
    public:
        AZ_RTTI(AnimGraphSyncTrack, "{5C49D549-4A2D-42A9-BB16-564BEA63C4B1}", MotionEventTrack);
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphSyncTrack();
        AnimGraphSyncTrack(Motion* motion);
        AnimGraphSyncTrack(const char* name, Motion* motion);
        AnimGraphSyncTrack(const AnimGraphSyncTrack& other);
        ~AnimGraphSyncTrack() override;

        AnimGraphSyncTrack& operator=(const AnimGraphSyncTrack& other);

        static void Reflect(AZ::ReflectContext* context);

        /**
        * @brief: Finds the event to the left and right of @p timeInSeconds
        *
        * @param[in]  timeInSeconds
        * @param[out] outIndexA The event whose start time is closest to, but less than, @p timeInSeconds
        * @param[out] outIndexB The event whose start time is closest to, but greater than, @p timeInSeconds
        *
        * @return: bool true iff an event pair was found
        */
        bool FindEventIndices(float timeInSeconds, size_t* outIndexA, size_t* outIndexB) const;

        /**
        * @brief: Finds the indices of the next event pair
        *
        * @param[in]  syncIndex Start the search from this event index
        * @param[in]  firstEventID The hash of the first event in the pair to search for
        * @param[in]  secondEventID The hash of the second event in the pair to search for
        * @param[out] outIndexA The index of the found event matching @p firstEventID
        * @param[out] outIndexB The index of the found event matching @p secondEventID
        * @param[in]  forward Direction of the search. True for forward playback, false otherwise
        * @param[in]  mirror True if the the motion using this event track is mirrored
        *
        * @return: bool true iff a valid matching pair of events was found.
        *
        * Search for matching event pairs. @p firstEventID and @p secondEventID
        * represent the hash of a sequential event pair in a different event
        * track. This method attempts to find a pair of sequential events that
        * have the same hash value.
        */
        bool FindMatchingEvents(size_t syncIndex, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool forward, bool mirror) const;
        float CalcSegmentLength(size_t indexA, size_t indexB) const;
        bool ExtractOccurrence(size_t occurrence, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool mirror) const;
        size_t CalcOccurrence(size_t indexA, size_t indexB, bool mirror) const;

        float GetDuration() const;

    private:
        using EventIterator = decltype(m_events)::const_iterator;
        bool FindMatchingEvents(EventIterator start, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool forward, bool mirror) const;

        bool AreEventsSyncable(const MotionEvent& eventA, const MotionEvent& eventB, bool isMirror) const;

    };
}   // namespace EMotionFX
