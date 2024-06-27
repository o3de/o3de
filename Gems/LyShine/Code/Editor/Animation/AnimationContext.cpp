/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "UiEditorAnimationBus.h"

#include "AnimationContext.h"

#include <LyShine/Animation/IUiAnimation.h>
#include "GameEngine.h"

#include "Animation/UiAnimViewSequence.h"
#include "Animation/UiAnimViewDialog.h"
#include "Animation/UiAnimViewUndo.h"

#include "Viewport.h"
#include "ViewManager.h"

#include "IPostRenderer.h"
#include "UiEditorAnimationBus.h"

#include <AzCore/Time/ITime.h>

namespace Internal
{
    float GetFrameDeltaTime()
    {
        const AZ::TimeUs frameDeltaTimeMs = AZ::GetSimulationTickDeltaTimeUs();
        return AZ::TimeUsToSeconds(frameDeltaTimeMs);
    }

    float GetFrameRate()
    {
        const float deltaTime = GetFrameDeltaTime();
        if (AZ::IsClose(deltaTime, 0.0f))
        {
            return 0.0f;
        }
        return 1.0f / deltaTime;
    }
}

//////////////////////////////////////////////////////////////////////////
// Animation Callback.
//////////////////////////////////////////////////////////////////////////
class CUiAnimationCallback
    : public IUiAnimationCallback
{
protected:
    void OnUiAnimationCallback(ECallbackReason reason, [[maybe_unused]] IUiAnimNode* pNode) override
    {
        switch (reason)
        {
        case CBR_CHANGENODE:
            // Invalidate nodes
            break;
        case CBR_CHANGETRACK:
        {
            // Invalidate tracks
            CUiAnimViewDialog* pUiAnimViewDialog = CUiAnimViewDialog::GetCurrentInstance();
            if (pUiAnimViewDialog)
            {
                pUiAnimViewDialog->InvalidateDopeSheet();
            }
        }
        break;
        }
    }
};

static CUiAnimationCallback s_uiAnimationCallback;

//////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//!
class CAnimationContextPostRender
    : public IPostRenderer
{
public:
    CAnimationContextPostRender(CUiAnimationContext* pAC)
        : m_pAC(pAC){}

    void OnPostRender() const { assert(m_pAC); m_pAC->OnPostRender(); }

protected:
    CUiAnimationContext* m_pAC;
};

