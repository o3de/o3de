/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// TODO - Determine if this code is deprecated. A CVar closely tied to its use was removed

#pragma once

#include <IMovieSystem.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/Time/ITime.h>
#include <CryCommon/StlUtils.h>
#include <CryCommon/TimeValue.h>

struct IConsoleCmdArgs;


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

class CMovieSystem
    : public IMovieSystem
{
    typedef AZStd::vector<PlayingSequence> PlayingSequences;

public:
    AZ_CLASS_ALLOCATOR(CMovieSystem, AZ::SystemAllocator);
    AZ_RTTI(CMovieSystem, "{760D45C1-08F2-4C70-A506-BD2E69085A48}", IMovieSystem);

    CMovieSystem(ISystem* system);
    CMovieSystem();
    ~CMovieSystem();

    void Release() override { delete this; };

    void SetUser(IMovieUser* pUser) override { m_pUser = pUser; }
    IMovieUser* GetUser() override { return m_pUser; }

    ISystem* GetSystem() override { return m_pSystem; }

    IAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0, SequenceType = kSequenceTypeDefault, AZ::EntityId entityId = AZ::EntityId()) override;

    void AddSequence(IAnimSequence* pSequence) override;
    void RemoveSequence(IAnimSequence* pSequence) override;
    IAnimSequence* FindLegacySequenceByName(const char* sequence) const override;
    IAnimSequence* FindSequence(const AZ::EntityId& componentEntitySequenceId) const override;
    IAnimSequence* FindSequenceById(uint32 id) const override;
    IAnimSequence* GetSequence(int i) const override;
    int GetNumSequences() const override;
    IAnimSequence* GetPlayingSequence(int i) const override;
    int GetNumPlayingSequences() const override;
    bool IsCutScenePlaying() const override;

    uint32 GrabNextSequenceId() override
    { return m_nextSequenceId++; }
    void OnSetSequenceId(uint32 sequenceId) override;

    int OnSequenceRenamed(const char* before, const char* after) override;
    int OnCameraRenamed(const char* before, const char* after) override;

    bool AddMovieListener(IAnimSequence* pSequence, IMovieListener* pListener) override;
    bool RemoveMovieListener(IAnimSequence* pSequence, IMovieListener* pListener) override;

    void RemoveAllSequences() override;

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    void PlaySequence(const char* sequence, IAnimSequence* parentSeq = nullptr, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX) override;
    void PlaySequence(IAnimSequence* seq, IAnimSequence* parentSeq = nullptr, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX) override;
    void PlayOnLoadSequences() override;

    bool StopSequence(const char* sequence) override;
    bool StopSequence(IAnimSequence* seq) override;
    bool AbortSequence(IAnimSequence* seq, bool bLeaveTime = false) override;

    void StopAllSequences() override;
    void StopAllCutScenes() override;

    void Reset(bool bPlayOnReset, bool bSeekToStart) override;
    void StillUpdate() override;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Render() override;

    void EnableFixedStepForCapture(float step) override;
    void DisableFixedStepForCapture() override;
    void StartCapture(const ICaptureKey& key, int frame) override;
    void EndCapture() override;
    void ControlCapture() override;
    bool IsCapturing() const override;

    bool IsPlaying(IAnimSequence* seq) const override;

    void Pause() override;
    void Resume() override;

    void PauseCutScenes() override;
    void ResumeCutScenes() override;

    void SetRecording(bool recording) override { m_bRecording = recording; }
    bool IsRecording() const override { return m_bRecording; }

    void SetCallback(IMovieCallback* pCallback) override { m_pCallback = pCallback; }
    IMovieCallback* GetCallback() override { return m_pCallback; }
    void Callback(IMovieCallback::ECallbackReason Reason, IAnimNode* pNode);

    const SCameraParams& GetCameraParams() const override { return m_ActiveCameraParams; }
    void SetCameraParams(const SCameraParams& Params) override;

    void SendGlobalEvent(const char* pszEvent) override;
    void SetSequenceStopBehavior(ESequenceStopBehavior behavior) override;
    IMovieSystem::ESequenceStopBehavior GetSequenceStopBehavior() override;

    float GetPlayingTime(IAnimSequence* pSeq) override;
    bool SetPlayingTime(IAnimSequence* pSeq, float fTime) override;

    float GetPlayingSpeed(IAnimSequence* pSeq) override;
    bool SetPlayingSpeed(IAnimSequence* pSeq, float fTime) override;

    bool GetStartEndTime(IAnimSequence* pSeq, float& fStartTime, float& fEndTime) override;
    bool SetStartEndTime(IAnimSequence* pSeq, const float fStartTime, const float fEndTime) override;

    void GoToFrame(const char* seqName, float targetFrame) override;

    const char* GetOverrideCamName() const override
    { return m_mov_overrideCam->GetString(); }

    bool IsPhysicsEventsEnabled() const override { return m_bPhysicsEventsEnabled; }
    void EnablePhysicsEvents(bool enable) override { m_bPhysicsEventsEnabled = enable; }

    void EnableBatchRenderMode(bool bOn) override { m_bBatchRenderMode = bOn; }
    bool IsInBatchRenderMode() const override { return m_bBatchRenderMode; }

    void SerializeNodeType(AnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags) override;
    void LoadParamTypeFromXml(CAnimParamType& animParamType, const XmlNodeRef& xmlNode, const uint version) override;
    void SaveParamTypeToXml(const CAnimParamType& animParamType, XmlNodeRef& xmlNode) override;
    void SerializeParamType(CAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version) override;

    const char* GetParamTypeName(const CAnimParamType& animParamType);

    void OnCameraCut();

    void LogUserNotificationMsg(const AZStd::string& msg) override;
    void ClearUserNotificationMsgs() override;
    const AZStd::string& GetUserNotificationMsgs() const override;

    void OnSequenceActivated(IAnimSequence* sequence) override;

    static void Reflect(AZ::ReflectContext* context);

    static float GetCameraPrecacheTime()
    {
        return m_mov_cameraPrecacheTime;
    }

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
    AnimNodeType GetNodeTypeFromString(const char* pString) const override;
    CAnimParamType GetParamTypeFromString(const char* pString) const override;
