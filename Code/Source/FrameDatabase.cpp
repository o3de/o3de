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

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/AnimGraphPose.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionInstancePool.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <Frame.h>
#include <Feature.h>
#include <FrameDatabase.h>
#include <MotionMatchEventData.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(FrameDatabase, MotionMatchAllocator, 0)

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

        void FrameDatabase::ExtractActiveMotionMatchEventDatas(const Motion* motion, float time, AZStd::vector<MotionMatchEventData*>& activeEventDatas)
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
                        auto motionMatchEventData = azdynamic_cast<const MotionMatchEventData*>(eventData.get());
                        if (motionMatchEventData)
                        {
                            activeEventDatas.emplace_back(const_cast<MotionMatchEventData*>(motionMatchEventData));
                        }
                    }
                }
            }
        }

        bool FrameDatabase::IsFrameDiscarded(const AZStd::vector<MotionMatchEventData*>& activeEventDatas) const
        {
            for (const MotionMatchEventData* eventData : activeEventDatas)
            {
                if (eventData->GetDiscardFrames())
                {
                    return true;
                }
            }

            return false;
        }

        AZStd::tuple<size_t, size_t> FrameDatabase::ImportFrames(Motion* motion, const FrameImportSettings& settings, bool mirrored)
        {
            AZ_Assert(motion, "The motion cannot be a nullptr");
            AZ_Assert(settings.m_sampleRate > 0, "The sample rate must be bigger than zero frames per second");
            AZ_Assert(settings.m_sampleRate <= 120, "The sample rate must be smaller than 120 frames per second");

            size_t numFramesImported = 0;
            size_t numFramesDiscarded = 0;

            // Calculate the number of frames we might need to import, in worst case.
            const double timeStep = 1.0 / aznumeric_cast<double>(settings.m_sampleRate);
            const size_t worstCaseNumFrames = aznumeric_cast<size_t>(ceil(motion->GetDuration() / timeStep)) + 1;

            // Try to pre-allocate memory for the worst case scenario.
            if (m_frames.capacity() < m_frames.size() + worstCaseNumFrames)
            {
                m_frames.reserve(m_frames.size() + worstCaseNumFrames);
            }

            AZStd::vector<MotionMatchEventData*> activeEvents;

            // Iterate over all sample positions in the motion.
            const double totalTime = aznumeric_cast<double>(motion->GetDuration());
            double curTime = 0.0;
            while (curTime <= totalTime)
            {
                const float floatTime = aznumeric_cast<float>(curTime);
                ExtractActiveMotionMatchEventDatas(motion, floatTime, activeEvents);
                if (!IsFrameDiscarded(activeEvents))
                {
                    ImportFrame(motion, floatTime, mirrored);
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
                const float floatTime = aznumeric_cast<float>(totalTime);
                ExtractActiveMotionMatchEventDatas(motion, floatTime, activeEvents);
                if (!IsFrameDiscarded(activeEvents))
                {
                    ImportFrame(motion, floatTime, mirrored);
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
    } // namespace MotionMatching
} // namespace EMotionFX
