/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// TODO - Determine if this code is deprecated. A CVar closely tied to its use was removed

#ifndef CRYINCLUDE_CRYMOVIE_MOVIE_H
#define CRYINCLUDE_CRYMOVIE_MOVIE_H

#pragma once
#include <AzCore/std/containers/map.h>

#include "IMovieSystem.h"

struct PlayingSequence
{
    //! Sequence playing
    AZStd::intrusive_ptr<IAnimSequence> sequence;

    //! Start/End/Current playing time for this sequence.
    float startTime;
    float endTime;
    float currentTime;
    float currentSpeed;

    //! Sequence from other sequence's sequence track
    bool trackedSequence;
    bool bSingleFrame;
};

class CLightAnimWrapper
    : public ILightAnimWrapper
{
public:
    // ILightAnimWrapper interface
    virtual bool Resolve();

public:
    static CLightAnimWrapper* Create(const char* name);
    static void ReconstructCache();
    static IAnimSequence* GetLightAnimSet();
    static void SetLightAnimSet(IAnimSequence* pLightAnimSet);
    static void InvalidateAllNodes();

private:
    typedef std::map<string, CLightAnimWrapper*> LightAnimWrapperCache;
    static StaticInstance<LightAnimWrapperCache> ms_lightAnimWrapperCache;
    static AZStd::intrusive_ptr<IAnimSequence> ms_pLightAnimSet;

private:
    static CLightAnimWrapper* FindLightAnim(const char* name);
    static void CacheLightAnim(const char* name, CLightAnimWrapper* p);
    static void RemoveCachedLightAnim(const char* name);

private:
    CLightAnimWrapper(const char* name);
    virtual ~CLightAnimWrapper();
};

struct IConsoleCmdArgs;

class CMovieSystem
    : public IMovieSystem
{
    typedef std::vector<PlayingSequence> PlayingSequences;

public:
    AZ_CLASS_ALLOCATOR(CMovieSystem, AZ::SystemAllocator, 0);
    AZ_RTTI(CMovieSystem, "{760D45C1-08F2-4C70-A506-BD2E69085A48}", IMovieSystem);

    CMovieSystem(ISystem* system);
    CMovieSystem();

    void Release() { delete this; };

    void SetUser(IMovieUser* pUser) { m_pUser = pUser; }
    IMovieUser* GetUser() { return m_pUser; }

    ISystem* GetSystem() { return m_pSystem; }

    IAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0, SequenceType = kSequenceTypeDefault, AZ::EntityId entityId = AZ::EntityId());

    void AddSequence(IAnimSequence* pSequence);
    void RemoveSequence(IAnimSequence* pSequence);
    IAnimSequence* FindLegacySequenceByName(const char* sequence) const override;
    IAnimSequence* FindSequence(const AZ::EntityId& componentEntitySequenceId) const override;
    IAnimSequence* FindSequenceById(uint32 id) const override;
    IAnimSequence* GetSequence(int i) const;
    int GetNumSequences() const;
    IAnimSequence* GetPlayingSequence(int i) const;
    int GetNumPlayingSequences() const;
    bool IsCutScenePlaying() const;

    uint32 GrabNextSequenceId() override
    { return m_nextSequenceId++; }
    void OnSetSequenceId(uint32 sequenceId) override;

    int OnSequenceRenamed(const char* before, const char* after);
    int OnCameraRenamed(const char* before, const char* after);

    bool AddMovieListener(IAnimSequence* pSequence, IMovieListener* pListener);
    bool RemoveMovieListener(IAnimSequence* pSequence, IMovieListener* pListener);

    void RemoveAllSequences();

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    void PlaySequence(const char* sequence, IAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX);
    void PlaySequence(IAnimSequence* seq, IAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX);
    void PlayOnLoadSequences();

    bool StopSequence(const char* sequence);
    bool StopSequence(IAnimSequence* seq);
    bool AbortSequence(IAnimSequence* seq, bool bLeaveTime = false);

    void StopAllSequences();
    void StopAllCutScenes();
    void Pause(bool bPause);

    void Reset(bool bPlayOnReset, bool bSeekToStart);
    void StillUpdate();
    void PreUpdate(const float dt);
    void PostUpdate(const float dt);
    void Render();

    void EnableFixedStepForCapture(float step);
    void DisableFixedStepForCapture();
    void StartCapture(const ICaptureKey& key, int frame);
    void EndCapture();
    void ControlCapture();
    bool IsCapturing() const;

    bool IsPlaying(IAnimSequence* seq) const;

    void Pause();
    void Resume();

    virtual void PauseCutScenes();
    virtual void ResumeCutScenes();

    void SetRecording(bool recording) { m_bRecording = recording; };
    bool IsRecording() const { return m_bRecording; };

    void EnableCameraShake(bool bEnabled){ m_bEnableCameraShake = bEnabled; };
    bool IsCameraShakeEnabled() const {return m_bEnableCameraShake; };

    void SetCallback(IMovieCallback* pCallback) { m_pCallback = pCallback; }
    IMovieCallback* GetCallback() { return m_pCallback; }
    void Callback(IMovieCallback::ECallbackReason Reason, IAnimNode* pNode);

    const SCameraParams& GetCameraParams() const { return m_ActiveCameraParams; }
    void SetCameraParams(const SCameraParams& Params);

    void SendGlobalEvent(const char* pszEvent);
    void SetSequenceStopBehavior(ESequenceStopBehavior behavior);
    IMovieSystem::ESequenceStopBehavior GetSequenceStopBehavior();

    float GetPlayingTime(IAnimSequence* pSeq);
    bool SetPlayingTime(IAnimSequence* pSeq, float fTime);

    float GetPlayingSpeed(IAnimSequence* pSeq);
    bool SetPlayingSpeed(IAnimSequence* pSeq, float fTime);

    bool GetStartEndTime(IAnimSequence* pSeq, float& fStartTime, float& fEndTime);
    bool SetStartEndTime(IAnimSequence* pSeq, const float fStartTime, const float fEndTime);

    void GoToFrame(const char* seqName, float targetFrame);

    const char* GetOverrideCamName() const
    { return m_mov_overrideCam->GetString(); }

    virtual bool IsPhysicsEventsEnabled() const { return m_bPhysicsEventsEnabled; }
    virtual void EnablePhysicsEvents(bool enable) { m_bPhysicsEventsEnabled = enable; }

    virtual void EnableBatchRenderMode(bool bOn) { m_bBatchRenderMode = bOn; }
    virtual bool IsInBatchRenderMode() const { return m_bBatchRenderMode; }

    ILightAnimWrapper* CreateLightAnimWrapper(const char* name) const;

    void SerializeNodeType(AnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags) override;
    virtual void LoadParamTypeFromXml(CAnimParamType& animParamType, const XmlNodeRef& xmlNode, const uint version) override;
    virtual void SaveParamTypeToXml(const CAnimParamType& animParamType, XmlNodeRef& xmlNode) override;
    virtual void SerializeParamType(CAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version);

    static const char* GetParamTypeName(const CAnimParamType& animParamType);

    void OnCameraCut();

    void LogUserNotificationMsg(const AZStd::string& msg) override;
    void ClearUserNotificationMsgs() override;
    const AZStd::string& GetUserNotificationMsgs() const override;

    void OnSequenceActivated(IAnimSequence* sequence) override;

    static void Reflect(AZ::ReflectContext* context);

