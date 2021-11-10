/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "AnimationContext.h"

// CryCommon
#include <CryCommon/Maestro/Bus/EditorSequenceBus.h>

// Editor
#include "TrackView/TrackViewDialog.h"
#include "ViewManager.h"
#include "Objects/SelectionGroup.h"
#include "Include/IObjectManager.h"
#include "Objects/EntityObject.h"

#include <AzCore/Time/ITime.h>

//////////////////////////////////////////////////////////////////////////
// Movie Callback.
//////////////////////////////////////////////////////////////////////////
class CMovieCallback
    : public IMovieCallback
{
protected:
    void OnMovieCallback(ECallbackReason reason, [[maybe_unused]] IAnimNode* pNode) override
    {
        switch (reason)
        {
        case CBR_CHANGENODE:
            // Invalidate nodes
            break;
        case CBR_CHANGETRACK:
        {
            // Invalidate tracks
            CTrackViewDialog* pTrackViewDialog = CTrackViewDialog::GetCurrentInstance();
            if (pTrackViewDialog)
            {
                pTrackViewDialog->InvalidateDopeSheet();
            }
        }
        break;
        }
    }

    void OnSetCamera(const SCameraParams& Params) override
    {
        // Only switch camera when in Play mode.
        GUID camObjId = GUID_NULL;
        if (Params.cameraEntityId.IsValid())
        {
            // Find owner editor entity.
            CEntityObject* pEditorEntity = CEntityObject::FindFromEntityId(Params.cameraEntityId);
            if (pEditorEntity)
            {
                camObjId = pEditorEntity->GetId();
            }
        }

        // Switch camera in active rendering view.
        if (GetIEditor()->GetViewManager())
        {
            GetIEditor()->GetViewManager()->SetCameraObjectId(camObjId);
        }
    };

    bool IsSequenceCamUsed() const override
    {
        if (gEnv->IsEditorGameMode() == true)
        {
            return true;
        }

        if (GetIEditor()->GetViewManager() == nullptr)
        {
            return false;
        }

        CViewport* pRendView = GetIEditor()->GetViewManager()->GetViewport(ET_ViewportCamera);
        if (pRendView)
        {
            return pRendView->IsSequenceCamera();
        }

        return false;
    }
};

static CMovieCallback s_movieCallback;

//////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//!
class CAnimationContextPostRender
    : public IPostRenderer
{
public:
    CAnimationContextPostRender(CAnimationContext* pAC)
        : m_pAC(pAC){}

    void OnPostRender() const override { assert(m_pAC); m_pAC->OnPostRender(); }

protected:
    CAnimationContext* m_pAC;
};

