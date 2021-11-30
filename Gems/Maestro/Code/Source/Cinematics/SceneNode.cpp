/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Components/CameraBus.h>

#include "MathConversion.h"
#include "SceneNode.h"
#include "AnimSequence.h"
#include "AnimTrack.h"
#include "EventTrack.h"
#include "ConsoleTrack.h"
#include "SequenceTrack.h"
#include "GotoTrack.h"
#include "CaptureTrack.h"
#include "ISystem.h"
#include "AnimAZEntityNode.h"
#include "AnimComponentNode.h"
#include "Movie.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

#include <AzCore/Math/MathUtils.h>

#include <IAudioSystem.h>
#include <IConsole.h>

#define s_nodeParamsInitialized s_nodeParamsInitializedScene
#define s_nodeParams s_nodeParamsSene
#define AddSupportedParam AddSupportedParamScene

namespace {
    bool s_nodeParamsInitialized = false;
    StaticInstance<std::vector<CAnimNode::SParamInfo>> s_nodeParams;

    void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType, int flags = 0)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        param.flags = (IAnimNode::ESupportedParamFlags)flags;
        s_nodeParams.push_back(param);
    }

    class CComponentEntitySceneCamera
        : public CAnimSceneNode::ISceneCamera
    {
    public:
        CComponentEntitySceneCamera(const AZ::EntityId& entityId)
            : m_cameraEntityId(entityId) {}

        virtual ~CComponentEntitySceneCamera() = default;

        const Vec3& GetPosition() const override
        {
            AZ::Vector3 pos;
            AZ::TransformBus::EventResult(pos, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
            m_vec3Buffer.Set(pos.GetX(), pos.GetY(), pos.GetZ());
            return m_vec3Buffer;
        }
        const Quat& GetRotation() const override
        {
            AZ::Quaternion quat(AZ::Quaternion::CreateIdentity());
            AZ::TransformBus::EventResult(quat, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
            m_quatBuffer = AZQuaternionToLYQuaternion(quat);
            return m_quatBuffer;
        }
        void SetPosition(const Vec3& localPosition) override
        {
            AZ::Vector3 pos(localPosition.x, localPosition.y, localPosition.z);
            AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTranslation, pos);
        }
        void SetRotation(const Quat& localRotation) override
        {
            AZ::Quaternion quat = LYQuaternionToAZQuaternion(localRotation);
            AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, quat);
        }
        float GetFoV() const override
        {
            float retFoV = DEFAULT_FOV;
            Camera::CameraRequestBus::EventResult(retFoV, m_cameraEntityId, &Camera::CameraComponentRequests::GetFovDegrees);
            return retFoV;
        }
        float GetNearZ() const override
        {
            float retNearZ = DEFAULT_NEAR;
            Camera::CameraRequestBus::EventResult(retNearZ, m_cameraEntityId, &Camera::CameraComponentRequests::GetNearClipDistance);
            return retNearZ;
        }
        void SetNearZAndFOVIfChanged(float fov, float nearZ) override
        {
            float degFoV = AZ::RadToDeg(fov);
            if (!AZ::IsClose(GetFoV(), degFoV, FLT_EPSILON))
            {
                Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraComponentRequests::SetFovDegrees, degFoV);
            }
            if (!AZ::IsClose(GetNearZ(), nearZ, FLT_EPSILON))
            {
                Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraComponentRequests::SetNearClipDistance, nearZ);
            }
        }
        void TransformPositionFromLocalToWorldSpace(Vec3& position) override
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            if (parentId.IsValid())
            {
                AZ::Vector3 pos(position.x, position.y, position.z);
                AZ::Transform worldTM;
                AZ::TransformBus::EventResult(worldTM, parentId, &AZ::TransformBus::Events::GetWorldTM);
                pos = worldTM.TransformPoint(pos);
                position.Set(pos.GetX(), pos.GetY(), pos.GetZ());
            }
        }
        void TransformPositionFromWorldToLocalSpace(Vec3& position) override
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            if (parentId.IsValid())
            {
                AZ::Vector3 pos(position.x, position.y, position.z);
                AZ::Transform worldTM;
                AZ::TransformBus::EventResult(worldTM, parentId, &AZ::TransformBus::Events::GetWorldTM);
                worldTM = worldTM.GetInverse();
                pos = worldTM.TransformPoint(pos);
                position.Set(pos.GetX(), pos.GetY(), pos.GetZ());
            }
        }
        void TransformRotationFromLocalToWorldSpace(Quat& rotation) override
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            if (parentId.IsValid())
            {
                AZ::Quaternion rot = LYQuaternionToAZQuaternion(rotation);
                AZ::Transform worldTM;
                AZ::TransformBus::EventResult(worldTM, parentId, &AZ::TransformBus::Events::GetWorldTM);
                AZ::Quaternion worldRot = worldTM.GetRotation();
                rot = worldRot * rot;
                rotation = AZQuaternionToLYQuaternion(rot);
            }
        }
        void SetWorldRotation(const Quat& rotation) override
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            if (parentId.IsValid())
            {
                AZ::Quaternion rot = LYQuaternionToAZQuaternion(rotation);
                AZ::Transform parentWorldTM;
                AZ::Transform worldTM;
                AZ::TransformBus::EventResult(parentWorldTM, parentId, &AZ::TransformBus::Events::GetWorldTM);
                AZ::TransformBus::EventResult(worldTM, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
                parentWorldTM.SetRotation(rot);
                parentWorldTM.SetTranslation(worldTM.GetTranslation());
                AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTM, parentWorldTM);
            }
            else
            {
                SetRotation(rotation);
            }
        }
        bool HasParent() const override
        {
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            return parentId.IsValid();
        }
    private:
        AZ::EntityId    m_cameraEntityId;
        mutable Vec3    m_vec3Buffer;       // buffer for returning references
        mutable Quat    m_quatBuffer;       // buffer for returning references
    };
}