//////////////////////////////////////////////////////////////////////////
CUiAnimationContext::CUiAnimationContext()
{
    m_paused = 0;
    m_playing = false;
    m_recording = false;
    m_bSavedRecordingState = false;
    m_timeRange.Set(0, 0);
    m_timeMarker.Set(0, 0);
    m_currTime = 0.0f;
    m_bForceUpdateInNextFrame = false;
    m_fTimeScale = 1.0f;
    m_pSequence = nullptr;
    m_bLooping = false;
    m_bSingleFrame = false;
    m_bPostRenderRegistered = false;
    m_bForcingAnimation = false;
    UiAnimUndoManager::Get()->AddListener(this);
    CUiAnimViewSequenceManager::GetSequenceManager()->AddListener(this);
    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimationContext::~CUiAnimationContext()
{
    CUiAnimViewSequenceManager::GetSequenceManager()->RemoveListener(this);
    UiAnimUndoManager::Get()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::Init()
{
    IUiAnimationSystem* pUiAnimationSystem = GetUiAnimationSystem();
    pUiAnimationSystem->SetCallback(&s_uiAnimationCallback);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::AddListener(IUiAnimationContextListener* pListener)
{
    stl::push_back_unique(m_contextListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::RemoveListener(IUiAnimationContextListener* pListener)
{
    stl::find_and_erase(m_contextListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimationSystem* CUiAnimationContext::GetUiAnimationSystem() const
{
    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);
    return animationSystem;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::ActiveCanvasChanged()
{
    m_sequenceName = "";
    m_sequenceTime = GetTime();
    m_paused = 0;
    m_recording = m_bSavedRecordingState = false;
    m_playing = false;
    SetSequence(nullptr, true, true);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::SetSequence(CUiAnimViewSequence* pSequence, bool bForce, bool bNoNotify, bool recordUndo)
{
    CUiAnimViewSequence* pCurrentSequence = m_pSequence;

    if (!bForce && pSequence == pCurrentSequence)
    {
        return;
    }

    if (!GetUiAnimationSystem())
    {
        // There is no canvas loaded in editor
        m_pSequence = pSequence;

        if (!bNoNotify)
        {
            for (size_t i = 0; i < m_contextListeners.size(); ++i)
            {
                m_contextListeners[i]->OnTimeChanged(0.0f);
                m_contextListeners[i]->OnSequenceChanged(m_pSequence);
            }
        }

        return;
    }

    // Prevent keys being created from time change
    const bool bRecording = m_recording;
    m_recording = false;
    GetUiAnimationSystem()->SetRecording(false);

    m_currTime = m_fRecordingCurrTime = 0.0f;

    if (m_pSequence)
    {
        m_pSequence->Deactivate();
        if (m_playing)
        {
            m_pSequence->EndCutScene();
        }

        m_pSequence->UnBindFromEditorObjects();
    }
    m_pSequence = pSequence;

    if (m_pSequence)
    {
        if (m_playing)
        {
            m_pSequence->BeginCutScene(true);
        }

        m_timeRange = m_pSequence->GetTimeRange();
        m_timeMarker = m_timeRange;
        m_pSequence->Activate();
        m_pSequence->PrecacheData(0.0f);

        m_pSequence->BindToEditorObjects();
    }

    ForceAnimation();

    if (!bNoNotify)
    {
        for (size_t i = 0; i < m_contextListeners.size(); ++i)
        {
            m_contextListeners[i]->OnTimeChanged(0.0f);
            m_contextListeners[i]->OnSequenceChanged(m_pSequence);
        }
    }

    if (recordUndo)
    {
        // Safely track sequence changes for clean undos
        UiAnimUndo undo("Change Sequence");
        UiAnimUndo::Record(new CUndoSequenceChange(pCurrentSequence, pSequence));
    }

    m_recording = bRecording;
    GetUiAnimationSystem()->SetRecording(bRecording);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::UpdateTimeRange()
{
    if (m_pSequence)
    {
        m_timeRange = m_pSequence->GetTimeRange();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::SetTime(float t)
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

    for (size_t i = 0; i < m_contextListeners.size(); ++i)
    {
        m_contextListeners[i]->OnTimeChanged(m_currTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::Pause()
{
    assert(m_paused >= 0);
    m_paused++;

    if (m_recording)
    {
        GetUiAnimationSystem()->SetRecording(false);
    }

    GetUiAnimationSystem()->Pause();
    if (m_pSequence)
    {
        m_pSequence->Pause();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::Resume()
{
    assert(m_paused > 0);
    m_paused--;
    if (m_recording && m_paused == 0)
    {
        GetUiAnimationSystem()->SetRecording(true);
    }
    GetUiAnimationSystem()->Resume();

    if (m_pSequence)
    {
        m_pSequence->Resume();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::SetRecording(bool recording)
{
    if (recording == m_recording)
    {
        return;
    }
    m_paused = 0;
    m_recording = recording;
    m_playing = false;

    GetUiAnimationSystem()->SetRecording(recording);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::SetPlaying(bool playing)
{
    if (playing == m_playing)
    {
        return;
    }

    m_paused = 0;
    m_playing = playing;
    m_recording = false;
    GetUiAnimationSystem()->SetRecording(false);

    if (playing)
    {
        IUiAnimationSystem* pUiAnimationSystem = GetUiAnimationSystem();

        pUiAnimationSystem->Resume();
        if (m_pSequence)
        {
            m_pSequence->Resume();
        }
    }
    else
    {
        IUiAnimationSystem* pUiAnimationSystem = GetUiAnimationSystem();

        pUiAnimationSystem->Pause();

        if (m_pSequence)
        {
            m_pSequence->Pause();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::Update()
{
    if (!GetUiAnimationSystem())
    {
        return;
    }

    const float lastTime = m_currTime;

    if (m_bForceUpdateInNextFrame)
    {
        ForceAnimation();
        m_bForceUpdateInNextFrame = false;
    }

    if (m_paused > 0 || !m_playing)
    {
        if (m_pSequence)
        {
            m_pSequence->StillUpdate();
        }

        if (!m_recording)
        {
            GetUiAnimationSystem()->StillUpdate();
        }

        return;
    }

    AnimateActiveSequence();

    const float frameDeltaTime = Internal::GetFrameDeltaTime();
    m_currTime += frameDeltaTime * m_fTimeScale;

    if (!m_recording)
    {
        GetUiAnimationSystem()->PreUpdate(frameDeltaTime);
        GetUiAnimationSystem()->PostUpdate(frameDeltaTime);
    }

    if (m_currTime > m_timeMarker.end)
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

    if (fabs(lastTime - m_currTime) > 0.001f)
    {
        for (size_t i = 0; i < m_contextListeners.size(); ++i)
        {
            m_contextListeners[i]->OnTimeChanged(m_currTime);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::ForceAnimation()
{
    if (m_bForcingAnimation)
    {
        // reentrant calls are possible when using subsequences
        return;
    }

    m_bForcingAnimation = true;

    AnimateActiveSequence();
    // Animate a second time to properly update camera DoF
    AnimateActiveSequence();

    m_bForcingAnimation = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::OnPostRender()
{
    if (m_pSequence)
    {
        SUiAnimContext ac;
        ac.dt = 0;
        ac.fps = Internal::GetFrameRate();
        ac.time = m_currTime;
        ac.bSingleFrame = true;
        ac.bForcePlay = true;
        m_pSequence->Render(ac);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::BeginUndoTransaction()
{
    m_bSavedRecordingState = m_recording;

    IUiAnimationSystem* uiAnimationSystem = GetUiAnimationSystem();
    if (uiAnimationSystem)
    {
        uiAnimationSystem->SetRecording(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::EndUndoTransaction()
{
    if (m_pSequence)
    {
        m_pSequence->BindToEditorObjects();
    }

    IUiAnimationSystem* uiAnimationSystem = GetUiAnimationSystem();
    if (uiAnimationSystem)
    {
        uiAnimationSystem->SetRecording(m_bSavedRecordingState);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::TogglePlay()
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
void CUiAnimationContext::OnSequenceRemoved(CUiAnimViewSequence* pSequence)
{
    if (m_pSequence == pSequence)
    {
        SetSequence(nullptr, true, false);
    }
}
//////////////////////////////////////////////////////////////////////////
void CUiAnimationContext::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    // If the UI Animation window is open but there is no canvas loaded in editor
    // then just return.
    if (!GetUiAnimationSystem())
    {
        return;
    }

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
            m_sequenceName = QString::fromUtf8(m_pSequence->GetName().c_str());
        }
        else
        {
            m_sequenceName = "";
        }

        m_sequenceTime = GetTime();

        m_bSavedRecordingState = m_recording;
        GetUiAnimationSystem()->SetRecording(false);
        SetSequence(nullptr, true, true);
        break;

    case eNotify_OnEndGameMode:
    case eNotify_OnEndSceneSave:
    case eNotify_OnEndLayerExport:
        m_currTime = m_sequenceTime;
        SetSequence(CUiAnimViewSequenceManager::GetSequenceManager()->GetSequenceByName(m_sequenceName), true, true);
        SetTime(m_sequenceTime);

        GetUiAnimationSystem()->SetRecording(m_bSavedRecordingState);
        break;

    case eNotify_OnCloseScene:
        SetSequence(nullptr, true, false);
        break;

    case eNotify_OnBeginNewScene:
        SetSequence(nullptr, false, false);
        break;

    case eNotify_OnBeginLoad:
    {
        m_bSavedRecordingState = m_recording;
        GetUiAnimationSystem()->SetRecording(false);
        CUiAnimationContext* ac = nullptr;
        UiEditorAnimationBus::BroadcastResult(ac, &UiEditorAnimationBus::Events::GetAnimationContext);
        ac->SetSequence(nullptr, false, false);
        break;
    }

    case eNotify_OnEndLoad:
        GetUiAnimationSystem()->SetRecording(m_bSavedRecordingState);
        break;

    case eNotify_CameraChanged:
        ForceAnimation();
        break;

    case eNotify_OnIdleUpdate:
        Update();
        break;
    }
}

void CUiAnimationContext::AnimateActiveSequence()
{
    if (!m_pSequence)
    {
        return;
    }

    SUiAnimContext ac;
    ac.dt = 0;
    ac.fps = Internal::GetFrameRate();
    ac.time = m_currTime;
    ac.bSingleFrame = true;
    ac.bForcePlay = true;

    m_pSequence->Animate(ac);
}
