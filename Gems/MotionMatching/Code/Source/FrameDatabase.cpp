/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <CsvSerializers.h>
#include <MotionMatchingInstance.h>
#include <Frame.h>
#include <Feature.h>
#include <FrameDatabase.h>
#include <EventData.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FrameDatabase, MotionMatchAllocator)

    FrameDatabase::FrameDatabase()
    {
    }

    FrameDatabase::~FrameDatabase()
    {
        Clear();
    }

    void FrameDatabase::Clear()
    {
        // Clear the frames.
        m_frames.clear();
        m_frames.shrink_to_fit();

        m_frameIndexByMotion.clear();

        // Clear other things.
        m_usedMotions.clear();
        m_usedMotions.shrink_to_fit();
    }

    void FrameDatabase::ExtractActiveMotionEventDatas(const Motion* motion, float time, AZStd::vector<EventData*>& activeEventDatas) const
    {
        activeEventDatas.clear();

        // Iterate over all motion event tracks and all events inside them.
        const MotionEventTable* eventTable = motion->GetEventTable();
        const size_t numTracks = eventTable->GetNumTracks();
        for (size_t t = 0; t < numTracks; ++t)
        {
            const MotionEventTrack* track = eventTable->GetTrack(t);
            const size_t numEvents = track->GetNumEvents();
            for (size_t e = 0; e < numEvents; ++e)
            {
                const MotionEvent& motionEvent = track->GetEvent(e);

                // Only handle range based events and events that include our time value.
                if (motionEvent.GetIsTickEvent() ||
                    motionEvent.GetStartTime() > time ||
                    motionEvent.GetEndTime() < time)
                {
                    continue;
                }

                for (auto eventData : motionEvent.GetEventDatas())
                {
                    activeEventDatas.emplace_back(const_cast<EventData*>(eventData.get()));
                }
            }
        }
    }

    bool FrameDatabase::IsFrameDiscarded(const Motion* motion, float frameTime, AZStd::vector<EventData*>& activeEvents) const
    {
        // Is frame discarded by a motion event?
        ExtractActiveMotionEventDatas(motion, frameTime, activeEvents);
        for (const EventData* eventData : activeEvents)
        {
            if (eventData->RTTI_GetType() == azrtti_typeid<DiscardFrameEventData>())
            {
                return true;
            }
        }

        return false;
    }

    AZStd::tuple<size_t, size_t> FrameDatabase::ImportFrames(Motion* motion, const FrameImportSettings& settings, bool mirrored)
    {
        AZ_PROFILE_SCOPE(Animation, "FrameDatabase::ImportFrames");

        AZ_Assert(motion, "The motion cannot be a nullptr");
        AZ_Assert(settings.m_sampleRate > 0, "The sample rate must be bigger than zero frames per second");
        AZ_Assert(settings.m_sampleRate <= 120, "The sample rate must be smaller than 120 frames per second");

        size_t numFramesImported = 0;
        size_t numFramesDiscarded = 0;

        // Calculate the number of frames we might need to import, in worst case.
        m_sampleRate = settings.m_sampleRate;
        const double timeStep = 1.0 / aznumeric_cast<double>(settings.m_sampleRate);
        const size_t worstCaseNumFrames = aznumeric_cast<size_t>(ceil(motion->GetDuration() / timeStep)) + 1;

        // Try to pre-allocate memory for the worst case scenario.
        if (m_frames.capacity() < m_frames.size() + worstCaseNumFrames)
        {
            m_frames.reserve(m_frames.size() + worstCaseNumFrames);
        }

        AZStd::vector<EventData*> activeEvents;

        // Iterate over all sample positions in the motion.
        const double totalTime = aznumeric_cast<double>(motion->GetDuration());
        double curTime = 0.0;
        while (curTime <= totalTime)
        {
            const float frameTime = aznumeric_cast<float>(curTime);
            if (!IsFrameDiscarded(motion, frameTime, activeEvents))
            {
                ImportFrame(motion, frameTime, mirrored);
                numFramesImported++;
            }
            else
            {
                numFramesDiscarded++;
            }
            curTime += timeStep;
        }

        // Make sure we include the last frame, if we stepped over it.
        if (curTime - timeStep < totalTime - 0.000001)
        {
            const float frameTime = aznumeric_cast<float>(totalTime);
            if (!IsFrameDiscarded(motion, frameTime, activeEvents))
            {
                ImportFrame(motion, frameTime, mirrored);
                numFramesImported++;
            }
            else
            {
                numFramesDiscarded++;
            }
        }

        // Automatically shrink the frame storage to their minimum size.
        if (settings.m_autoShrink)
        {
            m_frames.shrink_to_fit();
        }

        // Register the motion.
        if (AZStd::find(m_usedMotions.begin(), m_usedMotions.end(), motion) == m_usedMotions.end())
        {
            m_usedMotions.emplace_back(motion);
        }

        return { numFramesImported, numFramesDiscarded };
    }

    void FrameDatabase::ImportFrame(Motion* motion, float timeValue, bool mirrored)
    {
        m_frames.emplace_back(Frame(m_frames.size(), motion, timeValue, mirrored));
        m_frameIndexByMotion[motion].emplace_back(m_frames.back().GetFrameIndex());
    }

    size_t FrameDatabase::CalcMemoryUsageInBytes() const
    {
        size_t total = 0;

        total += m_frames.capacity() * sizeof(Frame);
        total += sizeof(m_frames);
        total += m_usedMotions.capacity() * sizeof(const Motion*);
        total += sizeof(m_usedMotions);

        return total;
    }

    size_t FrameDatabase::GetNumFrames() const
    {
        return m_frames.size();
    }

    size_t FrameDatabase::GetNumUsedMotions() const
    {
        return m_usedMotions.size();
    }

    const Motion* FrameDatabase::GetUsedMotion(size_t index) const
    {
        return m_usedMotions[index];
    }

    const Frame& FrameDatabase::GetFrame(size_t index) const
    {
        AZ_Assert(index < m_frames.size(), "Frame index is out of range!");
        return m_frames[index];
    }

    AZStd::vector<Frame>& FrameDatabase::GetFrames()
    {
        return m_frames;
    }

    const AZStd::vector<Frame>& FrameDatabase::GetFrames() const
    {
        return m_frames;
    }

    const AZStd::vector<const Motion*>& FrameDatabase::GetUsedMotions() const
    {
        return m_usedMotions;
    }

    size_t FrameDatabase::FindFrameIndex(Motion* motion, float playtime) const
    {
        auto iterator = m_frameIndexByMotion.find(motion);
        if (iterator == m_frameIndexByMotion.end())
        {
            return InvalidIndex;
        }

        const AZStd::vector<size_t>& frameIndices = iterator->second;
        for (const size_t frameIndex : frameIndices)
        {
            const Frame& frame = m_frames[frameIndex];
            if (playtime >= frame.GetSampleTime() &&
                frameIndex + 1 < m_frames.size() &&
                playtime <= m_frames[frameIndex + 1].GetSampleTime())
            {
                return frameIndex;
            }
        }

        return InvalidIndex;
    }

    void FrameDatabase::SaveAsCsv(const char* filename, ActorInstance* actorInstance, const ETransformSpace transformSpace, bool writePositions, bool writeRotations) const
    {
        PoseWriterCsv poseWriter;
        PoseWriterCsv::WriteSettings writeSettings{ writePositions, writeRotations };
        poseWriter.Begin(filename, actorInstance, writeSettings);

        Pose pose;
        pose.LinkToActorInstance(actorInstance);
        pose.InitFromBindPose(actorInstance);

        for (const Frame& currentFrame : m_frames)
        {
            currentFrame.SamplePose(&pose);
            poseWriter.WritePose(pose, transformSpace);
        }
    }
} // namespace EMotionFX::MotionMatching
