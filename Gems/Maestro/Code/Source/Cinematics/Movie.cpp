/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/Entity.h>
#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/Components/CameraBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include "Movie.h"
#include "AnimSplineTrack.h"
#include "AnimSerializer.h"
#include "AnimSequence.h"
#include "CVarNode.h"
#include "ScriptVarNode.h"
#include "SceneNode.h"
#include "MaterialNode.h"
#include "EventNode.h"
#include "AnimPostFXNode.h"
#include "AnimScreenFaderNode.h"
#include "CommentNode.h"
#include "LayerNode.h"
#include "ShadowsSetupNode.h"

#include <StlUtils.h>
#include <MathConversion.h>
#include <StaticInstance.h>

#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <ITimer.h>
#include <IRenderer.h>
#include <IViewSystem.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/SequenceType.h>
#include <Maestro/Types/AnimParamType.h>

int CMovieSystem::m_mov_NoCutscenes = 0;
float CMovieSystem::m_mov_cameraPrecacheTime = 1.f;
#if !defined(_RELEASE)
int CMovieSystem::m_mov_DebugEvents = 0;
int CMovieSystem::m_mov_debugCamShake = 0;
#endif

#if !defined(_RELEASE)
struct SMovieSequenceAutoComplete
    : public IConsoleArgumentAutoComplete
{
    virtual int GetCount() const
    {
        return gEnv->pMovieSystem->GetNumSequences();
    }

    virtual const char* GetValue(int nIndex) const
    {
        IAnimSequence* sequence = gEnv->pMovieSystem->GetSequence(nIndex);
        if (sequence)
        {
            return sequence->GetName();
        }

        return "";
    }
};
#endif //#if !defined(_RELEASE)

#if !defined(_RELEASE)
static SMovieSequenceAutoComplete s_movieSequenceAutoComplete;
#endif

//////////////////////////////////////////////////////////////////////////
namespace
{
    using AnimParamSystemString = AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::stateless_allocator>;

    template <typename KeyType, typename MappedType, typename Compare = AZStd::less<KeyType>>
    using AnimSystemOrderedMap = AZStd::map<KeyType, MappedType, Compare, AZStd::stateless_allocator>;
    template <typename KeyType, typename MappedType, typename Hasher = AZStd::hash<KeyType>, typename EqualKey = AZStd::equal_to<KeyType>>
    using AnimSystemUnorderedMap = AZStd::unordered_map<KeyType, MappedType, Hasher, EqualKey, AZStd::stateless_allocator>;
}

// Serialization for anim nodes & param types
#define REGISTER_NODE_TYPE(name) assert(!g_animNodeEnumToStringMap.contains(AnimNodeType::name)); \
    g_animNodeEnumToStringMap[AnimNodeType::name] = AZ_STRINGIZE(name);                          \
    g_animNodeStringToEnumMap[AnimParamSystemString(AZ_STRINGIZE(name))] = AnimNodeType::name;

#define REGISTER_PARAM_TYPE(name) assert(!g_animParamEnumToStringMap.contains(AnimParamType::name)); \
    g_animParamEnumToStringMap[AnimParamType::name] = AZ_STRINGIZE(name);                           \
    g_animParamStringToEnumMap[AnimParamSystemString(AZ_STRINGIZE(name))] = AnimParamType::name;

namespace
{
    AnimSystemUnorderedMap<AnimNodeType, AnimParamSystemString> g_animNodeEnumToStringMap;
    AnimSystemOrderedMap<AnimParamSystemString, AnimNodeType, stl::less_stricmp<AnimParamSystemString>> g_animNodeStringToEnumMap;

    AnimSystemUnorderedMap<AnimParamType, AnimParamSystemString> g_animParamEnumToStringMap;
    AnimSystemOrderedMap<AnimParamSystemString, AnimParamType, stl::less_stricmp<AnimParamSystemString>> g_animParamStringToEnumMap;

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
        REGISTER_NODE_TYPE(ShadowSetup)
        REGISTER_NODE_TYPE(Alembic)
        REGISTER_NODE_TYPE(GeomCache)
        REGISTER_NODE_TYPE(Environment)
        REGISTER_NODE_TYPE(AzEntity)
        REGISTER_NODE_TYPE(Component)
    }

    // If you get an assert in this function, it means two param types have the same enum value.
    void RegisterParamTypes()
    {
        REGISTER_PARAM_TYPE(FOV)
        REGISTER_PARAM_TYPE(Position)
        REGISTER_PARAM_TYPE(Rotation)
        REGISTER_PARAM_TYPE(Scale)
        REGISTER_PARAM_TYPE(Event)
        REGISTER_PARAM_TYPE(Visibility)
        REGISTER_PARAM_TYPE(Camera)
        REGISTER_PARAM_TYPE(Animation)
        REGISTER_PARAM_TYPE(Sound)
        REGISTER_PARAM_TYPE(Sequence)
        REGISTER_PARAM_TYPE(Console)
        REGISTER_PARAM_TYPE(Music)                      ///@deprecated in 1.11, left in for legacy serialization
        REGISTER_PARAM_TYPE(Float)
        REGISTER_PARAM_TYPE(LookAt)
        REGISTER_PARAM_TYPE(TrackEvent)
        REGISTER_PARAM_TYPE(ShakeAmplitudeA)
        REGISTER_PARAM_TYPE(ShakeAmplitudeB)
        REGISTER_PARAM_TYPE(ShakeFrequencyA)
        REGISTER_PARAM_TYPE(ShakeFrequencyB)
        REGISTER_PARAM_TYPE(ShakeMultiplier)
        REGISTER_PARAM_TYPE(ShakeNoise)
        REGISTER_PARAM_TYPE(ShakeWorking)
        REGISTER_PARAM_TYPE(ShakeAmpAMult)
        REGISTER_PARAM_TYPE(ShakeAmpBMult)
        REGISTER_PARAM_TYPE(ShakeFreqAMult)
        REGISTER_PARAM_TYPE(ShakeFreqBMult)
        REGISTER_PARAM_TYPE(DepthOfField)
        REGISTER_PARAM_TYPE(FocusDistance)
        REGISTER_PARAM_TYPE(FocusRange)
        REGISTER_PARAM_TYPE(BlurAmount)
        REGISTER_PARAM_TYPE(Capture)
        REGISTER_PARAM_TYPE(TransformNoise)
        REGISTER_PARAM_TYPE(TimeWarp)
        REGISTER_PARAM_TYPE(FixedTimeStep)
        REGISTER_PARAM_TYPE(NearZ)
        REGISTER_PARAM_TYPE(Goto)
        REGISTER_PARAM_TYPE(PositionX)
        REGISTER_PARAM_TYPE(PositionY)
        REGISTER_PARAM_TYPE(PositionZ)
        REGISTER_PARAM_TYPE(RotationX)
        REGISTER_PARAM_TYPE(RotationY)
        REGISTER_PARAM_TYPE(RotationZ)
        REGISTER_PARAM_TYPE(ScaleX)
        REGISTER_PARAM_TYPE(ScaleY)
        REGISTER_PARAM_TYPE(ScaleZ)
        REGISTER_PARAM_TYPE(ColorR)
        REGISTER_PARAM_TYPE(ColorG)
        REGISTER_PARAM_TYPE(ColorB)
        REGISTER_PARAM_TYPE(CommentText)
        REGISTER_PARAM_TYPE(ScreenFader)
        REGISTER_PARAM_TYPE(LightDiffuse)
        REGISTER_PARAM_TYPE(LightRadius)
        REGISTER_PARAM_TYPE(LightDiffuseMult)
        REGISTER_PARAM_TYPE(LightHDRDynamic)
        REGISTER_PARAM_TYPE(LightSpecularMult)
        REGISTER_PARAM_TYPE(LightSpecPercentage)
        REGISTER_PARAM_TYPE(MaterialDiffuse)
        REGISTER_PARAM_TYPE(MaterialSpecular)
        REGISTER_PARAM_TYPE(MaterialEmissive)
        REGISTER_PARAM_TYPE(MaterialEmissiveIntensity)
        REGISTER_PARAM_TYPE(MaterialOpacity)
        REGISTER_PARAM_TYPE(MaterialSmoothness)
        REGISTER_PARAM_TYPE(TimeRanges)
        REGISTER_PARAM_TYPE(Physics)
        REGISTER_PARAM_TYPE(GSMCache)
        REGISTER_PARAM_TYPE(ShutterSpeed)
        REGISTER_PARAM_TYPE(Physicalize)
        REGISTER_PARAM_TYPE(PhysicsDriven)
        REGISTER_PARAM_TYPE(SunLongitude)
        REGISTER_PARAM_TYPE(SunLatitude)
        REGISTER_PARAM_TYPE(MoonLongitude)
        REGISTER_PARAM_TYPE(MoonLatitude)
        REGISTER_PARAM_TYPE(ProceduralEyes)
    }
}

