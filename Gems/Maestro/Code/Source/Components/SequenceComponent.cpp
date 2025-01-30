/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SequenceComponent.h"

#include <IMovieSystem.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>

#include <Cinematics/Movie.h>
#include <Cinematics/AnimSplineTrack.h>
#include <Maestro/Types/AssetBlends.h>
#include <Maestro/Types/AssetBlendKey.h>
#include <Cinematics/AssetBlendTrack.h>
#include <Cinematics/CompoundSplineTrack.h>
#include <Cinematics/BoolTrack.h>
#include <Cinematics/CharacterTrack.h>
#include <Cinematics/CaptureTrack.h>
#include <Cinematics/CommentTrack.h>
#include <Cinematics/ConsoleTrack.h>
#include <Cinematics/EventTrack.h>
#include <Cinematics/GotoTrack.h>
#include <Cinematics/LookAtTrack.h>
#include <Cinematics/ScreenFaderTrack.h>
#include <Cinematics/SelectTrack.h>
#include <Cinematics/SequenceTrack.h>
#include <Cinematics/SoundTrack.h>
#include <Cinematics/TimeRangesTrack.h>
#include <Cinematics/TrackEventTrack.h>

#include <Cinematics/AnimSequence.h>
#include <Cinematics/AnimNode.h>
#include <Cinematics/AnimNodeGroup.h>
#include <Cinematics/SceneNode.h>
#include <Cinematics/AnimAZEntityNode.h>
#include <Cinematics/AnimComponentNode.h>
#include <Cinematics/AnimScreenFaderNode.h>
#include <Cinematics/CommentNode.h>
#include <Cinematics/CVarNode.h>
#include <Cinematics/ScriptVarNode.h>
#include <Cinematics/AnimPostFXNode.h>
#include <Cinematics/EventNode.h>
#include <Cinematics/LayerNode.h>
#include <Cinematics/ShadowsSetupNode.h>

