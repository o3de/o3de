/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include <AzCore/Serialization/SerializeContext.h>

struct PlayingUIAnimSequence
{
    //! Sequence playing
    AZStd::intrusive_ptr<IUiAnimSequence> sequence;

    //! Start/End/Current playing time for this sequence.
    float startTime;
    float endTime;
    float currentTime;
    float currentSpeed;

    //! Sequence from other sequence's sequence track
    bool trackedSequence;
    bool bSingleFrame;
};

struct IConsoleCmdArgs;

class UiAnimationSystem
    : public IUiAnimationSystem
{
    typedef AZStd::vector<PlayingUIAnimSequence> PlayingSequences;

public:
    AZ_CLASS_ALLOCATOR(UiAnimationSystem, AZ::SystemAllocator, 0)
    AZ_RTTI(UiAnimationSystem, "{2592269B-EF74-4409-B29F-682DC0B45DAF}", IUiAnimationSystem)

    UiAnimationSystem();

    ~UiAnimationSystem();

    void Release() { delete this; };

    bool Load(const char* pszFile, const char* pszMission);

    ISystem* GetSystem() { return m_pSystem; }

    IUiAnimTrack* CreateTrack(EUiAnimCurveType type);

    IUiAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0);
    IUiAnimSequence* LoadSequence(const char* pszFilePath);
    IUiAnimSequence* LoadSequence(XmlNodeRef& xmlNode, bool bLoadEmpty = true);

    void AddSequence(IUiAnimSequence* pSequence);
    void RemoveSequence(IUiAnimSequence* pSequence);
    IUiAnimSequence* FindSequence(const char* sequence) const;
    IUiAnimSequence* FindSequenceById(uint32 id) const;
    IUiAnimSequence* GetSequence(int i) const;
    int GetNumSequences() const;
    IUiAnimSequence* GetPlayingSequence(int i) const;
    int GetNumPlayingSequences() const;
    bool IsCutScenePlaying() const;

    uint32 GrabNextSequenceId()
    { return m_nextSequenceId++; }

    int OnSequenceRenamed(const char* before, const char* after);
    int OnCameraRenamed(const char* before, const char* after);

    bool AddUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener);
    bool RemoveUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener);

    void RemoveAllSequences();

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    void PlaySequence(const char* sequence, IUiAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX);
    void PlaySequence(IUiAnimSequence* seq, IUiAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX);
    void PlayOnLoadSequences();

    bool StopSequence(const char* sequence);
    bool StopSequence(IUiAnimSequence* seq);
    bool AbortSequence(IUiAnimSequence* seq, bool bLeaveTime = false);

    void StopAllSequences();
    void StopAllCutScenes();
    void Pause(bool bPause);

    void Reset(bool bPlayOnReset, bool bSeekToStart);
    void StillUpdate();
    void PreUpdate(const float dt);
    void PostUpdate(const float dt);
    void Render();

    bool IsPlaying(IUiAnimSequence* seq) const;

    void Pause();
    void Resume();

    void SetRecording(bool recording) { m_bRecording = recording; };
    bool IsRecording() const { return m_bRecording; };

    void SetCallback(IUiAnimationCallback* pCallback) { m_pCallback = pCallback; }
    IUiAnimationCallback* GetCallback() { return m_pCallback; }
    void Callback(IUiAnimationCallback::ECallbackReason Reason, IUiAnimNode* pNode);

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bRemoveOldNodes = false, bool bLoadEmpty = true);
    void InitPostLoad(bool remapIds, LyShine::EntityIdMap* entityIdMap);

    void SetSequenceStopBehavior(ESequenceStopBehavior behavior);
    IUiAnimationSystem::ESequenceStopBehavior GetSequenceStopBehavior();

    float GetPlayingTime(IUiAnimSequence* pSeq);
    bool SetPlayingTime(IUiAnimSequence* pSeq, float fTime);

    float GetPlayingSpeed(IUiAnimSequence* pSeq);
    bool SetPlayingSpeed(IUiAnimSequence* pSeq, float fTime);

    bool GetStartEndTime(IUiAnimSequence* pSeq, float& fStartTime, float& fEndTime);
    bool SetStartEndTime(IUiAnimSequence* pSeq, const float fStartTime, const float fEndTime);

    void GoToFrame(const char* seqName, float targetFrame);

    void SerializeNodeType(EUiAnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags);
    virtual void SerializeParamType(CUiAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version);
    virtual void SerializeParamData(UiAnimParamData& animParamData, XmlNodeRef& xmlNode, bool bLoading);

    static const char* GetParamTypeName(const CUiAnimParamType& animParamType);

    void OnCameraCut();

    static void StaticInitialize();

    static void Reflect(AZ::SerializeContext* serializeContext);

    void NotifyTrackEventListeners(const char* eventName, const char* valueName, IUiAnimSequence* pSequence) override;

private:
    void NotifyListeners(IUiAnimSequence* pSequence, IUiAnimationListener::EUiAnimationEvent event);

    void InternalStopAllSequences(bool bIsAbort, bool bAnimate);
    bool InternalStopSequence(IUiAnimSequence* seq, bool bIsAbort, bool bAnimate);

    bool FindSequence(IUiAnimSequence* sequence, PlayingSequences::const_iterator& sequenceIteratorOut) const;
    bool FindSequence(IUiAnimSequence* sequence, PlayingSequences::iterator& sequenceIteratorOut);

    static void DoNodeStaticInitialisation();
    void UpdateInternal(const float dt, const bool bPreUpdate);

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    virtual EUiAnimNodeType GetNodeTypeFromString(const char* pString) const;
    virtual CUiAnimParamType GetParamTypeFromString(const char* pString) const;
#endif

    ISystem* m_pSystem;

    IUiAnimationCallback* m_pCallback;

    CTimeValue m_lastUpdateTime;

    typedef AZStd::vector<AZStd::intrusive_ptr<IUiAnimSequence> > Sequences;
    Sequences m_sequences;

    PlayingSequences m_playingSequences;

    typedef std::vector<IUiAnimationListener*> TUiAnimationListenerVec;
    typedef std::map<IUiAnimSequence*, TUiAnimationListenerVec> TUiAnimationListenerMap;

    // a container which maps sequences to all interested listeners
    // listeners is a vector (could be a set in case we have a lot of listeners, stl::push_back_unique!)
    TUiAnimationListenerMap m_animationListenerMap;

    bool    m_bRecording;
    bool    m_bPaused;

    ESequenceStopBehavior m_sequenceStopBehavior;

    // A sequence which turned on the early animation update last time
    uint32 m_nextSequenceId;

    void ShowPlayedSequencesDebug();
};
