/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimAZEntityNode.h"
#include "AnimComponentNode.h"
#include "AnimSequence.h"
#include "AnimTrack.h"
#include "CaptureTrack.h"
#include "ConsoleTrack.h"
#include "EventTrack.h"
#include "GotoTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Movie.h"
#include "SceneNode.h"
#include "SequenceTrack.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Components/CameraBus.h>
#include <IAudioSystem.h>
#include <IConsole.h>
#include <ISystem.h>
#include <MathConversion.h>

namespace Maestro
{

    namespace AnimSceneNodeHelper
    {
        static bool s_nodeParamsInitialized = false;
        static AZStd::vector<CAnimNode::SParamInfo> s_nodeParams;

        static void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType, int flags = 0)
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
                : m_cameraEntityId(entityId)
            {
                AZ_Assert(entityId.IsValid(), "CComponentEntitySceneCamera ctor: invalid camera EntityId.");
                AZ::TransformBus::EventResult(m_cameraParentEntityId, m_cameraEntityId, &AZ::TransformBus::Events::GetParentId);
            }

            virtual ~CComponentEntitySceneCamera() = default;

            const AZ::Vector3 GetWorldPosition() const override
            {
                AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();
                AZ::TransformBus::EventResult(worldPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
                return worldPosition;
            }

            const AZ::Quaternion GetWorldRotation() const override
            {
                AZ::Quaternion worldRotation = AZ::Quaternion::CreateIdentity();
                AZ::TransformBus::EventResult(worldRotation, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                return worldRotation;
            }

            void SetWorldPosition(const AZ::Vector3& worldPosition) override
            {
                if (GetWorldPosition().IsClose(worldPosition))
                {
                    return;
                }
                AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTranslation, worldPosition);
            }

            void SetWorldRotation(const AZ::Quaternion& worldRotation) override
            {
                if (GetWorldRotation().IsClose(worldRotation))
                {
                    return;
                }
                if (!m_cameraParentEntityId.IsValid())
                {
                    AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, worldRotation);
                    return;
                }

                AZ::Transform parentWorldTM;
                AZ::Transform worldTM;
                AZ::TransformBus::EventResult(parentWorldTM, m_cameraParentEntityId, &AZ::TransformBus::Events::GetWorldTM);
                AZ::TransformBus::EventResult(worldTM, m_cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
                parentWorldTM.SetRotation(worldRotation);
                parentWorldTM.SetTranslation(worldTM.GetTranslation());
                AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetWorldTM, parentWorldTM);
            }

            float GetFoV() const override
            {
                float retFoV = -1;
                Camera::CameraRequestBus::EventResult(retFoV, m_cameraEntityId, &Camera::CameraComponentRequests::GetFovDegrees);
                return retFoV;
            }

            float GetNearZ() const override
            {
                float retNearZ = DEFAULT_NEAR;
                Camera::CameraRequestBus::EventResult(retNearZ, m_cameraEntityId, &Camera::CameraComponentRequests::GetNearClipDistance);
                return retNearZ;
            }