//////////////////////////////////////////////////////////////////////////
CMovieSystem::CMovieSystem(ISystem* pSystem)
{
    m_pSystem = pSystem;
    m_bRecording = false;
    m_pCallback = nullptr;
    m_pUser = nullptr;
    m_bPaused = false;
    m_bEnableCameraShake = true;
    m_bCutscenesPausedInEditor = true;
    m_sequenceStopBehavior = eSSB_GotoEndTime;
    m_lastUpdateTime.SetValue(0);
    m_bStartCapture = false;
    m_captureFrame = -1;
    m_bEndCapture = false;
    m_fixedTimeStepBackUp = 0;
    m_maxStepBackUp = 0;
    m_smoothingBackUp = 0;
    m_cvar_capture_frame_once = nullptr;
    m_cvar_capture_folder = nullptr;
    m_cvar_t_FixedStep = nullptr;
    m_cvar_t_MaxStep = nullptr;
    m_cvar_t_Smoothing = nullptr;
    m_cvar_sys_maxTimeStepForMovieSystem = nullptr;
    m_cvar_capture_frames = nullptr;
    m_cvar_capture_file_prefix = nullptr;
    m_bPhysicsEventsEnabled = true;
    m_bBatchRenderMode = false;

    m_nextSequenceId = 1;

    REGISTER_CVAR2("mov_NoCutscenes", &m_mov_NoCutscenes, 0, 0, "Disable playing of Cut-Scenes");
    REGISTER_CVAR2("mov_cameraPrecacheTime", &m_mov_cameraPrecacheTime, 1.f, VF_NULL, "");
    m_mov_overrideCam = REGISTER_STRING("mov_overrideCam", "", VF_NULL, "Set the camera used for the sequence which overrides the camera track info in the sequence.\nUse the Camera Name for Object Entity Cameras (Legacy) or the Entity ID for Component Entity Cameras.");

    DoNodeStaticInitialisation();

    RegisterNodeTypes();
    RegisterParamTypes();
}