namespace Maestro
{
    /**
    * Reflect the SequenceComponentNotificationBus to the Behavior Context
    */
    class BehaviorSequenceComponentNotificationBusHandler : public SequenceComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSequenceComponentNotificationBusHandler, "{3EC0FB38-4649-41E7-8409-0D351FE99A64}", AZ::SystemAllocator
            , OnStart
            , OnStop
            , OnPause
            , OnResume
            , OnAbort
            , OnUpdate
            , OnTrackEventTriggered
            );

        void OnStart(float startTime) override
        {
            Call(FN_OnStart, startTime);
        }
        void OnStop(float stopTime) override
        {
            Call(FN_OnStop, stopTime);
        }
        void OnPause() override
        {
            Call(FN_OnPause);
        }
        void OnResume() override
        {
            Call(FN_OnResume);
        }
        void OnAbort(float abortTime) override
        {
            Call(FN_OnAbort, abortTime);
        }
        void OnUpdate(float updateTime) override
        {
            Call(FN_OnUpdate, updateTime);
        }
        void OnTrackEventTriggered(const char* eventName, const char* eventValue) override
        {
            Call(FN_OnTrackEventTriggered, eventName, eventValue);
        }
    };

    SequenceComponent::SequenceComponent()
    {
        AZ_Trace("SequenceComponent", "SequenceComponent");
    }

    SequenceComponent::~SequenceComponent()
    {
        AZ_Trace("SequenceComponent", "~SequenceComponent");
    }

    void SequenceComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect the Cinematics library
        ReflectCinematicsLib(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SequenceComponent, AZ::Component>()
                ->Version(2)
                ->Field("Sequence", &SequenceComponent::m_sequence);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SequenceComponentRequestBus>("SequenceComponentRequestBus")
                ->Event("Play", &SequenceComponentRequestBus::Events::Play)
                ->Event("PlayBetweenTimes", &SequenceComponentRequestBus::Events::PlayBetweenTimes)
                ->Event("Stop", &SequenceComponentRequestBus::Events::Stop)
                ->Event("Pause", &SequenceComponentRequestBus::Events::Pause)
                ->Event("Resume", &SequenceComponentRequestBus::Events::Resume)
                ->Event("SetPlaySpeed", &SequenceComponentRequestBus::Events::SetPlaySpeed)
                ->Event("JumpToTime", &SequenceComponentRequestBus::Events::JumpToTime)
                ->Event("JumpToBeginning", &SequenceComponentRequestBus::Events::JumpToBeginning)
                ->Event("JumpToEnd", &SequenceComponentRequestBus::Events::JumpToEnd)
                ->Event("GetCurrentPlayTime", &SequenceComponentRequestBus::Events::GetCurrentPlayTime)
                ->Event("GetPlaySpeed", &SequenceComponentRequestBus::Events::GetPlaySpeed)
                ;
            behaviorContext->Class<SequenceComponent>()->RequestBus("SequenceComponentRequestBus");

            behaviorContext->EBus<SequenceComponentNotificationBus>("SequenceComponentNotificationBus")->
                Handler<BehaviorSequenceComponentNotificationBusHandler>();
        }
    }

    void SequenceComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void SequenceComponent::ReflectCinematicsLib(AZ::ReflectContext* context)
    {
        // The Movie System itself
        CMovieSystem::Reflect(context);
        
        // Tracks
        IAnimTrack::Reflect(context);
        TAnimSplineTrack<Vec2>::Reflect(context);
        CBoolTrack::Reflect(context);
        CCaptureTrack::Reflect(context);
        CCharacterTrack::Reflect(context);
        CCompoundSplineTrack::Reflect(context);
        CCommentTrack::Reflect(context);
        CConsoleTrack::Reflect(context);
        CEventTrack::Reflect(context);
        CGotoTrack::Reflect(context);
        CLookAtTrack::Reflect(context);
        CScreenFaderTrack::Reflect(context);
        CSelectTrack::Reflect(context);
        CSequenceTrack::Reflect(context);
        CSoundTrack::Reflect(context);
        CTrackEventTrack::Reflect(context);
        CAssetBlendTrack::Reflect(context);
        CTimeRangesTrack::Reflect(context);

        // Nodes
        IAnimSequence::Reflect(context);
        CAnimSequence::Reflect(context);
        CAnimSceneNode::Reflect(context);
        IAnimNode::Reflect(context);
        CAnimNode::Reflect(context);
        CAnimAzEntityNode::Reflect(context);
        CAnimComponentNode::Reflect(context);
        CAnimScreenFaderNode::Reflect(context);
        CCommentNode::Reflect(context);
        CAnimCVarNode::Reflect(context);
        CAnimScriptVarNode::Reflect(context);
        CAnimNodeGroup::Reflect(context);
        CAnimPostFXNode::Reflect(context);
        CAnimEventNode::Reflect(context);
        CLayerNode::Reflect(context);
        CShadowsSetupNode::Reflect(context);
    }

    void SequenceComponent::Init()
    {
        Component::Init();

        m_movieSystem = AZ::Interface<IMovieSystem>::Get();

        if (m_movieSystem)
        {
            if (m_sequence)
            {
                // Fix up the internal pointers in the sequence to match the deserialized structure
                m_sequence->InitPostLoad();

                // Register our deserialized sequence with the MovieSystem
                m_movieSystem->AddSequence(m_sequence.get());
            }
        }
        else
        {
            AZ_Warning("TrackView", false, "SequenceComponent::Init() called without m_movieSystem initialized yet, skipping creation of %s sequence.", m_entity->GetName().c_str());
        }
    }

    void SequenceComponent::Activate()
    {
        SequenceComponentRequestBus::Handler::BusConnect(GetEntityId());

        AZ_Trace("SequenceComponent::Activate", "SequenceComponentRequestBus connected to %s", GetEntityId().ToString().c_str())

        if (m_movieSystem)
        {
            if (m_sequence && (m_sequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset))
            {
                m_movieSystem->OnSequenceActivated(m_sequence.get());
            }
        }
    }

    void SequenceComponent::Deactivate()
    {
        SequenceComponentRequestBus::Handler::BusDisconnect();

        AZ_Trace(
            "SequenceComponent::Deactivate",
            "SequenceComponentRequestBus disconnected from %s",
            GetEntityId().ToString().c_str());

        // Remove this sequence from the game movie system.
        if (nullptr != m_movieSystem)
        {
            if (m_sequence)
            {
                m_movieSystem->RemoveSequence(m_sequence.get());
            }
        }
    }

    bool SequenceComponent::SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        const SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        bool changed = false;
        
        SequenceAgentComponentRequestBus::EventResult(
            changed, ebusId, &SequenceAgentComponentRequestBus::Events::SetAnimatedPropertyValue, animatableAddress, value);
        
        return changed;
    }

    bool SequenceComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress)
    {
        const SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        SequenceAgentComponentRequestBus::Event(
            ebusId, &SequenceAgentComponentRequestBus::Events::GetAnimatedPropertyValue, returnValue, animatableAddress);

        return true;
    }

    AZ::Uuid SequenceComponent::GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        const SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        SequenceAgentComponentRequestBus::EventResult(typeId, ebusId, &SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, animatableAddress);

        return typeId;
    }

    void SequenceComponent::GetAssetDuration(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)
    {
        const SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        SequenceAgentComponentRequestBus::Event(
            ebusId, &SequenceAgentComponentRequestBus::Events::GetAssetDuration, returnValue, componentId, assetId);
    }

    void SequenceComponent::Play()
    {
        if (m_sequence)
        {
            m_movieSystem->PlaySequence(m_sequence.get(), /*parentSeq =*/ nullptr, /*bResetFX =*/ true,/*bTrackedSequence =*/ false, /*float startTime =*/ -FLT_MAX, /*float endTime =*/ -FLT_MAX);
        }
    }

    void SequenceComponent::PlayBetweenTimes(float startTime, float endTime)
    {
        if (m_sequence)
        {
            m_movieSystem->PlaySequence(m_sequence.get(), /*parentSeq =*/ nullptr, /*bResetFX =*/ true,/*bTrackedSequence =*/ false, startTime, endTime);
        }
    }

    void SequenceComponent::Stop()
    {
        if (m_sequence)
        {
            m_movieSystem->StopSequence(m_sequence.get());
        }
    }

    void SequenceComponent::Pause()
    {
        if (m_sequence)
        {
            m_sequence.get()->Pause();
        }
    }

    void SequenceComponent::Resume()
    {
        if (m_sequence)
        {
            m_sequence.get()->Resume();
        }
    }

    void SequenceComponent::SetPlaySpeed(float newSpeed)
    {
        if (m_sequence)
        {
            m_movieSystem->SetPlayingSpeed(m_sequence.get(), newSpeed);
        }
    }

    void SequenceComponent::JumpToTime(float newTime)
    {
        if (m_sequence)
        {
            newTime = clamp_tpl(newTime, m_sequence.get()->GetTimeRange().start, m_sequence.get()->GetTimeRange().end);
            m_movieSystem->SetPlayingTime(m_sequence.get(), newTime);
        }
    }

    void SequenceComponent::JumpToEnd()
    {
        if (m_sequence)
        {
            m_movieSystem->SetPlayingTime(m_sequence.get(), m_sequence.get()->GetTimeRange().end);
        }
    }

    void SequenceComponent::JumpToBeginning()
    {
        if (m_sequence)
        {
            m_movieSystem->SetPlayingTime(m_sequence.get(), m_sequence.get()->GetTimeRange().start);
        }
    }

    float SequenceComponent::GetCurrentPlayTime()
    {
        if (m_sequence)
        {
            return m_movieSystem->GetPlayingTime(m_sequence.get());
        }
        return .0f;
    }

    float SequenceComponent::GetPlaySpeed()
    {
        if (m_sequence)
        {
            return m_movieSystem->GetPlayingSpeed(m_sequence.get());
        }
        return 1.0f;
    }

} // namespace Maestro
