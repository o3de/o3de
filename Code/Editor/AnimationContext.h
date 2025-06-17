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
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>

#include <CryCommon/Maestro/Bus/SequenceComponentBus.h>

struct IMovieSystem;
class CTrackViewSequence;

/** CAnimationContext listener interface
*/
struct IAnimationContextListener
{
    virtual void OnSequenceChanged([[maybe_unused]] CTrackViewSequence* pNewSequence) {}
    virtual void OnTimeChanged([[maybe_unused]] float newTime) {}
};

/** CAnimationContext stores information about current editable animation sequence.
        Stores information about whenever animation is being recorded know,
        current sequence, current time in sequence etc.
*/
class CAnimationContext
    : public IEditorNotifyListener
    , public IUndoManagerListener
    , public ITrackViewSequenceManagerListener
    , public AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
    , public Maestro::SequenceComponentNotificationBus::Handler

{
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

    /** Start animation recording.
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
    void OnSequenceDeactivated(AZ::EntityId entityId);

    /**
     * Returns true if Editor is in the Play Game mode.
     */
    bool IsInGameMode() const;

    /**
     * Returns true if Editor is in the Editing mode.
     */
    bool IsInEditingMode() const;

    /**
     * Returns true if a sequence is active and has "Autostart" (IAnimSequence::eSeqFlags_PlayOnReset) flag set.
     */
    bool IsSequenceAutostartFlagOn() const;

    /**
     * Switch camera, only in Editing mode, in the active Editor Viewport Widget to the newCameraEntityId 
     * (including invalid Id, which corresponds to the default Editor camera).
     */
    void SwitchEditorViewportCamera(const AZ::EntityId& newCameraEntityId);

private:
    static void GoToFrameCmd(IConsoleCmdArgs* pArgs);

    void NotifyTimeChangedListenersUsingCurrTime() const;

    //! IUndoManagerListener overrides
    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;

    //! PrefabPublicNotificationBus override
    void OnPrefabInstancePropagationEnd() override;

    //! ITrackViewSequenceManagerListener override
    void OnSequenceRemoved(CTrackViewSequence* pSequence) override;

    //! IEditorNotifyListener override
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    /** SequenceComponentNotificationBus override:
     *  Switches camera Id in the active editor viewport when a Sequence changes camera during playback in Track View,
     *  only if in Editing mode and if the "Autostart" flag is set in the active sequence. 
     */
    void OnCameraChanged([[maybe_unused]] const AZ::EntityId& oldCameraEntityId, const AZ::EntityId& newCameraEntityId) override;

    void AnimateActiveSequence();

    void SetRecordingInternal(bool enableRecording);

    /**
     * Store an active sequence when switching from Editing mode to Game mode or Saving mode.
     * @param isSwitchingToGameMode True if the function is called when switching from Editing mode to Game mode.
     */
    void StoreSequenceOnExitingEditMode(bool isSwitchingToGameMode);

    //! Restore a previously active sequence when switching back from Game mode or Saving mode to Editing mode.
    void RestoreSequenceOnEnteringEditMode();

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

    /** An entity ID of a current camera in an active editor viewport, at the moment a sequence is activated,
     *  or invalid if the default editor camera is used at the moment.
     *  Used to restore the default editor viewport camera when clearing "Autostart" property or deselecting a sequence.
     */
    AZ::EntityId m_defaulViewCameraEntityId = AZ::EntityId();

    //! Id of the active viewport camera to restore when switching back from game mode and saving mode to Editing mode.
    AZ::EntityId m_viewCameraEntityIdToRestore = AZ::EntityId();

    /** Switched On with EEditorNotifyEvent::eNotify_OnBeginGameMode,
     *  switched Off - after 2 frames in Update() since receiving EEditorNotifyEvent::eNotify_OnEndGameModein.
     */
    bool m_bIsInGameMode = false;

    /** Set to a positive value with EEditorNotifyEvent::eNotify_OnEndGameMode, in order to delay restoring a previously active
     *  sequence and Editor Viewport camera, and then resetting m_bIsInGameMode. Decreased to 0 in Update().
     *  Currently skipping 2 frames is needed, as Editor Viewport state goes from "Started" to "Stopping" and finally back to "Editor".
     */
    int m_countWaitingForExitingGameMode = 0;

    //! Currently active animation sequence.
    CTrackViewSequence* m_pSequence;

    /** Id of latest valid sequence that was selected. Useful for restoring the selected
     * sequence after undo has destroyed and recreated it.
     */
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

    IMovieSystem* m_movieSystem;

    //! Listeners
    std::vector<IAnimationContextListener*> m_contextListeners;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_ANIMATIONCONTEXT_H