//////////////////////////////////////////////////////////////////////////
CMovieSystem::CMovieSystem()
    : CMovieSystem(gEnv->pSystem)
{
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::DoNodeStaticInitialisation()
{
    CAnimMaterialNode::Initialize();
    CAnimPostFXNode::Initialize();
    CAnimSceneNode::Initialize();
    CAnimScreenFaderNode::Initialize();
    CCommentNode::Initialize();
    CLayerNode::Initialize();
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::CreateSequence(const char* sequenceName, bool load, uint32 id, SequenceType sequenceType, AZ::EntityId entityId)
{
    if (!load)
    {
        id = m_nextSequenceId++;
    }

    IAnimSequence* sequence = aznew CAnimSequence(this, id, sequenceType);
    sequence->SetName(sequenceName);
    sequence->SetSequenceEntityId(entityId);

    m_sequences.push_back(sequence);;

    return sequence;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::FindLegacySequenceByName(const char* pSequenceName) const
{
    assert(pSequenceName);
    if (!pSequenceName)
    {
        return NULL;
    }

    for (Sequences::const_iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        IAnimSequence* pCurrentSequence = it->get();
        const char* fullname = pCurrentSequence->GetName();

        if (_stricmp(fullname, pSequenceName) == 0)
        {
            return pCurrentSequence;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::FindSequence(const AZ::EntityId& componentEntitySequenceId) const
{
    IAnimSequence* retSequence = nullptr;
    if (componentEntitySequenceId.IsValid())
    {
        for (Sequences::const_iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
        {
            const AZ::EntityId& seqOwnerId = it->get()->GetSequenceEntityId();
            if (seqOwnerId == componentEntitySequenceId)
            {
                retSequence = it->get();
                break;
            }
        }
    }
    return retSequence;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::FindSequenceById(uint32 id) const
{
    if (id == 0 || id >= m_nextSequenceId)
    {
        return NULL;
    }

    for (Sequences::const_iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        IAnimSequence* pCurrentSequence = it->get();
        if (id == pCurrentSequence->GetId())
        {
            return pCurrentSequence;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::FindSequence(IAnimSequence* sequence, PlayingSequences::const_iterator& sequenceIteratorOut) const
{
    PlayingSequences::const_iterator itend = m_playingSequences.end();

    for (sequenceIteratorOut = m_playingSequences.begin(); sequenceIteratorOut != itend; ++sequenceIteratorOut)
    {
        if (sequenceIteratorOut->sequence == sequence)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::FindSequence(IAnimSequence* sequence, PlayingSequences::iterator& sequenceIteratorOut)
{
    PlayingSequences::const_iterator itend = m_playingSequences.end();

    for (sequenceIteratorOut = m_playingSequences.begin(); sequenceIteratorOut != itend; ++sequenceIteratorOut)
    {
        if (sequenceIteratorOut->sequence == sequence)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::GetSequence(int i) const
{
    assert(i >= 0 && i < GetNumSequences());

    if (i < 0 || i >= GetNumSequences())
    {
        return NULL;
    }

    return m_sequences[i].get();
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::GetNumSequences() const
{
    return static_cast<int>(m_sequences.size());
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::GetPlayingSequence(int i) const
{
    assert(i >= 0 && i < GetNumPlayingSequences());

    if (i < 0 || i >= GetNumPlayingSequences())
    {
        return NULL;
    }

    return m_playingSequences[i].sequence.get();
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::GetNumPlayingSequences() const
{
    return static_cast<int>(m_playingSequences.size());
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::AddSequence(IAnimSequence* sequence)
{
    m_sequences.push_back(sequence);
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::IsCutScenePlaying() const
{
    const uint numPlayingSequences = static_cast<uint>(m_playingSequences.size());
    for (uint i = 0; i < numPlayingSequences; ++i)
    {
        const IAnimSequence* pAnimSequence = m_playingSequences[i].sequence.get();
        if (pAnimSequence && (pAnimSequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene) != 0)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::RemoveSequence(IAnimSequence* sequence)
{
    AZ_Assert(sequence != nullptr, "sequence should not be nullptr.");

    if (sequence)
    {
        IMovieCallback* callback = GetCallback();
        SetCallback(nullptr);
        StopSequence(sequence);

        // remove from m_newlyActivatedSequences in the edge case something was added but not processed yet.
        for (auto iter = m_newlyActivatedSequences.begin(); iter != m_newlyActivatedSequences.end(); ++iter)
        {
            if (sequence == *iter)
            {
                m_newlyActivatedSequences.erase(iter);
                break;
            }
        }

        for (Sequences::iterator iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
        {
            if (sequence == *iter)
            {
                m_movieListenerMap.erase(sequence);
                m_sequences.erase(iter);
                break;
            }
        }

        SetCallback(callback);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::OnSetSequenceId(uint32 sequenceId)
{
    if (sequenceId >= m_nextSequenceId)
    {
        m_nextSequenceId = sequenceId + 1;
    }
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::OnSequenceRenamed(const char* before, const char* after)
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
    // For every sequence,
    for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        // Find a director node, if any.
        for (int k = 0; k < (*it)->GetNodeCount(); ++k)
        {
            IAnimNode* node = (*it)->GetNode(k);
            if (node->GetType() != AnimNodeType::Director)
            {
                continue;
            }

            // If there is a director node, check whether it has a sequence track.
            IAnimTrack* track = node->GetTrackForParameter(AnimParamType::Sequence);
            if (track)
            {
                for (int m = 0; m < track->GetNumKeys(); ++m)
                {
                    ISequenceKey seqKey;
                    track->GetKey(m, &seqKey);
                    // For each key that refers the sequence, update the name.
                    if (!seqKey.szSelection.empty() && (_stricmp(seqKey.szSelection.c_str(), before) == 0))
                    {
                        seqKey.szSelection = after;
                        track->SetKey(m, &seqKey);
                        ++count;
                    }
                }
            }
            break;
        }
    }

    return count;
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::OnCameraRenamed(const char* before, const char* after)
{
    int count = 0;
    // For every sequence,
    for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        // Find a director node, if any.
        for (int k = 0; k < (*it)->GetNodeCount(); ++k)
        {
            IAnimNode* node = (*it)->GetNode(k);

            if (node->GetType() != AnimNodeType::Director)
            {
                continue;
            }

            // If there is a director node, check whether it has a camera track.
            IAnimTrack* track = node->GetTrackForParameter(AnimParamType::Camera);
            if (track)
            {
                for (int m = 0; m < track->GetNumKeys(); ++m)
                {
                    ISelectKey selKey;
                    track->GetKey(m, &selKey);
                    // For each key that refers the camera, update the name.
                    if (_stricmp(selKey.szSelection.c_str(), before) == 0)
                    {
                        selKey.szSelection = after;
                        track->SetKey(m, &selKey);
                        ++count;
                    }
                }
            }
            break;
        }
    }

    // For every sequence,
    for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
    {
        // Find camera nodes.
        for (int k = 0; k < (*it)->GetNodeCount(); ++k)
        {
            IAnimNode* node = (*it)->GetNode(k);

            if (node->GetType() != AnimNodeType::Camera)
            {
                continue;
            }

            // Update its name, if it's a corresponding one.
            if (_stricmp(node->GetName(), before) == 0)
            {
                node->SetName(after);
            }
        }
    }

    return count;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::RemoveAllSequences()
{
    IMovieCallback* pCallback = GetCallback();
    SetCallback(NULL);
    InternalStopAllSequences(true, false);
    m_sequences.clear();

    for (TMovieListenerMap::iterator it = m_movieListenerMap.begin(); it != m_movieListenerMap.end(); )
    {
        if (it->first)
        {
            m_movieListenerMap.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    SetCallback(pCallback);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlaySequence(const char* pSequenceName, IAnimSequence* pParentSeq, bool bResetFx, bool bTrackedSequence, float startTime, float endTime)
{
    IAnimSequence* sequence = FindLegacySequenceByName(pSequenceName);
    if (sequence)
    {
        PlaySequence(sequence, pParentSeq, bResetFx, bTrackedSequence, startTime, endTime);
    }
    else
    {
        gEnv->pLog->Log ("CMovieSystem::PlaySequence: Error: Sequence \"%s\" not found", pSequenceName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlaySequence(IAnimSequence* sequence, IAnimSequence* parentSeq,
    bool bResetFx, bool bTrackedSequence, float startTime, float endTime)
{
    assert(sequence != 0);
    if (!sequence || IsPlaying(sequence))
    {
        return;
    }

    if ((sequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene) || (sequence->GetFlags() & IAnimSequence::eSeqFlags_NoHUD))
    {
        // Don't play cut-scene if this console variable set.
        if (m_mov_NoCutscenes != 0)
        {
            return;
        }
    }

    // If this sequence is cut scene disable player.
    if (sequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene)
    {
        OnCameraCut();

        sequence->SetParentSequence(parentSeq);

        if (!gEnv->IsEditing() || !m_bCutscenesPausedInEditor)
        {
            if (m_pUser)
            {
                m_pUser->BeginCutScene(sequence, sequence->GetCutSceneFlags(), bResetFx);
            }
        }
    }

    sequence->Activate();
    sequence->Resume();
    static_cast<CAnimSequence*>(sequence)->OnStart();

    PlayingSequence ps;
    ps.sequence = sequence;
    ps.startTime = startTime == -FLT_MAX ? sequence->GetTimeRange().start : startTime;
    ps.endTime = endTime == -FLT_MAX ? sequence->GetTimeRange().end : endTime;
    ps.currentTime = startTime == -FLT_MAX ? sequence->GetTimeRange().start : startTime;
    ps.currentSpeed = 1.0f;
    ps.trackedSequence = bTrackedSequence;
    ps.bSingleFrame = false;
    // Make sure all members are initialized before pushing.
    m_playingSequences.push_back(ps);

    // tell all interested listeners
    NotifyListeners(sequence, IMovieListener::eMovieEvent_Started);
}

void CMovieSystem::NotifyListeners(IAnimSequence* sequence, IMovieListener::EMovieEvent event)
{
    ////////////////////////////////
    // Legacy Notification System
    TMovieListenerMap::iterator found (m_movieListenerMap.find(sequence));
    if (found != m_movieListenerMap.end())
    {
        TMovieListenerVec listForSeq = (*found).second;
        TMovieListenerVec::iterator iter (listForSeq.begin());
        while (iter != listForSeq.end())
        {
            (*iter)->OnMovieEvent(event, sequence);
            ++iter;
        }
    }

    // 'NULL' ones are listeners interested in every sequence. Do not send "update" here
    if (event != IMovieListener::eMovieEvent_Updated)
    {
        TMovieListenerMap::iterator found2 (m_movieListenerMap.find((IAnimSequence*)0));
        if (found2 != m_movieListenerMap.end())
        {
            TMovieListenerVec listForSeq = (*found2).second;
            TMovieListenerVec::iterator iter (listForSeq.begin());
            while (iter != listForSeq.end())
            {
                (*iter)->OnMovieEvent(event, sequence);
                ++iter;
            }
        }
    }

    /////////////////////////////////////
    // SequenceComponentNotification EBus
    const AZ::EntityId& sequenceComponentEntityId = sequence->GetSequenceEntityId();
    switch (event)
    {
        /*
            * When a sequence is stopped, Resume is called just before stopped (not sure why). To ensure that a OnStop notification is sent out after the Resume,
            * notifications for eMovieEvent_Started and eMovieEvent_Stopped are handled in IAnimSequence::OnStart and IAnimSequence::OnStop
            */
        case IMovieListener::eMovieEvent_Aborted:
        {
            Maestro::SequenceComponentNotificationBus::Event(sequenceComponentEntityId, &Maestro::SequenceComponentNotificationBus::Events::OnAbort, GetPlayingTime(sequence));
            break;
        }
        case IMovieListener::eMovieEvent_Updated:
        {
            Maestro::SequenceComponentNotificationBus::Event(sequenceComponentEntityId, &Maestro::SequenceComponentNotificationBus::Events::OnUpdate, GetPlayingTime(sequence));
            break;
        }
        default:
        {
            // do nothing for unhandled IMovieListener events
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::StopSequence(const char* pSequenceName)
{
    IAnimSequence* sequence = FindLegacySequenceByName(pSequenceName);
    if (sequence)
    {
        return StopSequence(sequence);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::StopSequence(IAnimSequence* sequence)
{
    return InternalStopSequence(sequence, false, true);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::InternalStopAllSequences(bool bAbort, bool bAnimate)
{
    while (!m_playingSequences.empty())
    {
        InternalStopSequence(m_playingSequences.begin()->sequence.get(), bAbort, bAnimate);
    }

    stl::free_container(m_playingSequences);
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::InternalStopSequence(IAnimSequence* sequence, bool bAbort, bool bAnimate)
{
    assert(sequence != 0);

    bool bRet = false;
    PlayingSequences::iterator it;

    if (FindSequence(sequence, it))
    {
        if (bAnimate && sequence->IsActivated())
        {
            if (m_sequenceStopBehavior == eSSB_GotoEndTime)
            {
                SAnimContext ac;
                ac.singleFrame = true;
                ac.time = sequence->GetTimeRange().end;
                sequence->Animate(ac);
            }
            else if (m_sequenceStopBehavior == eSSB_GotoStartTime)
            {
                SAnimContext ac;
                ac.singleFrame = true;
                ac.time = sequence->GetTimeRange().start;
                sequence->Animate(ac);
            }

            sequence->Deactivate();
        }

        // If this sequence is cut scene end it.
        if (sequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene)
        {
            if (!gEnv->IsEditing() || !m_bCutscenesPausedInEditor)
            {
                if (m_pUser)
                {
                    m_pUser->EndCutScene(sequence, sequence->GetCutSceneFlags(true));
                }
            }

            sequence->SetParentSequence(NULL);
        }

        // tell all interested listeners
        NotifyListeners(sequence, bAbort ? IMovieListener::eMovieEvent_Aborted : IMovieListener::eMovieEvent_Stopped);

        // erase the sequence after notifying listeners so if they choose to they can get the ending time of this sequence
        if (FindSequence(sequence, it))
        {
            m_playingSequences.erase(it);
        }

        sequence->Resume();
        static_cast<CAnimSequence*>(sequence)->OnStop();
        bRet = true;
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::AbortSequence(IAnimSequence* sequence, bool bLeaveTime)
{
    assert(sequence);

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

    return InternalStopSequence(sequence, true, !bLeaveTime);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StopAllSequences()
{
    InternalStopAllSequences(false, true);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StopAllCutScenes()
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
            IAnimSequence* pCurrentSequence = it->sequence.get();
            if (pCurrentSequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene)
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
bool CMovieSystem::IsPlaying(IAnimSequence* sequence) const
{
    if (nullptr == sequence)
    {
        return false;
    }

    for (PlayingSequences::const_iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        if (it->sequence->GetSequenceEntityId() == sequence->GetSequenceEntityId())
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Reset(bool bPlayOnReset, bool bSeekToStart)
{
    InternalStopAllSequences(true, false);

    // Reset all sequences.
    for (Sequences::iterator iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        IAnimSequence* pCurrentSequence = iter->get();
        NotifyListeners(pCurrentSequence, IMovieListener::eMovieEvent_Started);
        pCurrentSequence->Reset(bSeekToStart);
        NotifyListeners(pCurrentSequence, IMovieListener::eMovieEvent_Stopped);
    }

    if (bPlayOnReset)
    {
        for (Sequences::iterator iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
        {
            IAnimSequence* pCurrentSequence = iter->get();
            if (pCurrentSequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset)
            {
                PlaySequence(pCurrentSequence);
            }
        }
    }

    // un-pause the movie system
    m_bPaused = false;

    // Reset camera.
    SCameraParams CamParams = GetCameraParams();
    CamParams.cameraEntityId.SetInvalid();
    CamParams.fov = 0.0f;
    CamParams.justActivated = true;
    SetCameraParams(CamParams);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlayOnLoadSequences()
{
    for (Sequences::iterator sit = m_sequences.begin(); sit != m_sequences.end(); ++sit)
    {
        IAnimSequence* sequence = sit->get();
        if (sequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset)
        {
            PlaySequence(sequence);
        }
    }

    // Reset camera.
    SCameraParams CamParams = GetCameraParams();
    CamParams.cameraEntityId.SetInvalid();
    CamParams.fov = 0.0f;
    CamParams.justActivated = true;
    SetCameraParams(CamParams);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StillUpdate()
{
    if (!gEnv->IsEditor())
    {
        return;
    }

    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingSequence& playingSequence = *it;

        playingSequence.sequence->StillUpdate();
    }

    // Check for end capture here while in the editor.
    // In some cases, we might have signaled an end capture when leaving Game mode,
    // but ControlCapture hasn't been given a tick by Game to actually end the capture.
    // So make sure any pending end capture signaled gets shutdown here.
    CheckForEndCapture();
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::ShowPlayedSequencesDebug()
{
    float y = 10.0f;
    std::vector<const char*> names;

    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingSequence& playingSequence = *it;

        if (playingSequence.sequence == NULL)
        {
            continue;
        }

        y += 16.0f;

        for (int i = 0; i < playingSequence.sequence->GetNodeCount(); ++i)
        {
            // Checks nodes which happen to be in several sequences.
            // Those can be a bug, since several sequences may try to control the same entity.
            const char* name = playingSequence.sequence->GetNode(i)->GetName();
            bool alreadyThere = false;
            for (size_t k = 0; k < names.size(); ++k)
            {
                if (strcmp(names[k], name) == 0)
                {
                    alreadyThere = true;
                    break;
                }
            }

            if (alreadyThere == false)
            {
                names.push_back(name);
            }
        }

        y += 32.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PreUpdate(float deltaTime)
{
    // Sequences can be spawned in game via a dynamic slice, so process newly activated sequences to see
    // if they should be auto played.
    for (auto iter = m_newlyActivatedSequences.begin(); iter != m_newlyActivatedSequences.end(); ++iter)
    {
        IAnimSequence* sequence = *iter;
        if (sequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset && !IsPlaying(sequence))
        {
            PlaySequence(sequence);
        }
    }
    m_newlyActivatedSequences.clear();

    UpdateInternal(m_cvar_t_FixedStep ? m_cvar_t_FixedStep->GetFVal() : deltaTime, true);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PostUpdate(float deltaTime)
{
    UpdateInternal(m_cvar_t_FixedStep ? m_cvar_t_FixedStep->GetFVal() : deltaTime, false);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::UpdateInternal(const float deltaTime, const bool bPreUpdate)
{
    SAnimContext animContext;

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

    std::vector<IAnimSequence*> stopSequences;

    const size_t numPlayingSequences = m_playingSequences.size();
    for (size_t i = 0; i < numPlayingSequences; ++i)
    {
        PlayingSequence& playingSequence = m_playingSequences[i];

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
        const bool bSequenceEarlyUpdate = (playingSequence.sequence->GetFlags() & IAnimSequence::eSeqFlags_EarlyMovieUpdate) != 0;
        if (bPreUpdate && !bSequenceEarlyUpdate || !bPreUpdate && bSequenceEarlyUpdate)
        {
            continue;
        }

        int nSeqFlags = playingSequence.sequence->GetFlags();
        if ((nSeqFlags& IAnimSequence::eSeqFlags_CutScene) && m_mov_NoCutscenes != 0)
        {
            // Don't play cut-scene if no cut scenes console variable set.
            stopSequences.push_back(playingSequence.sequence.get());
            continue;
        }

        animContext.time = playingSequence.currentTime;
        animContext.sequence = playingSequence.sequence.get();
        animContext.dt = scaledTimeDelta;
        animContext.fps = fps;
        animContext.startTime = playingSequence.startTime;

        // Check time out of range, setting up playingSequence for the next Update
        bool wasLooped = false;

        // Add a tolerance to this check because we want currentTime == currentTime to keep
        // animating so the last frame is animated. This comes into play with a fixed time step
        // like when capturing render output.
        const float precisionTolerance = 0.0001f;
        if ((playingSequence.currentTime - precisionTolerance) > playingSequence.endTime)
        {
            int seqFlags = playingSequence.sequence->GetFlags();
            bool isLoop = ((seqFlags& IAnimSequence::eSeqFlags_OutOfRangeLoop) != 0);
            bool isConstant = ((seqFlags& IAnimSequence::eSeqFlags_OutOfRangeConstant) != 0);

            if (m_bBatchRenderMode || (!isLoop && !isConstant))
            {
                // If we're batch rendering or no out-of-range type specified sequence stopped when time reaches end of range.
                // Queue sequence for stopping.
                if (playingSequence.trackedSequence == false)
                {
                    stopSequences.push_back(playingSequence.sequence.get());
                }
                continue;
            }

            // note we'll never get here if in batchRenderMode or if outOfRange is set to 'Once' (not loop or constant)
            if (isLoop)
            {
                // Time wrap's back to the start of the time range.
                playingSequence.currentTime = playingSequence.startTime;                 // should there be a fmodf here?
                wasLooped = true;
            }
            // Time just continues normally past the end of time range for isConstant (nothing to do for isConstant)
        }
        else
        {
            NotifyListeners(playingSequence.sequence.get(), IMovieListener::eMovieEvent_Updated);
        }

        animContext.singleFrame = playingSequence.bSingleFrame;
        playingSequence.bSingleFrame = false;

        // Animate sequence. (Can invalidate iterator)
        playingSequence.sequence->Animate(animContext);

        // we call OnLoop() *after* Animate() to reset sounds (for CAnimSceneNodes), for the next update (the looped update)
        if (wasLooped)
        {
            playingSequence.sequence->OnLoop();
        }
    }

#if !defined(_RELEASE)
    if (m_mov_DebugEvents)
    {
        ShowPlayedSequencesDebug();
    }
#endif //#if !defined(_RELEASE)

    // Stop queued sequences.
    for (int i = 0; i < (int)stopSequences.size(); i++)
    {
        StopSequence(stopSequences[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Render()
{
    for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
    {
        PlayingSequence& playingSequence = *it;
        playingSequence.sequence->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Callback(IMovieCallback::ECallbackReason reason, IAnimNode* pNode)
{
    if (m_pCallback)
    {
        m_pCallback->OnMovieCallback(reason, pNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CMovieSystem>()
            ->Version(1)
            ->Field("Sequences", &CMovieSystem::m_sequences);
    }

    AnimSerializer::ReflectAnimTypes(context);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SetCameraParams(const SCameraParams& Params)
{
    m_ActiveCameraParams = Params;

    // Make sure the camera entity is valid
    if (m_ActiveCameraParams.cameraEntityId.IsValid())
    {
        // Component Camera
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_ActiveCameraParams.cameraEntityId);
        if (entity)
        {
            // Make sure the camera component was not removed from an entity that is used as a camera.
            if (!(entity->FindComponent(CameraComponentTypeId) || entity->FindComponent(EditorCameraComponentTypeId)))
            {
                // if this entity does not have a camera component, do not use it.
                m_ActiveCameraParams.cameraEntityId.SetInvalid();
            }
        }
    }

    if (m_pUser)
    {
        m_pUser->SetActiveCamera(m_ActiveCameraParams);
    }

    if (m_pCallback)
    {
        m_pCallback->OnSetCamera(m_ActiveCameraParams);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SendGlobalEvent(const char* pszEvent)
{
    if (m_pUser)
    {
        m_pUser->SendGlobalEvent(pszEvent);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Pause()
{
    m_bPaused = true;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Resume()
{
    m_bPaused = false;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PauseCutScenes()
{
    m_bCutscenesPausedInEditor = true;

    if (m_pUser != NULL)
    {
        for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
        {
            if (it->sequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene)
            {
                m_pUser->EndCutScene(it->sequence.get(), it->sequence->GetCutSceneFlags(true));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::ResumeCutScenes()
{
    if (m_mov_NoCutscenes != 0)
    {
        return;
    }

    m_bCutscenesPausedInEditor = false;

    if (m_pUser != NULL)
    {
        for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
        {
            if (it->sequence->GetFlags() & IAnimSequence::eSeqFlags_CutScene)
            {
                m_pUser->BeginCutScene(it->sequence.get(), it->sequence->GetCutSceneFlags(), true);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
float CMovieSystem::GetPlayingTime(IAnimSequence* sequence)
{
    if (!sequence || !IsPlaying(sequence))
    {
        return -1.0;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(sequence, it))
    {
        return it->currentTime;
    }

    return -1.0f;
}

float CMovieSystem::GetPlayingSpeed(IAnimSequence* sequence)
{
    if (!sequence || !IsPlaying(sequence))
    {
        return -1.0f;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(sequence, it))
    {
        return it->currentSpeed;
    }

    return -1.0f;
}

bool CMovieSystem::SetPlayingTime(IAnimSequence* sequence, float fTime)
{
    if (!sequence || !IsPlaying(sequence))
    {
        return false;
    }

    PlayingSequences::iterator it;
    if (FindSequence(sequence, it) && !(sequence->GetFlags() & IAnimSequence::eSeqFlags_NoSeek))
    {
        it->currentTime = fTime;
        it->bSingleFrame = true;
        NotifyListeners(sequence, IMovieListener::eMovieEvent_Updated);
        return true;
    }

    return false;
}

bool CMovieSystem::SetPlayingSpeed(IAnimSequence* sequence, float fSpeed)
{
    if (!sequence)
    {
        return false;
    }

    PlayingSequences::iterator it;
    if (FindSequence(sequence, it) && !(sequence->GetFlags() & IAnimSequence::eSeqFlags_NoSpeed))
    {
        NotifyListeners(sequence, IMovieListener::eMovieEvent_Updated);
        it->currentSpeed = fSpeed;
        return true;
    }

    return false;
}

bool CMovieSystem::GetStartEndTime(IAnimSequence* sequence, float& fStartTime, float& fEndTime)
{
    fStartTime = 0.0f;
    fEndTime = 0.0f;

    if (!sequence || !IsPlaying(sequence))
    {
        return false;
    }

    PlayingSequences::const_iterator it;
    if (FindSequence(sequence, it))
    {
        fStartTime = it->startTime;
        fEndTime = it->endTime;
        return true;
    }

    return false;
}

bool CMovieSystem::SetStartEndTime(IAnimSequence* pSeq, const float fStartTime, const float fEndTime)
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

void CMovieSystem::SetSequenceStopBehavior(ESequenceStopBehavior behavior)
{
    m_sequenceStopBehavior = behavior;
}

IMovieSystem::ESequenceStopBehavior CMovieSystem::GetSequenceStopBehavior()
{
    return m_sequenceStopBehavior;
}

bool CMovieSystem::AddMovieListener(IAnimSequence* sequence, IMovieListener* pListener)
{
    assert (pListener != 0);
    if (sequence != NULL && std::find(m_sequences.begin(), m_sequences.end(), sequence) == m_sequences.end())
    {
        gEnv->pLog->Log ("CMovieSystem::AddMovieListener: Sequence %p unknown to CMovieSystem", sequence);
        return false;
    }

    return stl::push_back_unique(m_movieListenerMap[sequence], pListener);
}

bool CMovieSystem::RemoveMovieListener(IAnimSequence* sequence, IMovieListener* pListener)
{
    assert (pListener != 0);
    if (sequence != NULL
        && std::find(m_sequences.begin(), m_sequences.end(), sequence) == m_sequences.end())
    {
        gEnv->pLog->Log ("CMovieSystem::AddMovieListener: Sequence %p unknown to CMovieSystem", sequence);
        return false;
    }
    return stl::find_and_erase(m_movieListenerMap[sequence], pListener);
}

#if !defined(_RELEASE)
void CMovieSystem::GoToFrameCmd(IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 3)
    {
        gEnv->pLog->LogError("GoToFrame failed! You should provide two arguments of 'sequence name' & 'frame time'.");
        return;
    }

    const char* pSeqName = pArgs->GetArg(1);
    float targetFrame = (float)atof(pArgs->GetArg(2));

    ((CMovieSystem*)gEnv->pMovieSystem)->GoToFrame(pSeqName, targetFrame);
}
#endif //#if !defined(_RELEASE)

#if !defined(_RELEASE)
void CMovieSystem::ListSequencesCmd([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    int numSequences = gEnv->pMovieSystem->GetNumSequences();
    for (int i = 0; i < numSequences; i++)
    {
        IAnimSequence* pSeq = gEnv->pMovieSystem->GetSequence(i);
        if (pSeq)
        {
            CryLogAlways("%s", pSeq->GetName());
        }
    }
}
#endif //#if !defined(_RELEASE)

#if !defined(_RELEASE)
void CMovieSystem::PlaySequencesCmd(IConsoleCmdArgs* pArgs)
{
    const char* sequenceName = pArgs->GetArg(1);
    gEnv->pMovieSystem->PlaySequence(sequenceName, NULL, false, false);
}
#endif //#if !defined(_RELEASE)

void CMovieSystem::GoToFrame(const char* seqName, float targetFrame)
{
    assert(seqName != NULL);

    if (gEnv->IsEditor() && gEnv->IsEditorGameMode() == false)
    {
        AZStd::string editorCmd = AZStd::string::format("mov_goToFrameEditor %s %f", seqName, targetFrame);
        gEnv->pConsole->ExecuteString(editorCmd.c_str());
        return;
    }

    for (PlayingSequences::iterator it = m_playingSequences.begin();
         it != m_playingSequences.end(); ++it)
    {
        PlayingSequence& ps = *it;

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

void CMovieSystem::EnableFixedStepForCapture(float step)
{
    if (nullptr == m_cvar_t_FixedStep)
    {
        m_cvar_t_FixedStep = gEnv->pConsole->GetCVar("t_FixedStep");
    }

    m_fixedTimeStepBackUp = m_cvar_t_FixedStep->GetFVal();
    m_cvar_t_FixedStep->Set(step);

    if (nullptr == m_cvar_t_MaxStep)
    {
        m_cvar_t_MaxStep = gEnv->pConsole->GetCVar("t_MaxStep");
    }

    // Make sure to make the max step large enough
    m_maxStepBackUp = m_cvar_t_MaxStep->GetFVal();
    if (step > m_maxStepBackUp)
    {
        m_cvar_t_MaxStep->Set(step);
    }

    if (nullptr == m_cvar_t_Smoothing)
    {
        m_cvar_t_Smoothing = gEnv->pConsole->GetCVar("t_Smoothing");
    }

    // Turn off framerate smoothing
    m_smoothingBackUp = m_cvar_t_Smoothing->GetFVal();
    m_cvar_t_Smoothing->Set(0);

    if (nullptr == m_cvar_sys_maxTimeStepForMovieSystem)
    {
        m_cvar_sys_maxTimeStepForMovieSystem = gEnv->pConsole->GetCVar("sys_maxTimeStepForMovieSystem");
    }

    // Make sure the max step for the movie system is big enough
    m_maxTimeStepForMovieSystemBackUp = m_cvar_sys_maxTimeStepForMovieSystem->GetFVal();
    if (step > m_maxTimeStepForMovieSystemBackUp)
    {
        m_cvar_sys_maxTimeStepForMovieSystem->Set(step);
    }
}

void CMovieSystem::DisableFixedStepForCapture()
{
    m_cvar_t_FixedStep->Set(m_fixedTimeStepBackUp);
    m_cvar_t_MaxStep->Set(m_maxStepBackUp);
    m_cvar_t_Smoothing->Set(m_smoothingBackUp);
    m_cvar_sys_maxTimeStepForMovieSystem->Set(m_maxTimeStepForMovieSystemBackUp);
}

void CMovieSystem::StartCapture(const ICaptureKey& key, int frame)
{
    m_bStartCapture = true;
    m_captureKey = key;
    m_captureFrame = frame;
}

void CMovieSystem::EndCapture()
{
    m_bEndCapture = true;
}

void CMovieSystem::CheckForEndCapture()
{
    if (m_bEndCapture)
    {
        m_captureFrame = -1;
        m_cvar_capture_frames->Set(0);
        m_bEndCapture = false;
    }
}

void CMovieSystem::ControlCapture()
{
#if !defined(NDEBUG)
    bool bBothStartAndEnd = m_bStartCapture && m_bEndCapture;
#endif
    assert(!bBothStartAndEnd);

    bool bAllCVarsReady
        = m_cvar_capture_frame_once && m_cvar_capture_folder && m_cvar_capture_frames;

    if (!bAllCVarsReady)
    {
        m_cvar_capture_frame_once = gEnv->pConsole->GetCVar("capture_frame_once");
        m_cvar_capture_folder = gEnv->pConsole->GetCVar("capture_folder");
        m_cvar_capture_frames = gEnv->pConsole->GetCVar("capture_frames");
        m_cvar_capture_file_prefix = gEnv->pConsole->GetCVar("capture_file_prefix");
    }

    bAllCVarsReady
        = m_cvar_capture_frame_once
            && m_cvar_capture_folder && m_cvar_capture_frames
            && m_cvar_capture_file_prefix;
    assert(bAllCVarsReady);

    if (!bAllCVarsReady)
    {
        m_bStartCapture = m_bEndCapture = false;
        return;
    }

    if (m_bStartCapture)
    {
        m_cvar_capture_frame_once->Set(m_captureKey.once ? 1 : 0);
        m_cvar_capture_folder->Set(m_captureKey.folder.c_str());
        m_cvar_capture_file_prefix->Set(m_captureKey.prefix.c_str());

        // one-based frame number, zero disables capture.
        m_cvar_capture_frames->Set(1 + m_captureFrame);

        m_bStartCapture = false;
    }

    CheckForEndCapture();
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::IsCapturing() const
{
    return m_cvar_capture_frames ? m_cvar_capture_frames->GetIVal() != 0 : false;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SerializeNodeType(AnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags)
{
    static const char* kType = "Type";

    if (bLoading)
    {
        // Old serialization values that are no longer
        // defined in IMovieSystem.h, but needed for conversion:
        static const int kOldParticleNodeType = 0x18;

        animNodeType = AnimNodeType::Invalid;

        // In old versions there was special code for particles
        // that is now handles by generic entity node code
        if (version == 0 && static_cast<int>(animNodeType) == kOldParticleNodeType)
        {
            animNodeType = AnimNodeType::Entity;
            return;
        }

        // Convert light nodes that are not part of a light
        // animation set to common entity nodes
        if (version <= 1 && animNodeType == AnimNodeType::Light && !(flags & IAnimSequence::eSeqFlags_LightAnimationSet))
        {
            animNodeType = AnimNodeType::Entity;
            return;
        }

        if (version <= 2)
        {
            int type;
            if (xmlNode->getAttr(kType, type))
            {
                animNodeType = (AnimNodeType)type;
            }

            return;
        }
        else
        {
            XmlString nodeTypeString;
            if (xmlNode->getAttr(kType, nodeTypeString))
            {
                assert(g_animNodeStringToEnumMap.find(nodeTypeString.c_str()) != g_animNodeStringToEnumMap.end());
                animNodeType = stl::find_in_map(g_animNodeStringToEnumMap, nodeTypeString.c_str(), AnimNodeType::Invalid);
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

namespace CAnimParamTypeXmlNames
{
    static const char* kParamUserValue = "paramUserValue";
    static const char* kVirtualPropertyName = "virtualPropertyName";
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::LoadParamTypeFromXml(CAnimParamType& animParamType, const XmlNodeRef& xmlNode, const uint version)
{
    static const char* kByNameAttrName = "paramIdIsName";

    animParamType.m_type = AnimParamType::Invalid;

    if (version <= 6)
    {
        static const char* kParamId = "paramId";

        if (xmlNode->haveAttr(kByNameAttrName))
        {
            XmlString name;
            if (xmlNode->getAttr(kParamId, name))
            {
                animParamType.m_type = AnimParamType::ByString;
                animParamType.m_name = name.c_str();
            }
        }
        else
        {
            int type;
            xmlNode->getAttr(kParamId, type);
            animParamType.m_type = (AnimParamType)type;
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
                animParamType.m_type = AnimParamType::ByString;

                XmlString userValue;
                xmlNode->getAttr(CAnimParamTypeXmlNames::kParamUserValue, userValue);
                animParamType.m_name = userValue;
            }
            else if (paramTypeString == "User")
            {
                animParamType.m_type = AnimParamType::User;

                int type;
                xmlNode->getAttr(CAnimParamTypeXmlNames::kParamUserValue, type);
                animParamType.m_type = (AnimParamType)type;
            }
            else
            {
                XmlString virtualPropertyValue;
                if (xmlNode->getAttr(CAnimParamTypeXmlNames::kVirtualPropertyName, virtualPropertyValue))
                {
                    animParamType.m_name = virtualPropertyValue;
                }

                assert(g_animParamStringToEnumMap.find(paramTypeString.c_str()) != g_animParamStringToEnumMap.end());
                animParamType.m_type = stl::find_in_map(g_animParamStringToEnumMap, paramTypeString.c_str(), AnimParamType::Invalid);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SaveParamTypeToXml(const CAnimParamType& animParamType, XmlNodeRef& xmlNode)
{
    static const char* kParamType = "paramType";
    const char* pTypeString = "Invalid";

    if (animParamType.m_type == AnimParamType::ByString)
    {
        pTypeString = "ByString";
        xmlNode->setAttr(CAnimParamTypeXmlNames::kParamUserValue, animParamType.m_name.c_str());
    }
    else if (animParamType.m_type >= AnimParamType::User)
    {
        pTypeString = "User";
        xmlNode->setAttr(CAnimParamTypeXmlNames::kParamUserValue, (int)animParamType.m_type);
    }
    else
    {
        if (!animParamType.m_name.empty())
        {
            // we have a named parameter that is NOT an AnimParamType::ByString (handled above). This is used for VirtualProperty names for Component Entities.
            xmlNode->setAttr(CAnimParamTypeXmlNames::kVirtualPropertyName, animParamType.m_name.c_str());
        }

        assert(g_animParamEnumToStringMap.find(animParamType.m_type) != g_animParamEnumToStringMap.end());
        pTypeString = g_animParamEnumToStringMap[animParamType.m_type].c_str();
    }

    xmlNode->setAttr(kParamType, pTypeString);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SerializeParamType(CAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version)
{

    if (bLoading)
    {
        LoadParamTypeFromXml(animParamType, xmlNode, version);
    }
    else
    {
        SaveParamTypeToXml(animParamType, xmlNode);
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CMovieSystem::GetParamTypeName(const CAnimParamType& animParamType)
{
    if (animParamType.m_type == AnimParamType::ByString)
    {
        return animParamType.GetName();
    }
    else if (animParamType.m_type >= AnimParamType::User)
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
void CMovieSystem::OnCameraCut()
{
}

void CMovieSystem::LogUserNotificationMsg([[maybe_unused]] const AZStd::string& msg)
{
#if !defined(_RELEASE)
    if (!m_notificationLogMsgs.empty())
    {
        m_notificationLogMsgs.append("\n");
    }
    m_notificationLogMsgs.append(msg);
#endif
    AZ_Warning("TrackView", false, "%s", msg.c_str());
}

void CMovieSystem::ClearUserNotificationMsgs()
{
#if !defined(_RELEASE)
    m_notificationLogMsgs.clear();
#endif
}

const AZStd::string& CMovieSystem::GetUserNotificationMsgs() const
{
#if !defined(_RELEASE)
    return m_notificationLogMsgs;
#else
    static AZStd::string emptyMsg;
    return emptyMsg;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::OnSequenceActivated(IAnimSequence* sequence)
{
    // Queue for processing, sequences will be removed after checked for auto start.
    m_newlyActivatedSequences.push_back(sequence);
}

//////////////////////////////////////////////////////////////////////////
StaticInstance<CLightAnimWrapper::LightAnimWrapperCache> CLightAnimWrapper::ms_lightAnimWrapperCache;
AZStd::intrusive_ptr<IAnimSequence> CLightAnimWrapper::ms_pLightAnimSet;

//////////////////////////////////////////////////////////////////////////
CLightAnimWrapper::CLightAnimWrapper(const char* name)
    : ILightAnimWrapper(name)
{
    m_nRefCounter = 1;
}

//////////////////////////////////////////////////////////////////////////
CLightAnimWrapper::~CLightAnimWrapper()
{
    RemoveCachedLightAnim(m_name.c_str());
}

//////////////////////////////////////////////////////////////////////////
bool CLightAnimWrapper::Resolve()
{
    IF (!m_pNode, 0)
    {
        IAnimSequence* pSeq = GetLightAnimSet();
        m_pNode = pSeq ? pSeq->FindNodeByName(m_name.c_str(), 0) : 0;
    }

    return m_pNode != 0;
}

//////////////////////////////////////////////////////////////////////////
CLightAnimWrapper* CLightAnimWrapper::Create(const char* name)
{
    CLightAnimWrapper* pWrapper = FindLightAnim(name);

    IF (pWrapper, 1)
    {
        pWrapper->AddRef();
    }
    else
    {
        pWrapper = new CLightAnimWrapper(name);
        CacheLightAnim(name, pWrapper);
    }

    return pWrapper;
}

//////////////////////////////////////////////////////////////////////////
void CLightAnimWrapper::ReconstructCache()
{
#if !defined(_RELEASE)
    if (!ms_lightAnimWrapperCache.empty())
    {
        __debugbreak();
    }
#endif

    ms_lightAnimWrapperCache.clear();
    SetLightAnimSet(0);
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CLightAnimWrapper::GetLightAnimSet()
{
    return ms_pLightAnimSet.get();
}

//////////////////////////////////////////////////////////////////////////
void CLightAnimWrapper::SetLightAnimSet(IAnimSequence* pLightAnimSet)
{
    ms_pLightAnimSet = pLightAnimSet;
}

void CLightAnimWrapper::InvalidateAllNodes()
{
#if !defined(_RELEASE)
    if (!gEnv->IsEditor())
    {
        __debugbreak();
    }
#endif
    // !!! Will only work in Editor as the renderer runs in single threaded mode !!!
    // Invalidate all node pointers before the light anim set can get destroyed via SetLightAnimSet(0).
    // They'll get re-resolved in the next frame via Resolve(). Needed for Editor undo, import, etc.
    LightAnimWrapperCache::iterator it = ms_lightAnimWrapperCache.begin();
    LightAnimWrapperCache::iterator itEnd = ms_lightAnimWrapperCache.end();
    for (; it != itEnd; ++it)
    {
        (*it).second->m_pNode = 0;
    }
}

CLightAnimWrapper* CLightAnimWrapper::FindLightAnim(const char* name)
{
    LightAnimWrapperCache::const_iterator it = ms_lightAnimWrapperCache.find(name);
    return it != ms_lightAnimWrapperCache.end() ? (*it).second : 0;
}

//////////////////////////////////////////////////////////////////////////
void CLightAnimWrapper::CacheLightAnim(const char* name, CLightAnimWrapper* p)
{
    ms_lightAnimWrapperCache.insert(LightAnimWrapperCache::value_type(AZStd::string(name), p));
}

//////////////////////////////////////////////////////////////////////////
void CLightAnimWrapper::RemoveCachedLightAnim(const char* name)
{
    ms_lightAnimWrapperCache.erase(name);
}

#ifdef MOVIESYSTEM_SUPPORT_EDITING
//////////////////////////////////////////////////////////////////////////
AnimNodeType CMovieSystem::GetNodeTypeFromString(const char* pString) const
{
    return stl::find_in_map(g_animNodeStringToEnumMap, pString, AnimNodeType::Invalid);
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CMovieSystem::GetParamTypeFromString(const char* pString) const
{
    const AnimParamType paramType = stl::find_in_map(g_animParamStringToEnumMap, pString, AnimParamType::Invalid);

    if (paramType != AnimParamType::Invalid)
    {
        return CAnimParamType(paramType);
    }

    return CAnimParamType(pString);
}
#endif


