/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Animation/UiAnimViewSequenceManager.h"
#include <Range.h>
#include <LyShine/Animation/IUiAnimation.h>
#include "Undo/IUndoManagerListener.h"

class CUiAnimViewSequence;

/** CUiAnimationContext listener interface
*/
struct IUiAnimationContextListener
{
    virtual void OnSequenceChanged([[maybe_unused]] CUiAnimViewSequence* pNewSequence) {}
    virtual void OnTimeChanged([[maybe_unused]] float newTime) {}
};

/** CUiAnimationContext stores information about current editable animation sequence.
        Stores information about whenever animation is being recorded know,
        current sequence, current time in sequence etc.
*/
class CUiAnimationContext
    : public IEditorNotifyListener
    , public IUndoManagerListener
    , public IUiAnimViewSequenceManagerListener
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors.
    //////////////////////////////////////////////////////////////////////////
    /** Constructor.
    */
    CUiAnimationContext();
    ~CUiAnimationContext();

    //////////////////////////////////////////////////////////////////////////
    // Accessors
    //////////////////////////////////////////////////////////////////////////

    void Init();

    // Listeners
    void AddListener(IUiAnimationContextListener* pListener);
    void RemoveListener(IUiAnimationContextListener* pListener);

    // Get the animation system for the active canvas
    IUiAnimationSystem* GetUiAnimationSystem() const;

    //! Called when the active canvas changes - possibly to no canvas
    void ActiveCanvasChanged();

    /** Return current animation time in active sequence.
        @return Current time.
    */
    float GetTime() const { return m_currTime; };

    float GetTimeScale() const { return m_fTimeScale; }
    void SetTimeScale(float fScale) { m_fTimeScale = fScale; }

    /** Set active editing sequence.
        @param seq New active sequence.
    */
    void SetSequence(CUiAnimViewSequence* pSequence, bool bForce, bool bNoNotify, bool recordUndo = false);

    /** Get currently edited sequence.
    */
    CUiAnimViewSequence* GetSequence() const { return m_pSequence; };

    /** Set time markers to play within.
    */
    void SetMarkers(Range Marker) { m_timeMarker = Marker; }

    /** Get time markers to play within.
    */
    Range GetMarkers() { return m_timeMarker; }

    /** Get time range of active animation sequence.
    */
    Range GetTimeRange() const { return m_timeRange; }

    /** Returns true if editor is recording animations now.
    */
    bool IsRecording() const { return m_recording && m_paused == 0; };

    /** Returns true if editor is playing animation now.
    */
    bool IsPlaying() const { return m_playing && m_paused == 0; };

    /** Returns true if currently playing or recording is paused.
    */
    bool IsPaused() const { return m_paused > 0; }

    /** Return if animation context is now in playing mode.
            In difference from IsPlaying function this function not affected by pause state.
    */
    bool IsPlayMode() const { return m_playing; };

    /** Return if animation context is now in recording mode.
            In difference from IsRecording function this function not affected by pause state.
    */
    bool IsRecordMode() const { return m_recording; };

    /** Returns true if currently looping as activated.
    */
    bool IsLoopMode() const { return m_bLooping; }

    /** Enable/Disable looping.
    */
    void SetLoopMode(bool bLooping) { m_bLooping = bLooping; }

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////

    /** Set current animation time in active sequence.
        @param seq New active time.
    */
    void SetTime(float t);

    /** Set time in active sequence for reset animation.
        @param seq New active time.
    */
    void SetResetTime(float t) {m_resetTime = t; };

    /** Start animation recorduing.
        Automatically stop playing.
        @param recording True to start recording, false to stop.
    */
    void SetRecording(bool playing);

    /** Start/Stop animation playing.
        Automatically stop recording.
        @param playing True to start playing, false to stop.
    */
    void SetPlaying(bool playing);

    /** Pause animation playing/recording.
    */
    void Pause();

    /** Toggle playback
    */
    void TogglePlay();

    /** Resume animation playing/recording.
    */
    void Resume();

    /** Called every frame to update all animations if animation should be playing.
    */
    void Update();

    /** Force animation for current sequence.
    */
    void ForceAnimation();

    void OnPostRender();
    void UpdateTimeRange();

private:
    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;

    void OnSequenceRemoved(CUiAnimViewSequence* pSequence) override;

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void AnimateActiveSequence();

    //! Current time within active animation sequence.
    float m_currTime;

    //! Force update in next frame
    bool m_bForceUpdateInNextFrame;

    //! Time within active animation sequence while reset animation.
    float m_resetTime;

    float m_fTimeScale;

    // Recording time step.
    float m_fRecordingCurrTime;

    //! Time range of active animation sequence.
    Range m_timeRange;

    Range m_timeMarker;

    //! Currently active animation sequence.
    CUiAnimViewSequence* m_pSequence;

    //! Name of active sequence (for switching back from game mode and saving)
    QString m_sequenceName;

    //! Time of active sequence (for switching back from game mode and saving)
    float m_sequenceTime;

    bool m_bLooping;

    //! True if editor is recording animations now.
    bool m_recording;
    bool m_bSavedRecordingState;

    //! True if editor is playing animation now.
    bool m_playing;

    //! Stores how many times animation have been paused prior to calling resume.
    int m_paused;

    bool m_bSingleFrame;

    bool m_bPostRenderRegistered;
    bool m_bForcingAnimation;

    //! Listeners
    std::vector<IUiAnimationContextListener*> m_contextListeners;
};