private:

    void CheckForEndCapture();

    void NotifyListeners(IAnimSequence* pSequence, IMovieListener::EMovieEvent event);

    void InternalStopAllSequences(bool bIsAbort, bool bAnimate);
    bool InternalStopSequence(IAnimSequence* seq, bool bIsAbort, bool bAnimate);

    bool FindSequence(IAnimSequence* sequence, PlayingSequences::const_iterator& sequenceIteratorOut) const;
    bool FindSequence(IAnimSequence* sequence, PlayingSequences::iterator& sequenceIteratorOut);

#if !defined(_RELEASE)
    static void GoToFrameCmd(IConsoleCmdArgs* pArgs);
    static void ListSequencesCmd(IConsoleCmdArgs* pArgs);
    static void PlaySequencesCmd(IConsoleCmdArgs* pArgs);
    AZStd::string m_notificationLogMsgs;     // buffer to hold movie user warnings, errors and notifications for the editor.
#endif

    void DoNodeStaticInitialisation();
    void UpdateInternal(const float dt, const bool bPreUpdate);

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    virtual AnimNodeType GetNodeTypeFromString(const char* pString) const;
    virtual CAnimParamType GetParamTypeFromString(const char* pString) const;
#endif

    ISystem* m_pSystem;

    IMovieUser* m_pUser;
    IMovieCallback* m_pCallback;

    CTimeValue m_lastUpdateTime;

    typedef AZStd::vector<AZStd::intrusive_ptr<IAnimSequence> > Sequences;
    Sequences m_sequences;

    PlayingSequences m_playingSequences;

    // A list of sequences that just got Activated. Queue them up here
    // and process them in Update to see if the sequence should be auto-played.
    // We don't want to auto-play OnActivate because of the timing of
    // how entity ids get remapped in the editor and game.
    AZStd::vector<IAnimSequence*> m_newlyActivatedSequences;

    typedef AZStd::vector<IMovieListener*> TMovieListenerVec;
    typedef AZStd::map<IAnimSequence*, TMovieListenerVec> TMovieListenerMap;

    // a container which maps sequences to all interested listeners
    // listeners is a vector (could be a set in case we have a lot of listeners, stl::push_back_unique!)
    TMovieListenerMap m_movieListenerMap;

    bool    m_bRecording;
    bool    m_bPaused;
    bool    m_bCutscenesPausedInEditor;
    bool    m_bEnableCameraShake;

    SCameraParams m_ActiveCameraParams;

    ESequenceStopBehavior m_sequenceStopBehavior;

    bool m_bStartCapture;
    int m_captureFrame;
    bool m_bEndCapture;
    ICaptureKey m_captureKey;
    float m_fixedTimeStepBackUp;
    float m_maxStepBackUp;
    float m_smoothingBackUp;
    float m_maxTimeStepForMovieSystemBackUp;
    ICVar* m_cvar_capture_frame_once;
    ICVar* m_cvar_capture_folder;
    ICVar* m_cvar_t_FixedStep;
    ICVar* m_cvar_t_MaxStep;
    ICVar* m_cvar_t_Smoothing;
    ICVar* m_cvar_sys_maxTimeStepForMovieSystem;
    ICVar* m_cvar_capture_frames;
    ICVar* m_cvar_capture_file_prefix;
    ICVar* m_cvar_capture_buffer;

    static int m_mov_NoCutscenes;
    ICVar* m_mov_overrideCam;

    bool m_bPhysicsEventsEnabled;

    bool m_bBatchRenderMode;

    // next available sequenceId
    uint32 m_nextSequenceId;

    void ShowPlayedSequencesDebug();

public:
    static float m_mov_cameraPrecacheTime;
#if !defined(_RELEASE)
    static int m_mov_DebugEvents;
    static int m_mov_debugCamShake;
#endif
};

#endif // CRYINCLUDE_CRYMOVIE_MOVIE_H
