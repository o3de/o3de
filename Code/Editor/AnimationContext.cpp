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

#include <AzCore/Serialization/Locale.h>
#include <AzCore/Time/ITime.h>

#include <AzToolsFramework/API/EditorCameraBus.h>

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
    : m_movieSystem(AZ::Interface<IMovieSystem>::Get())
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
    AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CAnimationContext::~CAnimationContext()
{
    if (Maestro::SequenceComponentNotificationBus::Handler::BusIsConnected())
    {
        Maestro::SequenceComponentNotificationBus::Handler::BusDisconnect();
    }
    AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
    GetIEditor()->GetSequenceManager()->RemoveListener(this);
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Init()
{
    if (m_movieSystem)
    {
        m_movieSystem->SetCallback(&s_movieCallback);
    }

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
    if (!force && sequence == m_pSequence)
    {
        return;
    }

    if (!m_bIsInGameMode) // In Editor Play Game mode switching Editor Viewport cameras is currently not supported.
    {
        // Restore camera in the active Editor Viewport Widget to the value saved while a sequence was activated.
        SwitchEditorViewportCamera(m_defaulViewCameraEntityId);
    }

    // Prevent keys being created from time change
    const bool bRecording = m_recording;
    m_recording = false;
    SetRecordingInternal(false);

    const float newSeqStartTime = sequence ? sequence->GetTimeRange().start : 0.f;

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
        const auto oldSequenceEntityId = m_pSequence->GetSequenceComponentEntityId();
        if (Maestro::SequenceComponentNotificationBus::Handler::BusIsConnectedId(oldSequenceEntityId))
        {
            Maestro::SequenceComponentNotificationBus::Handler::BusDisconnect(oldSequenceEntityId);
        }

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

        // Get ready to handle camera switching in this sequence, if ever, in order to switch camera in Editor Viewport Widget
        Maestro::SequenceComponentNotificationBus::Handler::BusConnect(m_mostRecentSequenceId);

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
        // will not be reselected at unwanted events like an undo operation.
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
bool CAnimationContext::IsInGameMode() const
{
    const auto editor = GetIEditor();
    const bool isInGame = editor && editor->IsInGameMode();
    return m_bIsInGameMode || isInGame;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimationContext::IsInEditingMode() const
{
    const auto editor = GetIEditor();
    const bool isNotEditing = !editor || editor->IsInConsolewMode() || editor->IsInTestMode() || editor->IsInLevelLoadTestMode() || editor->IsInPreviewMode();
    return !m_bIsInGameMode && !isNotEditing;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimationContext::IsSequenceAutostartFlagOn() const
{
    const auto sequence = GetSequence();
    return sequence && ((sequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset) != 0);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::SwitchEditorViewportCamera(const AZ::EntityId& newCameraEntityId)
{
    if (!IsInEditingMode())
    {
        return; // Camera switching is currently supported in editing mode only.
    }

    AZ::EntityId currentEditorViewportCamId;
    Camera::EditorCameraRequestBus::BroadcastResult(currentEditorViewportCamId, &Camera::EditorCameraRequestBus::Events::GetCurrentViewEntityId);
    if (currentEditorViewportCamId == newCameraEntityId)
    {
        return; // Camera in Editor Viewport Widget is already set to the requested value, avoid unneeded actions.
    }

    Camera::EditorCameraRequestBus::Broadcast(&Camera::EditorCameraRequestBus::Events::SetViewFromEntityPerspective, newCameraEntityId);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnCameraChanged([[maybe_unused]] const AZ::EntityId&, const AZ::EntityId& newCameraEntityId)
{

    if (!newCameraEntityId.IsValid())
    {
        return; // Only valid camera Ids are sent to the active editor viewport
    }

    if (!IsInEditingMode())
    {
        return; // Camera switching is currently supported in editing mode only.
    }

    if (!IsSequenceAutostartFlagOn())
    {
        return; // The "Autostart" flag is not set for the active sequence.
    }

    AZ::EntityId currentEditorViewportCamId;
    Camera::EditorCameraRequestBus::BroadcastResult( currentEditorViewportCamId, &Camera::EditorCameraRequestBus::Events::GetCurrentViewEntityId);
    if (currentEditorViewportCamId == newCameraEntityId)
    {
        return; // Camera in Editor Viewport Widget is already set to the requested value, avoid unneeded actions.
    }

    Camera::EditorCameraRequestBus::Broadcast(&Camera::EditorCameraRequestBus::Events::SetViewFromEntityPerspective, newCameraEntityId);
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

                        // Notify time may have changed, use m_currTime in case it was clamped by SetTime()
                        TimeChanged(m_currTime);
                    }
                }
            }
        }
    }

    // Store initial Editor Viewport camera EntityId 
    Camera::EditorCameraRequestBus::BroadcastResult(m_defaulViewCameraEntityId, &Camera::EditorCameraRequestBus::Events::GetCurrentViewEntityId);
}

void CAnimationContext::OnSequenceDeactivated(AZ::EntityId entityId)
{
    auto editor = GetIEditor();
    if (editor != nullptr)
    {
        auto manager = editor->GetSequenceManager();
        if (manager != nullptr)
        {
            auto sequence = manager->GetSequenceByEntityId(entityId);
            if (sequence != nullptr && sequence == m_pSequence)
            {
                SetSequence(nullptr, true, false);
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

    if (m_movieSystem)
    {
        m_movieSystem->Pause();
    }

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

    if (m_movieSystem)
    {
        m_movieSystem->Resume();
    }

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

    if (m_movieSystem)
    {
        if (playing)
        {
            m_movieSystem->Resume();

            if (m_pSequence)
            {
                m_pSequence->Resume();

                IMovieUser* pMovieUser = m_movieSystem->GetUser();

                if (pMovieUser)
                {
                    m_pSequence->BeginCutScene(true);
                }
            }
            m_movieSystem->ResumeCutScenes();
        }
        else
        {
            m_movieSystem->Pause();

            if (m_pSequence)
            {
                m_pSequence->Pause();
            }

            m_movieSystem->PauseCutScenes();
            if (m_pSequence)
            {
                IMovieUser* pMovieUser = m_movieSystem->GetUser();

                if (pMovieUser)
                {
                    m_pSequence->EndCutScene();
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::Update()
{
    if (m_countWaitingForExitingGameMode > 0) // Waiting while Editor is exiting Play Game mode ?
    {
        if (--m_countWaitingForExitingGameMode == 0)  // The 2nd frame after StopPlayInEditor event sent ?
        {
            m_bIsInGameMode = false; // Now Editor Viewport Widget is in the "Editor" state,
            RestoreSequenceOnEnteringEditMode(); // So restore previously active sequence and camera in Editor Viewport.
        }
        else
        {
            return; // while the Editor Viewport state goes from "Started" to "Stopping" and finally back to "Editor".
        }
    }

    if (m_bForceUpdateInNextFrame)
    {
        ForceAnimation();
        m_bForceUpdateInNextFrame = false;
    }

    if (m_paused > 0 || !(m_playing || m_bAutoRecording))
    {
        if (m_pSequence)
        {
            m_pSequence->StillUpdate();
        }

        if (!m_recording)
        {
            if (m_movieSystem)
            {
                m_movieSystem->StillUpdate();
            }
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
            if (m_movieSystem)
            {
                m_movieSystem->PreUpdate(frameDeltaTime);
                m_movieSystem->PostUpdate(frameDeltaTime);
            }
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

    if (fabs(m_lastTimeChangedNotificationTime - m_currTime) > 0.001f)
    {
        NotifyTimeChangedListenersUsingCurrTime();
    }

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

    // console commands are in the invariant locale, for atof()
    AZ::Locale::ScopedSerializationLocale scopedLocale;
    float targetFrame = (float)atof(pArgs->GetArg(1));
    scopedLocale.Deactivate();

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

void CAnimationContext::OnPrefabInstancePropagationEnd()
{
    if (m_pSequence)
    {
        m_pSequence->BindToEditorObjects();
    }
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
void CAnimationContext::StoreSequenceOnExitingEditMode(bool isSwitchingToGameMode)
{
    // Store currently active Editor Viewport camera EntityId
    Camera::EditorCameraRequestBus::BroadcastResult(
        m_viewCameraEntityIdToRestore, &Camera::EditorCameraRequestBus::Events::GetCurrentViewEntityId);

    if (isSwitchingToGameMode)
    {
        SwitchEditorViewportCamera(AZ::EntityId()); // Switch Editor Viewport back to the default Editor camera
        m_bIsInGameMode = true; // and set the flag of Editor being switched into Play Game mode.
    }

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
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::RestoreSequenceOnEnteringEditMode()
{
    m_currTime = m_sequenceRestoreTime;
    SetSequence(GetIEditor()->GetSequenceManager()->GetSequenceByEntityId(m_sequenceToRestore), true, true);
    SetTime(m_sequenceRestoreTime);

    SetRecordingInternal(m_bSavedRecordingState);

    SwitchEditorViewportCamera(m_viewCameraEntityIdToRestore); // Switch Editor Viewport back to the stored camera, which was active prior to switching to Play Game mode.
}

//////////////////////////////////////////////////////////////////////////
void CAnimationContext::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        if (m_pSequence)
        {
            // Restart sequence at the beginning
            auto savedTime = m_currTime;
            AnimateActiveSequence();
            m_currTime = savedTime;

            // Force recent changes made in TrackView, updating in-memory prefab using Undo/Redo framework
            AzToolsFramework::ScopedUndoBatch undoBatch("Update TrackView Sequence");
            undoBatch.MarkEntityDirty(m_pSequence->GetSequenceComponentEntityId());
        }
        {
            // This notification arrives before even the OnStartPlayInEditorBegin and later OnStartPlayInEditor events
            // arrive to Editor Views, and thus switching cameras is still available.
            // So, after storing an active camera Id, rollback the Editor Viewport to default "Editor camera" in order
            // to help Editor correctly restore viewport state after switching back to Editing mode,
            // then set the 'm_bIsInGameMode' flag, store an active sequence and drop it.
            constexpr const bool isSwitchingToGameMode = true;
            StoreSequenceOnExitingEditMode(isSwitchingToGameMode);
        }
        break;

    case eNotify_OnBeginSceneSave:
    case eNotify_OnBeginLayerExport:
        {
            constexpr const bool isSwitchingToGameMode = false;
            // Store active sequence and camera Ids and drop this sequence.
            StoreSequenceOnExitingEditMode(isSwitchingToGameMode);
        }
        break;

    case eNotify_OnEndGameMode:
        // Delay restoring previously active sequence and Editor Viewport camera, and clearing 'm_bIsInGameMode' flag,
        // for 2 frames, while Editor Viewport state goes from "Started" to "Stopping" and finally back to "Editor",
        // and switching cameras is not supported.
        m_countWaitingForExitingGameMode = 2;
        break;

    case eNotify_OnEndSceneSave:
    case eNotify_OnEndLayerExport:
        // Restore previously active sequence and camera in Editor Viewport.
        RestoreSequenceOnEnteringEditMode();
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
    if (m_movieSystem)
    {
        m_movieSystem->SetRecording(enableRecording);
    }

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
