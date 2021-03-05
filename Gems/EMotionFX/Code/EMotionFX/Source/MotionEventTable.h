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
#include "BaseObject.h"
#include "AnimGraphSyncTrack.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/Outcome/Outcome.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class MotionInstance;
    class ActorInstance;
    class MotionEventTrack;
    class Motion;
    class AnimGraphEventBuffer;
    class AnimGraphSyncTrack;

    /**
     * The motion event table, which stores all events and their data on a memory efficient way.
     * Events have three generic properties: a time value, an event type string and a parameter string.
     * Unique strings are only stored once in memory, so if you have for example ten events of the type "SOUND"
     * only 1 string will be stored in memory, and the events will index into the table to retrieve the string.
     * The event table can also figure out what events to process within a given time range.
     * The handling of those events is done by the MotionEventHandler class that you specify to the MotionEventManager singleton.
     */
    class EMFX_API MotionEventTable
        : public BaseObject
    {
        friend class MotionEvent;

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(MotionEventTable, "{DB5BF142-99BE-4026-8D3E-3E5B30C14714}", BaseObject)

        MotionEventTable();

        ~MotionEventTable();

        static void Reflect(AZ::ReflectContext* context);

        void InitAfterLoading(Motion* motion);

        /**
         * Process all events within a given time range.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param motionInstance The motion instance which triggers the event.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ProcessEvents(float startTime, float endTime, const MotionInstance* motionInstance) const;

        /**
         * Extract all events within a given time range, and output them to an event buffer.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param motionInstance The motion instance which triggers the event.
         * @param outEventBuffer The output event buffer.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const;

        /**
         * Remove all motion event tracks.
         */
        void ReserveNumTracks(size_t numTracks);
        void RemoveAllTracks(bool delFromMem = true);
        void RemoveTrack(size_t index, bool delFromMem = true);
        void AddTrack(MotionEventTrack* track);
        void InsertTrack(size_t index, MotionEventTrack* track);

        AZ::Outcome<size_t> FindTrackIndexByName(const char* trackName) const;
        MotionEventTrack* FindTrackByName(const char* trackName) const;

        size_t GetNumTracks() const                    { return m_tracks.size(); }
        MotionEventTrack* GetTrack(size_t index) const { return m_tracks[index]; }
        AnimGraphSyncTrack* GetSyncTrack() const       { return m_syncTrack; }

        void AutoCreateSyncTrack(Motion* motion);

        void CopyTo(MotionEventTable* targetTable, Motion* targetTableMotion);

    private:
        /// The motion event tracks.
        AZStd::vector<MotionEventTrack*> m_tracks;

        /// A shortcut to the track containing sync events.
        AnimGraphSyncTrack* m_syncTrack;

    };
} // namespace EMotionFX
