/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/iterator.h>
#include <algorithm>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSyncTrack, MotionEventAllocator);

    /**
    * @brief: Advances an iterator +1 or -1, wrapping to the beginning when the
    *         end is reached, and wrapping to the beginning when the end is
    *         reached
    *
    * @param[in] in A bidirectional iterator
    * @param[in] direction True to increment the iterator, false to decrement
    * @param[in] Iterator& begin The begin() of the container
    * @param[in] Iterator& end The end() of the container
    */
    template<class Iterator>
    static Iterator AdvanceAndWrapIterator(Iterator in, bool direction, const Iterator& begin, const Iterator& end)
    {
        if (in == begin && !direction)
        {
            in = AZStd::next(end, -1);
        }
        else
        {
            direction ? ++in : --in;
            if (in == end)
            {
                in = begin;
            }
        }
        return in;
    }

    AnimGraphSyncTrack::AnimGraphSyncTrack() = default;

    AnimGraphSyncTrack::AnimGraphSyncTrack(Motion* motion)
        : MotionEventTrack(motion)
    {
    }

    AnimGraphSyncTrack::AnimGraphSyncTrack(const char* name, Motion* motion)
        : MotionEventTrack(name, motion)
    {
    }

    AnimGraphSyncTrack::~AnimGraphSyncTrack()
    {
    }

    AnimGraphSyncTrack::AnimGraphSyncTrack(const AnimGraphSyncTrack& other)
        : MotionEventTrack(other)
    {
    }

    AnimGraphSyncTrack& AnimGraphSyncTrack::operator=(const AnimGraphSyncTrack& other)
    {
        if (this == &other)
        {
            return *this;
        }
        MotionEventTrack::operator=(other);
        return *this;
    }

    void AnimGraphSyncTrack::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphSyncTrack, MotionEventTrack>()
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphSyncTrack>("AnimGraphSyncTrack", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }


    // find the event indices
    bool AnimGraphSyncTrack::FindEventIndices(float timeInSeconds, size_t* outIndexA, size_t* outIndexB) const
    {
        // if we have no events at all, or if we provide some incorrect time value
        const size_t numEvents = m_events.size();
        if (numEvents == 0 || timeInSeconds > GetDuration() || timeInSeconds < 0.0f)
        {
            *outIndexA = InvalidIndex;
            *outIndexB = InvalidIndex;
            return false;
        }

        // we have only one event, return the same one twice
        // "..t..[x]..."
        if (numEvents == 1)
        {
            *outIndexA = 0;
            *outIndexB = 0;
            return true;
        }

        // Use a binary search to find the event on the right
        // If there are multiple events with the same start time, only the last
        // one is considered
        EventIterator right = AZStd::upper_bound(m_events.cbegin(), m_events.cend(), timeInSeconds, [](const float timeInSeconds, const EMotionFX::MotionEvent& event)
        {
            return timeInSeconds < event.GetStartTime();
        });
        EventIterator left = AdvanceAndWrapIterator(right, false, m_events.cbegin(), m_events.cend());
        *outIndexA = AZStd::distance(m_events.cbegin(), left);
        *outIndexB = right == m_events.cend() ? 0 : AZStd::distance(m_events.cbegin(), right);
        return true;
    }


    // calculate the occurrence of a given index combination
    // the occurrence is basically the n-th time the combination shows up
    size_t AnimGraphSyncTrack::CalcOccurrence(size_t indexA, size_t indexB, bool mirror) const
    {
        // if we wrapped around when looping or this is the only event (then both indexA and indexB are equal)
        if (indexA > indexB || indexA == indexB)
        {
            return 0;
        }

        const MotionEvent& eventA = m_events[indexA];
        const MotionEvent& eventB = m_events[indexB];

        // check the looping section, which is occurence 0
        size_t occurrence = 0;
        if (AreEventsSyncable(m_events.back(), eventA, mirror) && AreEventsSyncable(m_events[0], eventB, mirror))
        {
            if ((m_events.size() - 1) == indexA && indexB == 0)
            {
                return occurrence;
            }
            else
            {
                occurrence++;
            }
        }

        // iterate over all events
        const size_t numEvents = m_events.size();
        for (size_t i = 0; i < numEvents - 1; ++i)
        {
            // if we have an event type ID match
            if (AreEventsSyncable(m_events[i], eventA, mirror) && AreEventsSyncable(m_events[i + 1], eventB, mirror))
            {
                // if this is the event we search for, return the current occurrence
                if (i == indexA && (i + 1) == indexB)
                {
                    return occurrence;
                }
                else
                {
                    occurrence++; // it was the same combination of events, but not this one
                }
            }
        }

        // actually we didn't find this combination
        return InvalidIndex;
    }


    // extract the n-th occurring given event combination
    bool AnimGraphSyncTrack::ExtractOccurrence(size_t occurrence, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool mirror) const
    {
        // if there are no events
        const size_t numEvents = m_events.size();
        if (numEvents == 0)
        {
            *outIndexA = InvalidIndex;
            *outIndexB = InvalidIndex;
            return false;
        }

        // if we have just one event
        if (numEvents == 1)
        {
            // if the given event is a match
            if (firstEventID == m_events[0].HashForSyncing(mirror) && secondEventID == m_events[0].HashForSyncing(mirror))
            {
                *outIndexA = 0;
                *outIndexB = 0;
                return true;
            }
            else
            {
                *outIndexA = InvalidIndex;
                *outIndexB = InvalidIndex;
                return false;
            }
        }

        // we have multiple events
        size_t currentOccurrence = 0;
        bool found = false;

        // continue while we're not done yet
        while (currentOccurrence <= occurrence)
        {
            // first check the looping special case
            if (m_events.back().HashForSyncing(mirror) == firstEventID && m_events[0].HashForSyncing(mirror) == secondEventID)
            {
                // if it's the given occurrence we need
                if (currentOccurrence == occurrence)
                {
                    *outIndexA = m_events.size() - 1;
                    *outIndexB = 0;
                    return true;
                }
                else
                {
                    currentOccurrence++; // look further
                    found = true;
                }
            }

            // iterate over all event segments
            for (size_t i = 0; i < numEvents - 1; ++i)
            {
                // if this event segment/combo is the one we're looking for
                if (m_events[i].HashForSyncing(mirror) == firstEventID && m_events[i + 1].HashForSyncing(mirror) == secondEventID)
                {
                    // if it's the given occurrence we need
                    if (currentOccurrence == occurrence)
                    {
                        *outIndexA = i;
                        *outIndexB = i + 1;
                        return true;
                    }
                    else
                    {
                        currentOccurrence++; // look further
                        found = true;
                    }
                }
            } // for all events

            // if we didn't find a single hit we won't find any other
            if (found == false)
            {
                *outIndexA = InvalidIndex;
                *outIndexB = InvalidIndex;
                return false;
            }
        }

        // we didn't find it
        *outIndexA = InvalidIndex;
        *outIndexB = InvalidIndex;
        return false;
    }

    bool AnimGraphSyncTrack::FindMatchingEvents(EventIterator start, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool forward, bool mirror) const
    {
        EventIterator current = start;

        do
        {
            // Note that, even when playing backward (forward==false), event
            // pairs are recognized by (current, current+1). Event pairs are
            // not played in reverse when playing backward.
            // If the track has events L--R--L--R, and the ids being searched
            // for are L--R, and current index is 2 (the last L), the result LR
            // pair found will be indexes (2,3), not (2,1), even when playing
            // backwards.
            EventIterator next = AdvanceAndWrapIterator(current, true, m_events.cbegin(), m_events.cend());
            if ((*current).HashForSyncing(mirror) == firstEventID && (*next).HashForSyncing(mirror) == secondEventID)
            {
                *outIndexA = AZStd::distance(m_events.cbegin(), current);
                *outIndexB = AZStd::distance(m_events.cbegin(), next);
                return true;
            }

            current = AdvanceAndWrapIterator(current, forward, m_events.cbegin(), m_events.cend());
        } while (current != start);

        *outIndexA = InvalidIndex;
        *outIndexB = InvalidIndex;
        return false;
    };

    // try to find a matching event combination
    bool AnimGraphSyncTrack::FindMatchingEvents(size_t syncIndex, size_t firstEventID, size_t secondEventID, size_t* outIndexA, size_t* outIndexB, bool forward, bool mirror) const
    {
        // if the number of events is zero, we certainly don't have a match
        const size_t numEvents = m_events.size();
        if (numEvents == 0)
        {
            *outIndexA = InvalidIndex;
            *outIndexB = InvalidIndex;
            return false;
        }

        // if the sync index is not set, start at the first pair (which starts from the last sync key)
        if (syncIndex == InvalidIndex)
        {
            if (forward)
            {
                syncIndex = numEvents - 1;
            }
            else
            {
                syncIndex = 0;
            }
        }

        EventIterator it = AZStd::next(m_events.cbegin(), syncIndex);
        if (!forward)
        {
            // When playing in reverse, we do not match the initial position
            // provided by the syncIndex, even if it would match. Instead we
            // start with initial position - 1.
            it = AdvanceAndWrapIterator(it, false, m_events.cbegin(), m_events.cend());
        }
        return FindMatchingEvents(it, firstEventID, secondEventID, outIndexA, outIndexB, forward, mirror);
    }


    // calculate the segment length in seconds
    float AnimGraphSyncTrack::CalcSegmentLength(size_t indexA, size_t indexB) const
    {
        if (indexA < indexB) // this is normal, the first event comes before the second
        {
            return m_events[indexB].GetStartTime() - m_events[indexA].GetStartTime();
        }
        else // looping case
        {
            return GetDuration() - m_events[indexA].GetStartTime() + m_events[indexB].GetStartTime();
        }
    }

    float AnimGraphSyncTrack::GetDuration() const
    {
        return m_motion->GetMotionData()->GetDuration();
    }


    bool AnimGraphSyncTrack::AreEventsSyncable(const MotionEvent& eventA, const MotionEvent& eventB, bool isMirror) const
    {
        return eventA.HashForSyncing(isMirror) == eventB.HashForSyncing(isMirror);
    }

} // namespace EMotionFX