            void SetFovAndNearZ(float degreesFoV, float nearZ) override
            {
                if ((degreesFoV > 0.0f && degreesFoV < 180.0f) && !AZ::IsClose(GetFoV(), degreesFoV, AZ::Constants::FloatEpsilon))
                {
                    Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraComponentRequests::SetFovDegrees, degreesFoV);
                }
                if ((nearZ > AZ::Constants::Tolerance) && !AZ::IsClose(GetNearZ(), nearZ, AZ::Constants::FloatEpsilon))
                {
                    Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraComponentRequests::SetNearClipDistance, nearZ);
                }
            }

        private:
            AZ::EntityId m_cameraEntityId;
            AZ::EntityId m_cameraParentEntityId;
        };
    }

    CAnimSceneNode::CAnimSceneNode(const int id)
        : CAnimNode(id, AnimNodeType::Director)
    {
        m_lastEventKey = -1;
        m_lastConsoleKey = -1;
        m_lastSequenceKey = -1;
        m_nLastGotoKey = -1;
        m_lastCaptureKey = -1;
        m_bLastCapturingEnded = true;
        m_captureFrameCount = 0;
        m_CurrentSelectTrack = nullptr;
        SetName("Scene");

        CAnimSceneNode::Initialize();

        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
    }

    CAnimSceneNode::CAnimSceneNode()
        : CAnimSceneNode(0)
    {
    }

    CAnimSceneNode::~CAnimSceneNode()
    {
        ReleaseSounds();
    }

    void CAnimSceneNode::Initialize()
    {
        using namespace AnimSceneNodeHelper;
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

    void CAnimSceneNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::Camera);
    }

    unsigned int CAnimSceneNode::GetParamCount() const
    {
        return static_cast<unsigned int>(AnimSceneNodeHelper::s_nodeParams.size());
    }

    CAnimParamType CAnimSceneNode::GetParamType(unsigned int nIndex) const
    {
        using namespace AnimSceneNodeHelper;
        if (nIndex < s_nodeParams.size())
        {
            return s_nodeParams[nIndex].paramType;
        }

        return AnimParamType::Invalid;
    }

    bool CAnimSceneNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
    {
        using namespace AnimSceneNodeHelper;
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

    void CAnimSceneNode::Activate(bool bActivate)
    {
        CAnimNode::Activate(bActivate);

        if (!bActivate)
        {
            RestoreOverriddenCameraIdNeeded(); // In case of an override, 1st restore the overridden camera, order is significant
        }

        const int trackCount = NumTracks();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            const CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();
            if (!pTrack)
            {
                continue;
            }

            switch (paramId.GetType())
            {
            case AnimParamType::Sequence:
                {
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
                break;
            case AnimParamType::Camera:
                {
                    CSelectTrack* pSelectTrack = (CSelectTrack*)pTrack;
                    if (bActivate)
                    {
                        pSelectTrack->CalculateDurationForEachKey(); // Ensure keys are sorted by time and fDuration is calculated.
                    }

                    const int numKeys = pSelectTrack->GetNumKeys();
                    for (int currKeyIdx = 0; currKeyIdx < numKeys; ++currKeyIdx)
                    {
                        ISelectKey currKey;
                        pSelectTrack->GetKey(currKeyIdx, &currKey);

                        if (bActivate)
                        {
                            // Store camera properties in key, if not yet stored (the key is not initialized)
                            if (InitializeCameraProperties(currKey))
                            {
                                // {re-}set the key, recalculating fDuration for all keys
                                pSelectTrack->SetKey(currKeyIdx, &currKey);
                            }
                        }
                        else
                        {
                            // When deactivating, restore cameras' properties, if the key was initialized
                            RestoreCameraProperties(currKey);
                        }
                    }
                }
                break;
            default:
                break;
            }
        }
    }

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
        const auto overriden = OverrideCameraIdNeeded(); // Check if a camera override is set by CVar, an apply it when needed

        // If no camera override by CVar, use the camera track
        // In Editor, the "Autostart" sequence flag may state that camera must be switched to when playing, made in Animation context.
        const bool isAutostart = (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset) != 0;
        if (!overriden && cameraTrack && isAutostart)
        {
            ISelectKey key;
            int currCameraKeyIdx = cameraTrack->GetActiveKey(ec.time, &key);

            if (currCameraKeyIdx >= 0 && key.CheckValid())
            {
                if (!key.IsInitialized())
                {
                    if (InitializeCameraProperties(key))
                    {
                        cameraTrack->SetKey(currCameraKeyIdx, &key);
                        currCameraKeyIdx = cameraTrack->GetActiveKey(ec.time, &key);
                    }
                    else
                    {
                        currCameraKeyIdx = -1;
                    }
                }

                if (currCameraKeyIdx >= 0)
                {
                    m_CurrentSelectTrack = cameraTrack;
                    ApplyCameraKey(currCameraKeyIdx, key, ec);
                }
            }
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

        if (pCaptureTrack && m_movieSystem->IsInBatchRenderMode() == false)
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
                        AZ_Assert(false, "Last capturing must be ended");
                        m_movieSystem->EndCapture();
                        m_bLastCapturingEnded = true;
                    }
                    m_movieSystem->EnableFixedStepForCapture(key.timeStep);
                    m_movieSystem->StartCapture(key, m_captureFrameCount);
                    if (key.once == false)
                    {
                        m_bLastCapturingEnded = false;
                    }
                    m_lastCaptureKey = nCaptureKey;
                }
                else if (justEnded)
                {
                    m_movieSystem->DisableFixedStepForCapture();
                    m_movieSystem->EndCapture();
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

    bool CAnimSceneNode::OverrideCameraIdNeeded()
    {
        // Check if a valid camera override is set by ICVar in CMovieSystem
        if (!m_OverrideCamId.IsValid())
        {
            AZ::EntityId overrideCamId = AZ::EntityId();
            const char* overrideCamName = m_movieSystem->GetOverrideCamName();
            if (overrideCamName != nullptr && strlen(overrideCamName) > 0)
            {
                // overriding with a Camera Component entity is done by entityId (as names are not unique among AZ::Entities) - try to
                // convert string to u64 to see if it's an id
                AZ::u64 u64Id = strtoull(overrideCamName, nullptr, /*base (radix)*/ 10);
                if (u64Id)
                {
                    overrideCamId = AZ::EntityId(u64Id);
                }
            }
            if (overrideCamId.IsValid()) 
            {
                AZ::Entity* cameraEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(cameraEntity, &AZ::ComponentApplicationBus::Events::FindEntity, overrideCamId);
                if (cameraEntity)
                {
                    m_OverrideCamId = overrideCamId;
                }
            }
        }
        if (!m_OverrideCamId.IsValid())
        {
            m_overriddenCameraProperties.Reset();
            return false;
        }

        const auto lastCameraEntityId = m_movieSystem->GetActiveCamera();
        if (lastCameraEntityId == m_OverrideCamId)
        {
            return true; // no need to change camera
        }

        if (gEnv->IsEditor() && gEnv->IsEditing())
        {
            // Broadcast camera changes: works in editing mode only (when animating in Track View)
            if (lastCameraEntityId != m_OverrideCamId)
            {
                SequenceComponentNotificationBus::Event(m_pSequence->GetSequenceEntityId(),
                    &SequenceComponentNotificationBus::Events::OnCameraChanged, lastCameraEntityId, m_OverrideCamId);

                // note: only update the active view if we're currently exporting/capturing a sequence
                if (m_movieSystem->IsInBatchRenderMode())
                {
                    Camera::CameraRequestBus::Event(m_OverrideCamId, &Camera::CameraRequestBus::Events::MakeActiveView);
                }
            }
            m_movieSystem->SetActiveCamera(m_OverrideCamId);
            m_overriddenCameraProperties.Reset();
            return true;
        }

        if (!lastCameraEntityId.IsValid())
        {
            AZ_Error("AnimSceneNode", false, "OverrideCameraIdNeeded(): invalid active camera EntityId in Game mode.");
            m_overriddenCameraProperties.Reset();
            return false;
        }

        // In Play Game mode the active camera parameters are to be stored and then changed.
        auto activeCameraHelper = static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(lastCameraEntityId));
        m_overriddenCameraProperties.szSelection = "StoredCamera";
        m_overriddenCameraProperties.cameraAzEntityId = lastCameraEntityId;
        m_overriddenCameraProperties.m_position = activeCameraHelper->GetWorldPosition(); // stash Transform
        m_overriddenCameraProperties.m_rotation = activeCameraHelper->GetWorldRotation();
        m_overriddenCameraProperties.m_FoV = activeCameraHelper->GetFoV(); // stash FoV from the first camera entity
        m_overriddenCameraProperties.m_nearZ = activeCameraHelper->GetNearZ(); // stash nearZ

        auto overrideCameraHelper = static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(m_OverrideCamId));
        activeCameraHelper->SetWorldPosition(overrideCameraHelper->GetWorldPosition());
        activeCameraHelper->SetWorldRotation(overrideCameraHelper->GetWorldRotation());
        activeCameraHelper->SetFovAndNearZ(overrideCameraHelper->GetFoV(), overrideCameraHelper->GetNearZ());
        delete overrideCameraHelper;

        delete activeCameraHelper;
        return true;
    }

    bool CAnimSceneNode::RestoreOverriddenCameraIdNeeded()
    {
        const auto invalidId = AZ::EntityId();
        m_OverrideCamId = invalidId;
        if (!m_overriddenCameraProperties.CheckValid())
        {
            return false;
        }
        auto overridenCameraHelper = static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(m_OverrideCamId));
        overridenCameraHelper->SetWorldPosition(m_overriddenCameraProperties.m_position);
        overridenCameraHelper->SetWorldRotation(m_overriddenCameraProperties.m_rotation);
        overridenCameraHelper->SetFovAndNearZ(m_overriddenCameraProperties.m_FoV, m_overriddenCameraProperties.m_nearZ);
        delete overridenCameraHelper;
        return true;
    }


    void CAnimSceneNode::OnReset()
    {
        RestoreOverriddenCameraIdNeeded(); // In case of an override, 1st restore the overridden camera, order is significant

        const int trackCount = NumTracks();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();

            switch (paramId.GetType())
            {
            case AnimParamType::Sequence:
                if (pTrack && m_lastSequenceKey >= 0)
                {
                    CSequenceTrack* pSequenceTrack = (CSequenceTrack*)m_tracks[paramIndex].get();
                    ISequenceKey prevKey;
                    pSequenceTrack->GetKey(m_lastSequenceKey, &prevKey);
                    IAnimSequence* sequence = GetSequenceFromSequenceKey(prevKey);
                    if (sequence)
                    {
                        GetMovieSystem()->StopSequence(sequence);
                    }
                    m_lastSequenceKey = -1;
                }
                break;
            case AnimParamType::Camera:
                if (pTrack)
                {
                    // restore cameras' properties, if available
                    CSelectTrack* pSelectTrack = (CSelectTrack*)pTrack;
                    for (int currKeyIdx = 0; currKeyIdx < pSelectTrack->GetNumKeys(); ++currKeyIdx)
                    {
                        ISelectKey currKey;
                        pSelectTrack->GetKey(currKeyIdx, &currKey);
                        RestoreCameraProperties(currKey);
                    }
                }
                break;
            default:
                break;
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

    bool CAnimSceneNode::InitializeCameraProperties(ISelectKey& key) const
    {
        if (!key.CheckValid() || key.IsInitialized())
        {
            return false;
        }

        ISceneCamera* currCamera =static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(key.cameraAzEntityId));
        key.m_FoV = currCamera->GetFoV();
        key.m_nearZ = currCamera->GetNearZ();
        key.m_position = currCamera->GetWorldPosition();
        key.m_rotation = currCamera->GetWorldRotation();
        delete currCamera;

        return true;
    }

    void CAnimSceneNode::RestoreCameraProperties(ISelectKey& key) const
    {
        if (!key.IsInitialized())
        {
            return;
        }

        ISceneCamera* currCamera = static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(key.cameraAzEntityId));
        currCamera->SetFovAndNearZ(key.m_FoV, key.m_nearZ);
        currCamera->SetWorldPosition(key.m_position);
        currCamera->SetWorldRotation(key.m_rotation);
        delete currCamera;
    }

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

    void CAnimSceneNode::ResetSounds()
    {
        for (int i = static_cast<int>(m_SoundInfo.size()); --i >= 0; )
        {
            m_SoundInfo[i].Reset();
        }
    }

    void CAnimSceneNode::ReleaseSounds()
    {
        // Stop all sounds on the global audio object,
        // but we want to have it filter based on the owner (this)
        // so we don't stop sounds that didn't originate with track view.

        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
        {
            Audio::ObjectRequest::StopAllTriggers stopAll;
            stopAll.m_filterByOwner = true;
            stopAll.m_owner = this;
            audioSystem->PushRequest(AZStd::move(stopAll));
        }
    }

    void CAnimSceneNode::ApplyCameraKey(int currKeyIdx, ISelectKey& currKey, const SAnimContext& ec)
    {
        if (!m_movieSystem || !m_pSequence || !m_CurrentSelectTrack || !currKey.IsInitialized())
        {
            [[maybe_unused]] const auto text =
                AZStd::string::format("ApplyCameraKey(%d, %s, time=%f)", currKeyIdx, currKey.cameraAzEntityId.ToString().c_str(), ec.time).c_str();
            AZ_Assert(m_movieSystem, "%s: invalid movie system pointer.", text);
            AZ_Assert(m_pSequence, "%s: invalid sequence pointer.", text);
            AZ_Assert(m_CurrentSelectTrack, "%s: invalid track pointer.", text);
            AZ_Assert(currKey.IsInitialized(), "%s: invalid key.", text);
            return; // internal error
        }

        const auto numKeys = m_CurrentSelectTrack->GetNumKeys();
        if (numKeys < 1)
        {
            AZ_Assert(false, "ApplyCameraKey(%d, %s, time=%f): no keys in track.", currKeyIdx, currKey.cameraAzEntityId.ToString().c_str(), ec.time);
            return; // internal error
        }

        // Find 2nd key to interpolate to, skipping invalid keys.
        int secondKeyIdx = -1;
        ISelectKey secondKey;
        for (int nextKeyIdx = currKeyIdx + 1; nextKeyIdx < numKeys; ++nextKeyIdx)
        {
            m_CurrentSelectTrack->GetKey(nextKeyIdx, &secondKey);
            if (secondKey.CheckValid())
            {
                secondKeyIdx = nextKeyIdx;
                break;
            }
        }

        if (secondKeyIdx < 0) // No valid differing key after the current key -> enforce the pair of same keys
        {
            secondKeyIdx = currKeyIdx;
            secondKey = currKey;
        }

        // In Play Game mode switching cameras is unsupported, so the active camera parameters are to be changed.
        const bool isEditing = gEnv->IsEditor() && gEnv->IsEditing();
        // In Editor, the "Autostart" sequence flag may state that camera must be switched to when playing, made in Animation context.
        const bool isAutostart = (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset) != 0;

        // Find the active camera
        AZ::EntityId activeCameraId; 
        Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequests::GetActiveCamera);

        if (!activeCameraId.IsValid()) // Invalid camera EntityId means that a default Editor view is in use.
        {
            if (!isEditing)
            {
                return; // When starting Play Game in Editor, camera EntityId may still be invalid for a couple of frames.
            }

            // Corner case: user switched to the default Editor camera before starting animation
            activeCameraId = currKey.cameraAzEntityId;
            m_movieSystem->SetActiveCamera(activeCameraId);
            if (isAutostart)
            {
                SequenceComponentNotificationBus::Event(m_pSequence->GetSequenceEntityId(),
                    &SequenceComponentNotificationBus::Events::OnCameraChanged, activeCameraId, activeCameraId);
                Camera::CameraRequestBus::Event(activeCameraId, &Camera::CameraRequestBus::Events::MakeActiveView);
                m_movieSystem->SetActiveCamera(activeCameraId);
            }
        }

        // Switch to the current camera if needed.
        const auto lastCameraEntityId = m_movieSystem->GetActiveCamera();
        if (lastCameraEntityId != currKey.cameraAzEntityId)
        {
            // Broadcast camera changes: works in editing mode only, when animating in Track View with the "Autostart" (eSeqFlags_PlayOnReset) flag cleared
            if (isEditing && !isAutostart)
            {
                SequenceComponentNotificationBus::Event(m_pSequence->GetSequenceEntityId(),
                    &SequenceComponentNotificationBus::Events::OnCameraChanged, lastCameraEntityId, currKey.cameraAzEntityId);
                // note: only update the active view if we're currently exporting/capturing a sequence
                if (m_movieSystem->IsInBatchRenderMode())
                {
                    Camera::CameraRequestBus::Event(currKey.cameraAzEntityId, &Camera::CameraRequestBus::Events::MakeActiveView);
                }
            }
            m_movieSystem->SetActiveCamera(currKey.cameraAzEntityId);
        }

        // Interpolate and apply camera properties always, unchanged values will not actually be transferred. 
        {
            // A valid Scene Camera (Camera Component Camera) helper is needed to apply camera properties.
            auto activeCamera = static_cast<ISceneCamera*>(new AnimSceneNodeHelper::CComponentEntitySceneCamera(activeCameraId));

            // time interpolation parameter
            float t = (currKey.fBlendTime < AZ::Constants::Tolerance)
                ? 0.0f // corner case for no blending
                : 1.0f - (secondKey.time - ec.time) / currKey.fBlendTime;
            t = AZ::GetClamp(t, 0.0f, 1.0f); // "t" can be negative when interpolating before blending starts;
            t = aznumeric_cast<float>(pow(t, 3) * (t * (t * 6 - 15) + 10)); // use a cubic curve for the camera blend

            // Interpolate and update camera's FOV (in degrees) and Near Clip Distance
            const float interpolatedFoV = currKey.m_FoV + (secondKey.m_FoV - currKey.m_FoV) * t;
            const float interpolatedNearZ = currKey.m_nearZ + (secondKey.m_nearZ - currKey.m_nearZ) * t;
            activeCamera->SetFovAndNearZ(interpolatedFoV, interpolatedNearZ);
            // Interpolate and update camera's Position linearly
            const AZ::Vector3& firstKeyPos = currKey.m_position;
            activeCamera->SetWorldPosition(firstKeyPos + (secondKey.m_position - firstKeyPos) * t);
            // Interpolate and update camera's Rotation linearly-spherically
            activeCamera->SetWorldRotation(currKey.m_rotation.Slerp(secondKey.m_rotation, t).GetNormalized());

            // clean-up
            delete activeCamera;
        }
    }

    void CAnimSceneNode::ApplyEventKey(IEventKey& key, [[maybe_unused]] SAnimContext& ec)
    {
        char funcName[1024];
        azstrcpy(funcName, AZ_ARRAY_SIZE(funcName), "Event_");
        azstrcat(funcName, AZ_ARRAY_SIZE(funcName), key.event.c_str());
        m_movieSystem->SendGlobalEvent(funcName);
    }

    void CAnimSceneNode::ApplyAudioKey(char const* const sTriggerName, bool const bPlay /* = true */)
    {
        Audio::TAudioControlID nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
        {
            nAudioTriggerID = audioSystem->GetAudioTriggerID(sTriggerName);

            if (nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                if (bPlay)
                {
                    Audio::ObjectRequest::ExecuteTrigger execTrigger;
                    execTrigger.m_triggerId = nAudioTriggerID;
                    execTrigger.m_owner = this;
                    audioSystem->PushRequest(AZStd::move(execTrigger));
                }
                else
                {
                    Audio::ObjectRequest::StopTrigger stopTrigger;
                    stopTrigger.m_triggerId = nAudioTriggerID;
                    stopTrigger.m_owner = this;
                    audioSystem->PushRequest(AZStd::move(stopTrigger));
                }
            }
        }
    }

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
        newAnimContext.time = AZStd::min(ec.time - key.time + key.fStartTime, key.fDuration + key.fStartTime);

        if (static_cast<CAnimSequence*>(pSequence)->GetTime() != newAnimContext.time)
        {
            pSequence->Animate(newAnimContext);
        }
    }

    void CAnimSceneNode::ApplyConsoleKey(IConsoleKey& key, [[maybe_unused]] SAnimContext& ec)
    {
        if (!key.command.empty())
        {
            gEnv->pConsole->ExecuteString(key.command.c_str());
        }
    }

    void CAnimSceneNode::ApplyGotoKey(CGotoTrack*   poGotoTrack, SAnimContext& ec)
    {
        IDiscreteFloatKey stDiscreteFloadKey;
        int nCurrentActiveKeyIndex(-1);

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
        }
    }

    void CAnimSceneNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType, [[maybe_unused]] AnimValueType remapValueType)
    {
        if (paramType.GetType() == AnimParamType::TimeWarp)
        {
            pTrack->SetValue(0.0f, 1.0f, true);
        }
    }

    /*static*/ IAnimSequence* CAnimSceneNode::GetSequenceFromSequenceKey(const ISequenceKey& sequenceKey)
    {
        IAnimSequence* retSequence = nullptr;

        IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
        if (movieSystem)
        {
            if (sequenceKey.sequenceEntityId.IsValid())
            {
                retSequence = movieSystem->FindSequence(sequenceKey.sequenceEntityId);
            }
            else if (!sequenceKey.szSelection.empty())
            {
                // legacy Deprecate ISequenceKey used names to identify sequences
                retSequence = movieSystem->FindLegacySequenceByName(sequenceKey.szSelection.c_str());
            }
        }

        return retSequence;
    }

} // namespace Maestro