//////////////////////////////////////////////////////////////////////////
CAnimationContext::CAnimationContext()
{
    m_paused = 0;
    m_playing = false;
    m_recording = false;
    m_bSavedRecordingState = false;
    m_timeRange.Set(0, 0);
    m_timeMarker.Set(0, 0);
    m_currTime = 0.0f;
    m_lastTimeChangedNotificationTime = .0f;
    m_bForceUpdateInNextFrame = false;
    m_fTimeScale = 1.0f;
    m_pSequence = nullptr;
    m_mostRecentSequenceId.SetInvalid();
    m_mostRecentSequenceTime = 0.0f;
    m_bLooping = false;
    m_bAutoRecording = false;
    m_fRecordingTimeStep = 0;
    m_bSingleFrame = false;
    m_bPostRenderRegistered = false;
    m_bForcingAnimation = false;
    GetIEditor()->GetUndoManager()->AddListener(this);
    GetIEditor()->GetSequenceManager()->AddListener(this);
    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CAnimationContext::~CAnimationContext()
{
    GetIEditor()->GetSequenceManager()->RemoveListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Init()
{
    gEnv->pMovieSystem->SetCallback(&s_movieCallback);

    REGISTER_COMMAND("mov_goToFrameEditor", (ConsoleCommandFunc)GoToFrameCmd, 0, "Make a specified sequence go to a given frame time in the editor.");
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::AddListener(IAnimationContextListener* pListener)
{
    stl::push_back_unique(m_contextListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::RemoveListener(IAnimationContextListener* pListener)
{
    stl::find_and_erase(m_contextListeners, pListener);
}

void CAnimationContext::NotifyTimeChangedListenersUsingCurrTime() const
{
    for (size_t i = 0; i < m_contextListeners.size(); ++i)
    {
        m_contextListeners[i]->OnTimeChanged(m_currTime);
    }
    m_lastTimeChangedNotificationTime = m_currTime;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SetSequence(CTrackViewSequence* sequence, bool force, bool noNotify, bool user)
{
    float newSeqStartTime = .0f;
    CTrackViewSequence* pCurrentSequence = m_pSequence;

    if (!force && sequence == pCurrentSequence)
    {
        return;
    }

    // Prevent keys being created from time change
    const bool bRecording = m_recording;
    m_recording = false;
    SetRecordingInternal(false);

    if (sequence)
    {
        newSeqStartTime = sequence->GetTimeRange().start;
    }

    m_currTime = newSeqStartTime;
    m_fRecordingCurrTime = newSeqStartTime;

    if (!m_bPostRenderRegistered)
    {
        if (GetIEditor() && GetIEditor()->GetViewManager())
        {
            CViewport* pViewport = GetIEditor()->GetViewManager()->GetViewport(ET_ViewportCamera);
            if (pViewport)
            {
                pViewport->AddPostRenderer(new CAnimationContextPostRender(this));
                m_bPostRenderRegistered = true;
            }
        }
    }

    if (m_pSequence)
    {
        m_pSequence->Deactivate();
        if (m_playing)
        {
            m_pSequence->EndCutScene();
        }

        m_pSequence->UnBindFromEditorObjects();
    }
    m_pSequence = sequence;

    // Notify a new sequence was just selected.
    Maestro::EditorSequenceNotificationBus::Broadcast(&Maestro::EditorSequenceNotificationBus::Events::OnSequenceSelected, m_pSequence ? m_pSequence->GetSequenceComponentEntityId() : AZ::EntityId());

    if (m_pSequence)
    {
        // Set the last valid sequence that was selected.
        m_mostRecentSequenceId = m_pSequence->GetSequenceComponentEntityId();

        if (m_playing)
        {
            m_pSequence->BeginCutScene(true);
        }

        m_timeRange = m_pSequence->GetTimeRange();
        m_timeMarker = m_timeRange;

        m_pSequence->Activate();
        m_pSequence->PrecacheData(newSeqStartTime);

        m_pSequence->BindToEditorObjects();
    }
    else if (user)
    {
        // If this was a sequence that was selected by the user in Track View
        // and it was "No Sequence" clear the m_mostRecentSequenceId so the sequence
        // will not be reselected at unwanted events like a slice reload or an undo operation.
        m_mostRecentSequenceId.SetInvalid();
    }

    ForceAnimation();

    if (!noNotify)
    {
        NotifyTimeChangedListenersUsingCurrTime();
        for (size_t i = 0; i < m_contextListeners.size(); ++i)
        {
            m_contextListeners[i]->OnSequenceChanged(m_pSequence);
        }
    }

    TimeChanged(newSeqStartTime);

    m_recording = bRecording;
    SetRecordingInternal(bRecording);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::UpdateTimeRange()
{
    if (m_pSequence)
    {
        m_timeRange = m_pSequence->GetTimeRange();

        // reset the current time to make sure it is clamped
        // to the new range.
        SetTime(m_currTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SetTime(float t)
{
    if (t < m_timeRange.start)
    {
        t = m_timeRange.start;
    }

    if (t > m_timeRange.end)
    {
        t = m_timeRange.end;
    }

    if (fabs(m_currTime - t) < 0.001f)
    {
        return;
    }

    m_currTime = t;
    m_fRecordingCurrTime = t;
    ForceAnimation();
    UpdateAnimatedLights();

    NotifyTimeChangedListenersUsingCurrTime();
}

void CAnimationContext::TimeChanged(float newTime)
{
    if (m_pSequence)
    {
        m_mostRecentSequenceTime = newTime;
        m_pSequence->TimeChanged(newTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnSequenceActivated(AZ::EntityId entityId)
{
    // If nothing is selected and there is a valid most recent selected
    // try to find that sequence by id and select it. This is useful
    // for restoring the selected sequence during undo and redo.
    if (m_pSequence == nullptr && m_mostRecentSequenceId.IsValid())
    {
        if (entityId == m_mostRecentSequenceId)
        {
            auto editor = GetIEditor();
            if (editor != nullptr)
            {
                auto manager = editor->GetSequenceManager();
                if (manager != nullptr)
                {
                    auto sequence = manager->GetSequenceByEntityId(m_mostRecentSequenceId);
                    if (sequence != nullptr)
                    {
                        // Hang onto this because SetSequence() will reset it.
                        float lastTime = m_mostRecentSequenceTime;

                        SetSequence(sequence, false, false);

                        // Restore the current time.
                        SetTime(lastTime);

                        // Notify time may have changed, use m_currTime incase it was clamped by SetTime()
                        TimeChanged(m_currTime);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Pause()
{
    assert(m_paused >= 0);
    m_paused++;

    if (m_recording)
    {
        SetRecordingInternal(false);
    }

    GetIEditor()->GetMovieSystem()->Pause();
    if (m_pSequence)
    {
        m_pSequence->Pause();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Resume()
{
    assert(m_paused > 0);
    m_paused--;
    if (m_recording && m_paused == 0)
    {
        SetRecordingInternal(true);
    }
    GetIEditor()->GetMovieSystem()->Resume();

    if (m_pSequence)
    {
        m_pSequence->Resume();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SetRecording(bool recording)
{
    if (recording == m_recording)
    {
        return;
    }
    m_paused = 0;
    m_recording = recording;
    m_playing = false;

    if (!recording && m_fRecordingTimeStep != 0)
    {
        SetAutoRecording(false, 0);
    }

    // If started recording, assume we have modified the document.
    GetIEditor()->SetModifiedFlag();

    SetRecordingInternal(recording);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SetPlaying(bool playing)
{
    if (playing == m_playing)
    {
        return;
    }

    m_paused = 0;
    m_playing = playing;
    m_recording = false;
    SetRecordingInternal(false);

    if (playing)
    {
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();

        pMovieSystem->Resume();
        if (m_pSequence)
        {
            m_pSequence->Resume();

            IMovieUser* pMovieUser = pMovieSystem->GetUser();

            if (pMovieUser)
            {
                m_pSequence->BeginCutScene(true);
            }
        }
        pMovieSystem->ResumeCutScenes();
    }
    else
    {
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();

        pMovieSystem->Pause();

        if (m_pSequence)
        {
            m_pSequence->Pause();
        }

        pMovieSystem->PauseCutScenes();
        if (m_pSequence)
        {
            IMovieUser* pMovieUser = pMovieSystem->GetUser();

            if (pMovieUser)
            {
                m_pSequence->EndCutScene();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Update()
{
    if (m_bForceUpdateInNextFrame)
    {
        ForceAnimation();
        m_bForceUpdateInNextFrame = false;
    }

    // If looking through camera object and recording animation, do not allow camera shake
    if ((GetIEditor()->GetViewManager()->GetCameraObjectId() != GUID_NULL) && GetIEditor()->GetAnimation()->IsRecording())
    {
        GetIEditor()->GetMovieSystem()->EnableCameraShake(false);
    }
    else
    {
        GetIEditor()->GetMovieSystem()->EnableCameraShake(true);
    }

    if (m_paused > 0 || !(m_playing || m_bAutoRecording))
    {
        if (m_pSequence)
        {
            m_pSequence->StillUpdate();
        }

        if (!m_recording)
        {
            GetIEditor()->GetMovieSystem()->StillUpdate();
        }

        return;
    }

    const AZ::TimeUs frameDeltaTimeUs = AZ::GetSimulationTickDeltaTimeUs();
    const float frameDeltaTime = AZ::TimeUsToSeconds(frameDeltaTimeUs);

    if (!m_bAutoRecording)
    {
        AnimateActiveSequence();

        m_currTime += frameDeltaTime * m_fTimeScale;

        if (!m_recording)
        {
            GetIEditor()->GetMovieSystem()->PreUpdate(frameDeltaTime);
            GetIEditor()->GetMovieSystem()->PostUpdate(frameDeltaTime);
        }
    }
    else
    {
        m_fRecordingCurrTime += frameDeltaTime * m_fTimeScale;
        if (fabs(m_fRecordingCurrTime - m_currTime) > m_fRecordingTimeStep)
        {
            m_currTime += m_fRecordingTimeStep;
        }
    }

    if (m_currTime > m_timeMarker.end)
    {
        if (m_bAutoRecording)
        {
            SetAutoRecording(false, 0);
        }
        else
        {
            if (m_bLooping)
            {
                m_currTime = m_timeMarker.start;
                if (m_pSequence)
                {
                    m_pSequence->OnLoop();
                }
            }
            else
            {
                SetPlaying(false);
                m_currTime = m_timeMarker.end;
            }
        }
    }

    if (m_bAutoRecording)
    {
        // This is auto recording mode.
        // Send sync with physics event to all selected entities.
        GetIEditor()->GetSelection()->SendEvent(EVENT_PHYSICS_GETSTATE);
    }

    if (fabs(m_lastTimeChangedNotificationTime - m_currTime) > 0.001f)
    {
        NotifyTimeChangedListenersUsingCurrTime();
    }

    UpdateAnimatedLights();
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::ForceAnimation()
{
    if (m_bForcingAnimation)
    {
        // reentrant calls are possible when using subsequences
        return;
    }

    m_bForcingAnimation = true;

    // Before animating node, pause recording.
    if (m_bAutoRecording)
    {
        Pause();
    }

    AnimateActiveSequence();
    // Animate a second time to properly update camera DoF
    AnimateActiveSequence();

    if (m_bAutoRecording)
    {
        Resume();
    }

    m_bForcingAnimation = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SetAutoRecording(bool bEnable, float fTimeStep)
{
    if (bEnable)
    {
        m_bAutoRecording = true;
        m_fRecordingTimeStep = fTimeStep;
        SetRecording(bEnable);
    }
    else
    {
        m_bAutoRecording = false;
        m_fRecordingTimeStep = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::GoToFrameCmd(IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 2)
    {
        gEnv->pLog->LogError("GoToFrame: You must provide a 'frame time' to go to");
        return;
    }

    assert(GetIEditor()->GetAnimation());
    CTrackViewSequence* pSeq = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSeq)
    {
        gEnv->pLog->LogError("GoToFrame: No active animation sequence");
        return;
    }

    float targetFrame = (float)atof(pArgs->GetArg(1));
    if (pSeq->GetTimeRange().start > targetFrame || targetFrame > pSeq->GetTimeRange().end)
    {
        gEnv->pLog->LogError("GoToFrame: requested time %f is outside the range of sequence %s (%f, %f)", targetFrame, pSeq->GetName().c_str(), pSeq->GetTimeRange().start, pSeq->GetTimeRange().end);
        return;
    }
    GetIEditor()->GetAnimation()->m_currTime = targetFrame;
    GetIEditor()->GetAnimation()->m_bSingleFrame = true;

    GetIEditor()->GetAnimation()->ForceAnimation();
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnPostRender()
{
    if (m_pSequence)
    {
        SAnimContext ac;
        ac.dt = 0;
        const AZ::TimeUs frameDeltaTimeUs = AZ::GetSimulationTickDeltaTimeUs();
        const float frameDeltaTime = AZ::TimeUsToSeconds(frameDeltaTimeUs);
        ac.fps = 1.0f / frameDeltaTime;
        ac.time = m_currTime;
        ac.singleFrame = true;
        ac.forcePlay = true;
        m_pSequence->Render(ac);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::UpdateAnimatedLights()
{
    bool bLightAnimationSetActive = m_pSequence && (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet);
    if (bLightAnimationSetActive == false)
    {
        return;
    }

    std::vector<CBaseObject*> entityObjects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CEntityObject::staticMetaObject, entityObjects);
    std::for_each(std::begin(entityObjects), std::end(entityObjects),
        [this](CBaseObject* pBaseObject)
        {
            CEntityObject* pEntityObject = static_cast<CEntityObject*>(pBaseObject);
            bool bLight = pEntityObject && pEntityObject->GetEntityClass().compare("Light") == 0;
            if (bLight)
            {
                bool bTimeScrubbing = pEntityObject->GetEntityPropertyBool("bTimeScrubbingInTrackView");
                if (bTimeScrubbing)
                {
                    pEntityObject->SetEntityPropertyFloat("_fTimeScrubbed", m_currTime);
                }
            }
        });
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::BeginUndoTransaction()
{
    m_bSavedRecordingState = m_recording;
    SetRecordingInternal(false);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::EndUndoTransaction()
{
    if (m_pSequence)
    {
        m_pSequence->BindToEditorObjects();
    }

    SetRecordingInternal(m_bSavedRecordingState);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::TogglePlay()
{
    if (!IsPlaying())
    {
        SetPlaying(true);
    }
    else
    {
        SetPlaying(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnSequenceRemoved(CTrackViewSequence* pSequence)
{
    if (m_pSequence == pSequence)
    {
        SetSequence(nullptr, true, false);
    }
}
//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        if (m_pSequence)
        {
            m_pSequence->Resume();
        }
    case eNotify_OnBeginSceneSave:
    case eNotify_OnBeginLayerExport:
        if (m_pSequence)
        {
            m_sequenceToRestore = m_pSequence->GetSequenceComponentEntityId();
        }
        else
        {
            m_sequenceToRestore.SetInvalid();
        }

        m_sequenceRestoreTime = GetTime();

        m_bSavedRecordingState = m_recording;
        SetRecordingInternal(false);
        SetSequence(nullptr, true, true);
        break;

    case eNotify_OnEndGameMode:
    case eNotify_OnEndSceneSave:
    case eNotify_OnEndLayerExport:
        m_currTime = m_sequenceRestoreTime;
        SetSequence(GetIEditor()->GetSequenceManager()->GetSequenceByEntityId(m_sequenceToRestore), true, true);
        SetTime(m_sequenceRestoreTime);

        SetRecordingInternal(m_bSavedRecordingState);
        break;

    case eNotify_OnQuit:
    case eNotify_OnCloseScene:
        SetSequence(nullptr, true, false);
        break;

    case eNotify_OnBeginNewScene:
        SetSequence(nullptr, false, false);
        break;

    case eNotify_OnBeginLoad:
        m_mostRecentSequenceId.SetInvalid();
        m_mostRecentSequenceTime = 0.0f;
        m_bSavedRecordingState = m_recording;
        SetRecordingInternal(false);
        GetIEditor()->GetAnimation()->SetSequence(nullptr, false, false);
        break;

    case eNotify_OnEndLoad:
        SetRecordingInternal(m_bSavedRecordingState);
        break;
    }
}

void CAnimationContext::SetRecordingInternal(bool enableRecording)
{
    GetIEditor()->GetMovieSystem()->SetRecording(enableRecording);
    if (m_pSequence)
    {
        m_pSequence->SetRecording(enableRecording);
    }
}

void CAnimationContext::AnimateActiveSequence()
{
    if (!m_pSequence)
    {
        return;
    }

    SAnimContext ac;
    ac.dt = 0;
    const AZ::TimeUs frameDeltaTimeUs = AZ::GetSimulationTickDeltaTimeUs();
    const float frameDeltaTime = AZ::TimeUsToSeconds(frameDeltaTimeUs);
    ac.fps = 1.0f / frameDeltaTime;
    ac.time = m_currTime;
    ac.singleFrame = true;
    ac.forcePlay = true;

    m_pSequence->Animate(ac);
    m_pSequence->SyncToConsole(ac);
}
