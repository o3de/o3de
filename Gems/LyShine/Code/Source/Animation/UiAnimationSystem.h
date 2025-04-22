/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Time/ITime.h>

#include <CryCommon/StlUtils.h>

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
    AZ_CLASS_ALLOCATOR(UiAnimationSystem, AZ::SystemAllocator)
    AZ_RTTI(UiAnimationSystem, "{2592269B-EF74-4409-B29F-682DC0B45DAF}", IUiAnimationSystem)

    UiAnimationSystem();

    void Release() override { delete this; };

    bool Load(const char* pszFile, const char* pszMission) override;

    ISystem* GetSystem() override { return m_pSystem; }

    IUiAnimTrack* CreateTrack(EUiAnimCurveType type) override;

    IUiAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0) override;
    IUiAnimSequence* LoadSequence(const char* pszFilePath);
    IUiAnimSequence* LoadSequence(XmlNodeRef& xmlNode, bool bLoadEmpty = true) override;

    void AddSequence(IUiAnimSequence* pSequence) override;
    void RemoveSequence(IUiAnimSequence* pSequence) override;
    IUiAnimSequence* FindSequence(const char* sequence) const override;
    IUiAnimSequence* FindSequenceById(uint32 id) const override;
    IUiAnimSequence* GetSequence(int i) const override;
    int GetNumSequences() const override;
    IUiAnimSequence* GetPlayingSequence(int i) const override;
    int GetNumPlayingSequences() const override;
    bool IsCutScenePlaying() const override;

    uint32 GrabNextSequenceId() override
    { return m_nextSequenceId++; }

    int OnSequenceRenamed(const char* before, const char* after) override;
    int OnCameraRenamed(const char* before, const char* after) override;

    bool AddUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener) override;
    bool RemoveUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener) override;

    void RemoveAllSequences() override;

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    void PlaySequence(const char* sequence, IUiAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX) override;
    void PlaySequence(IUiAnimSequence* seq, IUiAnimSequence* parentSeq = NULL, bool bResetFX = true,
        bool bTrackedSequence = false, float startTime = -FLT_MAX, float endTime = -FLT_MAX) override;
    void PlayOnLoadSequences() override;

    bool StopSequence(const char* sequence) override;
    bool StopSequence(IUiAnimSequence* seq) override;
    bool AbortSequence(IUiAnimSequence* seq, bool bLeaveTime = false) override;

    void StopAllSequences() override;
    void StopAllCutScenes() override;
    void Pause(bool bPause);

    void Reset(bool bPlayOnReset, bool bSeekToStart) override;
    void StillUpdate() override;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Render() override;

    bool IsPlaying(IUiAnimSequence* seq) const override;

    void Pause() override;
    void Resume() override;

    void SetRecording(bool recording) override { m_bRecording = recording; };
    bool IsRecording() const override { return m_bRecording; };

    void SetCallback(IUiAnimationCallback* pCallback) override { m_pCallback = pCallback; }
    IUiAnimationCallback* GetCallback() override { return m_pCallback; }
    void Callback(IUiAnimationCallback::ECallbackReason Reason, IUiAnimNode* pNode);

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bRemoveOldNodes = false, bool bLoadEmpty = true) override;
    void InitPostLoad(bool remapIds, LyShine::EntityIdMap* entityIdMap) override;

    void SetSequenceStopBehavior(ESequenceStopBehavior behavior) override;
    IUiAnimationSystem::ESequenceStopBehavior GetSequenceStopBehavior() override;

    float GetPlayingTime(IUiAnimSequence* pSeq) override;
    bool SetPlayingTime(IUiAnimSequence* pSeq, float fTime) override;

    float GetPlayingSpeed(IUiAnimSequence* pSeq) override;
    bool SetPlayingSpeed(IUiAnimSequence* pSeq, float fTime) override;

    bool GetStartEndTime(IUiAnimSequence* pSeq, float& fStartTime, float& fEndTime) override;
    bool SetStartEndTime(IUiAnimSequence* pSeq, const float fStartTime, const float fEndTime) override;

    void GoToFrame(const char* seqName, float targetFrame) override;

    void SerializeNodeType(EUiAnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags);
    void SerializeParamType(CUiAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version) override;
    void SerializeParamData(UiAnimParamData& animParamData, XmlNodeRef& xmlNode, bool bLoading) override;

    const char* GetParamTypeName(const CUiAnimParamType& animParamType);

    void OnCameraCut();

    static void Reflect(AZ::SerializeContext* serializeContext);

    void NotifyTrackEventListeners(const char* eventName, const char* valueName, IUiAnimSequence* pSequence) override;

private:
    void NotifyListeners(IUiAnimSequence* pSequence, IUiAnimationListener::EUiAnimationEvent event);

    void InternalStopAllSequences(bool bIsAbort, bool bAnimate);
    bool InternalStopSequence(IUiAnimSequence* seq, bool bIsAbort, bool bAnimate);

    bool FindSequence(IUiAnimSequence* sequence, PlayingSequences::const_iterator& sequenceIteratorOut) const;
    bool FindSequence(IUiAnimSequence* sequence, PlayingSequences::iterator& sequenceIteratorOut);

    void UpdateInternal(const float dt, const bool bPreUpdate);

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    EUiAnimNodeType GetNodeTypeFromString(const char* pString) const override;
    CUiAnimParamType GetParamTypeFromString(const char* pString) const override;
#endif

    ISystem* m_pSystem;

    IUiAnimationCallback* m_pCallback;

    AZ::TimeUs m_lastUpdateTime;

    using Sequences = AZStd::vector<AZStd::intrusive_ptr<IUiAnimSequence> >;
    Sequences m_sequences;

    PlayingSequences m_playingSequences;

    using TUiAnimationListenerVec = AZStd::vector<IUiAnimationListener*>;
    using TUiAnimationListenerMap = AZStd::map<IUiAnimSequence*, TUiAnimationListenerVec> ;

    // a container which maps sequences to all interested listeners
    // listeners is a vector (could be a set in case we have a lot of listeners, stl::push_back_unique!)
    TUiAnimationListenerMap m_animationListenerMap;

    bool    m_bRecording;
    bool    m_bPaused;

    ESequenceStopBehavior m_sequenceStopBehavior;

    // A sequence which turned on the early animation update last time
    uint32 m_nextSequenceId;


    using UiAnimParamSystemString = AZStd::string;
    template <typename KeyType, typename MappedType, typename Compare = stl::less_stricmp<KeyType>>
    using UiAnimSystemOrderedMap = AZStd::map<KeyType, MappedType, Compare>;
    template <typename KeyType, typename MappedType, typename Hasher = AZStd::hash<KeyType>, typename EqualKey = AZStd::equal_to<>>
    using UiAnimSystemUnorderedMap = AZStd::unordered_map<KeyType, MappedType, Hasher, EqualKey>;

    UiAnimSystemUnorderedMap<int, UiAnimParamSystemString> m_animNodeEnumToStringMap;
    UiAnimSystemOrderedMap<UiAnimParamSystemString, EUiAnimNodeType> m_animNodeStringToEnumMap;

    UiAnimSystemUnorderedMap<int, UiAnimParamSystemString> m_animParamEnumToStringMap;
    UiAnimSystemOrderedMap<UiAnimParamSystemString, EUiAnimParamType> m_animParamStringToEnumMap;

    void RegisterNodeTypes();
    void RegisterParamTypes();

    void ShowPlayedSequencesDebug();
};
