/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_ANIMATIONCONTEXT_H
#define CRYINCLUDE_EDITOR_ANIMATIONCONTEXT_H

#pragma once

#include "Undo/Undo.h"
#include "TrackView/TrackViewSequenceManager.h"
#include <Range.h>
#include <IMovieSystem.h>

class CTrackViewSequence;

/** CAnimationContext listener interface
*/
struct IAnimationContextListener
{
    virtual void OnSequenceChanged([[maybe_unused]] CTrackViewSequence* pNewSequence) {}
    virtual void OnTimeChanged([[maybe_unused]] float newTime) {}
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** CAnimationContext stores information about current editable animation sequence.
        Stores information about whenever animation is being recorded know,
        current sequence, current time in sequence etc.
*/
class SANDBOX_API CAnimationContext
    : public IEditorNotifyListener
    , public IUndoManagerListener
    , public ITrackViewSequenceManagerListener
{
    AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors.
    //////////////////////////////////////////////////////////////////////////
    /** Constructor.
    */
    CAnimationContext();
    ~CAnimationContext();

    //////////////////////////////////////////////////////////////////////////
    // Accessors
    //////////////////////////////////////////////////////////////////////////

    void Init();

    // Listeners
    void AddListener(IAnimationContextListener* pListener);
    void RemoveListener(IAnimationContextListener* pListener);

    /** Return current animation time in active sequence.
        @return Current time.
    */
    float GetTime() const { return m_currTime; };

    float GetTimeScale() const { return m_fTimeScale; }
    void SetTimeScale(float fScale) { m_fTimeScale = fScale; }

    /** Set active editing sequence.
        @param sequence New active sequence.
        @param force Set to true to always run all of the new active sequence code including listeners even if the sequences is already selected.
        @param noNotify Set to true to skip over notifying listeners when a new sequences is selected.
        @param user Set to true if the new sequence is being selected by the user, false if set by internal system code.
        */
    void SetSequence(CTrackViewSequence* sequence, bool force, bool noNotify, bool user = false);

    /** Get currently edited sequence.
    */
    CTrackViewSequence* GetSequence() const { return m_pSequence; };

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

    /** Enables/Disables automatic recording, sets the time step for each recorded frame.
    */
    void SetAutoRecording(bool bEnable, float fTimeStep);

    //! Check if auto recording enabled.
    bool IsAutoRecording() const { return m_bAutoRecording; };

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

    /** Notify after a time change is complete and time control is released to 'playback' controls, for example after
     *  a timeline drag
    */
    void TimeChanged(float newTime);

    /** Notify after a sequence has been activated, useful for Undo/Redo
    */
    void OnSequenceActivated(AZ::EntityId entityId);

private:
    static void GoToFrameCmd(IConsoleCmdArgs* pArgs);

    // Updates the animation time of lights animated by the light animation set.
    void UpdateAnimatedLights();

    void NotifyTimeChangedListenersUsingCurrTime() const;

    virtual void BeginUndoTransaction() override;
    virtual void EndUndoTransaction() override;

    virtual void OnSequenceRemoved(CTrackViewSequence* pSequence) override;

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void AnimateActiveSequence();

    void SetRecordingInternal(bool enableRecording);

    //! Current time within active animation sequence.
    float m_currTime;

    //! Used to stash the time we send out OnTimeChanged notifications
    mutable float m_lastTimeChangedNotificationTime;

    //! Force update in next frame
    bool m_bForceUpdateInNextFrame;

    //! Time within active animation sequence while reset animation.
    float m_resetTime;

    float m_fTimeScale;

    // Recording time step.
    float m_fRecordingTimeStep;
    float m_fRecordingCurrTime;

    bool m_bAutoRecording;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! Time range of active animation sequence.
    Range m_timeRange;

    Range m_timeMarker;

    //! Currently active animation sequence.
    CTrackViewSequence* m_pSequence;

    //! Id of latest valid sequence that was selected. Useful for restoring the selected
    //! sequence after undo has destroyed and recreated it.
    AZ::EntityId m_mostRecentSequenceId;

    //! The current time of the most recent selected sequence. It's very useful to restore this after an undo.
    float m_mostRecentSequenceTime;

    //! Id of active sequence to restore (for switching back from game mode and saving)
    AZ::EntityId m_sequenceToRestore;

    //! Time of active sequence (for switching back from game mode and saving)
    float m_sequenceRestoreTime;

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
    std::vector<IAnimationContextListener*> m_contextListeners;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_ANIMATIONCONTEXT_H
