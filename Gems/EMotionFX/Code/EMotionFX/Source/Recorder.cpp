/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include "Recorder.h"
#include "RecorderBus.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "AnimGraphManager.h"
#include "AnimGraphInstance.h"
#include "AnimGraph.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphStateMachine.h"
#include "EventManager.h"
#include "EMotionFXManager.h"
#include "ActorManager.h"
#include "MorphSetupInstance.h"
#include "MotionEvent.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Recorder, RecorderAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(Recorder::ActorInstanceData, RecorderAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(Recorder::RecordSettings, RecorderAllocator)


    void Recorder::TransformTracks::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Recorder::TransformTracks>()
            ->Version(1)
            ->Field("positions", &Recorder::TransformTracks::m_positions)
            ->Field("rotations", &Recorder::TransformTracks::m_rotations)
#ifndef EMFX_SCALE_DISABLED
            ->Field("scales", &Recorder::TransformTracks::m_scales)
#endif
            ;
    }

    void Recorder::ActorInstanceData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Recorder::ActorInstanceData>()
            ->Version(1)
            ->Field("transformTracks", &ActorInstanceData::m_transformTracks)
            ;
    }

    void Recorder::RecordSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Recorder::RecordSettings>()
            ->Version(1)
            ->Field("fps", &Recorder::RecordSettings::m_fps)
            ->Field("recordTransforms", &Recorder::RecordSettings::m_recordTransforms)
            ->Field("recordNodeHistory", &Recorder::RecordSettings::m_recordNodeHistory)
            ->Field("historyStatesOnly", &Recorder::RecordSettings::m_historyStatesOnly)
            ->Field("recordAnimGraphStates", &Recorder::RecordSettings::m_recordAnimGraphStates)
            ->Field("recordEvents", &Recorder::RecordSettings::m_recordEvents)
            ->Field("recordScale", &Recorder::RecordSettings::m_recordScale)
            ->Field("recordMorphs", &Recorder::RecordSettings::m_recordMorphs)
            ->Field("interpolate", &Recorder::RecordSettings::m_interpolate)
            ;
    }

    Recorder::EventHistoryItem::EventHistoryItem():
        m_eventIndex(InvalidIndex),
        m_trackIndex(InvalidIndex),
        m_emitterNodeId(AnimGraphNodeId()),
        m_animGraphId(MCORE_INVALIDINDEX32),
        m_color(AnimGraph::RandomGraphColor()),
        m_startTime(),
        m_endTime(),
        m_isTickEvent() {

    }

    Recorder::Recorder()
        : MCore::RefCounted()
    {
        m_isInPlayMode   = false;
        m_isRecording    = false;
        m_autoPlay       = false;
        m_recordTime     = 0.0f;
        m_lastRecordTime = 0.0f;
        m_currentPlayTime = 0.0f;

        EMotionFX::ActorInstanceNotificationBus::Handler::BusConnect();
    }

    Recorder::~Recorder()
    {
        EMotionFX::ActorInstanceNotificationBus::Handler::BusDisconnect();
        Clear();
    }

    // create a new recorder
    Recorder* Recorder::Create()
    {
        return aznew Recorder();
    }

    void Recorder::Reflect(AZ::ReflectContext* context)
    {
        Recorder::ActorInstanceData::Reflect(context);
        Recorder::TransformTracks::Reflect(context);
        Recorder::RecordSettings::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Recorder>()
            ->Version(1)
            ->Field("actorInstanceDatas", &Recorder::m_actorInstanceDatas)
            ->Field("timeDeltas", &Recorder::m_timeDeltas)
            ->Field("settings", &Recorder::m_recordSettings)
            ;
    }

    // enable or disable auto play mode
    void Recorder::SetAutoPlay(bool enabled)
    {
        m_autoPlay = enabled;
    }


    // set the current play time
    void Recorder::SetCurrentPlayTime(float timeInSeconds)
    {
        m_currentPlayTime = timeInSeconds;

        if (m_currentPlayTime < 0.0f)
        {
            m_currentPlayTime = 0.0f;
        }

        if (m_currentPlayTime > m_recordTime)
        {
            m_currentPlayTime = m_recordTime;
        }
    }


    // start playback mode
    void Recorder::StartPlayBack()
    {
        StopRecording();
        m_isInPlayMode = true;
    }


    // stop playback mode
    void Recorder::StopPlayBack()
    {
        m_isInPlayMode = false;
    }


    // rewind the playback
    void Recorder::Rewind()
    {
        m_currentPlayTime = 0.0f;
    }

    bool Recorder::HasRecording() const
    {
        return (GetRecordTime() > AZ::Constants::FloatEpsilon && !m_actorInstanceDatas.empty());
    }

    // clear any currently recorded data
    void Recorder::Clear()
    {
        Lock();

        m_isInPlayMode = false;
        m_isRecording = false;
        m_autoPlay = false;
        m_recordTime = 0.0f;
        m_lastRecordTime = 0.0f;
        m_currentPlayTime = 0.0f;
        m_recordSettings.m_actorInstances.clear();
        m_timeDeltas.clear();

        // delete all actor instance datas
        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            delete actorInstanceData;
        }
        m_actorInstanceDatas.clear();

        Unlock();
    }


    // start recording
    void Recorder::StartRecording(const RecordSettings& settings)
    {
        // clear any previous recorded data
        Clear();
        StopRecording();

        Lock();

        // Generate a new random UUID for this recording session.
        m_sessionUuid = AZ::Uuid::Create();

        // we are recording again
        m_recordSettings = settings;
        m_isRecording    = true;

        // Add all actor instances if we did not specify them explicitly.
        if (m_recordSettings.m_actorInstances.empty())
        {
            const size_t numActorInstances = GetActorManager().GetNumActorInstances();
            m_recordSettings.m_actorInstances.resize(numActorInstances);
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
                m_recordSettings.m_actorInstances[i] = actorInstance;
            }
        }

        // prepare for recording
        // this resizes arrays and allocates buffers upfront
        PrepareForRecording();

        // record the initial frame
        RecordCurrentFrame(0.0f);

        Unlock();
    }


    // update (when playback)
    void Recorder::UpdatePlayMode(float timeDelta)
    {
        // increase the playtime if we are in automatic play mode and playback is enabled
        if (m_isInPlayMode && m_autoPlay)
        {
            SetCurrentPlayTime(m_currentPlayTime + timeDelta);
        }
    }


    // update the recorder
    void Recorder::Update(float timeDelta)
    {
        Lock();

        // if we are not recording there is nothing to do
        if (m_isRecording == false)
        {
            Unlock();
            return;
        }

        // increase the time we record
        m_recordTime += timeDelta;

        // save a sample when more time passed than the desired sample rate
        const float sampleRate = 1.0f / (float)m_recordSettings.m_fps;
        if (m_recordTime - m_lastRecordTime >= sampleRate)
        {
            RecordCurrentFrame(timeDelta);
        }

        Unlock();
    }


    // stop recording
    void Recorder::StopRecording(bool lock)
    {
        if (lock)
        {
            Lock();
        }

        m_isRecording = false;
        FinalizeAllNodeHistoryItems();

        if (lock)
        {
            Unlock();
        }
    }


    // prepare for recording by resizing and preallocating space/arrays
    void Recorder::PrepareForRecording()
    {
        const size_t numActorInstances = m_recordSettings.m_actorInstances.size();
        m_actorInstanceDatas.resize(numActorInstances);
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            m_actorInstanceDatas[i] = aznew ActorInstanceData();
            ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[i];

            // link it to the right actor instance
            ActorInstance* actorInstance = m_recordSettings.m_actorInstances[i];
            actorInstanceData.m_actorInstance = actorInstance;

            // add the transform tracks
            if (m_recordSettings.m_recordTransforms)
            {
                // for all nodes in the actor instance
                const size_t numNodes = actorInstance->GetNumNodes();
                actorInstanceData.m_transformTracks.resize(numNodes);
                for (size_t n = 0; n < numNodes; ++n)
                {
                    actorInstanceData.m_transformTracks[n].m_positions.Reserve(m_recordSettings.m_numPreAllocTransformKeys);
                    actorInstanceData.m_transformTracks[n].m_rotations.Reserve(m_recordSettings.m_numPreAllocTransformKeys);

                    EMFX_SCALECODE
                    (
                        if (m_recordSettings.m_recordScale)
                        {
                            actorInstanceData.m_transformTracks[n].m_scales.Reserve(m_recordSettings.m_numPreAllocTransformKeys);
                        }
                    )
                }
            } // if recording transforms

            // if recording morph targets, resize the morphs array
            if (m_recordSettings.m_recordMorphs)
            {
                const size_t numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
                actorInstanceData.m_morphTracks.resize(numMorphs);
                for (size_t m = 0; m < numMorphs; ++m)
                {
                    actorInstanceData.m_morphTracks[m].Reserve(256);
                }
            }

            // add the animgraph data
            if (m_recordSettings.m_recordAnimGraphStates)
            {
                if (actorInstance->GetAnimGraphInstance())
                {
                    actorInstanceData.m_animGraphData = new AnimGraphInstanceData();
                    actorInstanceData.m_animGraphData->m_animGraphInstance = actorInstance->GetAnimGraphInstance();
                    if (m_recordSettings.m_initialAnimGraphAnimBytes > 0)
                    {
                        actorInstanceData.m_animGraphData->m_dataBuffer  = (uint8*)MCore::Allocate(m_recordSettings.m_initialAnimGraphAnimBytes, EMFX_MEMCATEGORY_RECORDER);
                    }

                    actorInstanceData.m_animGraphData->m_dataBufferSize  = m_recordSettings.m_initialAnimGraphAnimBytes;
                }
            } // if recording animgraphs
        } // for all actor instances
    }


    void Recorder::OptimizeRecording()
    {
        ShrinkTransformTracks();
    }


    void Recorder::ShrinkTransformTracks()
    {
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            const ActorInstance* actorInstance = actorInstanceData->m_actorInstance;
            if (actorInstanceData->m_transformTracks.empty())
            {
                continue;
            }

            const size_t numNodes = actorInstance->GetNumNodes();
            for (size_t n = 0; n < numNodes; ++n)
            {
                actorInstanceData->m_transformTracks[n].m_positions.Shrink();
                actorInstanceData->m_transformTracks[n].m_rotations.Shrink();

                EMFX_SCALECODE
                (
                    actorInstanceData->m_transformTracks[n].m_scales.Shrink();
                )
            }
        }
    }


    // save to a file
    bool Recorder::SaveToFile(const char* outFile)
    {
        // The template types used by the recorder result in an extremely
        // verbose serialized object stream. Use the binary format to attempt
        // to optimize the file size.
        return AZ::Utils::SaveObjectToFile(outFile, AZ::ObjectStream::ST_BINARY, this);
    }


    Recorder* Recorder::LoadFromFile(const char* filename)
    {
        return AZ::Utils::LoadObjectFromFile<Recorder>(filename);
    }


    // record the current frame
    void Recorder::RecordCurrentFrame(float timeDelta)
    {
        m_timeDeltas.emplace_back(timeDelta);

        // record the current transforms
        if (m_recordSettings.m_recordTransforms)
        {
            RecordCurrentTransforms();
        }

        // record the current anim graph states
        if (m_recordSettings.m_recordAnimGraphStates)
        {
            if (!RecordCurrentAnimGraphStates())
            {
                StopRecording();
                Clear();
                return;
            }
        }

        // always record the main transforms
        RecordMainLocalTransforms();

        // record morphs
        if (m_recordSettings.m_recordMorphs)
        {
            RecordMorphs();
        }

        // update (while recording) the node history items
        if (m_recordSettings.m_recordNodeHistory)
        {
            UpdateNodeHistoryItems();
        }

        // recordo the events
        if (m_recordSettings.m_recordEvents)
        {
            RecordEvents();
        }

        // update the last record time
        m_lastRecordTime = m_recordTime;
    }


    // record the morph weights
    void Recorder::RecordMorphs()
    {
        // for all actor instances
        const size_t numActorInstances = m_actorInstanceDatas.size();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[i];
            ActorInstance* actorInstance = actorInstanceData.m_actorInstance;

            const size_t numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
            for (size_t m = 0; m < numMorphs; ++m)
            {
                KeyTrackLinearDynamic<float, float>& morphTrack = actorInstanceData.m_morphTracks[i]; // morph animation data
                morphTrack.AddKey(m_recordTime, actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->GetWeight());
            }
        }
    }


    // record all actor instance main transformations, so not of the nodes, but of the actor instance itself
    void Recorder::RecordMainLocalTransforms()
    {
        // for all actor instances
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            ActorInstance* actorInstance = actorInstanceData->m_actorInstance;
            const Transform& transform = actorInstance->GetLocalSpaceTransform();

        #ifndef EMFX_SCALE_DISABLED
            AddTransformKey(actorInstanceData->m_actorLocalTransform, transform.m_position, transform.m_rotation, transform.m_scale);
        #else
            AddTransformKey(actorInstanceData->m_actorLocalTransform, transform.m_position, transform.m_rotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
        #endif
        }
    }


    // record current transforms
    void Recorder::RecordCurrentTransforms()
    {
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            ActorInstance* actorInstance = actorInstanceData->m_actorInstance;

            const TransformData* transformData = actorInstance->GetTransformData();
            {
                const size_t numNodes = actorInstance->GetNumNodes();
                for (size_t n = 0; n < numNodes; ++n)
                {
                    const Transform& localTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);

                #ifndef EMFX_SCALE_DISABLED
                    AddTransformKey(actorInstanceData->m_transformTracks[n], localTransform.m_position, localTransform.m_rotation, localTransform.m_scale);
                #else
                    AddTransformKey(actorInstanceData->m_transformTracks[n], localTransform.m_position, localTransform.m_rotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
                #endif
                }
            }
        }
    }


    // record current animgraph states
    bool Recorder::RecordCurrentAnimGraphStates()
    {
        // for all actor instances
        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            if (actorInstanceData->m_animGraphData == nullptr)
            {
                continue;
            }

            {
                // get some shortcuts
                AnimGraphInstanceData& animGraphInstanceData  = *actorInstanceData->m_animGraphData;
                AnimGraphInstance*     animGraphInstance      = animGraphInstanceData.m_animGraphInstance;
                const AnimGraph*       animGraph              = animGraphInstance->GetAnimGraph();

                // add a new frame
                AZStd::vector<AnimGraphAnimFrame>& frames = animGraphInstanceData.m_frames;
                if (!frames.empty())
                {
                    const size_t byteOffset = frames.back().m_byteOffset + frames.back().m_numBytes;
                    frames.emplace_back();
                    frames.back().m_byteOffset    = byteOffset;
                    frames.back().m_numBytes      = 0;
                }
                else
                {
                    frames.emplace_back();
                    frames.back().m_byteOffset    = 0;
                    frames.back().m_numBytes      = 0;
                }

                // get the current frame
                AnimGraphAnimFrame& currentFrame = frames.back();
                currentFrame.m_timeValue = m_recordTime;

                // save the parameter values
                const size_t numParams = animGraphInstance->GetAnimGraph()->GetNumValueParameters();
                currentFrame.m_parameterValues.resize(numParams);
                for (size_t p = 0; p < numParams; ++p)
                {
                    currentFrame.m_parameterValues[p] = AZStd::unique_ptr<MCore::Attribute>(animGraphInstance->GetParameterValue(p)->Clone());
                }

                // recursively save all unique datas
                if (!SaveUniqueData(animGraphInstance, animGraph->GetRootStateMachine(), animGraphInstanceData))
                {
                    return false;
                }

                // increase the frames counter
                animGraphInstanceData.m_numFrames++;
            }
        } // for all actor instances
        return true;
    }


    // recursively save the node's unique data
    bool Recorder::SaveUniqueData(AnimGraphInstance* animGraphInstance, AnimGraphObject* object, AnimGraphInstanceData& animGraphInstanceData)
    {
        // get the current frame's data pointer
        AnimGraphAnimFrame& currentFrame = animGraphInstanceData.m_frames.back();
        const size_t frameOffset = currentFrame.m_byteOffset;

        // prepare the objects array
        m_objects.clear();
        m_objects.reserve(1024);

        // collect the objects we are going to save for this frame
        object->RecursiveCollectObjects(m_objects);

        // resize the object infos array
        const size_t numObjects = m_objects.size();
        currentFrame.m_objectInfos.resize(numObjects);

        // calculate how much memory we need for this frame
        size_t requiredFrameBytes = 0;
        for (const AnimGraphObject* animGraphObject : m_objects)
        {
            requiredFrameBytes += animGraphObject->SaveUniqueData(animGraphInstance, nullptr);
        }

        // make sure we have at least the given amount of space in the buffer we are going to write the frame data to
        if (!AssureAnimGraphBufferSize(animGraphInstanceData, requiredFrameBytes + currentFrame.m_byteOffset))
        {
            return false;
        }
        uint8* dataPointer = &animGraphInstanceData.m_dataBuffer[frameOffset];

        // save all the unique datas for the objects
        for (size_t i = 0; i < numObjects; ++i)
        {
            // store the object info
            AnimGraphObject* curObject = m_objects[i];
            currentFrame.m_objectInfos[i].m_object = curObject;
            currentFrame.m_objectInfos[i].m_frameByteOffset = currentFrame.m_numBytes;

            // write the unique data
            const size_t numBytesWritten = curObject->SaveUniqueData(animGraphInstance, dataPointer);

            // increase some offsets/pointers
            currentFrame.m_numBytes += numBytesWritten;
            dataPointer += numBytesWritten;
        }

        // make sure we have a match here, otherwise some of the object->SaveUniqueData(dataPointer) returns different values than object->SaveUniqueData(nullptr)
        MCORE_ASSERT(requiredFrameBytes == currentFrame.m_numBytes);

        return true;
    }


    // make sure our anim graph anim buffer is big enough to hold a specified amount of bytes
    bool Recorder::AssureAnimGraphBufferSize(AnimGraphInstanceData& animGraphInstanceData, size_t numBytes)
    {
        // if the buffer is big enough, do nothing
        if (animGraphInstanceData.m_dataBufferSize >= numBytes)
        {
            return true;
        }

        // we need to reallocate to grow the buffer
        const size_t newNumBytes = animGraphInstanceData.m_dataBufferSize + (numBytes - animGraphInstanceData.m_dataBufferSize) * 100; // allocate 100 frames ahead
        void* newBuffer = MCore::Realloc(animGraphInstanceData.m_dataBuffer, newNumBytes, EMFX_MEMCATEGORY_RECORDER);
        MCORE_ASSERT(newBuffer);
        if (newBuffer)
        {
            animGraphInstanceData.m_dataBuffer = static_cast<uint8*>(newBuffer);
            animGraphInstanceData.m_dataBufferSize = newNumBytes;
            return true;
        }
        RecorderNotificationBus::Broadcast(&RecorderNotificationBus::Events::OnRecordingFailed,
            "There is not enough memory to continue the current EMotionFX "
            "recording. It was deleted to free memory in order to keep the "
            "editor stable."
            );
        return false;
    }


    // add a transform key
    void Recorder::AddTransformKey(TransformTracks& track, const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale)
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(scale);
    #endif

        // check if we need to add a position key at all
        if (track.m_positions.GetNumKeys() > 0)
        {
            const AZ::Vector3 lastPos = track.m_positions.GetLastKey()->GetValue();
            if (!pos.IsClose(lastPos, 0.0001f))
            {
                track.m_positions.AddKey(m_recordTime, pos);
            }
        }
        else
        {
            track.m_positions.AddKey(m_recordTime, pos);
        }


        // check if we need to add a rotation key at all
        if (track.m_rotations.GetNumKeys() > 0)
        {
            const AZ::Quaternion lastRot = track.m_rotations.GetLastKey()->GetValue();
            if (!rot.IsClose(lastRot, 0.0001f))
            {
                track.m_rotations.AddKey(m_recordTime, rot);
            }
        }
        else
        {
            track.m_rotations.AddKey(m_recordTime, rot);
        }


        EMFX_SCALECODE
        (
            if (m_recordSettings.m_recordScale)
            {
                // check if we need to add a scale key
                if (track.m_scales.GetNumKeys() > 0)
                {
                    const AZ::Vector3 lastScale = track.m_scales.GetLastKey()->GetValue();
                    if (!scale.IsClose(lastScale, 0.0001f))
                    {
                        track.m_scales.AddKey(m_recordTime, scale);
                    }
                }
                else
                {
                    track.m_scales.AddKey(m_recordTime, scale);
                }
            }
        )
    }

    void Recorder::SampleAndApplyTransforms(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = m_recordSettings.m_actorInstances;
        const auto iterator = AZStd::find(recordedActorInstances.begin(), recordedActorInstances.end(), actorInstance);
        if (iterator != recordedActorInstances.end())
        {
            const size_t index = iterator - recordedActorInstances.begin();
            SampleAndApplyTransforms(timeInSeconds, index);
        }
    }

    void Recorder::SampleAndApplyAnimGraphs(float timeInSeconds) const
    {
        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            if(actorInstanceData->m_animGraphData)
            {
                SampleAndApplyAnimGraphStates(timeInSeconds, *actorInstanceData->m_animGraphData);
            }
        }
    }

    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = m_recordSettings.m_actorInstances;
        const auto iterator = AZStd::find(recordedActorInstances.begin(), recordedActorInstances.end(), actorInstance);
        if (iterator != recordedActorInstances.end())
        {
            const size_t index = iterator - recordedActorInstances.begin();
            SampleAndApplyMainTransform(timeInSeconds, index);
        }
    }

    void Recorder::SampleAndApplyMorphs(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = m_recordSettings.m_actorInstances;
        const auto iterator = AZStd::find(recordedActorInstances.begin(), recordedActorInstances.end(), actorInstance);
        if (iterator != recordedActorInstances.end())
        {
            const size_t index = AZStd::distance(recordedActorInstances.begin(), iterator);
            const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[index];
            const size_t numMorphs = actorInstanceData.m_morphTracks.size();
            if (numMorphs == actorInstance->GetMorphSetupInstance()->GetNumMorphTargets())
            {
                for (size_t i = 0; i < numMorphs; ++i)
                {
                    actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->SetWeight(actorInstanceData.m_morphTracks[i].GetValueAtTime(timeInSeconds));
                }
            }
        }
    }

    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, size_t actorInstanceIndex) const
    {
        // get the actor instance
        const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.m_actorInstance;

        // sample and apply
        const TransformTracks& track = actorInstanceData.m_actorLocalTransform;
        actorInstance->SetLocalSpacePosition(track.m_positions.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate));
        actorInstance->SetLocalSpaceRotation(track.m_rotations.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate));
        EMFX_SCALECODE
        (
            if (m_recordSettings.m_recordScale)
            {
                actorInstance->SetLocalSpaceScale(track.m_scales.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate));
            }
        )
    }

    void Recorder::SampleAndApplyTransforms(float timeInSeconds, size_t actorInstanceIndex) const
    {
        const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.m_actorInstance;
        TransformData* transformData = actorInstance->GetTransformData();

        // for all nodes in the actor instance
        Transform outTransform;
        const size_t numNodes = actorInstance->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            outTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);
            const TransformTracks& track = actorInstanceData.m_transformTracks[n];

            // build the output transform by sampling the keytracks
            outTransform.m_position = track.m_positions.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate);
            outTransform.m_rotation = track.m_rotations.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate);

            EMFX_SCALECODE
            (
                if (m_recordSettings.m_recordScale)
                {
                    outTransform.m_scale = track.m_scales.GetValueAtTime(timeInSeconds, nullptr, nullptr, m_recordSettings.m_interpolate);
                }
            )

            // set the transform
            transformData->GetCurrentPose()->SetLocalSpaceTransform(n, outTransform);
        }
    }

    void Recorder::SampleAndApplyAnimGraphStates(float timeInSeconds, const AnimGraphInstanceData& animGraphInstanceData) const
    {
        // find out the frame number
        const size_t frameNumber = FindAnimGraphDataFrameNumber(timeInSeconds);
        if (frameNumber == InvalidIndex)
        {
            return;
        }

        // for all animgraph instances that we recorded, restore their internal states
        AnimGraphInstance* animGraphInstance = animGraphInstanceData.m_animGraphInstance;

        // get the real frame number (clamped)
        const size_t realFrameNumber = AZStd::min(frameNumber, animGraphInstanceData.m_frames.size() - 1);
        const AnimGraphAnimFrame& currentFrame = animGraphInstanceData.m_frames[realFrameNumber];

        // get the data and objects buffers
        const size_t byteOffset = currentFrame.m_byteOffset;
        const uint8* frameDataBuffer = &animGraphInstanceData.m_dataBuffer[byteOffset];
        const AZStd::vector<AnimGraphAnimObjectInfo>& frameObjects = currentFrame.m_objectInfos;

        // first lets update all parameter values
        MCORE_ASSERT(currentFrame.m_parameterValues.size() == animGraphInstance->GetAnimGraph()->GetNumParameters());
        const size_t numParameters = currentFrame.m_parameterValues.size();
        for (size_t p = 0; p < numParameters; ++p)
        {
            //  make sure the parameters are of the same type
            MCORE_ASSERT(animGraphInstance->GetParameterValue(p)->GetType() == currentFrame.m_parameterValues[p]->GetType());
            animGraphInstance->GetParameterValue(p)->InitFrom(currentFrame.m_parameterValues[p].get());
        }

        // process all objects for this frame
        [[maybe_unused]] size_t totalBytesRead = 0;
        const size_t numObjects = frameObjects.size();
        for (size_t a = 0; a < numObjects; ++a)
        {
            const AnimGraphAnimObjectInfo& objectInfo = frameObjects[a];
            const size_t numBytesRead = objectInfo.m_object->LoadUniqueData(animGraphInstance, &frameDataBuffer[objectInfo.m_frameByteOffset]);
            totalBytesRead += numBytesRead;
        }

        // make sure this matches, otherwise the data read is not the same as we have written
        MCORE_ASSERT(totalBytesRead == currentFrame.m_numBytes);
    }

    bool Recorder::GetHasRecorded(ActorInstance* actorInstance) const
    {
        return AZStd::find(m_recordSettings.m_actorInstances.begin(), m_recordSettings.m_actorInstances.end(), actorInstance)
            != m_recordSettings.m_actorInstances.end();
    }

    size_t Recorder::FindActorInstanceDataIndex(ActorInstance* actorInstance) const
    {
        const auto found = AZStd::find_if(begin(m_actorInstanceDatas), end(m_actorInstanceDatas), [actorInstance](const ActorInstanceData* data)
        {
            return data->m_actorInstance == actorInstance;
        });
        return found != end(m_actorInstanceDatas) ? AZStd::distance(begin(m_actorInstanceDatas), found) : InvalidIndex;
    }

    void Recorder::UpdateNodeHistoryItems()
    {
        // for all actor instances
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData->m_actorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&m_activeNodes);

            // get the history items as shortcut
            AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData->m_nodeHistoryItems;

            // finalize items
            for (NodeHistoryItem* curItem : historyItems)
            {
                if (curItem->m_isFinalized)
                {
                    continue;
                }

                // check if we have an active node for the given item
                const bool haveActiveNode = AZStd::find_if(begin(m_activeNodes), end(m_activeNodes), [curItem](const AnimGraphNode* activeNode)
                {
                    return activeNode->GetId() == curItem->m_nodeId;
                }) != end(m_activeNodes);

                // the node got deactivated, finalize the item
                if (haveActiveNode)
                {
                    curItem->m_globalWeights.Optimize(0.0001f);
                    curItem->m_localWeights.Optimize(0.0001f);
                    curItem->m_playTimes.Optimize(0.0001f);
                    curItem->m_isFinalized = true;
                    curItem->m_endTime = m_recordTime;
                    continue;
                }
            }

            // iterate over all active nodes
            for (const AnimGraphNode* activeNode : m_activeNodes)
            {
                if (activeNode == animGraphInstance->GetRootNode()) // skip the root node
                {
                    continue;
                }

                const AZ::TypeId typeID = azrtti_typeid(activeNode);

                // if the parent isn't a state machine then it isn't a state
                if (m_recordSettings.m_historyStatesOnly)
                {
                    if (azrtti_typeid(activeNode->GetParentNode()) != azrtti_typeid<AnimGraphStateMachine>())
                    {
                        continue;
                    }
                }

                // make sure this node is on our capture list
                if (!m_recordSettings.m_nodeHistoryTypes.empty())
                {
                    if (m_recordSettings.m_nodeHistoryTypes.find(typeID) == m_recordSettings.m_nodeHistoryTypes.end())
                    {
                        continue;
                    }
                }

                // skip node types we do not want to capture
                if (!m_recordSettings.m_nodeHistoryTypesToIgnore.empty())
                {
                    if (m_recordSettings.m_nodeHistoryTypesToIgnore.find(typeID) != m_recordSettings.m_nodeHistoryTypesToIgnore.end())
                    {
                        continue;
                    }
                }

                // try to locate an existing item
                NodeHistoryItem* item = FindNodeHistoryItem(*actorInstanceData, activeNode, m_recordTime);
                if (item == nullptr)
                {
                    item = new NodeHistoryItem();
                    item->m_name             = activeNode->GetName();
                    item->m_animGraphId      = animGraphInstance->GetAnimGraph()->GetID();
                    item->m_startTime        = m_recordTime;
                    item->m_isFinalized      = false;
                    item->m_trackIndex       = FindFreeNodeHistoryItemTrack(*actorInstanceData, item);
                    item->m_nodeId           = activeNode->GetId();
                    item->m_color            = activeNode->GetVisualizeColor();
                    item->m_typeColor        = activeNode->GetVisualColor();
                    item->m_categoryId       = (uint32)activeNode->GetPaletteCategory();
                    item->m_nodeType         = typeID;
                    item->m_animGraphInstance = animGraphInstance;
                    item->m_globalWeights.Reserve(1024);
                    item->m_localWeights.Reserve(1024);
                    item->m_playTimes.Reserve(1024);

                    // get the motion instance
                    if (typeID == azrtti_typeid<AnimGraphMotionNode>())
                    {
                        const AnimGraphMotionNode* motionNode = static_cast<const AnimGraphMotionNode*>(activeNode);
                        MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
                        if (motionInstance)
                        {
                            item->m_motionId = motionInstance->GetMotion()->GetID();
                            AzFramework::StringFunc::Path::GetFileName(item->m_motionFileName.c_str(), item->m_motionFileName);
                        }
                    }

                    historyItems.emplace_back(item);
                }

                // add the weight key and update infos
                const AnimGraphNodeData* uniqueData = activeNode->FindOrCreateUniqueNodeData(animGraphInstance);
                const float keyTime = m_recordTime - item->m_startTime;
                item->m_globalWeights.AddKey(keyTime, uniqueData->GetGlobalWeight());
                item->m_localWeights.AddKey(keyTime, uniqueData->GetLocalWeight());

                float normalizedTime = uniqueData->GetCurrentPlayTime();
                const float duration = uniqueData->GetDuration();
                if (duration > MCore::Math::epsilon)
                {
                    normalizedTime /= duration;
                }
                else
                {
                    normalizedTime = 0.0f;
                }

                item->m_playTimes.AddKey(keyTime, normalizedTime);
                item->m_endTime  = m_recordTime;
            }
        } // for all actor instances
    }


    // try to find a given node history item
    Recorder::NodeHistoryItem* Recorder::FindNodeHistoryItem(const ActorInstanceData& actorInstanceData, const AnimGraphNode* node, float recordTime) const
    {
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.m_nodeHistoryItems;
        for (NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->m_nodeId == node->GetId() && curItem->m_startTime <= recordTime && curItem->m_isFinalized == false)
            {
                return curItem;
            }

            if (curItem->m_nodeId == node->GetId() && curItem->m_startTime <= recordTime && curItem->m_endTime >= recordTime && curItem->m_isFinalized)
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // find a free track
    size_t Recorder::FindFreeNodeHistoryItemTrack(const ActorInstanceData& actorInstanceData, NodeHistoryItem* item) const
    {
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.m_nodeHistoryItems;

        bool found = false;
        size_t trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (const NodeHistoryItem* curItem : historyItems)
            {
                 if (curItem->m_trackIndex != trackIndex)
                {
                    continue;
                }

                // if the current item is not active anymore
                if (curItem->m_isFinalized)
                {
                    // if the start time of the item we try to insert is within the range of this item
                    if (item->m_startTime > curItem->m_startTime && item->m_startTime < curItem->m_endTime)
                    {
                        hasCollision = true;
                        break;
                    }

                    // if the end time of this item is within the range of this item
                    if (item->m_endTime > curItem->m_startTime && item->m_endTime < curItem->m_endTime)
                    {
                        hasCollision = true;
                        break;
                    }
                }
                else // if the current item is still active and has no real end time yet
                {
                    if (item->m_startTime >= curItem->m_startTime)
                    {
                        hasCollision = true;
                        break;
                    }
                }
            }

            if (hasCollision)
            {
                trackIndex++;
            }
            else
            {
                found = true;
            }
        }

        return trackIndex;
    }


    // find the maximum track index
    size_t Recorder::CalcMaxNodeHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const
    {
        size_t result = 0;
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.m_nodeHistoryItems;
        for (const NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->m_trackIndex > result)
            {
                result = curItem->m_trackIndex;
            }
        }

        return result;
    }


    // find the maximum event track index
    size_t Recorder::CalcMaxEventHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const
    {
        size_t result = 0;
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.m_eventHistoryItems;
        for (const EventHistoryItem* curItem : historyItems)
        {
            if (curItem->m_trackIndex > result)
            {
                result = curItem->m_trackIndex;
            }
        }

        return result;
    }


    // find the maximum track index
    size_t Recorder::CalcMaxNodeHistoryTrackIndex() const
    {
        size_t result = 0;

        // for all actor instances
        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            result = AZStd::max(result, CalcMaxNodeHistoryTrackIndex(*actorInstanceData));
        }

        return result;
    }


    // finalize all items
    void Recorder::FinalizeAllNodeHistoryItems()
    {
        // for all actor instances
        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData->m_actorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&m_activeNodes);

            // get the history items as shortcut
            const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData->m_nodeHistoryItems;

            // finalize all items
            for (NodeHistoryItem* historyItem : historyItems)
            {
                // remove unneeded key frames
                if (historyItem->m_isFinalized == false)
                {
                    historyItem->m_globalWeights.Optimize(0.0001f);
                    historyItem->m_localWeights.Optimize(0.0001f);
                    historyItem->m_playTimes.Optimize(0.0001f);
                    historyItem->m_isFinalized = true;
                }
            }
        }
    }


    // record events
    void Recorder::RecordEvents()
    {
        // for all actor instances
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData->m_actorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // get the event buffer
            const AnimGraphEventBuffer& eventBuffer = animGraphInstance->GetEventBuffer();

            // iterate over all events
            AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData->m_eventHistoryItems;
            const size_t numEvents = eventBuffer.GetNumEvents();
            for (size_t i = 0; i < numEvents; ++i)
            {
                const EventInfo& eventInfo = eventBuffer.GetEvent(i);
                if (eventInfo.m_eventState == EventInfo::EventInfo::ACTIVE)
                {
                    continue;
                }
                EventHistoryItem* item = FindEventHistoryItem(*actorInstanceData, eventInfo, m_recordTime);
                if (item == nullptr) // create a new one
                {
                    item = new EventHistoryItem();

                    // TODO
                    //item->m_eventIndex   = GetEventManager().FindEventTypeIndex(eventInfo.m_typeID);
                    item->m_eventInfo    = eventInfo;
                    item->m_isTickEvent  = eventInfo.m_event->GetIsTickEvent();
                    item->m_startTime    = m_recordTime;
                    item->m_animGraphId  = animGraphInstance->GetAnimGraph()->GetID();
                    item->m_emitterNodeId= eventInfo.m_emitter->GetId();
                    item->m_color        = eventInfo.m_emitter->GetVisualizeColor();

                    item->m_trackIndex   = FindFreeEventHistoryItemTrack(*actorInstanceData, item);

                    historyItems.emplace_back(item);
                }

                item->m_endTime  = m_recordTime;
            }
        }
    }


    // find a given event
    Recorder::EventHistoryItem* Recorder::FindEventHistoryItem(const ActorInstanceData& actorInstanceData, const EventInfo& eventInfo, float recordTime)
    {
        MCORE_UNUSED(recordTime);
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.m_eventHistoryItems;
        for (const EventHistoryItem* curItem : historyItems)
        {
            if (curItem->m_startTime < eventInfo.m_timeValue)
            {
                continue;
            }
        }

        return nullptr;
    }


    // find a free event track index
    size_t Recorder::FindFreeEventHistoryItemTrack(const ActorInstanceData& actorInstanceData, EventHistoryItem* item) const
    {
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.m_eventHistoryItems;
        bool found = false;
        size_t trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (const EventHistoryItem* curItem : historyItems)
            {
                if (curItem->m_trackIndex != trackIndex)
                {
                    continue;
                }

                if (MCore::Compare<float>::CheckIfIsClose(curItem->m_startTime, item->m_startTime, 0.01f))
                {
                    hasCollision = true;
                    break;
                }
            }

            if (hasCollision)
            {
                trackIndex++;
            }
            else
            {
                found = true;
            }
        }

        return trackIndex;
    }


    // find the frame number for a time value
    size_t Recorder::FindAnimGraphDataFrameNumber(float timeValue) const
    {
        // check if we recorded any actor instances at all
        if (m_actorInstanceDatas.empty())
        {
            return 0;
        }

        // just search in the first actor instances data
        const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[0];
        const AnimGraphInstanceData* animGraphData = actorInstanceData.m_animGraphData;
        if (animGraphData == nullptr)
        {
            return InvalidIndex;
        }

        const size_t numFrames = animGraphData->m_frames.size();
        if (numFrames == 0)
        {
            return InvalidIndex;
        }
        if (numFrames == 1)
        {
            return 0;
        }

        if (timeValue <= 0.0f)
        {
            return 0;
        }

        if (timeValue > animGraphData->m_frames.back().m_timeValue)
        {
            return animGraphData->m_frames.size() - 1;
        }

        for (size_t i = 0; i < numFrames - 1; ++i)
        {
            const AnimGraphAnimFrame& curFrame  = animGraphData->m_frames[i];
            const AnimGraphAnimFrame& nextFrame = animGraphData->m_frames[i + 1];
            if (curFrame.m_timeValue <= timeValue && nextFrame.m_timeValue > timeValue)
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    void Recorder::RemoveActorInstanceFromRecording(ActorInstance* actorInstance)
    {
        Lock();

        // Remove the actor instance from the record settings.
        AZStd::vector<ActorInstance*>& recordedActorInstances = m_recordSettings.m_actorInstances;
        recordedActorInstances.erase(AZStd::remove_if(recordedActorInstances.begin(), recordedActorInstances.end(),
            [&actorInstance](ActorInstance* recordedActorInstance){ return recordedActorInstance == actorInstance;}),
            recordedActorInstances.end());

        // Remove the actual recorded data.
        for (size_t i = 0; i < m_actorInstanceDatas.size();)
        {
            if (m_actorInstanceDatas[i]->m_actorInstance == actorInstance)
            {
                delete m_actorInstanceDatas[i];
                m_actorInstanceDatas.erase(AZStd::next(m_actorInstanceDatas.begin(), i));
            }
            else
            {
                i++;
            }
        }

        Unlock();
    }

    void Recorder::RemoveAnimGraphFromRecording(AnimGraph* animGraph)
    {
        Lock();

        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            if (actorInstanceData->m_animGraphData && actorInstanceData->m_animGraphData->m_animGraphInstance)
            {
                AnimGraph* curAnimGraph = actorInstanceData->m_animGraphData->m_animGraphInstance->GetAnimGraph();
                if (animGraph != curAnimGraph)
                {
                    continue;
                }

                delete actorInstanceData->m_animGraphData;
                actorInstanceData->m_animGraphData = nullptr;
            }
        }

        Unlock();
    }

    void Recorder::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
    {
        // Actor instances created by actor components do not use the command system and don't call a ClearRecorder command.
        // Thus, these actor instances will have to be removed from the recorder to avoid dangling data.
        RemoveActorInstanceFromRecording(actorInstance);
    }

    void Recorder::Lock()
    {
        m_lock.Lock();
    }

    void Recorder::Unlock()
    {
        m_lock.Unlock();
    }


    // extract sorted active items
    void Recorder::ExtractNodeHistoryItems(const ActorInstanceData& actorInstanceData, float timeValue, bool sort, EValueType valueType, AZStd::vector<ExtractedNodeHistoryItem>* outItems, AZStd::vector<size_t>* outMap) const
    {
        // Reinit the item array.
        const size_t maxIndex = CalcMaxNodeHistoryTrackIndex(actorInstanceData);
        outItems->resize(maxIndex + 1);
        for (size_t i = 0; i <= maxIndex; ++i)
        {
            ExtractedNodeHistoryItem& item = (*outItems)[i];
            item.m_nodeHistoryItem = nullptr;
            item.m_trackIndex = i;
            item.m_keyTrackSampleTime = 0.0f;
            item.m_value = 0.0f;
        }

        // find all node history items
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.m_nodeHistoryItems;
        for (NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->m_startTime <= timeValue && curItem->m_endTime > timeValue)
            {
                ExtractedNodeHistoryItem& item = (*outItems)[curItem->m_trackIndex];
                item.m_trackIndex        = curItem->m_trackIndex;
                item.m_keyTrackSampleTime = timeValue - curItem->m_startTime;
                item.m_nodeHistoryItem   = curItem;

                switch (valueType)
                {
                case VALUETYPE_GLOBALWEIGHT:
                    item.m_value = curItem->m_globalWeights.GetValueAtTime(item.m_keyTrackSampleTime, nullptr, nullptr, m_recordSettings.m_interpolate);
                    break;

                case VALUETYPE_LOCALWEIGHT:
                    item.m_value = curItem->m_localWeights.GetValueAtTime(item.m_keyTrackSampleTime, nullptr, nullptr, m_recordSettings.m_interpolate);
                    break;

                case VALUETYPE_PLAYTIME:
                    item.m_value = curItem->m_playTimes.GetValueAtTime(item.m_keyTrackSampleTime, nullptr, nullptr, m_recordSettings.m_interpolate);
                    break;

                default:
                    MCORE_ASSERT(false);    // unsupported mode
                    item.m_value = curItem->m_globalWeights.GetValueAtTime(item.m_keyTrackSampleTime, nullptr, nullptr, m_recordSettings.m_interpolate);
                }
            }
        }

        // build the map
        outMap->resize(maxIndex + 1);
        for (size_t i = 0; i <= maxIndex; ++i)
        {
            (*outMap)[i] = i;
        }

        // sort if desired
        if (sort)
        {
            AZStd::sort(begin(*outItems), end(*outItems));

            for (size_t i = 0; i <= maxIndex; ++i)
            {
                (*outMap)[outItems->at(i).m_trackIndex] = i;
            }
        }
    }


    size_t Recorder::CalcMaxNumActiveMotions(const ActorInstanceData& actorInstanceData) const
    {
        size_t result = 0;

        // Array of flags that each iteration will use to determine if a given track has already been counted in or not.
        AZStd::vector<bool> trackFlags;
        const size_t maxNumTracks = static_cast<size_t>(CalcMaxNodeHistoryTrackIndex()) + 1;
        trackFlags.resize(maxNumTracks);

        const size_t numNodeHistoryItems = actorInstanceData.m_nodeHistoryItems.size();
        for (size_t i = 0; i < numNodeHistoryItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* item = actorInstanceData.m_nodeHistoryItems[i];

            // Only process motion history items.
            if (item->m_motionId == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // Reset the track flags
            for (size_t t = 0; t < maxNumTracks; ++t)
            {
                trackFlags[t] = false;
            }

            // We at least have a single active motion.
            size_t intermediateResult = 1;
            trackFlags[item->m_trackIndex] = true;

            for (size_t j = 0; j < numNodeHistoryItems; ++j)
            {
                EMotionFX::Recorder::NodeHistoryItem* innerItem = actorInstanceData.m_nodeHistoryItems[j];

                // Did we count this track in already? If yes, skip.
                if (trackFlags[innerItem->m_trackIndex])
                {
                    continue;
                }

                // Skip self comparison and only process motion history items.
                if (i == j || innerItem->m_motionId == MCORE_INVALIDINDEX32)
                {
                    continue;
                }

                // Are the item and innerItem events overlapping?
                if ((item->m_startTime >= innerItem->m_startTime && item->m_startTime <= innerItem->m_endTime) ||
                    (item->m_endTime >= innerItem->m_startTime && item->m_endTime <= innerItem->m_endTime) ||
                    (innerItem->m_startTime >= item->m_startTime && innerItem->m_startTime <= item->m_endTime) ||
                    (innerItem->m_endTime >= item->m_startTime && innerItem->m_endTime <= item->m_endTime))
                {
                    intermediateResult++;
                    trackFlags[innerItem->m_trackIndex] = true;
                }
            }

            result = AZStd::max(result, intermediateResult);
        }

        return result;
    }


    size_t Recorder::CalcMaxNumActiveMotions() const
    {
        size_t result = 0;

        for (const ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            result = AZStd::max(result, CalcMaxNumActiveMotions(*actorInstanceData));
        }

        return result;
    }
} // namespace EMotionFX
