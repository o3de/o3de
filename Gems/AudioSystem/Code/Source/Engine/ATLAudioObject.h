/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/set.h>

#include <ATLEntities.h>
#include <ATLEntityData.h>

#include <climits>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

class ATLAudioObjectTest;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLTriggerImplState
    {
        TATLEnumFlagsType nFlags{ eATS_NONE };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLTriggerInstanceState
    {
        TATLEnumFlagsType nFlags{ eATS_NONE };
        TAudioControlID nTriggerID{ INVALID_AUDIO_CONTROL_ID };
        size_t numPlayingEvents{ 0 };
        void* pOwner{ nullptr };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // CATLAudioObjectBase-related typedefs

    using TObjectEventSet = ATLSetLookupType<TAudioEventID>;
    using TObjectTriggerInstanceSet = ATLSetLookupType<TAudioTriggerInstanceID>;
    using TObjectTriggerImplStates = ATLMapLookupType<TAudioTriggerImplID, SATLTriggerImplState>;
    using TObjectTriggerStates = ATLMapLookupType<TAudioTriggerInstanceID, SATLTriggerInstanceState>;
    using TObjectStateMap = ATLMapLookupType<TAudioControlID, TAudioSwitchStateID>;
    using TObjectRtpcMap = ATLMapLookupType<TAudioControlID, float>;
    using TObjectEnvironmentMap = ATLMapLookupType<TAudioEnvironmentID, float>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // this class wraps the common functionality shared by the AudioObject and the GlobalAudioObject
    class CATLAudioObjectBase
        : public CATLEntity<TAudioObjectID>
    {
    public:
        void TriggerInstanceStarting(TAudioTriggerInstanceID triggerInstanceId, TAudioControlID audioControlId);
        void TriggerInstanceStarted(TAudioTriggerInstanceID triggerInstanceId, void* owner);
        void TriggerInstanceFinished(TObjectTriggerStates::const_iterator iter);
        void EventStarted(const CATLEvent* const atlEvent);
        void EventFinished(const CATLEvent* const atlEvent);

        void SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID);
        void SetRtpc(const TAudioControlID nRtpcID, const float fValue);
        void SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fAmount);

        const TObjectTriggerImplStates& GetTriggerImpls() const;
        const TObjectRtpcMap& GetRtpcs() const;
        const TObjectEnvironmentMap& GetEnvironments() const;

        void ClearRtpcs();
        void ClearEnvironments();

        const TObjectEventSet& GetActiveEvents() const
        {
            return m_cActiveEvents;
        }

        bool HasActiveEvents() const;

        TObjectTriggerInstanceSet GetTriggerInstancesByOwner(void* const pOwner) const;

        void IncrementRefCount()
        {
            ++m_nRefCounter;
        }

        void DecrementRefCount()
        {
            AZ_Assert(m_nRefCounter > 0, "CATLAudioObjectBase - Too many refcount decrements!");
            --m_nRefCounter;
        }

        size_t GetRefCount() const
        {
            return m_nRefCounter;
        }

        void SetImplDataPtr(IATLAudioObjectData* const pImplData)
        {
            m_pImplData = pImplData;
        }

        IATLAudioObjectData* GetImplDataPtr() const
        {
            return m_pImplData;
        }

        virtual bool HasPosition() const = 0;

    protected:

        CATLAudioObjectBase(const TAudioObjectID nObjectID, const EATLDataScope eDataScope, IATLAudioObjectData* const pImplData = nullptr)
            : CATLEntity<TAudioObjectID>(nObjectID, eDataScope)
            , m_nRefCounter(0)
            , m_pImplData(pImplData)
        {}

        virtual void Clear();
        virtual void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition);

        TObjectEventSet m_cActiveEvents;
        TObjectTriggerStates m_cTriggers;
        TObjectTriggerImplStates m_cTriggerImpls;
        TObjectRtpcMap m_cRtpcs;
        TObjectEnvironmentMap m_cEnvironments;
        TObjectStateMap m_cSwitchStates;
        size_t m_nRefCounter;
        IATLAudioObjectData* m_pImplData;

