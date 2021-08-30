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
#include <FrameData.h>
#include <FrameDatabase.h>
#include <MotionMatchEventData.h>
#include <KdTree.h>
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
            m_kdTree = new KdTree();
        }

        FrameDatabase::~FrameDatabase()
        {
            Clear();
            delete m_kdTree;
        }

        void FrameDatabase::Clear()
        {
            // Clear the frame data.
            for (FrameData* frameData : m_frameDataVector)
            {
                delete frameData;
            }
            m_frameDatas.clear();
            m_frameDataVector.clear();

            // Clear the frames.
            m_frames.clear();
            m_frames.shrink_to_fit();

            // Clear other things.
            m_usedMotions.clear();
            m_usedMotions.shrink_to_fit();
            m_kdTree->Clear();
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
        }

        size_t FrameDatabase::CalcMemoryUsageInBytes() const
        {
            size_t total = 0;
            for (const FrameData* frameData : m_frameDataVector)
            {
                if (frameData && frameData->GetId().IsNull())
                {
                    continue;
                }

                total += frameData->CalcMemoryUsageInBytes();
            }

            total += m_frames.capacity() * sizeof(Frame);
            total += sizeof(m_usedMotions);
            total += sizeof(m_frameDatas);

            return total;
        }

        const FrameData* FrameDatabase::GetFrameData(size_t index) const
        {
            return m_frameDataVector[index];
        }

        const AZStd::vector<FrameData*>& FrameDatabase::GetFrameData() const
        {
            return m_frameDataVector;
        }

        void FrameDatabase::RegisterFrameData(FrameData* frameData)
        {
            // Try to see if there is a frame data with the same Id.
            auto location = AZStd::find_if(m_frameDatas.begin(), m_frameDatas.end(), [&frameData](const auto& curEntry) -> bool {
                return (frameData->GetId() == curEntry.second->GetId());
            });

            // If we already found it.
            if (location != m_frameDatas.end())
            {
                AZ_Assert(false, "Frame data with id '%s' has already been registered!", frameData->GetId().data);
                return;
            }

            m_frameDatas.emplace(frameData->GetId(), frameData);
            m_frameDataVector.emplace_back(frameData);
        }

        bool FrameDatabase::GenerateFrameDatas(ActorInstance* actorInstance, size_t maxKdTreeDepth, size_t minFramesPerKdTreeNode)
        {
            if (m_frames.size() == 0)
            {
                return true;
            }

            // Initialize all frame datas before we process each frame.
            for (FrameData* frameData : m_frameDataVector)
            {
                if (frameData && frameData->GetId().IsNull())
                {
                    return false;
                }

                FrameData::InitSettings frameSettings;
                frameSettings.m_actorInstance = actorInstance;
                frameData->SetData(this);
                if (!frameData->Init(frameSettings))
                {
                    return false;
                }
            }

            // Iterate over all frames and extract the data for this frame.
            const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
            MotionInstancePool& motionInstancePool = GetMotionInstancePool();
            MotionInstance* motionInstance = motionInstancePool.RequestNew(m_frames[0].GetSourceMotion(), actorInstance);
            MotionInstance* motionInstanceNext = motionInstancePool.RequestNew(m_frames[0].GetSourceMotion(), actorInstance);
            AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(actorInstance->GetThreadIndex())->GetPosePool();
            AnimGraphPose* pose = posePool.RequestPose(actorInstance);
            AnimGraphPose* previousPose = posePool.RequestPose(actorInstance);
            AnimGraphPose* nextPose = posePool.RequestPose(actorInstance);
            Motion* previousSourceMotion = nullptr;
            FrameData::ExtractFrameContext context;
            context.m_pose = &pose->GetPose();
            context.m_previousPose = &previousPose->GetPose();
            context.m_nextPose = &nextPose->GetPose();
            context.m_motionInstance = motionInstance;
            bool lastNextValid = false;
            for (const Frame& frame : m_frames)
            {
                // Sample the pose.
                if (lastNextValid)
                {
                    *pose = *nextPose;
                }
                else
                {
                    motionInstance->SetMirrorMotion(frame.GetMirrored());
                    FrameData::SamplePose(frame.GetSampleTime(), bindPose, frame.GetSourceMotion(), motionInstance, const_cast<Pose*>(context.m_pose));
                }

                size_t nextFrameIndex = frame.GetFrameIndex() + 1;
                if (frame.GetFrameIndex() > m_frames.size() - 2 || m_frames[nextFrameIndex].GetSourceMotion() != frame.GetSourceMotion())
                {
                    *nextPose = *pose;
                    lastNextValid = false;
                    context.m_nextFrameIndex = frame.GetFrameIndex();
                }
                else
                {
                    const Frame& nextFrame = m_frames[nextFrameIndex];
                    motionInstanceNext->SetMirrorMotion(nextFrame.GetMirrored());
                    FrameData::SamplePose(nextFrame.GetSampleTime(), bindPose, nextFrame.GetSourceMotion(), motionInstanceNext, const_cast<Pose*>(context.m_nextPose));
                    lastNextValid = true;
                    context.m_timeDelta = nextFrame.GetSampleTime() - frame.GetSampleTime();
                    context.m_nextFrameIndex = nextFrame.GetFrameIndex();
                }

                context.m_data = this;
                context.m_frameIndex = frame.GetFrameIndex();
                if (frame.GetFrameIndex() == 0 || frame.GetSourceMotion() != previousSourceMotion)
                {
                    *previousPose = *pose;
                    context.m_prevFrameIndex = frame.GetFrameIndex();
                }
                else
                {
                    context.m_prevFrameIndex = frame.GetFrameIndex() - 1;
                }

                // Extract all frame datas.
                for (const auto& frameDataEntry : m_frameDatas)
                {
                    FrameData* frameData = frameDataEntry.second;
                    context.m_motionInstance->SetMirrorMotion(frame.GetMirrored());
                    frameData->ExtractFrameData(context);
                }

                *previousPose = *pose;
                previousSourceMotion = frame.GetSourceMotion();
            }

            posePool.FreePose(pose);
            posePool.FreePose(previousPose);
            posePool.FreePose(nextPose);
            motionInstancePool.Free(motionInstance);
            motionInstancePool.Free(motionInstanceNext);

            // Initialize the kd-tree used to accelerate the searches.
            if (!m_kdTree->Init(*this, maxKdTreeDepth, minFramesPerKdTreeNode)) // Internally automatically clears any existing contents.
            {
                AZ_Error("EMotionFX", false, "Failed to initialize KdTree acceleration structure inside motion matching behavior.");
                return false;
            }

            return true;
        }

        size_t FrameDatabase::GetNumFrames() const
        {
            return m_frames.size();
        }

        size_t FrameDatabase::GetNumFrameDataTypes() const
        {
            return m_frameDataVector.size();
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

        FrameData* FrameDatabase::FindFrameData(const AZ::TypeId& frameDataTypeId) const
        {
            const auto result = m_frameDatas.find(frameDataTypeId);
            if (result == m_frameDatas.end())
            {
                return nullptr;
            }

            return result->second;
        }

        void FrameDatabase::DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance)
        {
            for (FrameData* frameData: m_frameDataVector)
            {
                if (frameData && frameData->GetId().IsNull())
                {
                    continue;
                }

                if (frameData->GetDebugDrawEnabled())
                {
                    frameData->DebugDraw(draw, behaviorInstance);
                }
            }
        }

        FrameData* FrameDatabase::CreateFrameDataByType(const AZ::TypeId& typeId)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!context)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return nullptr;
            }

            const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeId);
            if (!classData)
            {
                AZ_Warning("EMotionFX", false, "Can't find class data for this type.");
                return nullptr;
            }

            FrameData* frameDataObject = reinterpret_cast<FrameData*>(classData->m_factory->Create(classData->m_name));
            return frameDataObject;
        }

        size_t FrameDatabase::CalcNumDataDimensionsForKdTree() const
        {
            size_t totalDimensions = 0;
            for (FrameData* frameData: m_frameDataVector)
            {
                if ((frameData && frameData->GetId().IsNull()) || !frameData->GetIncludeInKdTree())
                {
                    continue;
                }

                totalDimensions += frameData->GetNumDimensionsForKdTree();
            }
            return totalDimensions;
        }

        AZStd::vector<float> FrameDatabase::CalcMedians() const
        {
            AZStd::vector<float> medians;
            medians.resize(CalcNumDataDimensionsForKdTree());

            size_t dimensionIndex = 0;
            for (const FrameData* frameData : m_frameDataVector)
            {
                if ((frameData && frameData->GetId().IsNull()) || !frameData->GetIncludeInKdTree())
                {
                    continue;
                }

                frameData->CalcMedians(medians, dimensionIndex);
                dimensionIndex += frameData->GetNumDimensionsForKdTree();
            }

            return medians;
        }

    } // namespace MotionMatching
} // namespace EMotionFX
