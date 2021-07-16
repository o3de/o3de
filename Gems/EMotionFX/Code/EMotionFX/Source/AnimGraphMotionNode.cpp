/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphMotionNode.h"
#include "MotionInstance.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "AnimGraphInstance.h"
#include "MotionSet.h"
#include "AnimGraphManager.h"
#include "MotionManager.h"
#include "MotionInstancePool.h"
#include "Motion.h"
#include "EMotionFXManager.h"
#include "MotionEventTable.h"
#include "AnimGraph.h"
#include <EMotionFX/Source/MotionData/MotionData.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    const float AnimGraphMotionNode::s_defaultWeight = 1.0f;

    AnimGraphMotionNode::AnimGraphMotionNode()
        : AnimGraphNode()
        , m_playSpeed(1.0f)
        , m_indexMode(INDEXMODE_RANDOMIZE)
        , m_loop(true)
        , m_retarget(true)
        , m_reverse(false)
        , m_emitEvents(true)
        , m_mirrorMotion(false)
        , m_motionExtraction(true)
        , m_nextMotionAfterLoop(false)
        , m_rewindOnZeroWeight(false)
        , m_inPlace(false)
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("Play Speed", INPUTPORT_PLAYSPEED, PORTID_INPUT_PLAYSPEED);
        SetupInputPortAsNumber("In Place", INPUTPORT_INPLACE, PORTID_INPUT_INPLACE);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
        SetupOutputPortAsMotionInstance("Motion", OUTPUTPORT_MOTION, PORTID_OUTPUT_MOTION);
    }


    AnimGraphMotionNode::~AnimGraphMotionNode()
    {
    }


    void AnimGraphMotionNode::Reinit()
    {
        OnMotionIdsChanged();
        AnimGraphNode::Reinit();
    }


    bool AnimGraphMotionNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* AnimGraphMotionNode::GetPaletteName() const
    {
        return "Motion";
    }


    AnimGraphObject::ECategory AnimGraphMotionNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    bool AnimGraphMotionNode::GetIsInPlace(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::BlendTreeConnection* inPlaceConnection = GetInputPort(INPUTPORT_INPLACE).mConnection;
        if (inPlaceConnection)
        {
            return GetInputNumberAsBool(animGraphInstance, INPUTPORT_INPLACE);
        }

        return m_inPlace;
    }

    void AnimGraphMotionNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the input nodes
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection && mDisabled == false)
        {
            playSpeedConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // clear the event buffer
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        // trigger the motion update
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance && !animGraphInstance->GetIsResynced(mObjectIndex))
        {
            // update the time values and extract events into the event buffer
            motionInstance->SetWeight(uniqueData->GetLocalWeight());
            motionInstance->UpdateByTimeValues(uniqueData->GetPreSyncTime(), uniqueData->GetCurrentPlayTime(), &data->GetEventBuffer());

            // mark all events to be emitted from this node
            data->GetEventBuffer().UpdateEmitters(this);
        }
        else
        {
            return;
        }

        // extract current delta
        Transform trajectoryDelta;
        const bool isMirrored = motionInstance->GetMirrorMotion();
        motionInstance->ExtractMotion(trajectoryDelta);
        data->SetTrajectoryDelta(trajectoryDelta);

        // extract mirrored version of the current delta
        motionInstance->SetMirrorMotion(!isMirrored);
        motionInstance->ExtractMotion(trajectoryDelta);
        data->SetTrajectoryDeltaMirrored(trajectoryDelta);

        // restore current mirrored flag
        motionInstance->SetMirrorMotion(isMirrored);
    }


    // top down update
    void AnimGraphMotionNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        // check if we have multiple motions in this node
        const size_t numMotions = GetNumMotions();
        if (numMotions > 1)
        {
            // check if we reached the end of the motion, if so, pick a new one
            if (uniqueData->mMotionInstance)
            {
                if (uniqueData->mMotionInstance->GetHasLooped() && m_nextMotionAfterLoop)
                {
                    PickNewActiveMotion(animGraphInstance, uniqueData);
                }
            }
        }

        // rewind when the weight reaches 0 when we want to
        if (!m_loop)
        {
            if (uniqueData->mMotionInstance && uniqueData->GetLocalWeight() < MCore::Math::epsilon && m_rewindOnZeroWeight)
            {
                uniqueData->mMotionInstance->SetCurrentTime(0.0f);
                uniqueData->SetCurrentPlayTime(0.0f);
                uniqueData->SetPreSyncTime(0.0f);
            }
        }

        // sync all input nodes
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        for (BlendTreeConnection* connection : mConnections)
        {
            connection->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // update the motion instance
    void AnimGraphMotionNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the input nodes
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection && mDisabled == false)
        {
            UpdateIncomingNode(animGraphInstance, playSpeedConnection->GetSourceNode(), timePassedInSeconds);
        }

        if (!mDisabled)
        {
            UpdateIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_INPLACE), timePassedInSeconds);
        }

        // update the motion instance (current time etc)
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance == nullptr || mDisabled)
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                if (mDisabled == false)
                {
                    if (motionInstance == nullptr)
                    {
                        SetHasError(uniqueData, true);
                    }
                }
            }

            uniqueData->Clear();
            return;
        }

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }

        // if there is a node connected to the speed input port, read that value and use it as internal speed if not use the playspeed property
        const float customSpeed = ExtractCustomPlaySpeed(animGraphInstance);

        // set the internal speed and play speeds etc
        motionInstance->SetPlaySpeed(uniqueData->GetPlaySpeed());
        uniqueData->SetPlaySpeed(customSpeed);
        uniqueData->SetPreSyncTime(motionInstance->GetCurrentTime());

        // Make sure we use the correct play properties.
        motionInstance->SetPlayMode(m_playInfo.mPlayMode);
        motionInstance->SetRetargetingEnabled(m_playInfo.mRetarget && animGraphInstance->GetRetargetingEnabled());
        motionInstance->SetMotionEventsEnabled(m_playInfo.mEnableMotionEvents);
        motionInstance->SetMirrorMotion(m_playInfo.mMirrorMotion);
        motionInstance->SetEventWeightThreshold(m_playInfo.mEventWeightThreshold);
        motionInstance->SetMaxLoops(m_playInfo.mNumLoops);
        motionInstance->SetMotionExtractionEnabled(m_playInfo.mMotionExtractionEnabled);
        motionInstance->SetIsInPlace(GetIsInPlace(animGraphInstance));
        motionInstance->SetFreezeAtLastFrame(m_playInfo.mFreezeAtLastFrame);

        if (!animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) || animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_IS_SYNCLEADER))
        {
            // See where we would end up when we would forward in time.
            const MotionInstance::PlayStateOut newPlayState = motionInstance->CalcPlayStateAfterUpdate(timePassedInSeconds);

            // set the current time to the new calculated time
            uniqueData->ClearInheritFlags();
            uniqueData->SetCurrentPlayTime(newPlayState.m_currentTime);
            motionInstance->SetLastCurrentTime(motionInstance->GetCurrentTime());
            motionInstance->SetCurrentTime(newPlayState.m_currentTime, false);
        }

        uniqueData->SetDuration(motionInstance->GetDuration());

        // make sure the motion is not paused
        motionInstance->SetPause(false);

        uniqueData->SetSyncTrack(motionInstance->GetMotion()->GetEventTable()->GetSyncTrack());
        uniqueData->SetIsMirrorMotion(motionInstance->GetMirrorMotion());

        // update some flags
        if (motionInstance->GetPlayMode() == PLAYMODE_BACKWARD)
        {
            uniqueData->SetBackwardFlag();
        }
    }


    void AnimGraphMotionNode::UpdatePlayBackInfo(AnimGraphInstance* animGraphInstance)
    {
        m_playInfo.mPlayMode                 = (m_reverse) ? PLAYMODE_BACKWARD : PLAYMODE_FORWARD;
        m_playInfo.mNumLoops                 = (m_loop) ? EMFX_LOOPFOREVER : 1;
        m_playInfo.mFreezeAtLastFrame        = true;
        m_playInfo.mEnableMotionEvents       = m_emitEvents;
        m_playInfo.mMirrorMotion             = m_mirrorMotion;
        m_playInfo.mPlaySpeed                = ExtractCustomPlaySpeed(animGraphInstance);
        m_playInfo.mMotionExtractionEnabled  = m_motionExtraction;
        m_playInfo.mRetarget                 = m_retarget;
        m_playInfo.mInPlace                  = GetIsInPlace(animGraphInstance);
    }


    // create the motion instance
    MotionInstance* AnimGraphMotionNode::CreateMotionInstance(ActorInstance* actorInstance, UniqueData* uniqueData)
    {
        AnimGraphInstance* animGraphInstance = uniqueData->GetAnimGraphInstance();

        // update the last motion ID
        UpdatePlayBackInfo(animGraphInstance);

        // try to find the motion to use for this actor instance in this blend node
        Motion*         motion      = nullptr;
        PlayBackInfo    playInfo    = m_playInfo;

        // reset playback properties
        const float curLocalWeight = uniqueData->GetLocalWeight();
        const float curGlobalWeight = uniqueData->GetGlobalWeight();
        uniqueData->Clear();

        // remove the motion instance if it already exists
        if (uniqueData->mMotionInstance && uniqueData->mReload)
        {
            GetMotionInstancePool().Free(uniqueData->mMotionInstance);
            uniqueData->mMotionInstance = nullptr;
            uniqueData->mMotionSetID    = MCORE_INVALIDINDEX32;
            uniqueData->mReload         = false;
        }

        // get the motion set
        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        if (!motionSet)
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return nullptr;
        }

        // get the motion from the motion set, load it on demand and make sure the motion loaded successfully
        if (uniqueData->mActiveMotionIndex != MCORE_INVALIDINDEX32)
        {
            motion = motionSet->RecursiveFindMotionById(GetMotionId(uniqueData->mActiveMotionIndex));
        }

        if (!motion)
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return nullptr;
        }

        uniqueData->mMotionSetID = motionSet->GetID();

        // create the motion instance
        MotionInstance* motionInstance = GetMotionInstancePool().RequestNew(motion, actorInstance);
        motionInstance->InitFromPlayBackInfo(playInfo, true);
        motionInstance->SetRetargetingEnabled(animGraphInstance->GetRetargetingEnabled() && playInfo.mRetarget);

        uniqueData->SetSyncTrack(motionInstance->GetMotion()->GetEventTable()->GetSyncTrack());
        uniqueData->SetIsMirrorMotion(motionInstance->GetMirrorMotion());


        // make sure it is not in pause mode
        motionInstance->UnPause();
        motionInstance->SetIsActive(true);
        motionInstance->SetWeight(1.0f, 0.0f);

        // update play info
        uniqueData->mMotionInstance = motionInstance;
        uniqueData->SetDuration(motionInstance->GetDuration());
        const float curPlayTime = motionInstance->GetCurrentTime();
        uniqueData->SetCurrentPlayTime(curPlayTime);
        uniqueData->SetPreSyncTime(curPlayTime);
        uniqueData->SetGlobalWeight(curGlobalWeight);
        uniqueData->SetLocalWeight(curLocalWeight);

        // trigger an event
        GetEventManager().OnStartMotionInstance(motionInstance, &playInfo);
        return motionInstance;
    }


    // the main process method of the final node
    void AnimGraphMotionNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // if this motion is disabled, output the bind pose
        if (mDisabled)
        {
            // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // output the playspeed node
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection)
        {
            OutputIncomingNode(animGraphInstance, playSpeedConnection->GetSourceNode());
        }

        // create and register the motion instance when this is the first time its being when it hasn't been registered yet
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        MotionInstance* motionInstance = nullptr;
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->mReload)
        {
            motionInstance = CreateMotionInstance(actorInstance, uniqueData);
            uniqueData->mReload = false;
        }
        else
        {
            motionInstance = uniqueData->mMotionInstance;
        }

        // update the motion instance output port
        GetOutputMotionInstance(animGraphInstance, OUTPUTPORT_MOTION)->SetValue(motionInstance);

        if (motionInstance == nullptr)
        {
            // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return;
        }

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }

        EMotionFX::BlendTreeConnection* inPlaceConnection = GetInputPort(INPUTPORT_INPLACE).mConnection;
        if (inPlaceConnection)
        {
            OutputIncomingNode(animGraphInstance, inPlaceConnection->GetSourceNode());
        }

        // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        Pose& outputTransformPose = outputPose->GetPose();

        // fill the output with the bind pose
        outputPose->InitFromBindPose(actorInstance); // TODO: is this really needed?

        // we use as input pose the same as the output, as this blend tree node takes no input
        motionInstance->GetMotion()->Update(&outputTransformPose, &outputTransformPose, motionInstance);

        // compensate for motion extraction
        // we already moved our actor instance's position and rotation at this point
        // so we have to cancel/compensate this delta offset from the motion extraction node, so that we don't double-transform
        // basically this will keep the motion in-place rather than moving it away from the origin
        if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled() && !motionInstance->GetMotion()->GetMotionData()->IsAdditive())
        {
            outputTransformPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    }


    // get the motion instance for a given anim graph instance
    MotionInstance* AnimGraphMotionNode::FindMotionInstance(AnimGraphInstance* animGraphInstance) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->mMotionInstance;
    }


    // set the current play time
    void AnimGraphMotionNode::SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->SetCurrentPlayTime(timeInSeconds);
        if (uniqueData->mMotionInstance)
        {
            uniqueData->mMotionInstance->SetCurrentTime(timeInSeconds);
        }
    }


    // unique data constructor
    AnimGraphMotionNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    AnimGraphMotionNode::UniqueData::~UniqueData()
    {
        GetMotionInstancePool().Free(mMotionInstance);
    }

    void AnimGraphMotionNode::UniqueData::Reset()
    {
        // stop and delete the motion instance
        if (mMotionInstance)
        {
            mMotionInstance->Stop(0.0f);
            GetMotionInstancePool().Free(mMotionInstance);
        }

        // reset the unique data
        mMotionSetID    = MCORE_INVALIDINDEX32;
        mMotionInstance = nullptr;
        mReload         = true;
        mPlaySpeed      = 1.0f;
        mCurrentTime    = 0.0f;
        mDuration       = 0.0f;
        mActiveMotionIndex = MCORE_INVALIDINDEX32;
        SetSyncTrack(nullptr);

        Invalidate();
    }

    void AnimGraphMotionNode::UniqueData::Update()
    {
        AnimGraphMotionNode* motionNode = azdynamic_cast<AnimGraphMotionNode*>(mObject);
        AZ_Assert(motionNode, "Unique data linked to incorrect node type.");

        AnimGraphInstance* animGraphInstance = GetAnimGraphInstance();
        motionNode->PickNewActiveMotion(animGraphInstance, this);

        if (!mMotionInstance)
        {
            motionNode->CreateMotionInstance(animGraphInstance->GetActorInstance(), this);
        }

        // get the id of the currently used the motion set
        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        uint32 motionSetID = MCORE_INVALIDINDEX32;
        if (motionSet)
        {
            motionSetID = motionSet->GetID();
        }

        // update the internally stored playback info
        motionNode->UpdatePlayBackInfo(animGraphInstance);

        // update play info
        if (mMotionInstance)
        {
            MotionInstance* motionInstance = mMotionInstance;
            const float currentTime = motionInstance->GetCurrentTime();
            SetDuration(motionInstance->GetDuration());
            SetCurrentPlayTime(currentTime);
            SetPreSyncTime(currentTime);
            SetSyncTrack(motionInstance->GetMotion()->GetEventTable()->GetSyncTrack());
            SetIsMirrorMotion(motionInstance->GetMirrorMotion());
        }
    }

    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphMotionNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(mObjectIndex));

        // rewind is not necessary if unique data is not created yet
        if (!uniqueData)
        {
            return;
        }

        // find the motion instance for the given anim graph and return directly in case it is invalid
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance == nullptr)
        {
            return;
        }

        // reset several settings to rewind the motion instance
        motionInstance->ResetTimes();
        motionInstance->SetIsFrozen(false);
        SetSyncIndex(animGraphInstance, MCORE_INVALIDINDEX32);
        uniqueData->SetCurrentPlayTime(motionInstance->GetCurrentTime());
        uniqueData->SetDuration(motionInstance->GetDuration());
        uniqueData->SetPreSyncTime(uniqueData->GetCurrentPlayTime());
        //uniqueData->SetPlaySpeed( uniqueData->GetPlaySpeed() );

        PickNewActiveMotion(animGraphInstance, uniqueData);
    }

    // get the speed from the connection if there is one connected, if not use the node's playspeed
    float AnimGraphMotionNode::ExtractCustomPlaySpeed(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;

        // if there is a node connected to the speed input port, read that value and use it as internal speed
        float customSpeed;
        if (playSpeedConnection)
        {
            customSpeed = MCore::Max(0.0f, GetInputNumberAsFloat(animGraphInstance, INPUTPORT_PLAYSPEED));
        }
        else
        {
            customSpeed = m_playSpeed; // otherwise use the node's playspeed
        }

        return customSpeed;
    }

    void AnimGraphMotionNode::PickNewActiveMotion(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        PickNewActiveMotion(animGraphInstance, uniqueData);
    }

    // pick a new motion from the list
    void AnimGraphMotionNode::PickNewActiveMotion(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        const size_t numMotions = m_motionRandomSelectionCumulativeWeights.size();
        if (numMotions == 1)
        {
            uniqueData->mActiveMotionIndex = 0;
        }
        else if (numMotions > 1)
        {
            uniqueData->mReload = true;
            switch (m_indexMode)
            {
            // pick a random one, but make sure its not the same as the last one we played
            case INDEXMODE_RANDOMIZE_NOREPEAT:
            {
                if (uniqueData->mActiveMotionIndex == MCORE_INVALIDINDEX32)
                {
                    SelectAnyRandomMotionIndex(animGraphInstance, uniqueData);
                    return;
                }

                AZ::u32 curIndex = uniqueData->mActiveMotionIndex;

                // Make sure we're in a valid range.
                if (curIndex >= numMotions)
                {
                    curIndex = static_cast<AZ::u32>(numMotions - 1);
                }

                // Removing the cumulative probability range for the element that we do not want to choose
                const float previousIndexCumulativeWeight = curIndex > 0 ? m_motionRandomSelectionCumulativeWeights[curIndex - 1].second : 0;
                const float currentIndexCumulativeWeight = m_motionRandomSelectionCumulativeWeights[curIndex].second;
                const float randomRange = previousIndexCumulativeWeight + m_motionRandomSelectionCumulativeWeights.back().second - currentIndexCumulativeWeight;

                // Picking a random number between [0, randomRange)
                const float randomValue = animGraphInstance->GetLcgRandom().GetRandomFloat() * randomRange;
                // Remapping the value onto the existing non normalized cumulative probabilities
                float remappedRandomValue = randomValue;
                if (randomValue > previousIndexCumulativeWeight)
                {
                    remappedRandomValue = randomValue - previousIndexCumulativeWeight + currentIndexCumulativeWeight;
                }
                const int index = FindCumulativeProbabilityIndex(remappedRandomValue);
                AZ_Assert(index >= 0, "Unable to find random value in motion random weights");
                uniqueData->mActiveMotionIndex = index;
            }
            break;

            // pick the next motion from the list
            case INDEXMODE_SEQUENTIAL:
            {
                uniqueData->mActiveMotionIndex++;
                if (uniqueData->mActiveMotionIndex >= numMotions)
                {
                    uniqueData->mActiveMotionIndex = 0;
                }
            }
            break;

            // just pick a random one, this can result in the same one we already play
            case INDEXMODE_RANDOMIZE:
            default:
            {
                SelectAnyRandomMotionIndex(animGraphInstance, uniqueData);
            }
            }
        }
        else
        {
            uniqueData->mActiveMotionIndex = MCORE_INVALIDINDEX32;
        }
    }

    void AnimGraphMotionNode::SelectAnyRandomMotionIndex(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphMotionNode::UniqueData* uniqueData)
    {
        // Selecting a random number between [0, m_motionIdRandomWeights.back().second)
        const float randomValue = animGraphInstance->GetLcgRandom().GetRandomFloat() * m_motionRandomSelectionCumulativeWeights.back().second;
        const int index = FindCumulativeProbabilityIndex(randomValue);
        AZ_Assert(index >= 0, "Error: unable to find random value among motion random weights");
        uniqueData->mActiveMotionIndex = index;
    }

    int AnimGraphMotionNode::FindCumulativeProbabilityIndex(float randomValue) const
    {
        for (unsigned int i = 0; i < m_motionRandomSelectionCumulativeWeights.size(); ++i)
        {
            if (randomValue < m_motionRandomSelectionCumulativeWeights[i].second)
            {
                return i;
            }
        }
        return -1;
    }

    size_t AnimGraphMotionNode::GetNumMotions() const
    {
        return m_motionRandomSelectionCumulativeWeights.size();
    }


    const char* AnimGraphMotionNode::GetMotionId(size_t index) const
    {
        if (m_motionRandomSelectionCumulativeWeights.size() <= index)
        {
            return "";
        }

        return m_motionRandomSelectionCumulativeWeights[index].first.c_str();
    }


    void AnimGraphMotionNode::ReplaceMotionId(const char* oldId, const char* replaceWith)
    {
        for (auto& motionIdRandomWeightPair : m_motionRandomSelectionCumulativeWeights)
        {
            if (motionIdRandomWeightPair.first == oldId)
            {
                motionIdRandomWeightPair.first = replaceWith;
            }
        }

        UpdateNodeInfo();
    }


    void AnimGraphMotionNode::AddMotionId(const AZStd::string& name)
    {
        for (const auto& pair : m_motionRandomSelectionCumulativeWeights)
        {
            if (pair.first == name)
            {
                return;
            }
        }
        float weightSum = 0.0f;
        if (!m_motionRandomSelectionCumulativeWeights.empty())
        {
            weightSum = m_motionRandomSelectionCumulativeWeights.back().second;
        }
        m_motionRandomSelectionCumulativeWeights.emplace_back(name, weightSum + s_defaultWeight);
    }

    void AnimGraphMotionNode::ReloadAndInvalidateUniqueDatas()
    {
        if (!mAnimGraph)
        {
            return;
        }

        const size_t numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(mObjectIndex));
            if (uniqueData)
            {
                uniqueData->mReload = true;
                uniqueData->Invalidate();
            }
        }
    }

    void AnimGraphMotionNode::OnActorMotionExtractionNodeChanged()
    {
        ReloadAndInvalidateUniqueDatas();
    }

    void AnimGraphMotionNode::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        AnimGraphNode::RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(mObjectIndex));
        if (uniqueData)
        {
            uniqueData->mReload = true;
            uniqueData->Invalidate();
        }
    }

    void AnimGraphMotionNode::OnMotionIdsChanged()
    {
        ReloadAndInvalidateUniqueDatas();

        // Set the node info text.
        UpdateNodeInfo();
        SyncVisualObject();
    }

    void AnimGraphMotionNode::UpdateNodeInfo()
    {
        const size_t numMotions = m_motionRandomSelectionCumulativeWeights.size();
        if (numMotions == 1)
        {
            SetNodeInfo(GetMotionId(0));
        }
        else if (numMotions > 1)
        {
            SetNodeInfo("<Multiple>");
        }
        else
        {
            SetNodeInfo("<None>");
        }
    }

    AZ::Crc32 AnimGraphMotionNode::GetRewindOnZeroWeightVisibility() const
    {
        return m_loop ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }


    AZ::Crc32 AnimGraphMotionNode::GetMultiMotionWidgetsVisibility() const
    {
        return GetNumMotions() > 1 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void AnimGraphMotionNode::SetRewindOnZeroWeight(bool rewindOnZeroWeight)
    {
        m_rewindOnZeroWeight = rewindOnZeroWeight;
    }

    void AnimGraphMotionNode::SetNextMotionAfterLoop(bool nextMotionAfterLoop)
    {
        m_nextMotionAfterLoop = nextMotionAfterLoop;
    }

    void AnimGraphMotionNode::SetIndexMode(EIndexMode eIndexMode)
    {
        m_indexMode = eIndexMode;
    }

    void AnimGraphMotionNode::SetMotionPlaySpeed(float playSpeed)
    {
        m_playSpeed = playSpeed;
    }

    void AnimGraphMotionNode::SetEmitEvents(bool emitEvents)
    {
        m_emitEvents = emitEvents;
    }

    void AnimGraphMotionNode::SetMotionExtraction(bool motionExtraction)
    {
        m_motionExtraction = motionExtraction;
    }

    void AnimGraphMotionNode::SetMirrorMotion(bool mirrorMotion)
    {
        m_mirrorMotion = mirrorMotion;
    }

    void AnimGraphMotionNode::SetReverse(bool reverse)
    {
        m_reverse = reverse;
    }

    void AnimGraphMotionNode::SetRetarget(bool retarget)
    {
        m_retarget = retarget;
    }

    void AnimGraphMotionNode::SetLoop(bool loop)
    {
        m_loop = loop;
    }

    void AnimGraphMotionNode::SetMotionIds(const AZStd::vector<AZStd::string>& motionIds)
    {
        InitializeDefaultMotionIdsRandomWeights(motionIds, m_motionRandomSelectionCumulativeWeights);
    }

    void AnimGraphMotionNode::InitializeDefaultMotionIdsRandomWeights(const AZStd::vector<AZStd::string>& motionIds, AZStd::vector<AZStd::pair<AZStd::string, float> >& motionIdsRandomWeights)
    {
        motionIdsRandomWeights.clear();
        const size_t count = motionIds.size();
        motionIdsRandomWeights.reserve(count);

        float currentCumulativeProbability = 0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            currentCumulativeProbability += s_defaultWeight;
            motionIdsRandomWeights.emplace_back(motionIds[i], currentCumulativeProbability);
        }
    }

    bool AnimGraphMotionNode::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            int motionIdsIndex = classElement.FindElement(AZ_CRC("motionIds", 0x3a3274c6));
            if (motionIdsIndex < 0)
            {
                return false;
            }
            AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(motionIdsIndex);
            AZStd::vector<AZStd::string> oldMotionIds;
            AZStd::vector<AZStd::pair<AZStd::string, float> > motionIdsWothRandomWeights;
            const bool result = dataElementNode.GetData<AZStd::vector<AZStd::string> >(oldMotionIds);
            if (!result)
            {
                return false;
            }
            InitializeDefaultMotionIdsRandomWeights(oldMotionIds, motionIdsWothRandomWeights);
            classElement.RemoveElement(motionIdsIndex);
            classElement.AddElementWithData(context, "motionIds", motionIdsWothRandomWeights);
        }
        return true;
    }

    void AnimGraphMotionNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphMotionNode, AnimGraphNode>()
            ->Version(3, VersionConverter)
            ->Field("motionIds", &AnimGraphMotionNode::m_motionRandomSelectionCumulativeWeights)
            ->Field("loop", &AnimGraphMotionNode::m_loop)
            ->Field("retarget", &AnimGraphMotionNode::m_retarget)
            ->Field("reverse", &AnimGraphMotionNode::m_reverse)
            ->Field("emitEvents", &AnimGraphMotionNode::m_emitEvents)
            ->Field("mirrorMotion", &AnimGraphMotionNode::m_mirrorMotion)
            ->Field("motionExtraction", &AnimGraphMotionNode::m_motionExtraction)
            ->Field("inPlace", &AnimGraphMotionNode::m_inPlace)
            ->Field("playSpeed", &AnimGraphMotionNode::m_playSpeed)
            ->Field("indexMode", &AnimGraphMotionNode::m_indexMode)
            ->Field("nextMotionAfterLoop", &AnimGraphMotionNode::m_nextMotionAfterLoop)
            ->Field("rewindOnZeroWeight", &AnimGraphMotionNode::m_rewindOnZeroWeight)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphMotionNode>("Motion", "Motion attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("MotionSetMotionIdsRandomSelectionWeights", 0xc882da3c), &AnimGraphMotionNode::m_motionRandomSelectionCumulativeWeights, "Motions", "")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::OnMotionIdsChanged)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_loop, "Loop", "Loop the motion?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_retarget, "Retarget", "Is this motion allowed to be retargeted?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_reverse, "Reverse", "Playback reversed?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_emitEvents, "Emit Events", "Emit motion events?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_inPlace, "In Place", "Should the motion be in place and not move? This is most likely only used if you do not use motion extraction but your motion data moves the character away from the origin.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_mirrorMotion, "Mirror Motion", "Mirror the motion?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::ReloadAndInvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_motionExtraction, "Motion Extraction", "Enable motion extraction?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionNode::InvalidateUniqueDatas)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AnimGraphMotionNode::m_playSpeed, "Play Speed", "The playback speed factor.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphMotionNode::m_indexMode, "Indexing Mode", "The indexing mode to use when using multiple motions inside this motion node.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionNode::GetMultiMotionWidgetsVisibility)
            ->EnumAttribute(INDEXMODE_RANDOMIZE,           "Randomize")
            ->EnumAttribute(INDEXMODE_RANDOMIZE_NOREPEAT,  "Random No Repeat")
            ->EnumAttribute(INDEXMODE_SEQUENTIAL,          "Sequential")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_nextMotionAfterLoop, "Next Motion After Loop", "Switch to the next motion after this motion has ended/looped?")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionNode::GetMultiMotionWidgetsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionNode::m_rewindOnZeroWeight, "Rewind On Zero Weight", "Rewind the motion when its local weight is near zero. Useful to restart non-looping motions. Looping needs to be disabled for this to work.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionNode::GetRewindOnZeroWeightVisibility)
        ;
    }
} // namespace EMotionFX
