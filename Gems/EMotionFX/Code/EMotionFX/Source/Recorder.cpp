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
    AZ_CLASS_ALLOCATOR_IMPL(Recorder, RecorderAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(Recorder::ActorInstanceData, RecorderAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(Recorder::RecordSettings, RecorderAllocator, 0)


    void Recorder::TransformTracks::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Recorder::TransformTracks>()
            ->Version(1)
            ->Field("positions", &Recorder::TransformTracks::mPositions)
            ->Field("rotations", &Recorder::TransformTracks::mRotations)
#ifndef EMFX_SCALE_DISABLED
            ->Field("scales", &Recorder::TransformTracks::mScales)
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
            ->Field("fps", &Recorder::RecordSettings::mFPS)
            ->Field("recordTransforms", &Recorder::RecordSettings::mRecordTransforms)
            ->Field("recordNodeHistory", &Recorder::RecordSettings::mRecordNodeHistory)
            ->Field("historyStatesOnly", &Recorder::RecordSettings::mHistoryStatesOnly)
            ->Field("recordAnimGraphStates", &Recorder::RecordSettings::mRecordAnimGraphStates)
            ->Field("recordEvents", &Recorder::RecordSettings::mRecordEvents)
            ->Field("recordScale", &Recorder::RecordSettings::mRecordScale)
            ->Field("recordMorphs", &Recorder::RecordSettings::mRecordMorphs)
            ->Field("interpolate", &Recorder::RecordSettings::mInterpolate)
            ;
    }

    Recorder::Recorder()
        : BaseObject()
    {
        mIsInPlayMode   = false;
        mIsRecording    = false;
        mAutoPlay       = false;
        mRecordTime     = 0.0f;
        mLastRecordTime = 0.0f;
        mCurrentPlayTime = 0.0f;

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
            ->Field("settings", &Recorder::mRecordSettings)
            ;
    }

    // enable or disable auto play mode
    void Recorder::SetAutoPlay(bool enabled)
    {
        mAutoPlay = enabled;
    }


    // set the current play time
    void Recorder::SetCurrentPlayTime(float timeInSeconds)
    {
        mCurrentPlayTime = timeInSeconds;

        if (mCurrentPlayTime < 0.0f)
        {
            mCurrentPlayTime = 0.0f;
        }

        if (mCurrentPlayTime > mRecordTime)
        {
            mCurrentPlayTime = mRecordTime;
        }
    }


    // start playback mode
    void Recorder::StartPlayBack()
    {
        StopRecording();
        mIsInPlayMode = true;
    }


    // stop playback mode
    void Recorder::StopPlayBack()
    {
        mIsInPlayMode = false;
    }


    // rewind the playback
    void Recorder::Rewind()
    {
        mCurrentPlayTime = 0.0f;
    }

    bool Recorder::HasRecording() const
    {
        return (GetRecordTime() > AZ::Constants::FloatEpsilon && !m_actorInstanceDatas.empty());
    }

    // clear any currently recorded data
    void Recorder::Clear()
    {
        Lock();

        mIsInPlayMode = false;
        mIsRecording = false;
        mAutoPlay = false;
        mRecordTime = 0.0f;
        mLastRecordTime = 0.0f;
        mCurrentPlayTime = 0.0f;
        mRecordSettings.m_actorInstances.clear();
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
        mRecordSettings = settings;
        mIsRecording    = true;

        // Add all actor instances if we did not specify them explicitly.
        if (mRecordSettings.m_actorInstances.empty())
        {
            const size_t numActorInstances = GetActorManager().GetNumActorInstances();
            mRecordSettings.m_actorInstances.resize(numActorInstances);
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
                mRecordSettings.m_actorInstances[i] = actorInstance;
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
        if (mIsInPlayMode && mAutoPlay)
        {
            SetCurrentPlayTime(mCurrentPlayTime + timeDelta);
        }
    }


    // update the recorder
    void Recorder::Update(float timeDelta)
    {
        Lock();

        // if we are not recording there is nothing to do
        if (mIsRecording == false)
        {
            Unlock();
            return;
        }

        // increase the time we record
        mRecordTime += timeDelta;

        // save a sample when more time passed than the desired sample rate
        const float sampleRate = 1.0f / (float)mRecordSettings.mFPS;
        if (mRecordTime - mLastRecordTime >= sampleRate)
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

        mIsRecording = false;
        FinalizeAllNodeHistoryItems();

        if (lock)
        {
            Unlock();
        }
    }


    // prepare for recording by resizing and preallocating space/arrays
    void Recorder::PrepareForRecording()
    {
        const size_t numActorInstances = mRecordSettings.m_actorInstances.size();
        m_actorInstanceDatas.resize(numActorInstances);
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            m_actorInstanceDatas[i] = aznew ActorInstanceData();
            ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[i];

            // link it to the right actor instance
            ActorInstance* actorInstance = mRecordSettings.m_actorInstances[i];
            actorInstanceData.mActorInstance = actorInstance;

            // add the transform tracks
            if (mRecordSettings.mRecordTransforms)
            {
                // for all nodes in the actor instance
                const size_t numNodes = actorInstance->GetNumNodes();
                actorInstanceData.m_transformTracks.resize(numNodes);
                for (size_t n = 0; n < numNodes; ++n)
                {
                    actorInstanceData.m_transformTracks[n].mPositions.Reserve(mRecordSettings.mNumPreAllocTransformKeys);
                    actorInstanceData.m_transformTracks[n].mRotations.Reserve(mRecordSettings.mNumPreAllocTransformKeys);

                    EMFX_SCALECODE
                    (
                        if (mRecordSettings.mRecordScale)
                        {
                            actorInstanceData.m_transformTracks[n].mScales.Reserve(mRecordSettings.mNumPreAllocTransformKeys);
                        }
                    )
                }
            } // if recording transforms

            // if recording morph targets, resize the morphs array
            if (mRecordSettings.mRecordMorphs)
            {
                const size_t numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
                actorInstanceData.mMorphTracks.resize(numMorphs);
                for (size_t m = 0; m < numMorphs; ++m)
                {
                    actorInstanceData.mMorphTracks[m].Reserve(256);
                }
            }

            // add the animgraph data
            if (mRecordSettings.mRecordAnimGraphStates)
            {
                if (actorInstance->GetAnimGraphInstance())
                {
                    actorInstanceData.mAnimGraphData = new AnimGraphInstanceData();
                    actorInstanceData.mAnimGraphData->mAnimGraphInstance = actorInstance->GetAnimGraphInstance();
                    if (mRecordSettings.mInitialAnimGraphAnimBytes > 0)
                    {
                        actorInstanceData.mAnimGraphData->mDataBuffer  = (uint8*)MCore::Allocate(mRecordSettings.mInitialAnimGraphAnimBytes, EMFX_MEMCATEGORY_RECORDER);
                    }

                    actorInstanceData.mAnimGraphData->mDataBufferSize  = mRecordSettings.mInitialAnimGraphAnimBytes;
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
            const ActorInstance* actorInstance = actorInstanceData->mActorInstance;
            if (actorInstanceData->m_transformTracks.empty())
            {
                continue;
            }

            const size_t numNodes = actorInstance->GetNumNodes();
            for (size_t n = 0; n < numNodes; ++n)
            {
                actorInstanceData->m_transformTracks[n].mPositions.Shrink();
                actorInstanceData->m_transformTracks[n].mRotations.Shrink();

                EMFX_SCALECODE
                (
                    actorInstanceData->m_transformTracks[n].mScales.Shrink();
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
        if (mRecordSettings.mRecordTransforms)
        {
            RecordCurrentTransforms();
        }

        // record the current anim graph states
        if (mRecordSettings.mRecordAnimGraphStates)
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
        if (mRecordSettings.mRecordMorphs)
        {
            RecordMorphs();
        }

        // update (while recording) the node history items
        if (mRecordSettings.mRecordNodeHistory)
        {
            UpdateNodeHistoryItems();
        }

        // recordo the events
        if (mRecordSettings.mRecordEvents)
        {
            RecordEvents();
        }

        // update the last record time
        mLastRecordTime = mRecordTime;
    }


    // record the morph weights
    void Recorder::RecordMorphs()
    {
        // for all actor instances
        const size_t numActorInstances = m_actorInstanceDatas.size();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[i];
            ActorInstance* actorInstance = actorInstanceData.mActorInstance;

            const size_t numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
            for (size_t m = 0; m < numMorphs; ++m)
            {
                KeyTrackLinearDynamic<float, float>& morphTrack = actorInstanceData.mMorphTracks[i]; // morph animation data
                morphTrack.AddKey(mRecordTime, actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->GetWeight());
            }
        }
    }


    // record all actor instance main transformations, so not of the nodes, but of the actor instance itself
    void Recorder::RecordMainLocalTransforms()
    {
        // for all actor instances
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            ActorInstance* actorInstance = actorInstanceData->mActorInstance;
            const Transform& transform = actorInstance->GetLocalSpaceTransform();

        #ifndef EMFX_SCALE_DISABLED
            AddTransformKey(actorInstanceData->mActorLocalTransform, transform.mPosition, transform.mRotation, transform.mScale);
        #else
            AddTransformKey(actorInstanceData->mActorLocalTransform, transform.mPosition, transform.mRotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
        #endif
        }
    }


    // record current transforms
    void Recorder::RecordCurrentTransforms()
    {
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            ActorInstance* actorInstance = actorInstanceData->mActorInstance;

            const TransformData* transformData = actorInstance->GetTransformData();
            {
                const size_t numNodes = actorInstance->GetNumNodes();
                for (size_t n = 0; n < numNodes; ++n)
                {
                    const Transform& localTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);

                #ifndef EMFX_SCALE_DISABLED
                    AddTransformKey(actorInstanceData->m_transformTracks[n], localTransform.mPosition, localTransform.mRotation, localTransform.mScale);
                #else
                    AddTransformKey(actorInstanceData->m_transformTracks[n], localTransform.mPosition, localTransform.mRotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
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
            if (actorInstanceData->mAnimGraphData == nullptr)
            {
                continue;
            }

            {
                // get some shortcuts
                AnimGraphInstanceData& animGraphInstanceData  = *actorInstanceData->mAnimGraphData;
                AnimGraphInstance*     animGraphInstance      = animGraphInstanceData.mAnimGraphInstance;
                const AnimGraph*       animGraph              = animGraphInstance->GetAnimGraph();

                // add a new frame
                AZStd::vector<AnimGraphAnimFrame>& frames = animGraphInstanceData.mFrames;
                if (!frames.empty())
                {
                    const size_t byteOffset = frames.back().mByteOffset + frames.back().mNumBytes;
                    frames.emplace_back();
                    frames.back().mByteOffset    = byteOffset;
                    frames.back().mNumBytes      = 0;
                }
                else
                {
                    frames.emplace_back();
                    frames.back().mByteOffset    = 0;
                    frames.back().mNumBytes      = 0;
                }

                // get the current frame
                AnimGraphAnimFrame& currentFrame = frames.back();
                currentFrame.mTimeValue = mRecordTime;

                // save the parameter values
                const size_t numParams = animGraphInstance->GetAnimGraph()->GetNumValueParameters();
                currentFrame.mParameterValues.resize(numParams);
                for (size_t p = 0; p < numParams; ++p)
                {
                    currentFrame.mParameterValues[p] = AZStd::unique_ptr<MCore::Attribute>(animGraphInstance->GetParameterValue(p)->Clone());
                }

                // recursively save all unique datas
                if (!SaveUniqueData(animGraphInstance, animGraph->GetRootStateMachine(), animGraphInstanceData))
                {
                    return false;
                }

                // increase the frames counter
                animGraphInstanceData.mNumFrames++;

                //MCore::LogInfo("Frame %d = %d bytes (offset=%d), with %d objects - dataBuffer = %d kb", animGraphInstanceData.mNumFrames, frames.GetLast().mNumBytes, frames.GetLast().mByteOffset, frames.GetLast().mObjectInfos.GetLength(), animGraphInstanceData.mDataBufferSize / 1024);
            }
        } // for all actor instances
        return true;
    }


    // recursively save the node's unique data
    bool Recorder::SaveUniqueData(AnimGraphInstance* animGraphInstance, AnimGraphObject* object, AnimGraphInstanceData& animGraphInstanceData)
    {
        // get the current frame's data pointer
        AnimGraphAnimFrame& currentFrame = animGraphInstanceData.mFrames.back();
        const size_t frameOffset = currentFrame.mByteOffset;

        // prepare the objects array
        mObjects.clear();
        mObjects.reserve(1024);

        // collect the objects we are going to save for this frame
        object->RecursiveCollectObjects(mObjects);

        // resize the object infos array
        const size_t numObjects = mObjects.size();
        currentFrame.mObjectInfos.resize(numObjects);

        // calculate how much memory we need for this frame
        size_t requiredFrameBytes = 0;
        for (const AnimGraphObject* animGraphObject : mObjects)
        {
            requiredFrameBytes += animGraphObject->SaveUniqueData(animGraphInstance, nullptr);
        }

        // make sure we have at least the given amount of space in the buffer we are going to write the frame data to
        if (!AssureAnimGraphBufferSize(animGraphInstanceData, requiredFrameBytes + currentFrame.mByteOffset))
        {
            return false;
        }
        uint8* dataPointer = &animGraphInstanceData.mDataBuffer[frameOffset];

        // save all the unique datas for the objects
        for (size_t i = 0; i < numObjects; ++i)
        {
            // store the object info
            AnimGraphObject* curObject = mObjects[i];
            currentFrame.mObjectInfos[i].mObject = curObject;
            currentFrame.mObjectInfos[i].mFrameByteOffset = currentFrame.mNumBytes;

            // write the unique data
            const size_t numBytesWritten = curObject->SaveUniqueData(animGraphInstance, dataPointer);

            // increase some offsets/pointers
            currentFrame.mNumBytes += numBytesWritten;
            dataPointer += numBytesWritten;
        }

        // make sure we have a match here, otherwise some of the object->SaveUniqueData(dataPointer) returns different values than object->SaveUniqueData(nullptr)
        MCORE_ASSERT(requiredFrameBytes == currentFrame.mNumBytes);

        return true;
    }


    // make sure our anim graph anim buffer is big enough to hold a specified amount of bytes
    bool Recorder::AssureAnimGraphBufferSize(AnimGraphInstanceData& animGraphInstanceData, size_t numBytes)
    {
        // if the buffer is big enough, do nothing
        if (animGraphInstanceData.mDataBufferSize >= numBytes)
        {
            return true;
        }

        // we need to reallocate to grow the buffer
        const size_t newNumBytes = animGraphInstanceData.mDataBufferSize + (numBytes - animGraphInstanceData.mDataBufferSize) * 100; // allocate 100 frames ahead
        void* newBuffer = MCore::Realloc(animGraphInstanceData.mDataBuffer, newNumBytes, EMFX_MEMCATEGORY_RECORDER);
        MCORE_ASSERT(newBuffer);
        if (newBuffer)
        {
            animGraphInstanceData.mDataBuffer = static_cast<uint8*>(newBuffer);
            animGraphInstanceData.mDataBufferSize = newNumBytes;
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
        if (track.mPositions.GetNumKeys() > 0)
        {
            const AZ::Vector3 lastPos = track.mPositions.GetLastKey()->GetValue();
            if (!pos.IsClose(lastPos, 0.0001f))
            {
                track.mPositions.AddKey(mRecordTime, pos);
            }
        }
        else
        {
            track.mPositions.AddKey(mRecordTime, pos);
        }


        // check if we need to add a rotation key at all
        if (track.mRotations.GetNumKeys() > 0)
        {
            const AZ::Quaternion lastRot = track.mRotations.GetLastKey()->GetValue();
            if (!rot.IsClose(lastRot, 0.0001f))
            {
                track.mRotations.AddKey(mRecordTime, rot);
            }
        }
        else
        {
            track.mRotations.AddKey(mRecordTime, rot);
        }


        EMFX_SCALECODE
        (
            if (mRecordSettings.mRecordScale)
            {
                // check if we need to add a scale key
                if (track.mScales.GetNumKeys() > 0)
                {
                    const AZ::Vector3 lastScale = track.mScales.GetLastKey()->GetValue();
                    if (!scale.IsClose(lastScale, 0.0001f))
                    {
                        track.mScales.AddKey(mRecordTime, scale);
                    }
                }
                else
                {
                    track.mScales.AddKey(mRecordTime, scale);
                }
            }
        )
    }

    void Recorder::SampleAndApplyTransforms(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = mRecordSettings.m_actorInstances;
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
            if(actorInstanceData->mAnimGraphData)
            {
                SampleAndApplyAnimGraphStates(timeInSeconds, *actorInstanceData->mAnimGraphData);
            }
        }
    }

    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = mRecordSettings.m_actorInstances;
        const auto iterator = AZStd::find(recordedActorInstances.begin(), recordedActorInstances.end(), actorInstance);
        if (iterator != recordedActorInstances.end())
        {
            const size_t index = iterator - recordedActorInstances.begin();
            SampleAndApplyMainTransform(timeInSeconds, index);
        }
    }

    void Recorder::SampleAndApplyMorphs(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const AZStd::vector<ActorInstance*>& recordedActorInstances = mRecordSettings.m_actorInstances;
        const auto iterator = AZStd::find(recordedActorInstances.begin(), recordedActorInstances.end(), actorInstance);
        if (iterator != recordedActorInstances.end())
        {
            const size_t index = AZStd::distance(recordedActorInstances.begin(), iterator);
            const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[index];
            const size_t numMorphs = actorInstanceData.mMorphTracks.size();
            if (numMorphs == actorInstance->GetMorphSetupInstance()->GetNumMorphTargets())
            {
                for (size_t i = 0; i < numMorphs; ++i)
                {
                    actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->SetWeight(actorInstanceData.mMorphTracks[i].GetValueAtTime(timeInSeconds));
                }
            }
        }
    }

    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, size_t actorInstanceIndex) const
    {
        // get the actor instance
        const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.mActorInstance;

        // sample and apply
        const TransformTracks& track = actorInstanceData.mActorLocalTransform;
        actorInstance->SetLocalSpacePosition(track.mPositions.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate));
        actorInstance->SetLocalSpaceRotation(track.mRotations.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate));
        EMFX_SCALECODE
        (
            if (mRecordSettings.mRecordScale)
            {
                actorInstance->SetLocalSpaceScale(track.mScales.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate));
            }
        )
    }

    void Recorder::SampleAndApplyTransforms(float timeInSeconds, size_t actorInstanceIndex) const
    {
        const ActorInstanceData& actorInstanceData = *m_actorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.mActorInstance;
        TransformData* transformData = actorInstance->GetTransformData();

        // for all nodes in the actor instance
        Transform outTransform;
        const size_t numNodes = actorInstance->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            outTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);
            const TransformTracks& track = actorInstanceData.m_transformTracks[n];

            // build the output transform by sampling the keytracks
            outTransform.mPosition = track.mPositions.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate);
            outTransform.mRotation = track.mRotations.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate);

            EMFX_SCALECODE
            (
                if (mRecordSettings.mRecordScale)
                {
                    outTransform.mScale = track.mScales.GetValueAtTime(timeInSeconds, nullptr, nullptr, mRecordSettings.mInterpolate);
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
        AnimGraphInstance* animGraphInstance = animGraphInstanceData.mAnimGraphInstance;

        // get the real frame number (clamped)
        const size_t realFrameNumber = AZStd::min(frameNumber, animGraphInstanceData.mFrames.size() - 1);
        const AnimGraphAnimFrame& currentFrame = animGraphInstanceData.mFrames[realFrameNumber];

        // get the data and objects buffers
        const size_t byteOffset = currentFrame.mByteOffset;
        const uint8* frameDataBuffer = &animGraphInstanceData.mDataBuffer[byteOffset];
        const AZStd::vector<AnimGraphAnimObjectInfo>& frameObjects = currentFrame.mObjectInfos;

        // first lets update all parameter values
        MCORE_ASSERT(currentFrame.mParameterValues.size() == animGraphInstance->GetAnimGraph()->GetNumParameters());
        const size_t numParameters = currentFrame.mParameterValues.size();
        for (size_t p = 0; p < numParameters; ++p)
        {
            //  make sure the parameters are of the same type
            MCORE_ASSERT(animGraphInstance->GetParameterValue(p)->GetType() == currentFrame.mParameterValues[p]->GetType());
            animGraphInstance->GetParameterValue(p)->InitFrom(currentFrame.mParameterValues[p].get());
        }

        // process all objects for this frame
        size_t totalBytesRead = 0;
        const size_t numObjects = frameObjects.size();
        for (size_t a = 0; a < numObjects; ++a)
        {
            const AnimGraphAnimObjectInfo& objectInfo = frameObjects[a];
            const size_t numBytesRead = objectInfo.mObject->LoadUniqueData(animGraphInstance, &frameDataBuffer[objectInfo.mFrameByteOffset]);
            totalBytesRead += numBytesRead;
        }

        // make sure this matches, otherwise the data read is not the same as we have written
        MCORE_ASSERT(totalBytesRead == currentFrame.mNumBytes);
    }

    bool Recorder::GetHasRecorded(ActorInstance* actorInstance) const
    {
        return AZStd::find(mRecordSettings.m_actorInstances.begin(), mRecordSettings.m_actorInstances.end(), actorInstance)
            != mRecordSettings.m_actorInstances.end();
    }

    size_t Recorder::FindActorInstanceDataIndex(ActorInstance* actorInstance) const
    {
        const auto found = AZStd::find_if(begin(m_actorInstanceDatas), end(m_actorInstanceDatas), [actorInstance](const ActorInstanceData* data)
        {
            return data->mActorInstance == actorInstance;
        });
        return found != end(m_actorInstanceDatas) ? AZStd::distance(begin(m_actorInstanceDatas), found) : InvalidIndex;
    }

    void Recorder::UpdateNodeHistoryItems()
    {
        // for all actor instances
        for (ActorInstanceData* actorInstanceData : m_actorInstanceDatas)
        {
            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData->mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&mActiveNodes);

            // get the history items as shortcut
            AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData->mNodeHistoryItems;

            // finalize items
            for (NodeHistoryItem* curItem : historyItems)
            {
                if (curItem->mIsFinalized)
                {
                    continue;
                }

                // check if we have an active node for the given item
                const bool haveActiveNode = AZStd::find_if(begin(mActiveNodes), end(mActiveNodes), [curItem](const AnimGraphNode* activeNode)
                {
                    return activeNode->GetId() == curItem->mNodeId;
                }) != end(mActiveNodes);

                // the node got deactivated, finalize the item
                if (haveActiveNode)
                {
                    curItem->mGlobalWeights.Optimize(0.0001f);
                    curItem->mLocalWeights.Optimize(0.0001f);
                    curItem->mPlayTimes.Optimize(0.0001f);
                    curItem->mIsFinalized = true;
                    curItem->mEndTime = mRecordTime;
                    continue;
                }
            }

            // iterate over all active nodes
            for (const AnimGraphNode* activeNode : mActiveNodes)
            {
                if (activeNode == animGraphInstance->GetRootNode()) // skip the root node
                {
                    continue;
                }

                const AZ::TypeId typeID = azrtti_typeid(activeNode);

                // if the parent isn't a state machine then it isn't a state
                if (mRecordSettings.mHistoryStatesOnly)
                {
                    if (azrtti_typeid(activeNode->GetParentNode()) != azrtti_typeid<AnimGraphStateMachine>())
                    {
                        continue;
                    }
                }

                // make sure this node is on our capture list
                if (!mRecordSettings.mNodeHistoryTypes.empty())
                {
                    if (mRecordSettings.mNodeHistoryTypes.find(typeID) == mRecordSettings.mNodeHistoryTypes.end())
                    {
                        continue;
                    }
                }

                // skip node types we do not want to capture
                if (!mRecordSettings.mNodeHistoryTypesToIgnore.empty())
                {
                    if (mRecordSettings.mNodeHistoryTypesToIgnore.find(typeID) != mRecordSettings.mNodeHistoryTypesToIgnore.end())
                    {
                        continue;
                    }
                }

                // try to locate an existing item
                NodeHistoryItem* item = FindNodeHistoryItem(*actorInstanceData, activeNode, mRecordTime);
                if (item == nullptr)
                {
                    item = new NodeHistoryItem();
                    item->mName             = activeNode->GetName();
                    item->mAnimGraphID      = animGraphInstance->GetAnimGraph()->GetID();
                    item->mStartTime        = mRecordTime;
                    item->mIsFinalized      = false;
                    item->mTrackIndex       = FindFreeNodeHistoryItemTrack(*actorInstanceData, item);
                    item->mNodeId           = activeNode->GetId();
                    item->mColor            = activeNode->GetVisualizeColor();
                    item->mTypeColor        = activeNode->GetVisualColor();
                    item->mCategoryID       = (uint32)activeNode->GetPaletteCategory();
                    item->mNodeType         = typeID;
                    item->mAnimGraphInstance = animGraphInstance;
                    item->mGlobalWeights.Reserve(1024);
                    item->mLocalWeights.Reserve(1024);
                    item->mPlayTimes.Reserve(1024);

                    // get the motion instance
                    if (typeID == azrtti_typeid<AnimGraphMotionNode>())
                    {
                        const AnimGraphMotionNode* motionNode = static_cast<const AnimGraphMotionNode*>(activeNode);
                        MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
                        if (motionInstance)
                        {
                            item->mMotionID = motionInstance->GetMotion()->GetID();
                            AzFramework::StringFunc::Path::GetFileName(item->mMotionFileName.c_str(), item->mMotionFileName);
                        }
                    }

                    historyItems.emplace_back(item);
                }

                // add the weight key and update infos
                const AnimGraphNodeData* uniqueData = activeNode->FindOrCreateUniqueNodeData(animGraphInstance);
                const float keyTime = mRecordTime - item->mStartTime;
                item->mGlobalWeights.AddKey(keyTime, uniqueData->GetGlobalWeight());
                item->mLocalWeights.AddKey(keyTime, uniqueData->GetLocalWeight());

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

                item->mPlayTimes.AddKey(keyTime, normalizedTime);
                item->mEndTime  = mRecordTime;
            }
        } // for all actor instances
    }


    // try to find a given node history item
    Recorder::NodeHistoryItem* Recorder::FindNodeHistoryItem(const ActorInstanceData& actorInstanceData, const AnimGraphNode* node, float recordTime) const
    {
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        for (NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->mNodeId == node->GetId() && curItem->mStartTime <= recordTime && curItem->mIsFinalized == false)
            {
                return curItem;
            }

            if (curItem->mNodeId == node->GetId() && curItem->mStartTime <= recordTime && curItem->mEndTime >= recordTime && curItem->mIsFinalized)
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // find a free track
    size_t Recorder::FindFreeNodeHistoryItemTrack(const ActorInstanceData& actorInstanceData, NodeHistoryItem* item) const
    {
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;

        bool found = false;
        size_t trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (const NodeHistoryItem* curItem : historyItems)
            {
                 if (curItem->mTrackIndex != trackIndex)
                {
                    continue;
                }

                // if the current item is not active anymore
                if (curItem->mIsFinalized)
                {
                    // if the start time of the item we try to insert is within the range of this item
                    if (item->mStartTime > curItem->mStartTime && item->mStartTime < curItem->mEndTime)
                    {
                        hasCollision = true;
                        break;
                    }

                    // if the end time of this item is within the range of this item
                    if (item->mEndTime > curItem->mStartTime && item->mEndTime < curItem->mEndTime)
                    {
                        hasCollision = true;
                        break;
                    }
                }
                else // if the current item is still active and has no real end time yet
                {
                    if (item->mStartTime >= curItem->mStartTime)
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
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        for (const NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->mTrackIndex > result)
            {
                result = curItem->mTrackIndex;
            }
        }

        return result;
    }


    // find the maximum event track index
    size_t Recorder::CalcMaxEventHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const
    {
        size_t result = 0;
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        for (const EventHistoryItem* curItem : historyItems)
        {
            if (curItem->mTrackIndex > result)
            {
                result = curItem->mTrackIndex;
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
            AnimGraphInstance* animGraphInstance = actorInstanceData->mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&mActiveNodes);

            // get the history items as shortcut
            const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData->mNodeHistoryItems;

            // finalize all items
            for (NodeHistoryItem* historyItem : historyItems)
            {
                // remove unneeded key frames
                if (historyItem->mIsFinalized == false)
                {
                    historyItem->mGlobalWeights.Optimize(0.0001f);
                    historyItem->mLocalWeights.Optimize(0.0001f);
                    historyItem->mPlayTimes.Optimize(0.0001f);
                    historyItem->mIsFinalized = true;
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
            AnimGraphInstance* animGraphInstance = actorInstanceData->mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // get the event buffer
            const AnimGraphEventBuffer& eventBuffer = animGraphInstance->GetEventBuffer();

            // iterate over all events
            AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData->mEventHistoryItems;
            const size_t numEvents = eventBuffer.GetNumEvents();
            for (size_t i = 0; i < numEvents; ++i)
            {
                const EventInfo& eventInfo = eventBuffer.GetEvent(i);
                if (eventInfo.m_eventState == EventInfo::EventInfo::ACTIVE)
                {
                    continue;
                }
                EventHistoryItem* item = FindEventHistoryItem(*actorInstanceData, eventInfo, mRecordTime);
                if (item == nullptr) // create a new one
                {
                    item = new EventHistoryItem();

                    // TODO
                    //item->mEventIndex   = GetEventManager().FindEventTypeIndex(eventInfo.mTypeID);
                    item->mEventInfo    = eventInfo;
                    item->mIsTickEvent  = eventInfo.mEvent->GetIsTickEvent();
                    item->mStartTime    = mRecordTime;
                    item->mAnimGraphID  = animGraphInstance->GetAnimGraph()->GetID();
                    item->mEmitterNodeId= eventInfo.mEmitter->GetId();
                    item->mColor        = eventInfo.mEmitter->GetVisualizeColor();

                    item->mTrackIndex   = FindFreeEventHistoryItemTrack(*actorInstanceData, item);

                    historyItems.emplace_back(item);
                }

                item->mEndTime  = mRecordTime;
            }
        }
    }


    // find a given event
    Recorder::EventHistoryItem* Recorder::FindEventHistoryItem(const ActorInstanceData& actorInstanceData, const EventInfo& eventInfo, float recordTime)
    {
        MCORE_UNUSED(recordTime);
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        for (const EventHistoryItem* curItem : historyItems)
        {
            if (curItem->mStartTime < eventInfo.mTimeValue)
            {
                continue;
            }
        }

        return nullptr;
    }


    // find a free event track index
    size_t Recorder::FindFreeEventHistoryItemTrack(const ActorInstanceData& actorInstanceData, EventHistoryItem* item) const
    {
        const AZStd::vector<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        bool found = false;
        size_t trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (const EventHistoryItem* curItem : historyItems)
            {
                if (curItem->mTrackIndex != trackIndex)
                {
                    continue;
                }

                if (MCore::Compare<float>::CheckIfIsClose(curItem->mStartTime, item->mStartTime, 0.01f))
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
        const AnimGraphInstanceData* animGraphData = actorInstanceData.mAnimGraphData;
        if (animGraphData == nullptr)
        {
            return InvalidIndex;
        }

        const size_t numFrames = animGraphData->mFrames.size();
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

        if (timeValue > animGraphData->mFrames.back().mTimeValue)
        {
            return animGraphData->mFrames.size() - 1;
        }

        for (size_t i = 0; i < numFrames - 1; ++i)
        {
            const AnimGraphAnimFrame& curFrame  = animGraphData->mFrames[i];
            const AnimGraphAnimFrame& nextFrame = animGraphData->mFrames[i + 1];
            if (curFrame.mTimeValue <= timeValue && nextFrame.mTimeValue > timeValue)
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
        AZStd::vector<ActorInstance*>& recordedActorInstances = mRecordSettings.m_actorInstances;
        recordedActorInstances.erase(AZStd::remove_if(recordedActorInstances.begin(), recordedActorInstances.end(),
            [&actorInstance](ActorInstance* recordedActorInstance){ return recordedActorInstance == actorInstance;}),
            recordedActorInstances.end());

        // Remove the actual recorded data.
        for (size_t i = 0; i < m_actorInstanceDatas.size();)
        {
            if (m_actorInstanceDatas[i]->mActorInstance == actorInstance)
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
            if (actorInstanceData->mAnimGraphData && actorInstanceData->mAnimGraphData->mAnimGraphInstance)
            {
                AnimGraph* curAnimGraph = actorInstanceData->mAnimGraphData->mAnimGraphInstance->GetAnimGraph();
                if (animGraph != curAnimGraph)
                {
                    continue;
                }

                delete actorInstanceData->mAnimGraphData;
                actorInstanceData->mAnimGraphData = nullptr;
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
        mLock.Lock();
    }

    void Recorder::Unlock()
    {
        mLock.Unlock();
    }


    // extract sorted active items
    void Recorder::ExtractNodeHistoryItems(const ActorInstanceData& actorInstanceData, float timeValue, bool sort, EValueType valueType, AZStd::vector<ExtractedNodeHistoryItem>* outItems, AZStd::vector<size_t>* outMap) const
    {
        // clear the map array
        const size_t maxIndex = CalcMaxNodeHistoryTrackIndex(actorInstanceData);
        outItems->resize(maxIndex + 1);
        for (size_t i = 0; i <= maxIndex; ++i)
        {
            ExtractedNodeHistoryItem item;
            item.mTrackIndex        = i;
            item.mValue             = 0.0f;
            item.mKeyTrackSampleTime = 0.0f;
            item.mNodeHistoryItem   = nullptr;
            outItems->emplace(AZStd::next(begin(*outItems), i), AZStd::move(item));
        }

        // find all node history items
        const AZStd::vector<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        for (NodeHistoryItem* curItem : historyItems)
        {
            if (curItem->mStartTime <= timeValue && curItem->mEndTime > timeValue)
            {
                ExtractedNodeHistoryItem item;
                item.mTrackIndex        = curItem->mTrackIndex;
                item.mKeyTrackSampleTime = timeValue - curItem->mStartTime;
                item.mNodeHistoryItem   = curItem;

                switch (valueType)
                {
                case VALUETYPE_GLOBALWEIGHT:
                    item.mValue = curItem->mGlobalWeights.GetValueAtTime(item.mKeyTrackSampleTime, nullptr, nullptr, mRecordSettings.mInterpolate);
                    break;

                case VALUETYPE_LOCALWEIGHT:
                    item.mValue = curItem->mLocalWeights.GetValueAtTime(item.mKeyTrackSampleTime, nullptr, nullptr, mRecordSettings.mInterpolate);
                    break;

                case VALUETYPE_PLAYTIME:
                    item.mValue = curItem->mPlayTimes.GetValueAtTime(item.mKeyTrackSampleTime, nullptr, nullptr, mRecordSettings.mInterpolate);
                    break;

                default:
                    MCORE_ASSERT(false);    // unsupported mode
                    item.mValue = curItem->mGlobalWeights.GetValueAtTime(item.mKeyTrackSampleTime, nullptr, nullptr, mRecordSettings.mInterpolate);
                }

                outItems->emplace(AZStd::next(begin(*outItems), curItem->mTrackIndex), item);
            }
        }

        // build the map
        outMap->resize(maxIndex + 1);
        for (size_t i = 0; i <= maxIndex; ++i)
        {
            outMap->emplace(AZStd::next(begin(*outMap), i), i);
        }

        // sort if desired
        if (sort)
        {
            AZStd::sort(begin(*outItems), end(*outItems));

            for (size_t i = 0; i <= maxIndex; ++i)
            {
                outMap->emplace(AZStd::next(begin(*outMap), outItems->at(i).mTrackIndex), i);
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

        const size_t numNodeHistoryItems = actorInstanceData.mNodeHistoryItems.size();
        for (size_t i = 0; i < numNodeHistoryItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* item = actorInstanceData.mNodeHistoryItems[i];

            // Only process motion history items.
            if (item->mMotionID == MCORE_INVALIDINDEX32)
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
            trackFlags[item->mTrackIndex] = true;

            for (size_t j = 0; j < numNodeHistoryItems; ++j)
            {
                EMotionFX::Recorder::NodeHistoryItem* innerItem = actorInstanceData.mNodeHistoryItems[j];

                // Did we count this track in already? If yes, skip.
                if (trackFlags[innerItem->mTrackIndex])
                {
                    continue;
                }

                // Skip self comparison and only process motion history items.
                if (i == j || innerItem->mMotionID == MCORE_INVALIDINDEX32)
                {
                    continue;
                }

                // Are the item and innerItem events overlapping?
                if ((item->mStartTime >= innerItem->mStartTime && item->mStartTime <= innerItem->mEndTime) ||
                    (item->mEndTime >= innerItem->mStartTime && item->mEndTime <= innerItem->mEndTime) ||
                    (innerItem->mStartTime >= item->mStartTime && innerItem->mStartTime <= item->mEndTime) ||
                    (innerItem->mEndTime >= item->mStartTime && innerItem->mEndTime <= item->mEndTime))
                {
                    intermediateResult++;
                    trackFlags[innerItem->mTrackIndex] = true;
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