#endif

    ISystem* m_pSystem;

    IMovieUser* m_pUser;
    IMovieCallback* m_pCallback;

    AZ::TimeUs m_lastUpdateTime;

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

    SCameraParams m_ActiveCameraParams;

    ESequenceStopBehavior m_sequenceStopBehavior;

    bool m_bStartCapture;
    int m_captureFrame;
    bool m_bEndCapture;
    ICaptureKey m_captureKey;
    AZ::TimeUs m_fixedTimeStepBackUp;
    float m_maxTimeStepForMovieSystemBackUp;
    ICVar* m_cvar_capture_frame_once;
    ICVar* m_cvar_capture_folder;
    ICVar* m_cvar_sys_maxTimeStepForMovieSystem;
    ICVar* m_cvar_capture_frames;
    ICVar* m_cvar_capture_file_prefix;

    static int m_mov_NoCutscenes;
    ICVar* m_mov_overrideCam;

    bool m_bPhysicsEventsEnabled;

    bool m_bBatchRenderMode;

    // next available sequenceId
    uint32 m_nextSequenceId;

    void ShowPlayedSequencesDebug();


    using AnimParamSystemString = AZStd::string;

    template <typename KeyType, typename MappedType, typename Compare = stl::less_stricmp<KeyType>>
    using AnimSystemOrderedMap = AZStd::map<KeyType, MappedType, Compare>;
    template <typename KeyType, typename MappedType, typename Hasher = AZStd::hash<KeyType>, typename EqualKey = AZStd::equal_to<>>
    using AnimSystemUnorderedMap = AZStd::unordered_map<KeyType, MappedType, Hasher, EqualKey>;

    AnimSystemUnorderedMap<AnimNodeType, AnimParamSystemString> m_animNodeEnumToStringMap;
    AnimSystemOrderedMap<AnimParamSystemString, AnimNodeType> m_animNodeStringToEnumMap;

    AnimSystemUnorderedMap<AnimParamType, AnimParamSystemString> m_animParamEnumToStringMap;
    AnimSystemOrderedMap<AnimParamSystemString, AnimParamType> m_animParamStringToEnumMap;

    void RegisterNodeTypes();
    void RegisterParamTypes();

private:
    static float m_mov_cameraPrecacheTime;
#if !defined(_RELEASE)
    static int m_mov_DebugEvents;
    static int m_mov_debugCamShake;
#endif
};
