/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of IAnimSequence interface.



#include <Maestro/Bus/EditorSequenceComponentBus.h>

#include "AnimSequence.h"
#include "AnimAZEntityNode.h"
#include "AnimComponentNode.h"
#include "CVarNode.h"
#include "ScriptVarNode.h"
#include "SceneNode.h"
#include "StlUtils.h"
#include "MaterialNode.h"
#include "EventNode.h"
#include "LayerNode.h"
#include "CommentNode.h"
#include "AnimPostFXNode.h"
#include "AnimScreenFaderNode.h"
#include "ShadowsSetupNode.h"
#include "SequenceTrack.h"
#include "AnimNodeGroup.h"
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/SequenceType.h>
#include <Maestro/Types/AnimParamType.h>

//////////////////////////////////////////////////////////////////////////
CAnimSequence::CAnimSequence(IMovieSystem* pMovieSystem, uint32 id, SequenceType sequenceType)
    : m_refCount(0)
{
    m_nextGenId = 1;
    m_pMovieSystem = pMovieSystem;
    m_flags = 0;
    m_pParentSequence = nullptr;
    m_timeRange.Set(0, 10);
    m_bPaused = false;
    m_bActive = false;
    m_legacySequenceObject = nullptr;
    m_activeDirector = nullptr;
    m_activeDirectorNodeId = -1;
    m_precached = false;
    m_bResetting = false;
    m_sequenceType = sequenceType;
    m_time = -FLT_MAX;
    SetId(id);

    m_pEventStrings = aznew CAnimStringTable;
    m_expanded = true;
}

//////////////////////////////////////////////////////////////////////////
CAnimSequence::CAnimSequence()
    : CAnimSequence((gEnv) ? gEnv->pMovieSystem : nullptr, 0, SequenceType::SequenceComponent)
{
}