//////////////////////////////////////////////////////////////////////////
CAnimSceneNode::CAnimSceneNode(const int id)
    : CAnimNode(id, AnimNodeType::Director)
{
    m_lastCameraKey = -1;
    m_lastEventKey = -1;
    m_lastConsoleKey = -1;
    m_lastSequenceKey = -1;
    m_nLastGotoKey = -1;
    m_lastCaptureKey = -1;
    m_bLastCapturingEnded = true;
    m_captureFrameCount = 0;
    m_pCamNodeOnHoldForInterp = nullptr;
    m_CurrentSelectTrack = nullptr;
    m_CurrentSelectTrackKeyNumber = 0;
    m_lastPrecachePoint = -1.f;
    SetName("Scene");

    CAnimSceneNode::Initialize();

    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
}

//////////////////////////////////////////////////////////////////////////
CAnimSceneNode::CAnimSceneNode()
    : CAnimSceneNode(0)
{
}


//////////////////////////////////////////////////////////////////////////
CAnimSceneNode::~CAnimSceneNode()
{
    ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Initialize()
{
    if (!s_nodeParamsInitialized)
    {
        s_nodeParamsInitialized = true;
        s_nodeParams.reserve(9);
        AddSupportedParam("Camera", AnimParamType::Camera, AnimValueType::Select);
        AddSupportedParam("Event", AnimParamType::Event, AnimValueType::Unknown);
        AddSupportedParam("Sound", AnimParamType::Sound, AnimValueType::Unknown);
        AddSupportedParam("Sequence", AnimParamType::Sequence, AnimValueType::Unknown);
        AddSupportedParam("Console", AnimParamType::Console, AnimValueType::Unknown);
        AddSupportedParam("GoTo", AnimParamType::Goto, AnimValueType::DiscreteFloat);
        AddSupportedParam("Capture", AnimParamType::Capture, AnimValueType::Unknown);
        AddSupportedParam("Timewarp", AnimParamType::TimeWarp, AnimValueType::Float);
        AddSupportedParam("FixedTimeStep", AnimParamType::FixedTimeStep, AnimValueType::Float);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::Camera);
};

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimSceneNode::GetParamCount() const
{
    return static_cast<unsigned int>(s_nodeParams.size());
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimSceneNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex < s_nodeParams.size())
    {
        return s_nodeParams[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSceneNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (int i = 0; i < (int)s_nodeParams.size(); i++)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Activate(bool bActivate)
{
    CAnimNode::Activate(bActivate);

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (paramId.GetType() != AnimParamType::Sequence)
        {
            continue;
        }

        CSequenceTrack* pSequenceTrack = (CSequenceTrack*)pTrack;

        for (int currKey = 0; currKey < pSequenceTrack->GetNumKeys(); currKey++)
        {
            ISequenceKey key;
            pSequenceTrack->GetKey(currKey, &key);

            IAnimSequence* pSequence = GetSequenceFromSequenceKey(key);
            if (pSequence)
            {
                if (bActivate)
                {
                    pSequence->Activate();

                    if (key.bOverrideTimes)
                    {
                        key.fDuration = (key.fEndTime - key.fStartTime) > 0.0f ? (key.fEndTime - key.fStartTime) : 0.0f;
                    }
                    else
                    {
                        key.fDuration = pSequence->GetTimeRange().Length();
                    }

                    pTrack->SetKey(currKey, &key);
                }
                else
                {
                    pSequence->Deactivate();
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Animate(SAnimContext& ec)
{
    if (ec.resetting)
    {
        return;
    }

    CSelectTrack* cameraTrack = nullptr;
    CEventTrack* pEventTrack = nullptr;
    CSequenceTrack* pSequenceTrack = nullptr;
    CConsoleTrack* pConsoleTrack = nullptr;
    CGotoTrack* pGotoTrack = nullptr;
    CCaptureTrack* pCaptureTrack = nullptr;
    /*
    bool bTimeJump = false;
    if (ec.time < m_time)
        bTimeJump = true;
    */

    if (gEnv->IsEditor() && m_time > ec.time)
    {
        m_lastPrecachePoint = -1.f;
    }

    PrecacheDynamic(ec.time);

    size_t nNumAudioTracks = 0;
    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            continue;
        }

        if (pTrack->IsMasked(ec.trackMask))
        {
            continue;
        }

        switch (paramId.GetType())
        {
        case AnimParamType::Camera:
            cameraTrack = (CSelectTrack*)pTrack;
            break;
        case AnimParamType::Event:
            pEventTrack = (CEventTrack*)pTrack;
            break;
        case AnimParamType::Sequence:
            pSequenceTrack = (CSequenceTrack*)pTrack;
            break;
        case AnimParamType::Console:
            pConsoleTrack = (CConsoleTrack*)pTrack;
            break;
        case AnimParamType::Capture:
            pCaptureTrack = (CCaptureTrack*)pTrack;
            break;
        case AnimParamType::Goto:
            pGotoTrack = (CGotoTrack*)pTrack;
            break;

        case AnimParamType::Sound:

            ++nNumAudioTracks;
            if (nNumAudioTracks > m_SoundInfo.size())
            {
                m_SoundInfo.resize(nNumAudioTracks);
            }

            AnimateSound(m_SoundInfo, ec, pTrack, nNumAudioTracks);

            break;

        case AnimParamType::TimeWarp:
        {
            float timeScale = 1.0f;
            pTrack->GetValue(ec.time, timeScale);
            if (timeScale < .0f)
            {
                timeScale = .0f;
            }
            
            if (auto* timeSystem = AZ::Interface<AZ::ITime>::Get())
            {
                m_simulationTickOverrideBackup = timeSystem->GetSimulationTickDeltaOverride();
                // if set, disable fixed time step cvar so timewarping will have an affect.
                timeSystem->SetSimulationTickDeltaOverride(AZ::Time::ZeroTimeMs);

                m_timeScaleBackup = timeSystem->GetSimulationTickScale();
                timeSystem->SetSimulationTickScale(timeScale);
            }
        }
        break;
        case AnimParamType::FixedTimeStep:
        {
            float timeStep = 0;
            pTrack->GetValue(ec.time, timeStep);
            if (timeStep < 0)
            {
                timeStep = 0;
            }

            if (auto* timeSystem = AZ::Interface<AZ::ITime>::Get())
            {
                m_simulationTickOverrideBackup = timeSystem->GetSimulationTickDeltaOverride();
                // if set, disable fixed time step cvar so timewarping will have an affect.
                timeSystem->SetSimulationTickDeltaOverride(AZ::SecondsToTimeMs(timeStep));
            }
        }
        break;
        }
    }

    // Animate Camera Track (aka Select Track)
    
    // Check if a camera override is set by CVar
    const char* overrideCamName = gEnv->pMovieSystem->GetOverrideCamName();
    AZ::EntityId overrideCamId;
    if (overrideCamName != nullptr && strlen(overrideCamName) > 0)
    {
        // overriding with a Camera Component entity is done by entityId (as names are not unique among AZ::Entities) - try to convert string to u64 to see if it's an id
        AZ::u64 u64Id = strtoull(overrideCamName, nullptr, /*base (radix)*/ 10);
        if (u64Id)
        {
            overrideCamId = AZ::EntityId(u64Id);
        }
    }

    if (overrideCamId.IsValid())      // There is a valid overridden camera.
    {
        if (overrideCamId != gEnv->pMovieSystem->GetCameraParams().cameraEntityId)
        {
            ISelectKey key;
            key.szSelection = overrideCamName;
            key.cameraAzEntityId = overrideCamId;
            ApplyCameraKey(key, ec);
        }
    }
    else if (cameraTrack)           // No camera override by CVar, use the camera track
    {
        ISelectKey key;
        int cameraKey = cameraTrack->GetActiveKey(ec.time, &key);
        m_CurrentSelectTrackKeyNumber = cameraKey;
        m_CurrentSelectTrack = cameraTrack;
        ApplyCameraKey(key, ec);
        m_lastCameraKey = cameraKey;
    }

    if (pEventTrack)
    {
        IEventKey key;
        int nEventKey = pEventTrack->GetActiveKey(ec.time, &key);
        if (nEventKey != m_lastEventKey && nEventKey >= 0)
        {
            bool bNotTrigger = key.bNoTriggerInScrubbing && ec.singleFrame && key.time != ec.time;
            if (!bNotTrigger)
            {
                ApplyEventKey(key, ec);
            }
        }
        m_lastEventKey = nEventKey;
    }

    if (pConsoleTrack)
    {
        IConsoleKey key;
        int nConsoleKey = pConsoleTrack->GetActiveKey(ec.time, &key);
        if (nConsoleKey != m_lastConsoleKey && nConsoleKey >= 0)
        {
            if (!ec.singleFrame || key.time == ec.time) // If Single frame update key time must match current time.
            {
                ApplyConsoleKey(key, ec);
            }
        }
        m_lastConsoleKey = nConsoleKey;
    }

    if (pSequenceTrack)
    {
        ISequenceKey key;
        int nSequenceKey = pSequenceTrack->GetActiveKey(ec.time, &key);
        IAnimSequence* pSequence = GetSequenceFromSequenceKey(key);

        if (!gEnv->IsEditing() && (nSequenceKey != m_lastSequenceKey || !GetMovieSystem()->IsPlaying(pSequence)))
        {
            ApplySequenceKey(pSequenceTrack, m_lastSequenceKey, nSequenceKey, key, ec);
        }
        m_lastSequenceKey = nSequenceKey;
    }

    if (pGotoTrack)
    {
        ApplyGotoKey(pGotoTrack, ec);
    }

    if (pCaptureTrack && gEnv->pMovieSystem->IsInBatchRenderMode() == false)
    {
        ICaptureKey key;
        int nCaptureKey = pCaptureTrack->GetActiveKey(ec.time, &key);
        bool justEnded = false;
        if (!m_bLastCapturingEnded && key.time + key.duration < ec.time)
        {
            justEnded = true;
        }

        if (!ec.singleFrame && !(gEnv->IsEditor() && gEnv->IsEditing()))
        {
            if (nCaptureKey != m_lastCaptureKey && nCaptureKey >= 0)
            {
                if (m_bLastCapturingEnded == false)
                {
                    assert(0);
                    gEnv->pMovieSystem->EndCapture();
                    m_bLastCapturingEnded = true;
                }
                gEnv->pMovieSystem->EnableFixedStepForCapture(key.timeStep);
                gEnv->pMovieSystem->StartCapture(key, m_captureFrameCount);
                if (key.once == false)
                {
                    m_bLastCapturingEnded = false;
                }
                m_lastCaptureKey = nCaptureKey;
            }
            else if (justEnded)
            {
                gEnv->pMovieSystem->DisableFixedStepForCapture();
                gEnv->pMovieSystem->EndCapture();
                m_bLastCapturingEnded = true;
            }
        }

        m_captureFrameCount++;
    }

    m_time = ec.time;
    if (m_pOwner)
    {
        m_pOwner->OnNodeAnimated(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::OnReset()
{
    if (m_lastSequenceKey >= 0)
    {
        {
            int trackCount = NumTracks();
            for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
            {
                CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
                IAnimTrack* pTrack = m_tracks[paramIndex].get();

                if (paramId.GetType() != AnimParamType::Sequence)
                {
                    continue;
                }

                CSequenceTrack* pSequenceTrack = (CSequenceTrack*)pTrack;
                ISequenceKey prevKey;

                pSequenceTrack->GetKey(m_lastSequenceKey, &prevKey);
                IAnimSequence* sequence = GetSequenceFromSequenceKey(prevKey);
                if (sequence)
                {
                    GetMovieSystem()->StopSequence(sequence);
                }
            }
        }
    }

    // If the last capturing hasn't finished properly, end it here.
    if (m_bLastCapturingEnded == false)
    {
        GetMovieSystem()->EndCapture();
        m_bLastCapturingEnded = true;
    }

    m_lastEventKey = -1;
    m_lastConsoleKey = -1;
    m_lastSequenceKey = -1;
    m_nLastGotoKey = -1;
    m_lastCaptureKey = -1;
    m_bLastCapturingEnded = true;
    m_captureFrameCount = 0;

    if (auto* timeSystem = AZ::Interface<AZ::ITime>::Get())
    {
        if (GetTrackForParameter(AnimParamType::TimeWarp))
        {
            timeSystem->SetSimulationTickScale(m_timeScaleBackup);
            timeSystem->SetSimulationTickDeltaOverride(m_simulationTickOverrideBackup);
        }

        if (GetTrackForParameter(AnimParamType::FixedTimeStep))
        {
            timeSystem->SetSimulationTickDeltaOverride(m_simulationTickOverrideBackup);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::OnStart()
{
    ResetSounds();
}

void CAnimSceneNode::OnPause()
{
}

void CAnimSceneNode::OnLoop()
{
    ResetSounds();
}

void CAnimSceneNode::OnStop()
{
    ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ResetSounds()
{
    for (int i = static_cast<int>(m_SoundInfo.size()); --i >= 0; )
    {
        m_SoundInfo[i].Reset();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ReleaseSounds()
{
    // Stop all sounds on the global audio object,
    // but we want to have it filter based on the owner (this)
    // so we don't stop sounds that didn't originate with track view.
    Audio::SAudioRequest request;
    request.nFlags = Audio::eARF_PRIORITY_HIGH;
    request.pOwner = this;

    Audio::SAudioObjectRequestData<Audio::eAORT_STOP_ALL_TRIGGERS> requestData(/*filterByOwner = */ true);
    request.pData = &requestData;
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, request);
}

//////////////////////////////////////////////////////////////////////////
// InterpolateCameras()
//
// This rather long function takes care of the interpolation (or blending) of
// two camera keys, specifically FoV, nearZ, position and rotation blending.
//
void CAnimSceneNode::InterpolateCameras(SCameraParams& retInterpolatedCameraParams, ISceneCamera* firstCamera,
                                        ISelectKey& firstKey, ISelectKey& secondKey, float time)
{
    if (!secondKey.cameraAzEntityId.IsValid())
    {
        // abort - can't interpolate if there isn't a valid Id for a component entity camera
        return;
    }

    float interpolatedFoV;

    ISceneCamera* secondCamera = static_cast<ISceneCamera*>(new CComponentEntitySceneCamera(secondKey.cameraAzEntityId));

    float t = 1 - ((secondKey.time - time) / firstKey.fBlendTime);
    t = min(t, 1.0f);
    t = aznumeric_cast<float>(pow(t, 3) * (t * (t * 6 - 15) + 10));                // use a cubic curve for the camera blend

    bool haveStashedInterpData = (m_InterpolatingCameraStartStates.find(m_CurrentSelectTrackKeyNumber) != m_InterpolatingCameraStartStates.end());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // at the start of the blend, stash the starting point first camera data to use throughout the interpolation
    if (!haveStashedInterpData)
    {
        InterpolatingCameraStartState camData;

        camData.m_interpolatedCamFirstPos = firstCamera->GetPosition();
        camData.m_interpolatedCamFirstRot = firstCamera->GetRotation();     

        // stash FoV from the first camera entity
        camData.m_FoV = firstCamera->GetFoV();
        
        // stash nearZ
        camData.m_nearZ = firstCamera->GetNearZ();

        m_InterpolatingCameraStartStates.insert(AZStd::make_pair(m_CurrentSelectTrackKeyNumber, camData));
    }

    const auto& retStashedInterpCamData = m_InterpolatingCameraStartStates.find(m_CurrentSelectTrackKeyNumber);
    InterpolatingCameraStartState stashedInterpCamData = retStashedInterpCamData->second;

    // interpolate FOV
    float secondCameraFOV = secondCamera->GetFoV();   

    interpolatedFoV = stashedInterpCamData.m_FoV + (secondCameraFOV - stashedInterpCamData.m_FoV) * t;
    // store the interpolated FoV to be returned, in radians
    retInterpolatedCameraParams.fov = DEG2RAD(interpolatedFoV);

    // interpolate NearZ
    float secondCameraNearZ = secondCamera->GetNearZ();

    retInterpolatedCameraParams.nearZ = stashedInterpCamData.m_nearZ + (secondCameraNearZ - stashedInterpCamData.m_nearZ) * t;

    // update the Camera entity's component FOV and nearZ directly if needed (if they weren't set via anim node SetParamValue() above)
    firstCamera->SetNearZAndFOVIfChanged(retInterpolatedCameraParams.fov, retInterpolatedCameraParams.nearZ);

    ////////////////////////
    // interpolate Position
    Vec3 vFirstCamPos = stashedInterpCamData.m_interpolatedCamFirstPos;
    Vec3 secondKeyPos = secondCamera->GetPosition();
    Vec3 interpolatedPos = vFirstCamPos + (secondKeyPos - vFirstCamPos) * t;

    firstCamera->SetPosition(interpolatedPos);

    ////////////////////////
    // interpolate Rotation
    Quat firstCameraRotation = stashedInterpCamData.m_interpolatedCamFirstRot;
    Quat secondCameraRotation = secondCamera->GetRotation();
    
    Quat interpolatedRotation;
    interpolatedRotation.SetSlerp(firstCameraRotation, secondCameraRotation, t);

    firstCamera->SetWorldRotation(interpolatedRotation);

    // clean-up
    if (secondCamera)
    {
        delete secondCamera;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyCameraKey(ISelectKey& key, SAnimContext& ec)
{
    ISelectKey nextKey;
    int nextCameraKeyNumber = m_CurrentSelectTrackKeyNumber + 1;
    bool bInterpolateCamera = false;

    if (nextCameraKeyNumber < m_CurrentSelectTrack->GetNumKeys())
    {
        m_CurrentSelectTrack->GetKey(nextCameraKeyNumber, &nextKey);

        float fInterTime = nextKey.time - ec.time;
        if (fInterTime >= 0 && fInterTime <= key.fBlendTime)
        {
            bInterpolateCamera = true;
        }
    }

    // check if we're finished interpolating and there is a camera node on hold for interpolation. If so, unset it from hold.
    if (!bInterpolateCamera && m_pCamNodeOnHoldForInterp)
    {
        m_pCamNodeOnHoldForInterp->SetSkipInterpolatedCameraNode(false);
        m_pCamNodeOnHoldForInterp = nullptr;
    }

    SCameraParams cameraParams;
    cameraParams.cameraEntityId.SetInvalid();
    cameraParams.fov = 0;
    cameraParams.justActivated = true;

    // With component entities, the fov and near plane may be animated on an 
    // entity with a Camera component. Don't stomp the values if this update happens
    // after those properties are animated.

    ///////////////////////////////////////////////////////////////////
    // find the Scene Camera (Camera Component Camera)  
    ISceneCamera* firstSceneCamera = nullptr;

    if (key.cameraAzEntityId.IsValid())
    {
        // camera component entity
        cameraParams.cameraEntityId = key.cameraAzEntityId;
        firstSceneCamera = static_cast<ISceneCamera*>(new CComponentEntitySceneCamera(key.cameraAzEntityId));
    }

    if (firstSceneCamera)
    {
        cameraParams.fov = DEG2RAD(firstSceneCamera->GetFoV());
    }   

    if (bInterpolateCamera && firstSceneCamera)
    {
        InterpolateCameras(cameraParams, firstSceneCamera, key, nextKey, ec.time);
    }
    
    // Broadcast camera changes
    const SCameraParams& lastCameraParams = gEnv->pMovieSystem->GetCameraParams();
    if (lastCameraParams.cameraEntityId != cameraParams.cameraEntityId)
    {
        Maestro::SequenceComponentNotificationBus::Event(
            m_pSequence->GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnCameraChanged,
            lastCameraParams.cameraEntityId, cameraParams.cameraEntityId);

        // note: only update the active view if we're currently exporting/capturing a sequence
        if (gEnv->pMovieSystem->IsInBatchRenderMode())
        {
            Camera::CameraRequestBus::Event(
                cameraParams.cameraEntityId, &Camera::CameraRequestBus::Events::MakeActiveView);
        }
    }
    
    gEnv->pMovieSystem->SetCameraParams(cameraParams);

    // This detects when we've switched from one Camera to another on the Camera Track
    // If cameras were interpolated (blended), reset cameras to their pre-interpolated positions and
    // clean up cached data used for the interpolation
    if (m_lastCameraKey != m_CurrentSelectTrackKeyNumber && m_lastCameraKey >= 0)
    {
        const auto& retStashedData = m_InterpolatingCameraStartStates.find(m_lastCameraKey);
        if (retStashedData != m_InterpolatingCameraStartStates.end())
        {
            InterpolatingCameraStartState stashedData = retStashedData->second;
            ISelectKey prevKey;
            ISceneCamera* prevSceneCamera = nullptr;

            m_CurrentSelectTrack->GetKey(m_lastCameraKey, &prevKey);

            if (prevKey.cameraAzEntityId.IsValid())
            {
                prevSceneCamera = static_cast<ISceneCamera*>(new CComponentEntitySceneCamera(prevKey.cameraAzEntityId));
            }
            
            if (prevSceneCamera)
            {
                prevSceneCamera->SetPosition(stashedData.m_interpolatedCamFirstPos);
                prevSceneCamera->SetRotation(stashedData.m_interpolatedCamFirstRot);
            }

            IAnimNode* prevCameraAnimNode = m_pSequence->FindNodeByName(prevKey.szSelection.c_str(), this);
            if (prevCameraAnimNode == nullptr)
            {
                prevCameraAnimNode = m_pSequence->FindNodeByName(prevKey.szSelection.c_str(), nullptr);
            }

            if (prevCameraAnimNode && prevCameraAnimNode->GetType() == AnimNodeType::Camera && prevCameraAnimNode->GetTrackForParameter(AnimParamType::FOV))
            {
                prevCameraAnimNode->SetParamValue(ec.time, AnimParamType::FOV, stashedData.m_FoV);
            }
            else if (prevSceneCamera)
            {
                prevSceneCamera->SetNearZAndFOVIfChanged(DEG2RAD(stashedData.m_FoV), stashedData.m_nearZ);
            }

            m_InterpolatingCameraStartStates.erase(m_lastCameraKey);

            // clean up
            if (prevSceneCamera)
            {
                delete prevSceneCamera;
            }
        }
    }

    // clean up
    if (firstSceneCamera)
    {
        delete firstSceneCamera;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyEventKey(IEventKey& key, [[maybe_unused]] SAnimContext& ec)
{
    char funcName[1024];
    azstrcpy(funcName, AZ_ARRAY_SIZE(funcName), "Event_");
    azstrcat(funcName, AZ_ARRAY_SIZE(funcName), key.event.c_str());
    gEnv->pMovieSystem->SendGlobalEvent(funcName);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyAudioKey(char const* const sTriggerName, bool const bPlay /* = true */)
{
    Audio::TAudioControlID nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
    Audio::AudioSystemRequestBus::BroadcastResult(nAudioTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, sTriggerName);
    if (nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
    {
        Audio::SAudioRequest oRequest;
        oRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        oRequest.pOwner = this;

        if (bPlay)
        {
            Audio::SAudioObjectRequestData<Audio::eAORT_EXECUTE_TRIGGER> oRequestData(nAudioTriggerID, 0.0f);
            oRequest.pData = &oRequestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            Audio::SAudioObjectRequestData<Audio::eAORT_STOP_TRIGGER> oRequestData(nAudioTriggerID);
            oRequest.pData = &oRequestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplySequenceKey(IAnimTrack* pTrack, [[maybe_unused]] int nPrevKey, int nCurrKey, ISequenceKey& key, SAnimContext& ec)
{
    if (nCurrKey < 0)
    {
        return;
    }
    IAnimSequence* pSequence = GetSequenceFromSequenceKey(key);
    if (!pSequence)
    {
        return;
    }

    if (key.bOverrideTimes)
    {
        key.fDuration = (key.fEndTime - key.fStartTime) > 0.0f ? (key.fEndTime - key.fStartTime) : 0.0f;
    }
    else
    {
        key.fDuration = pSequence->GetTimeRange().Length();
    }

    pTrack->SetKey(nCurrKey, &key);

    SAnimContext newAnimContext = ec;
    newAnimContext.time = std::min(ec.time - key.time + key.fStartTime, key.fDuration + key.fStartTime);

    if (static_cast<CAnimSequence*>(pSequence)->GetTime() != newAnimContext.time)
    {
        pSequence->Animate(newAnimContext);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyConsoleKey(IConsoleKey& key, [[maybe_unused]] SAnimContext& ec)
{
    if (!key.command.empty())
    {
        gEnv->pConsole->ExecuteString(key.command.c_str());
    }
}

void CAnimSceneNode::ApplyGotoKey(CGotoTrack*   poGotoTrack, SAnimContext& ec)
{
    IDiscreteFloatKey           stDiscreteFloadKey;
    int                                     nCurrentActiveKeyIndex(-1);

    nCurrentActiveKeyIndex = poGotoTrack->GetActiveKey(ec.time, &stDiscreteFloadKey);
    if (nCurrentActiveKeyIndex != m_nLastGotoKey && nCurrentActiveKeyIndex >= 0)
    {
        if (!ec.singleFrame)
        {
            if (stDiscreteFloadKey.m_fValue >= 0)
            {
                AZStd::string fullname = m_pSequence->GetName();
                GetMovieSystem()->GoToFrame(fullname.c_str(), stDiscreteFloadKey.m_fValue);
            }
        }
    }

    m_nLastGotoKey = nCurrentActiveKeyIndex;
}

/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimSceneNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);

    // To enable renaming even for previously saved director nodes
    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
}

void CAnimSceneNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAnimSceneNode, CAnimNode>()
            ->Version(1);
    }
}

void CAnimSceneNode::PrecacheStatic(float startTime)
{
    m_lastPrecachePoint = -1.f;

    const uint numTracks = GetTrackCount();
    for (uint trackIndex = 0; trackIndex < numTracks; ++trackIndex)
    {
        IAnimTrack* pAnimTrack = GetTrackByIndex(trackIndex);
        if (pAnimTrack->GetParameterType() == AnimParamType::Sequence)
        {
            CSequenceTrack* pSequenceTrack = static_cast<CSequenceTrack*>(pAnimTrack);

            const uint numKeys = pSequenceTrack->GetNumKeys();
            for (uint keyIndex = 0; keyIndex < numKeys; ++keyIndex)
            {
                ISequenceKey key;
                pSequenceTrack->GetKey(keyIndex, &key);

                CAnimSequence* pSubSequence = static_cast<CAnimSequence*>(GetSequenceFromSequenceKey(key));
                if (pSubSequence)
                {
                    pSubSequence->PrecacheStatic(startTime - (key.fStartTime + key.time));
                }
            }
        }
    }
}

void CAnimSceneNode::PrecacheDynamic(float time)
{
    const uint numTracks = GetTrackCount();
    float fLastPrecachePoint = m_lastPrecachePoint;

    for (uint trackIndex = 0; trackIndex < numTracks; ++trackIndex)
    {
        IAnimTrack* pAnimTrack = GetTrackByIndex(trackIndex);
        if (pAnimTrack->GetParameterType() == AnimParamType::Sequence)
        {
            CSequenceTrack* pSequenceTrack = static_cast<CSequenceTrack*>(pAnimTrack);

            const uint numKeys = pSequenceTrack->GetNumKeys();
            for (uint keyIndex = 0; keyIndex < numKeys; ++keyIndex)
            {
                ISequenceKey key;
                pSequenceTrack->GetKey(keyIndex, &key);

                CAnimSequence* pSubSequence = static_cast<CAnimSequence*>(GetSequenceFromSequenceKey(key));
                if (pSubSequence)
                {
                    pSubSequence->PrecacheDynamic(time - (key.fStartTime + key.time));
                }
            }
        }
        else if (pAnimTrack->GetParameterType() == AnimParamType::Camera)
        {
            const float fPrecacheCameraTime = CMovieSystem::m_mov_cameraPrecacheTime;
            if (fPrecacheCameraTime > 0.f)
            {
                CSelectTrack* pCameraTrack = static_cast<CSelectTrack*>(pAnimTrack);

                ISelectKey key;
                pCameraTrack->GetActiveKey(time + fPrecacheCameraTime, &key);

                if (time < key.time && (time + fPrecacheCameraTime) > key.time && key.time > m_lastPrecachePoint)
                {
                    fLastPrecachePoint = max(key.time, fLastPrecachePoint);
                }
            }
        }
    }

    m_lastPrecachePoint = fLastPrecachePoint;
}

void CAnimSceneNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    if (paramType.GetType() == AnimParamType::TimeWarp)
    {
        pTrack->SetValue(0.0f, 1.0f, true);
    }
}

/*static*/ IAnimSequence* CAnimSceneNode::GetSequenceFromSequenceKey(const ISequenceKey& sequenceKey)
{
    IAnimSequence* retSequence = nullptr;
    if (gEnv && gEnv->pMovieSystem)
    {
        if (sequenceKey.sequenceEntityId.IsValid())
        {
            retSequence = gEnv->pMovieSystem->FindSequence(sequenceKey.sequenceEntityId);
        }
        else if (!sequenceKey.szSelection.empty())
        {
            // legacy Deprecate ISequenceKey used names to identify sequences
            retSequence = gEnv->pMovieSystem->FindLegacySequenceByName(sequenceKey.szSelection.c_str());
        }
    }
    
    return retSequence;
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

