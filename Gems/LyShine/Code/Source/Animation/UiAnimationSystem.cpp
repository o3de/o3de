/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAnimationSystem.h"
#include "AnimSplineTrack.h"
#include "AnimSequence.h"
#include "AzEntityNode.h"
#include "UiAnimSerialize.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <StlUtils.h>

#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <ITimer.h>
#include <IRenderer.h>
#include <IViewSystem.h>

//////////////////////////////////////////////////////////////////////////
namespace
{
    using UiAnimParamSystemString = AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::stateless_allocator>;

    template <typename KeyType, typename MappedType, typename Compare = AZStd::less<KeyType>>
    using UiAnimSystemOrderedMap = AZStd::map<KeyType, MappedType, Compare, AZStd::stateless_allocator>;
    template <typename KeyType, typename MappedType, typename Hasher = AZStd::hash<KeyType>, typename EqualKey = AZStd::equal_to<KeyType>>
    using UiAnimSystemUnorderedMap = AZStd::unordered_map<KeyType, MappedType, Hasher, EqualKey, AZStd::stateless_allocator>;
}
// Serialization for anim nodes & param types
#define REGISTER_NODE_TYPE(name) assert(!g_animNodeEnumToStringMap.contains(eUiAnimNodeType_ ## name)); \
    g_animNodeEnumToStringMap[eUiAnimNodeType_ ## name] = AZ_STRINGIZE(name);                                                            \
    g_animNodeStringToEnumMap[UiAnimParamSystemString(AZ_STRINGIZE(name))] = eUiAnimNodeType_ ## name;

#define REGISTER_PARAM_TYPE(name) assert(!g_animParamEnumToStringMap.contains(eUiAnimParamType_ ## name)); \
    g_animParamEnumToStringMap[eUiAnimParamType_ ## name] = AZ_STRINGIZE(name);                                                              \
    g_animParamStringToEnumMap[UiAnimParamSystemString(AZ_STRINGIZE(name))] = eUiAnimParamType_ ## name;

namespace
{
    UiAnimSystemUnorderedMap<int, UiAnimParamSystemString> g_animNodeEnumToStringMap;
    UiAnimSystemOrderedMap<UiAnimParamSystemString, EUiAnimNodeType, stl::less_stricmp<UiAnimParamSystemString>> g_animNodeStringToEnumMap;

    UiAnimSystemUnorderedMap<int, UiAnimParamSystemString> g_animParamEnumToStringMap;
    UiAnimSystemOrderedMap<UiAnimParamSystemString, EUiAnimParamType, stl::less_stricmp<UiAnimParamSystemString>> g_animParamStringToEnumMap;

    // If you get an assert in this function, it means two node types have the same enum value.
    void RegisterNodeTypes()
    {
        REGISTER_NODE_TYPE(Entity)
        REGISTER_NODE_TYPE(Director)
        REGISTER_NODE_TYPE(Camera)
        REGISTER_NODE_TYPE(CVar)
        REGISTER_NODE_TYPE(ScriptVar)
        REGISTER_NODE_TYPE(Material)
        REGISTER_NODE_TYPE(Event)
        REGISTER_NODE_TYPE(Group)
        REGISTER_NODE_TYPE(Layer)
        REGISTER_NODE_TYPE(Comment)
        REGISTER_NODE_TYPE(RadialBlur)
        REGISTER_NODE_TYPE(ColorCorrection)
        REGISTER_NODE_TYPE(DepthOfField)
        REGISTER_NODE_TYPE(ScreenFader)
        REGISTER_NODE_TYPE(Light)
        REGISTER_NODE_TYPE(HDRSetup)
        REGISTER_NODE_TYPE(ShadowSetup)
        REGISTER_NODE_TYPE(Alembic)
        REGISTER_NODE_TYPE(GeomCache)
        REGISTER_NODE_TYPE(Environment)
        REGISTER_NODE_TYPE(ScreenDropsSetup)
        REGISTER_NODE_TYPE(AzEntity)
    }

    // If you get an assert in this function, it means two param types have the same enum value.
    void RegisterParamTypes()
    {
        REGISTER_PARAM_TYPE(Event)
        REGISTER_PARAM_TYPE(Float)
        REGISTER_PARAM_TYPE(TrackEvent)
        REGISTER_PARAM_TYPE(AzComponentField)
    }
}

//////////////////////////////////////////////////////////////////////////
UiAnimationSystem::UiAnimationSystem()
{
    m_pSystem = (gEnv) ? gEnv->pSystem : nullptr;
    m_bRecording = false;
    m_pCallback = NULL;
    m_bPaused = false;
    m_sequenceStopBehavior = eSSB_GotoEndTime;
    m_lastUpdateTime.SetValue(0);

    m_nextSequenceId = 1;
}

//////////////////////////////////////////////////////////////////////////
UiAnimationSystem::~UiAnimationSystem()
{
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::DoNodeStaticInitialisation()
{
    CUiAnimAzEntityNode::Initialize();
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::Load(const char* pszFile, const char* pszMission)
{
    INDENT_LOG_DURING_SCOPE (true, "UI Animation system is loading the file '%s' (mission='%s')", pszFile, pszMission);

    XmlNodeRef rootNode = m_pSystem->LoadXmlFromFile(pszFile);
    if (!rootNode)
    {
        return false;
    }

    XmlNodeRef Node = NULL;

    for (int i = 0; i < rootNode->getChildCount(); i++)
    {
        XmlNodeRef missionNode = rootNode->getChild(i);
        XmlString sName;
        if (!(sName = missionNode->getAttr("Name")))
        {
            continue;
        }
        if (_stricmp(sName.c_str(), pszMission))
        {
            continue;
        }
        Node = missionNode;
        break;
    }

    if (!Node)
    {
        return false;
    }

    Serialize(Node, true, true, false);
    return true;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack* UiAnimationSystem::CreateTrack([[maybe_unused]] EUiAnimCurveType type)
{
#if 0
    switch (type)
    {
    case eUiAnimCurveType_TCBFloat:
        return new CTcbFloatTrack;
    case eUiAnimCurveType_TCBVector:
        return new CTcbVectorTrack;
    case eUiAnimCurveType_TCBQuat:
        return new CTcbQuatTrack;
    }
    ;
#endif
    assert(0);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::CreateSequence(const char* pSequenceName, bool bLoad, uint32 id)
{
    if (!bLoad)
    {
        id = m_nextSequenceId++;
    }

    IUiAnimSequence* pSequence = aznew CUiAnimSequence(this, id);
    pSequence->SetName(pSequenceName);
    m_sequences.push_back(pSequence);
    return pSequence;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::LoadSequence(const char* pszFilePath)
{
    XmlNodeRef sequenceNode = m_pSystem->LoadXmlFromFile(pszFilePath);
    if (sequenceNode)
    {
        return LoadSequence(sequenceNode);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::LoadSequence(XmlNodeRef& xmlNode, bool bLoadEmpty)
{
    IUiAnimSequence* pSequence = aznew CUiAnimSequence(this, 0);
    pSequence->Serialize(xmlNode, true, bLoadEmpty);

    // Delete previous sequence with the same name.
    const char* pFullName = pSequence->GetName();

    IUiAnimSequence* pPrevSeq = FindSequence(pFullName);
    if (pPrevSeq)
    {
        RemoveSequence(pPrevSeq);
    }

    m_sequences.push_back(pSequence);
    return pSequence;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::FindSequence(const char* pSequenceName) const
{
    assert(pSequenceName);
    if (!pSequenceName)
    {
        return NULL;
    }

    for (Sequences::const_iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        IUiAnimSequence* pCurrentSequence = it->get();
        const char* fullname = pCurrentSequence->GetName();

        if (_stricmp(fullname, pSequenceName) == 0)
        {
            return pCurrentSequence;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::FindSequenceById(uint32 id) const
{
    if (id == 0 || id >= m_nextSequenceId)
    {
        return NULL;
    }

    for (Sequences::const_iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        IUiAnimSequence* pCurrentSequence = it->get();
        if (id == pCurrentSequence->GetId())
        {
            return pCurrentSequence;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::FindSequence(IUiAnimSequence* pSequence, PlayingSequences::const_iterator& sequenceIteratorOut) const
{
    PlayingSequences::const_iterator itend = m_playingSequences.end();

    for (sequenceIteratorOut = m_playingSequences.begin(); sequenceIteratorOut != itend; ++sequenceIteratorOut)
    {
        if (sequenceIteratorOut->sequence == pSequence)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::FindSequence(IUiAnimSequence* pSequence, PlayingSequences::iterator& sequenceIteratorOut)
{
    PlayingSequences::const_iterator itend = m_playingSequences.end();

    for (sequenceIteratorOut = m_playingSequences.begin(); sequenceIteratorOut != itend; ++sequenceIteratorOut)
    {
        if (sequenceIteratorOut->sequence == pSequence)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::GetSequence(int i) const
{
    assert(i >= 0 && i < GetNumSequences());

    if (i < 0 || i >= GetNumSequences())
    {
        return NULL;
    }

    return m_sequences[i].get();
}

//////////////////////////////////////////////////////////////////////////
int UiAnimationSystem::GetNumSequences() const
{
    return static_cast<int>(m_sequences.size());
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence* UiAnimationSystem::GetPlayingSequence(int i) const
{
    assert(i >= 0 && i < GetNumPlayingSequences());

    if (i < 0 || i >= GetNumPlayingSequences())
    {
        return NULL;
    }

    return m_playingSequences[i].sequence.get();
}

//////////////////////////////////////////////////////////////////////////
int UiAnimationSystem::GetNumPlayingSequences() const
{
    return static_cast<int>(m_playingSequences.size());
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::AddSequence(IUiAnimSequence* pSequence)
{
    m_sequences.push_back(pSequence);
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::IsCutScenePlaying() const
{
    const uint numPlayingSequences = static_cast<uint>(m_playingSequences.size());
    for (uint i = 0; i < numPlayingSequences; ++i)
    {
        const IUiAnimSequence* pAnimSequence = m_playingSequences[i].sequence.get();
        if (pAnimSequence && (pAnimSequence->GetFlags() & IUiAnimSequence::eSeqFlags_CutScene) != 0)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::RemoveSequence(IUiAnimSequence* pSequence)
{
    assert(pSequence != 0);
    if (pSequence)
    {
        IUiAnimationCallback* pCallback = GetCallback();
        SetCallback(NULL);
        StopSequence(pSequence);

        for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
        {
            if (pSequence == *it)
            {
                m_animationListenerMap.erase(pSequence);
                m_sequences.erase(it);
                break;
            }
        }
        SetCallback(pCallback);
    }
}

//////////////////////////////////////////////////////////////////////////
int UiAnimationSystem::OnSequenceRenamed(const char* before, const char* after)
{
    assert(before && after);
    if (before == NULL || after == NULL)
    {
        return 0;
    }
    if (_stricmp(before, after) == 0)
    {
        return 0;
    }

    int count = 0;

    // UI_ANIMATION_REVISIT : this only did anything for director nodes

    return count;
}

//////////////////////////////////////////////////////////////////////////
int UiAnimationSystem::OnCameraRenamed([[maybe_unused]] const char* before, [[maybe_unused]] const char* after)
{
    // UI_ANIMATION_REVISIT - not used
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::RemoveAllSequences()
{
    IUiAnimationCallback* pCallback = GetCallback();
    SetCallback(NULL);
    InternalStopAllSequences(true, false);

    stl::free_container(m_sequences);

    for (TUiAnimationListenerMap::iterator it = m_animationListenerMap.begin(); it != m_animationListenerMap.end(); )
    {
        if (it->first)
        {
            m_animationListenerMap.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    SetCallback(pCallback);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::PlaySequence(const char* pSequenceName, IUiAnimSequence* pParentSeq, bool bResetFx, bool bTrackedSequence, float startTime, float endTime)
{
    IUiAnimSequence* pSequence = FindSequence(pSequenceName);
    if (pSequence)
    {
        PlaySequence(pSequence, pParentSeq, bResetFx, bTrackedSequence, startTime, endTime);
    }
    else
    {
        gEnv->pLog->Log ("UiAnimationSystem::PlaySequence: Error: Sequence \"%s\" not found", pSequenceName);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::PlaySequence(IUiAnimSequence* pSequence, IUiAnimSequence* parentSeq,
    [[maybe_unused]] bool bResetFx, bool bTrackedSequence, float startTime, float endTime)
{
    assert(pSequence != 0);
    if (!pSequence || IsPlaying(pSequence))
    {
        return;
    }

    // If this sequence is cut scene disable player.
    if (pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_CutScene)
    {
        OnCameraCut();

        pSequence->SetParentSequence(parentSeq);
    }

    pSequence->Activate();
    pSequence->Resume();
    static_cast<CUiAnimSequence*>(pSequence)->OnStart();

    PlayingUIAnimSequence ps;
    ps.sequence = pSequence;
    ps.startTime = startTime == -FLT_MAX ? pSequence->GetTimeRange().start : startTime;
    ps.endTime = endTime == -FLT_MAX ? pSequence->GetTimeRange().end : endTime;
    ps.currentTime = startTime == -FLT_MAX ? pSequence->GetTimeRange().start : startTime;
    ps.currentSpeed = 1.0f;
    ps.trackedSequence = bTrackedSequence;
    ps.bSingleFrame = false;
    // Make sure all members are initialized before pushing.
    m_playingSequences.push_back(ps);

    // tell all interested listeners
    NotifyListeners(pSequence, IUiAnimationListener::eUiAnimationEvent_Started);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::NotifyListeners(IUiAnimSequence* pSequence, IUiAnimationListener::EUiAnimationEvent event)
{
    TUiAnimationListenerMap::iterator found (m_animationListenerMap.find(pSequence));
    if (found != m_animationListenerMap.end())
    {
        TUiAnimationListenerVec listForSeq = (*found).second;
        TUiAnimationListenerVec::iterator iter (listForSeq.begin());
        while (iter != listForSeq.end())
        {
            (*iter)->OnUiAnimationEvent(event, pSequence);
            ++iter;
        }
    }

    // 'NULL' ones are listeners interested in every sequence. Do not send "update" here
    if (event != IUiAnimationListener::eUiAnimationEvent_Updated)
    {
        TUiAnimationListenerMap::iterator found2 (m_animationListenerMap.find((IUiAnimSequence*)0));
        if (found2 != m_animationListenerMap.end())
        {
            TUiAnimationListenerVec listForSeq = (*found2).second;
            TUiAnimationListenerVec::iterator iter (listForSeq.begin());
            while (iter != listForSeq.end())
            {
                (*iter)->OnUiAnimationEvent(event, pSequence);
                ++iter;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::NotifyTrackEventListeners(const char* eventName, const char* valueName, IUiAnimSequence* pSequence)
{
    TUiAnimationListenerMap::iterator found(m_animationListenerMap.find(pSequence));
    if (found != m_animationListenerMap.end())
    {
        TUiAnimationListenerVec listForSeq = (*found).second;
        TUiAnimationListenerVec::iterator iter(listForSeq.begin());
        while (iter != listForSeq.end())
        {
            (*iter)->OnUiTrackEvent(AZStd::string(eventName), AZStd::string(valueName), pSequence);
            ++iter;
        }
    }

    // 'NULL' ones are listeners interested in every sequence.
    TUiAnimationListenerMap::iterator found2(m_animationListenerMap.find(static_cast<IUiAnimSequence*>(nullptr)));
    if (found2 != m_animationListenerMap.end())
    {
        TUiAnimationListenerVec listForSeq = (*found2).second;
        TUiAnimationListenerVec::iterator iter(listForSeq.begin());
        while (iter != listForSeq.end())
        {
            (*iter)->OnUiTrackEvent(AZStd::string(eventName), AZStd::string(valueName), pSequence);
            ++iter;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::StopSequence(const char* pSequenceName)
{
    IUiAnimSequence* pSequence = FindSequence(pSequenceName);
    if (pSequence)
    {
        return StopSequence(pSequence);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::StopSequence(IUiAnimSequence* pSequence)
{
    return InternalStopSequence(pSequence, false, true);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::InternalStopAllSequences(bool bAbort, bool bAnimate)
{
    while (!m_playingSequences.empty())
    {
        InternalStopSequence(m_playingSequences.begin()->sequence.get(), bAbort, bAnimate);
    }

    stl::free_container(m_playingSequences);
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::InternalStopSequence(IUiAnimSequence* pSequence, bool bAbort, bool bAnimate)
{
    assert(pSequence != 0);

    bool bRet = false;
    PlayingSequences::iterator it;

    if (FindSequence(pSequence, it))
    {
        if (bAnimate)
        {
            if (m_sequenceStopBehavior == eSSB_GotoEndTime)
            {
                SUiAnimContext ac;
                ac.bSingleFrame = true;
                ac.time = pSequence->GetTimeRange().end;
                pSequence->Animate(ac);
            }
            else if (m_sequenceStopBehavior == eSSB_GotoStartTime)
            {
                SUiAnimContext ac;
                ac.bSingleFrame = true;
                ac.time = pSequence->GetTimeRange().start;
                pSequence->Animate(ac);
            }

            pSequence->Deactivate();
        }

        // tell all interested listeners
        NotifyListeners(pSequence, bAbort ? IUiAnimationListener::eUiAnimationEvent_Aborted : IUiAnimationListener::eUiAnimationEvent_Stopped);

        // erase the sequence after notifying listeners so if they choose to they can get the ending time of this sequence
        if (FindSequence(pSequence, it))
        {
            m_playingSequences.erase(it);
        }

        pSequence->Resume();
        static_cast<CUiAnimSequence*>(pSequence)->OnStop();
        bRet = true;
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::AbortSequence(IUiAnimSequence* pSequence, bool bLeaveTime)
{
    assert(pSequence);

    // to avoid any camera blending after aborting a cut scene
    IViewSystem* pViewSystem = gEnv->pSystem->GetIViewSystem();
    if (pViewSystem)
    {
        pViewSystem->SetBlendParams(0, 0, 0);
        IView* pView = pViewSystem->GetActiveView();
        if (pView)
        {
            pView->ResetBlending();
        }
    }

    return InternalStopSequence(pSequence, true, !bLeaveTime);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::StopAllSequences()
{
    InternalStopAllSequences(false, true);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::StopAllCutScenes()
{
    bool bAnyStoped;
    PlayingSequences::iterator next;
    do
    {
        bAnyStoped = false;
        for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); it = next)
        {
            next = it;
            ++next;
            IUiAnimSequence* pCurrentSequence = it->sequence.get();
            if (pCurrentSequence->GetFlags() & IUiAnimSequence::eSeqFlags_CutScene)
            {
                bAnyStoped = true;
                StopSequence(pCurrentSequence);
                break;
            }
        }
    } while (bAnyStoped);

    if (m_playingSequences.empty())
    {
        stl::free_container(m_playingSequences);
    }
}

//////////////////////////////////////////////////////////////////////////
bool UiAnimationSystem::IsPlaying(IUiAnimSequence* pSequence) const
{
    for (PlayingSequences::const_iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        if (it->sequence == pSequence)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Reset(bool bPlayOnReset, bool bSeekToStart)
{
    InternalStopAllSequences(true, false);

    // Reset all sequences.
    for (Sequences::iterator iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        IUiAnimSequence* pCurrentSequence = iter->get();
        NotifyListeners(pCurrentSequence, IUiAnimationListener::eUiAnimationEvent_Started);
        pCurrentSequence->Reset(bSeekToStart);
        NotifyListeners(pCurrentSequence, IUiAnimationListener::eUiAnimationEvent_Stopped);
    }

    if (bPlayOnReset)
    {
        for (Sequences::iterator iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
        {
            IUiAnimSequence* pCurrentSequence = iter->get();
            if (pCurrentSequence->GetFlags() & IUiAnimSequence::eSeqFlags_PlayOnReset)
            {
                PlaySequence(pCurrentSequence);
            }
        }
    }

    // un-pause the UI animation system
    m_bPaused = false;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::PlayOnLoadSequences()
{
    for (Sequences::iterator sit = m_sequences.begin(); sit != m_sequences.end(); ++sit)
    {
        IUiAnimSequence* pSequence = sit->get();
        if (pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_PlayOnReset)
        {
            PlaySequence(pSequence);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::StillUpdate()
{
    if (!gEnv->IsEditor())
    {
        return;
    }

    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingUIAnimSequence& playingSequence = *it;

        playingSequence.sequence->StillUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::ShowPlayedSequencesDebug()
{
    //f32 green[4] = {0, 1, 0, 1};
    //f32 purple[4] = {1, 0, 1, 1};
    //f32 white[4] = {1, 1, 1, 1};
    float y = 10.0f;
    AZStd::vector<AZStd::string> names;

    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingUIAnimSequence& playingSequence = *it;

        if (playingSequence.sequence == NULL)
        {
            continue;
        }

        AZ_Assert(false,"gEnv->pRenderer is always null so it can't be used here");
        //const char* fullname = playingSequence.sequence->GetName();
        //gEnv->pRenderer->Draw2dLabel(1.0f, y, 1.3f, green, false, "Sequence %s : %f (x %f)", fullname, playingSequence.currentTime, playingSequence.currentSpeed);

        y += 16.0f;

        for (int i = 0; i < playingSequence.sequence->GetNodeCount(); ++i)
        {
            // Checks nodes which happen to be in several sequences.
            // Those can be a bug, since several sequences may try to control the same entity.
            AZStd::string name = playingSequence.sequence->GetNode(i)->GetName();
            bool alreadyThere = false;
            if (AZStd::find(names.begin(), names.end(), name) != names.end())
            {
                alreadyThere = true;
            }
            else
            {
                names.push_back(name);
            }

            //gEnv->pRenderer->Draw2dLabel((21.0f + 100.0f * i), ((i % 2) ? (y + 8.0f) : y), 1.0f, alreadyThere ? white : purple, false, "%s", name.c_str());
        }

        y += 32.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::PreUpdate(float deltaTime)
{
    UpdateInternal(deltaTime, true);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::PostUpdate(float deltaTime)
{
    UpdateInternal(deltaTime, false);
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::UpdateInternal(const float deltaTime, const bool bPreUpdate)
{
    SUiAnimContext animContext;

    if (m_bPaused)
    {
        return;
    }

    // don't update more than once if dt==0.0
    CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
    if (deltaTime == 0.0f && curTime == m_lastUpdateTime && !gEnv->IsEditor())
    {
        return;
    }

    m_lastUpdateTime = curTime;

    float fps = 60.0f;

    std::vector<IUiAnimSequence*> stopSequences;

    const size_t numPlayingSequences = m_playingSequences.size();
    for (size_t i = 0; i < numPlayingSequences; ++i)
    {
        PlayingUIAnimSequence& playingSequence = m_playingSequences[i];

        if (playingSequence.sequence->IsPaused())
        {
            continue;
        }

        const float scaledTimeDelta = deltaTime * playingSequence.currentSpeed;

        // Increase play time in pre-update
        if (bPreUpdate)
        {
            playingSequence.currentTime += scaledTimeDelta;
        }

        // Skip sequence if current update does not apply
        const bool bSequenceEarlyUpdate = (playingSequence.sequence->GetFlags() & IUiAnimSequence::eSeqFlags_EarlyAnimationUpdate) != 0;
        if (bPreUpdate && !bSequenceEarlyUpdate || !bPreUpdate && bSequenceEarlyUpdate)
        {
            continue;
        }

        animContext.time = playingSequence.currentTime;
        animContext.pSequence = playingSequence.sequence.get();
        animContext.dt = scaledTimeDelta;
        animContext.fps = fps;
        animContext.startTime = playingSequence.startTime;

        // Check time out of range, setting up playingSequence for the next Update
        bool wasLooped = false;
        if (playingSequence.currentTime > playingSequence.endTime)
        {
            int seqFlags = playingSequence.sequence->GetFlags();
            if (seqFlags & IUiAnimSequence::eSeqFlags_OutOfRangeLoop)
            {
                // Time wrap's back to the start of the time range.
                playingSequence.currentTime = playingSequence.startTime; // should there be a fmodf here?
                wasLooped = true;
            }
            else if (seqFlags & IUiAnimSequence::eSeqFlags_OutOfRangeConstant)
            {
                // Time just continues normally past the end of time range.
            }
            else
            {
                // If no out-of-range type specified sequence stopped when time reaches end of range.
                // Que sequence for stopping.
                if (playingSequence.trackedSequence == false)
                {
                    stopSequences.push_back(playingSequence.sequence.get());
                }
                continue;
            }
        }
        else
        {
            NotifyListeners(playingSequence.sequence.get(), IUiAnimationListener::eUiAnimationEvent_Updated);
        }

        animContext.bSingleFrame = playingSequence.bSingleFrame;
        playingSequence.bSingleFrame = false;

        // Animate sequence. (Can invalidate iterator)
        playingSequence.sequence->Animate(animContext);

        // we call OnLoop() *after* Animate() to reset sounds (for CUiAnimSceneNodes), for the next update (the looped update)
        if (wasLooped)
        {
            playingSequence.sequence->OnLoop();
        }
    }

    // Stop queued sequences.
    for (int i = 0; i < (int)stopSequences.size(); i++)
    {
        StopSequence(stopSequences[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Render()
{
    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingUIAnimSequence& playingSequence = *it;
        playingSequence.sequence->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Callback(IUiAnimationCallback::ECallbackReason reason, IUiAnimNode* pNode)
{
    if (m_pCallback)
    {
        m_pCallback->OnUiAnimationCallback(reason, pNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Serialize(XmlNodeRef& xmlNode, bool bLoading, [[maybe_unused]] bool bRemoveOldNodes, bool bLoadEmpty)
{
    if (bLoading)
    {
        //////////////////////////////////////////////////////////////////////////
        // Load sequences from XML.
        //////////////////////////////////////////////////////////////////////////
        XmlNodeRef seqNode = xmlNode->findChild("SequenceData");
        if (seqNode)
        {
            RemoveAllSequences();

            INDENT_LOG_DURING_SCOPE(true, "SequenceData tag contains %u sequences", seqNode->getChildCount());

            for (int i = 0; i < seqNode->getChildCount(); i++)
            {
                XmlNodeRef childNode = seqNode->getChild(i);
                if (!LoadSequence(childNode, bLoadEmpty))
                {
                    return;
                }
            }
        }
    }
    else
    {
        XmlNodeRef sequencesNode = xmlNode->newChild("SequenceData");
        for (int i = 0; i < GetNumSequences(); ++i)
        {
            IUiAnimSequence* pSequence = GetSequence(i);
            XmlNodeRef sequenceNode = sequencesNode->newChild("Sequence");
            pSequence->Serialize(sequenceNode, false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::InitPostLoad(bool remapIds, LyShine::EntityIdMap* entityIdMap)
{
    for (int i = 0; i < GetNumSequences(); ++i)
    {
        IUiAnimSequence* pSequence = GetSequence(i);
        pSequence->InitPostLoad(this, remapIds, entityIdMap);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Pause()
{
    m_bPaused = true;
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Resume()
{
    m_bPaused = false;
}

//////////////////////////////////////////////////////////////////////////
float UiAnimationSystem::GetPlayingTime(IUiAnimSequence* pSequence)
{
    if (!pSequence || !IsPlaying(pSequence))
    {
        return -1.0;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(pSequence, it))
    {
        return it->currentTime;
    }

    return -1.0f;
}

float UiAnimationSystem::GetPlayingSpeed(IUiAnimSequence* pSequence)
{
    if (!pSequence || !IsPlaying(pSequence))
    {
        return -1.0f;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(pSequence, it))
    {
        return it->currentSpeed;
    }

    return -1.0f;
}

bool UiAnimationSystem::SetPlayingTime(IUiAnimSequence* pSequence, float fTime)
{
    if (!pSequence || !IsPlaying(pSequence))
    {
        return false;
    }

    PlayingSequences::iterator it;
    if (FindSequence(pSequence, it) && !(pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_NoSeek))
    {
        it->currentTime = fTime;
        it->bSingleFrame = true;
        NotifyListeners(pSequence, IUiAnimationListener::eUiAnimationEvent_Updated);
        return true;
    }

    return false;
}

bool UiAnimationSystem::SetPlayingSpeed(IUiAnimSequence* pSequence, float fSpeed)
{
    if (!pSequence)
    {
        return false;
    }

    PlayingSequences::iterator it;
    if (FindSequence(pSequence, it) && !(pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_NoSpeed))
    {
        NotifyListeners(pSequence, IUiAnimationListener::eUiAnimationEvent_Updated);
        it->currentSpeed = fSpeed;
        return true;
    }

    return false;
}

bool UiAnimationSystem::GetStartEndTime(IUiAnimSequence* pSequence, float& fStartTime, float& fEndTime)
{
    fStartTime = 0.0f;
    fEndTime = 0.0f;

    if (!pSequence || !IsPlaying(pSequence))
    {
        return false;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(pSequence, it))
    {
        fStartTime = it->startTime;
        fEndTime = it->endTime;
        return true;
    }

    return false;
}

bool UiAnimationSystem::SetStartEndTime(IUiAnimSequence* pSeq, const float fStartTime, const float fEndTime)
{
    if (!pSeq || !IsPlaying(pSeq))
    {
        return false;
    }

    PlayingSequences::iterator it;
    if (FindSequence(pSeq, it))
    {
        it->startTime = fStartTime;
        it->endTime = fEndTime;
        return true;
    }

    return false;
}

void UiAnimationSystem::SetSequenceStopBehavior(ESequenceStopBehavior behavior)
{
    m_sequenceStopBehavior = behavior;
}

IUiAnimationSystem::ESequenceStopBehavior UiAnimationSystem::GetSequenceStopBehavior()
{
    return m_sequenceStopBehavior;
}

bool UiAnimationSystem::AddUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener)
{
    assert (pListener != 0);
    if (pSequence != NULL && std::find(m_sequences.begin(), m_sequences.end(), pSequence) == m_sequences.end())
    {
        gEnv->pLog->Log ("UiAnimationSystem::AddUiAnimationListener: Sequence %p unknown to UiAnimationSystem", pSequence);
        return false;
    }

    return stl::push_back_unique(m_animationListenerMap[pSequence], pListener);
}

bool UiAnimationSystem::RemoveUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener)
{
    assert (pListener != 0);
    if (pSequence != NULL
        && std::find(m_sequences.begin(), m_sequences.end(), pSequence) == m_sequences.end())
    {
        gEnv->pLog->Log ("UiAnimationSystem::AddUiAnimationListener: Sequence %p unknown to UiAnimationSystem", pSequence);
        return false;
    }
    return stl::find_and_erase(m_animationListenerMap[pSequence], pListener);
}

void UiAnimationSystem::GoToFrame(const char* seqName, float targetFrame)
{
    assert(seqName != NULL);

    for (PlayingSequences::iterator it = m_playingSequences.begin();
         it != m_playingSequences.end(); ++it)
    {
        PlayingUIAnimSequence& ps = *it;

        const char* fullname = ps.sequence->GetName();
        if (strcmp(fullname, seqName) == 0)
        {
            assert(ps.sequence->GetTimeRange().start <= targetFrame && targetFrame <= ps.sequence->GetTimeRange().end);
            ps.currentTime = targetFrame;
            ps.bSingleFrame = true;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::SerializeNodeType(EUiAnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags)
{
    static const char* kType = "Type";

    if (bLoading)
    {
        // Old serialization values that are no longer
        // defined in IUiAnimationSystem.h, but needed for conversion:
        static const int kOldParticleNodeType = 0x18;

        animNodeType = eUiAnimNodeType_Invalid;

        // In old versions there was special code for particles
        // that is now handles by generic entity node code
        if (version == 0 && animNodeType == kOldParticleNodeType)
        {
            animNodeType = eUiAnimNodeType_Entity;
            return;
        }

        // Convert light nodes that are not part of a light
        // animation set to common entity nodes
        if (version <= 1 && animNodeType == eUiAnimNodeType_Light && !(flags & IUiAnimSequence::eSeqFlags_LightAnimationSet))
        {
            animNodeType = eUiAnimNodeType_Entity;
            return;
        }

        if (version <= 2)
        {
            int type;
            if (xmlNode->getAttr(kType, type))
            {
                animNodeType = (EUiAnimNodeType)type;
            }

            return;
        }
        else
        {
            XmlString nodeTypeString;
            if (xmlNode->getAttr(kType, nodeTypeString))
            {
                assert(g_animNodeStringToEnumMap.find(nodeTypeString.c_str()) != g_animNodeStringToEnumMap.end());
                animNodeType = stl::find_in_map(g_animNodeStringToEnumMap, nodeTypeString.c_str(), eUiAnimNodeType_Invalid);
            }
        }
    }
    else
    {
        const char* pTypeString = "Invalid";
        assert(g_animNodeEnumToStringMap.find(animNodeType) != g_animNodeEnumToStringMap.end());
        pTypeString = g_animNodeEnumToStringMap[animNodeType].c_str();
        xmlNode->setAttr(kType, pTypeString);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::SerializeParamType(CUiAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version)
{
    static const char* kByNameAttrName = "paramIdIsName";
    static const char* kParamUserValue = "paramUserValue";

    if (bLoading)
    {
        animParamType.m_type = eUiAnimParamType_Invalid;

        if (version <= 6)
        {
            static const char* kParamId = "paramId";

            if (xmlNode->haveAttr(kByNameAttrName))
            {
                XmlString name;
                if (xmlNode->getAttr(kParamId, name))
                {
                    animParamType.m_type = eUiAnimParamType_ByString;
                    animParamType.m_name = name.c_str();
                }
            }
            else
            {
                int type;
                xmlNode->getAttr(kParamId, type);
                animParamType.m_type = (EUiAnimParamType)type;
            }
        }
        else
        {
            static const char* kParamType = "paramType";

            XmlString paramTypeString;
            if (xmlNode->getAttr(kParamType, paramTypeString))
            {
                if (paramTypeString == "ByString")
                {
                    animParamType.m_type = eUiAnimParamType_ByString;

                    XmlString userValue;
                    xmlNode->getAttr(kParamUserValue, userValue);
                    animParamType.m_name = userValue;
                }
                else if (paramTypeString == "User")
                {
                    animParamType.m_type = eUiAnimParamType_User;

                    int type;
                    xmlNode->getAttr(kParamUserValue, type);
                    animParamType.m_type = (EUiAnimParamType)type;
                }
                else
                {
                    assert(g_animParamStringToEnumMap.find(paramTypeString.c_str()) != g_animParamStringToEnumMap.end());
                    animParamType.m_type = stl::find_in_map(g_animParamStringToEnumMap, paramTypeString.c_str(), eUiAnimParamType_Invalid);
                }
            }
        }
    }
    else
    {
        static const char* kParamType = "paramType";
        const char* pTypeString = "Invalid";

        if (animParamType.m_type == eUiAnimParamType_ByString)
        {
            pTypeString = "ByString";
            xmlNode->setAttr(kParamUserValue, animParamType.m_name.c_str());
        }
        else if (animParamType.m_type >= eUiAnimParamType_User)
        {
            pTypeString = "User";
            xmlNode->setAttr(kParamUserValue, (int)animParamType.m_type);
        }
        else
        {
            assert(g_animParamEnumToStringMap.find(animParamType.m_type) != g_animParamEnumToStringMap.end());
            pTypeString = g_animParamEnumToStringMap[animParamType.m_type].c_str();
        }

        xmlNode->setAttr(kParamType, pTypeString);
    }
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::SerializeParamData(UiAnimParamData& animParamData, XmlNodeRef& xmlNode, bool bLoading)
{
    static const char* kLabelComponentIdHi = "ParamComponentIdHi";
    static const char* kLabelComponentIdLo = "ParamComponentIdLo";
    static const char* kLabelTypeId = "ParamTypeId";
    static const char* kLabelName = "ParamName";
    static const char* kLabelOffset = "ParamOffset";

    if (bLoading)
    {
        unsigned long idHi = 0;
        unsigned long idLo = 0;
        XmlString uuidStr;
        XmlString nameStr;
        size_t offset = 0;

        xmlNode->getAttr(kLabelComponentIdHi, idHi);
        xmlNode->getAttr(kLabelComponentIdLo, idLo);
        xmlNode->getAttr(kLabelTypeId, uuidStr);
        xmlNode->getAttr(kLabelName, nameStr);
        xmlNode->getAttr(kLabelOffset, offset);

        AZ::u64 id64 = ((AZ::u64)idHi) << 32 | idLo;
        AZ::Uuid uuid(uuidStr.c_str(), uuidStr.length());

        animParamData = UiAnimParamData(id64, nameStr.c_str(), uuid, offset);
    }
    else
    {
        AZ::u64 id64 = animParamData.GetComponentId();
        unsigned long idHi = id64 >> 32;
        unsigned long idLo = id64 & 0xFFFFFFFF;

        XmlString uuidStr = animParamData.GetTypeId().ToString<XmlString>();
        XmlString nameStr(animParamData.GetName());
        size_t offset = animParamData.GetOffset();

        xmlNode->setAttr(kLabelComponentIdHi, idHi);
        xmlNode->setAttr(kLabelComponentIdLo, idLo);
        xmlNode->setAttr(kLabelTypeId, uuidStr);
        xmlNode->setAttr(kLabelName, nameStr);
        xmlNode->setAttr(kLabelOffset, offset);
    }
}

//////////////////////////////////////////////////////////////////////////
const char* UiAnimationSystem::GetParamTypeName(const CUiAnimParamType& animParamType)
{
    if (animParamType.m_type == eUiAnimParamType_ByString)
    {
        return animParamType.GetName();
    }
    else if (animParamType.m_type >= eUiAnimParamType_User)
    {
        return "User";
    }
    else
    {
        if (g_animParamEnumToStringMap.find(animParamType.m_type) != g_animParamEnumToStringMap.end())
        {
            return g_animParamEnumToStringMap[animParamType.m_type].c_str();
        }
    }

    return "Invalid";
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::OnCameraCut()
{
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::StaticInitialize()
{
    DoNodeStaticInitialisation();

    RegisterNodeTypes();
    RegisterParamTypes();
}

//////////////////////////////////////////////////////////////////////////
void UiAnimationSystem::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<UiAnimationSystem>()
        ->Version(1)
        ->Field("Sequences", &UiAnimationSystem::m_sequences);

    UiAnimSerialize::ReflectUiAnimTypes(serializeContext);

    // These rely on these classes having a friend declaration for this class
    serializeContext->Class<CUiAnimParamType>()
        ->Version(1)
        ->Field("Type", &CUiAnimParamType::m_type);

    serializeContext->Class<UiAnimParamData>()
        ->Version(1)
        ->Field("ComponentId", &UiAnimParamData::m_componentId)
        ->Field("TypeId", &UiAnimParamData::m_typeId)
        ->Field("Name", &UiAnimParamData::m_name);
}

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
//////////////////////////////////////////////////////////////////////////
EUiAnimNodeType UiAnimationSystem::GetNodeTypeFromString(const char* pString) const
{
    return stl::find_in_map(g_animNodeStringToEnumMap, pString, eUiAnimNodeType_Invalid);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimParamType UiAnimationSystem::GetParamTypeFromString(const char* pString) const
{
    const EUiAnimParamType paramType = stl::find_in_map(g_animParamStringToEnumMap, pString, eUiAnimParamType_Invalid);

    if (paramType != eUiAnimParamType_Invalid)
    {
        return CUiAnimParamType(paramType);
    }

    return CUiAnimParamType(pString);
}
#endif
