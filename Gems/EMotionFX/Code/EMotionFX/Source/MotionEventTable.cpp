/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionEventTable.h"
#include "MotionEvent.h"
#include "MotionEventTrack.h"
#include "EventManager.h"
#include "EventHandler.h"
#include <EMotionFX/Source/Allocators.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionEventTable, MotionEventAllocator)

    MotionEventTable::~MotionEventTable()
    {
        RemoveAllTracks();
    }

    void MotionEventTable::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEventTable>()
            ->Version(1)
            ->Field("tracks", &MotionEventTable::m_tracks)
            ;
    }

    void MotionEventTable::InitAfterLoading(Motion* motion)
    {
        for (MotionEventTrack* track : m_tracks)
        {
            track->SetMotion(motion);
        }

        AutoCreateSyncTrack(motion);
    }

    // process all events within a given time range
    void MotionEventTable::ProcessEvents(float startTime, float endTime, const MotionInstance* motionInstance) const
    {
        // process all event tracks
        for (MotionEventTrack* track : m_tracks)
        {
            // process the track's events
            if (track->GetIsEnabled())
            {
                track->ProcessEvents(startTime, endTime, motionInstance);
            }
        }
    }

    // extract all events from all tracks
    void MotionEventTable::ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const
    {
        // iterate over all tracks
        for (MotionEventTrack* track : m_tracks)
        {
            // process the track's events
            if (track->GetIsEnabled())
            {
                track->ExtractEvents(startTime, endTime, motionInstance, outEventBuffer);
            }
        }
    }

    // remove all event tracks
    void MotionEventTable::RemoveAllTracks(bool delFromMem)
    {
        // remove the tracks from memory if we want to
        if (delFromMem)
        {
            for (MotionEventTrack* track : m_tracks)
            {
                delete track;
            }
        }

        m_tracks.clear();
    }


    // remove a specific track
    void MotionEventTable::RemoveTrack(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete m_tracks[index];
        }

        m_tracks.erase(AZStd::next(m_tracks.begin(), index));
    }


    // add a new track
    void MotionEventTable::AddTrack(MotionEventTrack* track)
    {
        m_tracks.emplace_back(track);
    }


    // insert a track
    void MotionEventTable::InsertTrack(size_t index, MotionEventTrack* track)
    {
        m_tracks.insert(AZStd::next(m_tracks.begin(), index), track);
    }


    // reserve space for a given amount of tracks to prevent re-allocs
    void MotionEventTable::ReserveNumTracks(size_t numTracks)
    {
        m_tracks.reserve(numTracks);
    }


    // find a track by its name
    AZ::Outcome<size_t> MotionEventTable::FindTrackIndexByName(const char* trackName) const
    {
        const size_t numTracks = m_tracks.size();
        for (size_t i = 0; i < numTracks; ++i)
        {
            if (m_tracks[i]->GetNameString() == trackName)
            {
                return AZ::Success(i);
            }
        }

        return AZ::Failure();
    }


    // find a track name
    MotionEventTrack* MotionEventTable::FindTrackByName(const char* trackName) const
    {
        const AZ::Outcome<size_t> trackIndex = FindTrackIndexByName(trackName);
        if (!trackIndex.IsSuccess())
        {
            return nullptr;
        }

        return m_tracks[trackIndex.GetValue()];
    }


    // copy the table contents to another table
    void MotionEventTable::CopyTo(MotionEventTable* targetTable, Motion* targetTableMotion)
    {
        // remove all tracks in the target table
        targetTable->RemoveAllTracks();

        // copy over the tracks
        targetTable->ReserveNumTracks(m_tracks.size()); // pre-alloc space
        for (const MotionEventTrack* track : m_tracks)
        {
            // create the new track
            MotionEventTrack* newTrack = MotionEventTrack::Create(targetTableMotion);

            // copy its contents
            track->CopyTo(newTrack);

            // add the new track to the target
            targetTable->AddTrack(newTrack);
        }
    }


    // automatically create the sync track
    void MotionEventTable::AutoCreateSyncTrack(Motion* motion)
    {
        // check if the sync track is already there, if not create it
        MotionEventTrack* track = FindTrackByName("Sync");
        AnimGraphSyncTrack* syncTrack;
        if (!track)
        {
            syncTrack = aznew AnimGraphSyncTrack("Sync", motion);
            InsertTrack(0, syncTrack);
        }
        else
        {
            syncTrack = static_cast<AnimGraphSyncTrack*>(track);
            AZ_Assert(syncTrack, "Unable to cast MotionEventTrack named \"Sync\" to AnimGraphSyncTrack*!");
        }

        // make the sync track undeletable
        syncTrack->SetIsDeletable(false);

        // set the sync track shortcut
        m_syncTrack = syncTrack;
    }
} // namespace EMotionFX