CAnimSequence::~CAnimSequence()
{
    // clear reference to me from all my nodes
    for (int i = static_cast<int>(m_nodes.size()); --i >= 0;)
    {
        if (m_nodes[i])
        {
            m_nodes[i]->SetSequence(nullptr);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetName(const char* name)
{
    if (!m_pMovieSystem)
    {
        return;   // should never happen, null pointer guard
    }

    AZStd::string originalName = GetName();

    m_name = name;
    m_pMovieSystem->OnSequenceRenamed(originalName.c_str(), m_name.c_str());

    // the sequence named LIGHT_ANIMATION_SET_NAME is a singleton sequence to hold all light animations.
    if (m_name == LIGHT_ANIMATION_SET_NAME)
    {
        // ensure it stays a singleton. If one already exists, deregister it.
        if (CLightAnimWrapper::GetLightAnimSet())
        {
            CLightAnimWrapper::InvalidateAllNodes();
            CLightAnimWrapper::SetLightAnimSet(0);
        }

        CLightAnimWrapper::SetLightAnimSet(this);
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CAnimSequence::GetName() const
{
    return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ResetId()
{
    if (!m_pMovieSystem)
    {
        return;   // should never happen, null pointer guard
    }

    SetId(m_pMovieSystem->GrabNextSequenceId());
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetCutSceneFlags(const bool localFlags) const
{
    int currentFlags = m_flags & (eSeqFlags_NoHUD | eSeqFlags_NoPlayer | eSeqFlags_NoGameSounds | eSeqFlags_NoAbort);

    if (m_pParentSequence != NULL)
    {
        if (localFlags == true)
        {
            currentFlags &= ~m_pParentSequence->GetCutSceneFlags();
        }
        else
        {
            currentFlags |= m_pParentSequence->GetCutSceneFlags();
        }
    }

    return currentFlags;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetParentSequence(IAnimSequence* pParentSequence)
{
    m_pParentSequence = pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
const IAnimSequence* CAnimSequence::GetParentSequence() const
{
    return m_pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetNodeCount() const
{
    return static_cast<int>(m_nodes.size());
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::GetNode(int index) const
{
    assert(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].get();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddNode(IAnimNode* animNode)
{
    AZ_Assert(animNode, "Expected valid animNode");
    if (!animNode)
    {
        return false;
    }

    animNode->SetSequence(this);
    animNode->SetTimeRange(m_timeRange);

    // Check if this node already in sequence. If found, don't add it again.
    bool found = false;
    for (int i = 0; i < (int)m_nodes.size(); i++)
    {
        if (animNode == m_nodes[i].get())
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        m_nodes.push_back(AZStd::intrusive_ptr<IAnimNode>(animNode));
    }

    const int nodeId = static_cast<CAnimNode*>(animNode)->GetId();
    if (nodeId >= (int)m_nextGenId)
    {
        m_nextGenId = nodeId + 1;
    }

    // Make sure m_nextTrackId is bigger than the biggest existing track id.
    // m_nextTrackId is not serialized (track id's are) so this code will
    // exercise every time a sequence is loaded.
    int trackCount = animNode->GetTrackCount();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
    {
        IAnimTrack* track = animNode->GetTrackByIndex(trackIndex);
        AZ_Assert(track, "Expected valid track");
        if (track->GetId() >= m_nextTrackId)
        {
            m_nextTrackId = track->GetId() + 1;
        }

        int subTrackCount = track->GetSubTrackCount();
        for (int subTrackIndex = 0; subTrackIndex < subTrackCount; subTrackIndex++)
        {
            IAnimTrack* subTrack = track->GetSubTrack(subTrackIndex);
            AZ_Assert(subTrack, "Expected valid subtrack.");
            if (subTrack->GetId() >= m_nextTrackId)
            {
                m_nextTrackId = subTrack->GetId() + 1;
            }
        }
    }
     
    if (animNode->NeedToRender())
    {
        AddNodeNeedToRender(animNode);
    }

    bool bNewDirectorNode = m_activeDirector == NULL && animNode->GetType() == AnimNodeType::Director;
    if (bNewDirectorNode)
    {
        m_activeDirector = animNode;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNodeInternal(AnimNodeType nodeType, uint32 nNodeId)
{
    if (!m_pMovieSystem)
    {
        return nullptr;   // should never happen, null pointer guard
    }

    CAnimNode* animNode = nullptr;

    if (nNodeId == -1)
    {
        nNodeId = m_nextGenId;
    }

    switch (nodeType)
    {
        case AnimNodeType::AzEntity:
            animNode = aznew CAnimAzEntityNode(nNodeId);
            break;
        case AnimNodeType::Component:
            animNode = aznew CAnimComponentNode(nNodeId);
            break;
        case AnimNodeType::CVar:
            animNode = aznew CAnimCVarNode(nNodeId);
            break;
        case AnimNodeType::ScriptVar:
            animNode = aznew CAnimScriptVarNode(nNodeId);
            break;
        case AnimNodeType::Director:
            animNode = aznew CAnimSceneNode(nNodeId);
            break;
        case AnimNodeType::Material:
            animNode = aznew CAnimMaterialNode(nNodeId);
            break;
        case AnimNodeType::Event:
            animNode = aznew CAnimEventNode(nNodeId);
            break;
        case AnimNodeType::Group:
            animNode = aznew CAnimNodeGroup(nNodeId);
            break;
        case  AnimNodeType::Layer:
            animNode = aznew CLayerNode(nNodeId);
            break;
        case AnimNodeType::Comment:
            animNode = aznew CCommentNode(nNodeId);
            break;
        case AnimNodeType::RadialBlur:
        case AnimNodeType::ColorCorrection:
        case AnimNodeType::DepthOfField:
            animNode = CAnimPostFXNode::CreateNode(nNodeId, nodeType);
            break;
        case AnimNodeType::ShadowSetup:
            animNode = aznew CShadowsSetupNode(nNodeId);
            break;
        case AnimNodeType::ScreenFader:
            animNode = aznew CAnimScreenFaderNode(nNodeId);
            break;
        default:     
            m_pMovieSystem->LogUserNotificationMsg("AnimNode cannot be added because it is an unsupported object type.");
            break;
    }

    if (animNode)
    {
        if (AddNode(animNode))
        {
            // If there isn't an active director, set it now.
            if (m_activeDirector == nullptr && animNode->GetType() == AnimNodeType::Director)
            {
                SetActiveDirector(animNode);
            }
        }
    }

    return animNode;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNode(AnimNodeType nodeType)
{
    return CreateNodeInternal(nodeType);
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNode(XmlNodeRef node)
{
    if (!GetMovieSystem())
    {
        return nullptr;   // should never happen, null pointer guard
    }

    AnimNodeType type;
    GetMovieSystem()->SerializeNodeType(type, node, true, IAnimSequence::kSequenceVersion, 0);

    XmlString name;
    if (!node->getAttr("Name", name))
    {
        return nullptr;
    }

    IAnimNode* pNewNode = CreateNode(type);
    if (!pNewNode)
    {
        return nullptr;
    }

    pNewNode->SetName(name);
    pNewNode->Serialize(node, true, true);

    CAnimNode* newAnimNode = static_cast<CAnimNode*>(pNewNode);

    // Make sure de-serializing this node didn't just create an id conflict. This can happen sometimes
    // when copy/pasting nodes from a different sequence to this one. 
    for (const auto& curNode : m_nodes)
    {
        CAnimNode* animNode = static_cast<CAnimNode*>(curNode.get());
        if (animNode->GetId() == newAnimNode->GetId() && animNode != newAnimNode)
        {
            // Conflict detected, resolve it by assigning a new id to the new node.
            newAnimNode->SetId(m_nextGenId++);
        }
    }

    return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveNode(IAnimNode* node, bool removeChildRelationships)
{
    assert(node != 0);

    static_cast<CAnimNode*>(node)->Activate(false);
    static_cast<CAnimNode*>(node)->OnReset();

    for (int i = 0; i < (int)m_nodes.size(); )
    {
        if (node == m_nodes[i].get())
        {
            m_nodes.erase(m_nodes.begin() + i);

            if (node->NeedToRender())
            {
                RemoveNodeNeedToRender(node);
            }

            continue;
        }
        if (removeChildRelationships && m_nodes[i]->GetParent() == node)
        {
            m_nodes[i]->SetParent(nullptr);
        }

        i++;
    }

    // The removed one was the active director node.
    if (m_activeDirector == node)
    {
        // Clear the active one.
        m_activeDirector = nullptr;
        m_activeDirectorNodeId = -1;

        // If there is another director node, set it as active.
        for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pNode = it->get();
            if (pNode->GetType() == AnimNodeType::Director)
            {
                SetActiveDirector(pNode);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveAll()
{
    stl::free_container(m_nodes);
    stl::free_container(m_events);
    stl::free_container(m_nodesNeedToRender);
    m_activeDirector = nullptr;
    m_activeDirectorNodeId = -1;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Reset(bool bSeekToStart)
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    m_precached = false;
    m_bResetting = true;

    if (!bSeekToStart)
    {
        for (const auto& it :m_nodes)
        {
            IAnimNode* animNode = it.get();
            static_cast<CAnimNode*>(animNode)->OnReset();
        }
        m_bResetting = false;
        return;
    }

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SAnimContext ec;
    ec.singleFrame = true;
    ec.resetting = true;
    ec.sequence = this;
    ec.time = m_timeRange.start;
    Animate(ec);

    if (!bWasActive)
    {
        Deactivate();
    }
    else
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* animNode = it->get();
            static_cast<CAnimNode*>(animNode)->OnReset();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ResetHard()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    m_bResetting = true;

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SAnimContext ec;
    ec.singleFrame = true;
    ec.resetting = true;
    ec.sequence = this;
    ec.time = m_timeRange.start;
    Animate(ec);

    if (!bWasActive)
    {
        Deactivate();
    }
    else
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* animNode = it->get();
            static_cast<CAnimNode*>(animNode)->OnResetHard();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Pause()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet || m_bPaused)
    {
        return;
    }

    m_bPaused = true;

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->OnPause();
    }

    // Notify EBus listeners
    Maestro::SequenceComponentNotificationBus::Event(GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnPause);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Resume()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    if (m_bPaused)
    {
        m_bPaused = false;

        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* animNode = it->get();
            static_cast<CAnimNode*>(animNode)->OnResume();
        }

        // Notify EBus listeners
        Maestro::SequenceComponentNotificationBus::Event(GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnResume);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnLoop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->OnLoop();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::IsPaused() const
{
    return m_bPaused;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnStart()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->OnStart();
    }

    // notify listeners
    Maestro::SequenceComponentNotificationBus::Event(GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnStart, m_time);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnStop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->OnStop();
    }

    // notify listeners
    Maestro::SequenceComponentNotificationBus::Event(GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnStop, m_time);
}

void CAnimSequence::TimeChanged(float newTime)
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        animNode->TimeChanged(newTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::StillUpdate()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        animNode->StillUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Animate(const SAnimContext& ec)
{
    assert(m_bActive);

    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    SAnimContext animContext = ec;
    animContext.sequence = this;
    m_time = animContext.time;

    // Evaluate all animation nodes in sequence.
    // The director first.
    if (m_activeDirector)
    {
        m_activeDirector->Animate(animContext);
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        // Make sure correct animation block is binded to node.
        IAnimNode* animNode = it->get();

        // All other (inactive) director nodes are skipped.
        if (animNode->GetType() == AnimNodeType::Director)
        {
            continue;
        }

        // If this is a descendant of a director node and that director is currently not active, skip this one.
        IAnimNode* parentDirector = animNode->HasDirectorAsParent();
        if (parentDirector && parentDirector != m_activeDirector)
        {
            continue;
        }

        if (animNode->AreFlagsSetOnNodeOrAnyParent(eAnimNodeFlags_Disabled))
        {
            continue;
        }

        // Animate node.
        animNode->Animate(animContext);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Render()
{
    for (AnimNodes::iterator it = m_nodesNeedToRender.begin(); it != m_nodesNeedToRender.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        animNode->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Activate()
{
    if (m_bActive)
    {
        return;
    }

    m_bActive = true;
    // Assign animation block to all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->OnReset();
        static_cast<CAnimNode*>(animNode)->Activate(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Deactivate()
{
    if (!m_bActive)
    {
        return;
    }

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->Activate(false);
        static_cast<CAnimNode*>(animNode)->OnReset();
    }

    // Audio: Release precached sound

    m_bActive = false;
    m_precached = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheData(float startTime)
{
    PrecacheStatic(startTime);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheStatic(const float startTime)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->PrecacheStatic(startTime);
    }

    PrecacheDynamic(startTime);

    if (m_precached)
    {
        return;
    }

    gEnv->pLog->Log("=== Precaching render data for cutscene: %s ===", GetName());

    m_precached = true;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheDynamic(float time)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        static_cast<CAnimNode*>(animNode)->PrecacheDynamic(time);
    }
}

void CAnimSequence::SetId(uint32 newId)
{
    // Notify movie system of new Id
    if (GetMovieSystem())
    {
        GetMovieSystem()->OnSetSequenceId(newId);
    }
    m_id = newId;
}

//////////////////////////////////////////////////////////////////////////
static bool AnimSequenceVersionConverter(
    AZ::SerializeContext& serializeContext,
    AZ::SerializeContext::DataElementNode& rootElement)
{
    if (rootElement.GetVersion() < 5)
    {
        rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimSequence>());
    }

    return true;
}

void CAnimSequence::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAnimSequence, IAnimSequence>()
            ->Version(IAnimSequence::kSequenceVersion, &AnimSequenceVersionConverter)
            ->Field("Name", &CAnimSequence::m_name)
            ->Field("SequenceEntityId", &CAnimSequence::m_sequenceEntityId)
            ->Field("Flags", &CAnimSequence::m_flags)
            ->Field("TimeRange", &CAnimSequence::m_timeRange)
            ->Field("ID", &CAnimSequence::m_id)
            ->Field("Nodes", &CAnimSequence::m_nodes)
            ->Field("SequenceType", &CAnimSequence::m_sequenceType)
            ->Field("Events", &CAnimSequence::m_events)
            ->Field("Expanded", &CAnimSequence::m_expanded)
            ->Field("ActiveDirectorNodeId", &CAnimSequence::m_activeDirectorNodeId);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::InitPostLoad()
{
    if (m_pMovieSystem)
    {
        // Notify the MovieSystem of the new sequence Id (updates the next available Id if needed)
        m_pMovieSystem->OnSetSequenceId(GetId());

        // check of sequence ID collision and resolve if needed
        if (m_pMovieSystem->FindSequenceById(GetId()))
        {
            // A collision found - resolve by resetting Id. TODO: resolve all references to previous Id
            ResetId();
        }
    }

    IAnimNode* firstDirectorFound = nullptr;

    int nodeCount = GetNodeCount();
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        IAnimNode* animNode = GetNode(nodeIndex);
        if (animNode)
        {
            AddNode(animNode);
            animNode->InitPostLoad(this);

            // store the first director found
            if (!firstDirectorFound && animNode->GetType() == AnimNodeType::Director)
            {
                firstDirectorFound = animNode;
            }

            // m_activeDirectorNodeId is serialized in the sequences, so set
            // this node as the active director if id's match.
            if (animNode->GetId() == m_activeDirectorNodeId)
            {
                SetActiveDirector(animNode);
            }
        }
    }

    // All nodes and track have been added and m_nextTrackId is set higher than any
    // existing track id. Go over the Tracks and make sure all of track id's are assigned.
    // Track Id's are serialized and should never be zero, unless this is track data from
    // before Track Ids were added.
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        IAnimNode* animNode = GetNode(nodeIndex);
        if (animNode)
        {
            int trackCount = animNode->GetTrackCount();
            for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
            {
                IAnimTrack* track = animNode->GetTrackByIndex(trackIndex);
                AZ_Assert(track, "Expected valid track.");
                if (track->GetId() == 0)
                {
                    track->SetId(GetUniqueTrackIdAndGenerateNext());
                }

                int subTrackCount = track->GetSubTrackCount();
                for (int subTrackIndex = 0; subTrackIndex < subTrackCount; subTrackIndex++)
                {
                    IAnimTrack* subTrack = track->GetSubTrack(subTrackIndex);
                    AZ_Assert(subTrack, "Expected valid sub track.");
                    if (subTrack->GetId() == 0)
                    {
                        subTrack->SetId(GetUniqueTrackIdAndGenerateNext());
                    }
                }
            }
        }
    }

    // If the active director was not set, but there was a director found,
    // set it as the active director now. This can happen if the sequence
    // was serialized before the ActiveDirectorNodeId was added.
    if (!m_activeDirector && firstDirectorFound)
    {
        SetActiveDirector(firstDirectorFound);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetTimeRange(Range timeRange)
{
    m_timeRange = timeRange;
    // Set this time range for every track in animation.
    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* anode = it->get();
        anode->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::AdjustKeysToTimeRange(const Range& timeRange)
{
    float offset = timeRange.start - m_timeRange.start;
    // Calculate scale ratio.
    float scale = timeRange.Length() / m_timeRange.Length();
    m_timeRange = timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();

        int trackCount = animNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IAnimTrack* pTrack = animNode->GetTrackByIndex(paramIndex);
            int nkey = pTrack->GetNumKeys();
            for (int k = 0; k < nkey; k++)
            {
                float keytime = pTrack->GetKeyTime(k);
                keytime = offset + keytime * scale;
                pTrack->SetKeyTime(k, keytime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ComputeTimeRange()
{
    Range timeRange = m_timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();

        int trackCount = animNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IAnimTrack* pTrack = animNode->GetTrackByIndex(paramIndex);
            int nkey = pTrack->GetNumKeys();
            if (nkey > 0)
            {
                timeRange.start = std::min(timeRange.start, pTrack->GetKeyTime(0));
                timeRange.end = std::max(timeRange.end, pTrack->GetKeyTime(nkey - 1));
            }
        }
    }

    if (timeRange.start > 0)
    {
        timeRange.start = 0;
    }

    m_timeRange = timeRange;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    if (stl::push_back_unique(m_events, szEvent))
    {
        NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Added, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::RemoveTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    if (stl::find_and_erase(m_events, szEvent))
    {
        NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Removed, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::RenameTrackEvent(const char* szEvent, const char* szNewEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    CRY_ASSERT(szNewEvent && szNewEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            m_events[i] = szNewEvent;
            NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Renamed, szEvent, szNewEvent);
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::MoveUpTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            assert(i > 0);
            if (i > 0)
            {
                std::swap(m_events[i - 1], m_events[i]);
                NotifyTrackEvent(ITrackEventListener::eTrackEventReason_MovedUp, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::MoveDownTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            assert(i < m_events.size() - 1);
            if (i < m_events.size() - 1)
            {
                std::swap(m_events[i], m_events[i + 1]);
                NotifyTrackEvent(ITrackEventListener::eTrackEventReason_MovedDown, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ClearTrackEvents()
{
    m_events.clear();
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetTrackEventsCount() const
{
    return (int)m_events.size();
}

//////////////////////////////////////////////////////////////////////////
char const* CAnimSequence::GetTrackEvent(int iIndex) const
{
    char const* szResult = nullptr;
    const bool bValid = (iIndex >= 0 && iIndex < GetTrackEventsCount());
    CRY_ASSERT(bValid);

    if (bValid)
    {
        szResult = m_events[iIndex].c_str();
    }

    return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::NotifyTrackEvent(ITrackEventListener::ETrackEventReason reason,
    const char* event, const char* param)
{
    // Notify listeners
    for (TTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
    {
        (*j)->OnTrackEvent(this, reason, event, (void*)param);
    }

    // Notification via Event Bus
    Maestro::SequenceComponentNotificationBus::Event(GetSequenceEntityId(), &Maestro::SequenceComponentNotificationBus::Events::OnTrackEventTriggered, event, param);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::TriggerTrackEvent(const char* event, const char* param)
{
    NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Triggered, event, param);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::AddTrackEventListener(ITrackEventListener* pListener)
{
    if (std::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
    {
        m_listeners.push_back(pListener);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveTrackEventListener(ITrackEventListener* pListener)
{
    TTrackEventListeners::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
    if (it != m_listeners.end())
    {
        m_listeners.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeById(int nNodeId)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        if (static_cast<CAnimNode*>(animNode)->GetId() == nNodeId)
        {
            return animNode;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        // Case insensitive name comparison.
        if (_stricmp(static_cast<CAnimNode*>(animNode)->GetNameFast(), sNodeName) == 0)
        {
            bool bParentDirectorCheck = animNode->HasDirectorAsParent() == pParentDirector;
            if (bParentDirectorCheck)
            {
                return animNode;
            }
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ReorderNode(IAnimNode* pNode, IAnimNode* pPivotNode, bool bNext)
{
    if (pNode == pPivotNode || !pNode)
    {
        return;
    }

    AZStd::intrusive_ptr<IAnimNode> pTempHolder(pNode); // Keep reference to node so it is not deleted by erasing from list.
    stl::find_and_erase_if(m_nodes, [pNode](const AZStd::intrusive_ptr<IAnimNode>& sp) { return sp.get() == pNode; });

    AnimNodes::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* animNode = it->get();
        if (animNode == pPivotNode)
        {
            if (bNext)
            {
                m_nodes.insert(it + 1, AZStd::intrusive_ptr<IAnimNode>(pNode));
            }
            else
            {
                m_nodes.insert(it, AZStd::intrusive_ptr<IAnimNode>(pNode));
            }
            break;
        }
    }

    if (it == m_nodes.end())
    {
        m_nodes.insert(m_nodes.begin(), AZStd::intrusive_ptr<IAnimNode>(pNode));
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::CopyNodeChildren(XmlNodeRef& xmlNode, IAnimNode* animNode)
{
    for (int k = 0; k < GetNodeCount(); ++k)
    {
        if (GetNode(k)->GetParent() == animNode)
        {
            XmlNodeRef childNode = xmlNode->newChild("Node");
            GetNode(k)->Serialize(childNode, false, true);
            if (GetNode(k)->GetType() == AnimNodeType::Group
                || animNode->GetType() == AnimNodeType::Director)
            {
                CopyNodeChildren(xmlNode, GetNode(k));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        IAnimNode* animNode = pSelectedNodes[i];
        if (animNode)
        {
            XmlNodeRef xn = xmlNode->newChild("Node");
            animNode->Serialize(xn, false, true);
            // If it is a group node, copy its children also.
            if (animNode->GetType() == AnimNodeType::Group || animNode->GetType() == AnimNodeType::Director)
            {
                CopyNodeChildren(xmlNode, animNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent)
{
    int type, id;
    std::map<int, IAnimNode*> idToNode;
    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef xn = xmlNode->getChild(i);

        if (!xn->getAttr("Type", type))
        {
            continue;
        }

        xn->getAttr("Id", id);

        IAnimNode* node = CreateNode((AnimNodeType)type);
        if (!node)
        {
            continue;
        }

        idToNode[id] = node;

        xn->setAttr("Id", static_cast<CAnimNode*>(node)->GetId());
        node->Serialize(xn, true, true);

        int parentId = 0;
        if (xn->getAttr("ParentNode", parentId))
        {
            node->SetParent(idToNode[parentId]);
        }
        else
        // This means a top-level node.
        {
            if (pParent)
            {
                node->SetParent(pParent);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddNodeNeedToRender(IAnimNode* pNode)
{
    assert(pNode != 0);
    return stl::push_back_unique(m_nodesNeedToRender, AZStd::intrusive_ptr<IAnimNode>(pNode));
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveNodeNeedToRender(IAnimNode* pNode)
{
    assert(pNode != 0);
    stl::find_and_erase_if(m_nodesNeedToRender, [pNode](const AZStd::intrusive_ptr<IAnimNode>& sp) { return sp.get() == pNode; });
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetSequenceEntityId(const AZ::EntityId& sequenceEntityId)
{ 
    m_sequenceEntityId = sequenceEntityId;
}
//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetActiveDirector(IAnimNode* pDirectorNode)
{
    if (!pDirectorNode)
    {
        return;
    }

    assert(pDirectorNode->GetType() == AnimNodeType::Director);
    if (pDirectorNode->GetType() != AnimNodeType::Director)
    {
        return;     // It's not a director node.
    }

    if (pDirectorNode->GetSequence() != this)
    {
        return;     // It's not a node belong to this sequence.
    }

    m_activeDirector = pDirectorNode;
    m_activeDirectorNodeId = pDirectorNode->GetId();
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::GetActiveDirector() const
{
    return m_activeDirector;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::IsAncestorOf(const IAnimSequence* sequence) const
{
    assert(this != sequence);
    if (this == sequence)
    {
        return true;
    }

    if (!GetMovieSystem())
    {
        return false;   // should never happen, null pointer guard
    }

    for (const auto& it :m_nodes)
    {
        IAnimNode* pNode = it.get();
        if (pNode->GetType() == AnimNodeType::Director)
        {
            IAnimTrack* pSequenceTrack = pNode->GetTrackForParameter(AnimParamType::Sequence);
            if (pSequenceTrack)
            {
                PREFAST_ASSUME(sequence);
                for (int i = 0; i < pSequenceTrack->GetNumKeys(); ++i)
                {
                    ISequenceKey key;
                    pSequenceTrack->GetKey(i, &key);
                    if (_stricmp(key.szSelection.c_str(), sequence->GetName()) == 0)
                    {
                        return true;
                    }

                    IAnimSequence* pChild = CAnimSceneNode::GetSequenceFromSequenceKey(key);
                    if (pChild && pChild->IsAncestorOf(sequence))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