#if !defined(AUDIO_RELEASE)
    public:
        void CheckBeforeRemoval(const CATLDebugNameStore* const pDebugNameStore);

    protected:
        ///////////////////////////////////////////////////////////////////////////////////////////////
        class CStateDebugDrawData
        {
        public:
            CStateDebugDrawData(const TAudioSwitchStateID nState = INVALID_AUDIO_SWITCH_STATE_ID);
            ~CStateDebugDrawData();

            void Update(const TAudioSwitchStateID nNewState);

            TAudioSwitchStateID nCurrentState;
            float fCurrentAlpha;

        private:
            static const float fMaxAlpha;
            static const float fMinAlpha;
            static const int nMaxToMinUpdates;
        };

        using TStateDrawInfoMap = ATLMapLookupType<TAudioControlID, CStateDebugDrawData>;

        AZStd::string GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore);
        AZStd::string GetEventIDs(const char* const sSeparator);

        mutable TStateDrawInfoMap m_cStateDrawInfoMap;
#endif // !AUDIO_RELEASE
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLGlobalAudioObject
        : public CATLAudioObjectBase
    {
    public:
        CATLGlobalAudioObject(const TAudioObjectID nID, IATLAudioObjectData* const pImplData)
            : CATLAudioObjectBase(nID, eADS_GLOBAL, pImplData)
        {}

        bool HasPosition() const override
        {
            return false;
        }
    };


    // Physics-related obstruction/occlusion raycasting...


    static constexpr AZ::u16 s_maxHitResultsPerRaycast = 5;
    static constexpr AZ::u16 s_maxRaysPerObject = 5;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioRaycastRequest
    {
        AzPhysics::RayCastRequest m_request{};
        TAudioObjectID m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
        size_t m_rayIndex = 0;

        AudioRaycastRequest(const AzPhysics::RayCastRequest& request, TAudioObjectID audioObjectId, size_t rayId)
            : m_request(request)
            , m_audioObjectId(audioObjectId)
            , m_rayIndex(rayId)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioRaycastResult
    {
        AZStd::vector<AzPhysics::SceneQueryHit> m_result{};
        TAudioObjectID m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
        size_t m_rayIndex = 0;

        AudioRaycastResult(AZStd::vector<AzPhysics::SceneQueryHit>&& result, TAudioObjectID audioObjectId, size_t rayId)
            : m_result(AZStd::move(result))
            , m_audioObjectId(audioObjectId)
            , m_rayIndex(rayId)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioRaycastRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioRaycastRequests() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // No mutex!  All audio raycast requests are initiated and received on the Audio Thread.

        virtual void PushAudioRaycastRequest(const AudioRaycastRequest&) = 0;
    };

    using AudioRaycastRequestBus = AZ::EBus<AudioRaycastRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioRaycastNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioRaycastNotifications() = default;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        // No mutex!  All audio raycast notifications are initiated and received on the Audio Thread.
        using BusIdType = TAudioObjectID;

        virtual void OnAudioRaycastResults(const AudioRaycastResult&) = 0;
    };

    using AudioRaycastNotificationBus = AZ::EBus<AudioRaycastNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct RaycastInfo
    {
        AZStd::fixed_vector<AzPhysics::SceneQueryHit, s_maxHitResultsPerRaycast> m_hits;
        AzPhysics::RayCastRequest m_raycastRequest;
        float m_contribution = 0.f;
        float m_cacheTimerMs = 0.f;
        AZ::u16 m_numHits = 0;
        bool m_pending = false; //!< Whether the ray has been requested and is still pending.
        bool m_cached = false;

        void UpdateContribution();
        void Reset()
        {
            m_hits.clear();
            m_contribution = 0.f;
            m_cacheTimerMs = 0.f;
            m_numHits = 0;
            m_pending = false;
            m_cached = false;
        }

        float GetDistanceScaledContribution() const;
        float GetNearestHitDistance() const;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class RaycastProcessor
    {
        friend class CATLAudioObject;

    public:
        RaycastProcessor(const TAudioObjectID objectId, const SATLWorldPosition& objectPosition);

        void Update(float deltaMs);
        void Reset();
        void SetType(ObstructionType calcType);
        bool CanRun() const;
        void Run(const SATLWorldPosition& listenerPosition);
        void CastRay(const AZ::Vector3& origin, const AZ::Vector3& dest, const AZ::u16 rayIndex);

        float GetObstruction() const
        {
            return AZ::GetClamp(m_obstructionValue.GetCurrent(), 0.f, 1.f);
        }
        float GetOcclusion() const
        {
            return AZ::GetClamp(m_occlusionValue.GetCurrent(), 0.f, 1.f);
        }

        void SetupTestRay(AZ::u16 rayIndex);

#if !defined(AUDIO_RELEASE)
        void DrawObstructionRays(AzFramework::DebugDisplayRequests& debugDisplay) const;
#endif // !AUDIO_RELEASE

        static constexpr float s_epsilon = 1e-3f;
        static bool s_raycastsEnabled;

    private:
        AZStd::fixed_vector<RaycastInfo, s_maxRaysPerObject> m_rayInfos;
        const SATLWorldPosition& m_position;
        CSmoothFloat m_obstructionValue;
        CSmoothFloat m_occlusionValue;
        TAudioObjectID m_audioObjectId;
        ObstructionType m_obstOccType;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLAudioObject
        : public CATLAudioObjectBase
        , public AudioRaycastNotificationBus::Handler
    {
        friend class ::ATLAudioObjectTest;
    public:
        explicit CATLAudioObject(const TAudioObjectID nID, IATLAudioObjectData* const pImplData = nullptr)
            : CATLAudioObjectBase(nID, eADS_NONE, pImplData)
            , m_nFlags(eAOF_NONE)
            , m_fPreviousVelocity(0.0f)
            , m_raycastProcessor(nID, m_oPosition)
        {
        }

        ~CATLAudioObject() override
        {
            AudioRaycastNotificationBus::Handler::BusDisconnect();
        }

        CATLAudioObject(const CATLAudioObject&) = delete;           // not defined; calls will fail at compile time
        CATLAudioObject& operator=(const CATLAudioObject&) = delete; // not defined; calls will fail at compile time

        // CATLAudioObjectBase
        bool HasPosition() const override
        {
            return true;
        }
        void Clear() override;
        void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition) override;
        // ~CATLAudioObjectBase

        void SetPosition(const SATLWorldPosition& oNewPosition);

        void SetRaycastCalcType(const ObstructionType type);
        void RunRaycasts(const SATLWorldPosition& listenerPos);
        bool CanRunRaycasts() const;
        void GetObstOccData(SATLSoundPropagationData& data) const;

        // AudioRaycastNotificationBus::Handler
        void OnAudioRaycastResults(const AudioRaycastResult& result) override;

        void SetVelocityTracking(const bool bTrackingOn);
        bool GetVelocityTracking() const
        {
            return (m_nFlags & eAOF_TRACK_VELOCITY) != 0;
        }
        void UpdateVelocity(const float fUpdateIntervalMS);

    private:
        TATLEnumFlagsType m_nFlags;
        float m_fPreviousVelocity;
        SATLWorldPosition m_oPosition;
        SATLWorldPosition m_oPreviousPosition;

        RaycastProcessor m_raycastProcessor;

#if !defined(AUDIO_RELEASE)
    public:
        void DrawDebugInfo(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& listenerPos,
            const CATLDebugNameStore* const debugNameStore) const;
        const SATLWorldPosition& GetPosition() const
        {
            return m_oPosition;
        }
#endif // !AUDIO_RELEASE
    };

} // namespace Audio
