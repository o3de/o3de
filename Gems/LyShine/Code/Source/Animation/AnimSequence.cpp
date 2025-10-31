/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of IAnimSequence interface.


#include "AnimSequence.h"

#include "StlUtils.h"
#include "EventNode.h"
#include "AzEntityNode.h"
#include "UiAnimSerialize.h"

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::CUiAnimSequence()
    : CUiAnimSequence(nullptr, 0)
{
}

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::CUiAnimSequence(IUiAnimationSystem* pUiAnimationSystem, uint32 id)
    : m_refCount(0)
{
    m_nextGenId = 1;
    m_pUiAnimationSystem = pUiAnimationSystem;
    m_flags = 0;
    m_pParentSequence = NULL;
    m_timeRange.Set(0, 10);
    m_bPaused = false;
    m_bActive = false;
    m_pOwner = NULL;
    m_pActiveDirector = NULL;
    m_precached = false;
    m_bResetting = false;
    m_id = id;
    m_time = -FLT_MAX;

    m_pEventStrings = aznew CUiAnimStringTable;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::~CUiAnimSequence()
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
void CUiAnimSequence::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetName(const char* name)
{
    if (!m_pUiAnimationSystem)
    {
        return;   // should never happen, null pointer guard
    }

    AZStd::string originalName = GetName();

    m_name = name;
    m_pUiAnimationSystem->OnSequenceRenamed(originalName.c_str(), m_name.c_str());

    if (GetOwner())
    {
        GetOwner()->OnModified();
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CUiAnimSequence::GetName() const
{
    return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetCutSceneFlags(const bool localFlags) const
{
    int currentFlags = m_flags & (eSeqFlags_NoHUD | eSeqFlags_NoPlayer | eSeqFlags_16To9 | eSeqFlags_NoGameSounds | eSeqFlags_NoAbort);

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
void CUiAnimSequence::SetParentSequence(IUiAnimSequence* pParentSequence)
{
    m_pParentSequence = pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
const IUiAnimSequence* CUiAnimSequence::GetParentSequence() const
{
    return m_pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetNodeCount() const
{
    return static_cast<int>(m_nodes.size());
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::GetNode(int index) const
{
    assert(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].get();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::AddNode(IUiAnimNode* pAnimNode)
{
    AZ_Assert(pAnimNode, "Expected valid animNode");
    if(!pAnimNode)
    {
        return false;
    }

    pAnimNode->SetSequence(this);
    pAnimNode->SetTimeRange(m_timeRange);

    // Check if this node already in sequence.
    bool found = false;
    for (int i = 0; i < (int)m_nodes.size(); i++)
    {
        if (pAnimNode == m_nodes[i].get())
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        m_nodes.push_back(AZStd::intrusive_ptr<IUiAnimNode>(pAnimNode));
    }

    const int nodeId = static_cast<CUiAnimNode*>(pAnimNode)->GetId();
    if (nodeId >= static_cast<int>(m_nextGenId))
    {
        m_nextGenId = nodeId + 1;
    }

    if (pAnimNode->NeedToRender())
    {
        AddNodeNeedToRender(pAnimNode);
    }

    bool bNewDirectorNode = m_pActiveDirector == NULL && pAnimNode->GetType() == eUiAnimNodeType_Director;
    if (bNewDirectorNode)
    {
        m_pActiveDirector = pAnimNode;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNodeInternal(EUiAnimNodeType nodeType, uint32 nNodeId)
{
    CUiAnimNode* pAnimNode = NULL;

    if (nNodeId == -1)
    {
        nNodeId = m_nextGenId;
    }

    switch (nodeType)
    {
        case eUiAnimNodeType_AzEntity:
            pAnimNode = aznew CUiAnimAzEntityNode(nNodeId);
            break;
        case eUiAnimNodeType_Event:
            pAnimNode = aznew CUiAnimEventNode(nNodeId);
            break;
    }

    if (pAnimNode)
    {
        AddNode(pAnimNode);
    }

    return pAnimNode;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNode(EUiAnimNodeType nodeType)
{
    return CreateNodeInternal(nodeType);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNode(XmlNodeRef node)
{
    if (!GetUiAnimationSystem())
    {
        return 0;   // should never happen, null pointer guard
    }

    EUiAnimNodeType type;
    static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(type, node, true, IUiAnimSequence::kSequenceVersion, 0);

    XmlString name;
    if (!node->getAttr("Name", name))
    {
        return 0;
    }

    IUiAnimNode* pNewNode = CreateNode(type);
    if (!pNewNode)
    {
        return 0;
    }

    pNewNode->SetName(name);
    pNewNode->Serialize(node, true, true);

    CUiAnimNode* newAnimNode = static_cast<CUiAnimNode*>(pNewNode);

    // Make sure de-serializing this node didn't just create an id conflict. This can happen sometimes
    // when copy/pasting nodes from a different sequence to this one.
    for (auto curNode : m_nodes)
    {
        CUiAnimNode* animNode = static_cast<CUiAnimNode*>(curNode.get());
        if (animNode->GetId() == newAnimNode->GetId() && animNode != newAnimNode)
        {
            // Conflict detected, resolve it by assigning a new id to the new node.
            newAnimNode->SetId(m_nextGenId++);
        }
    }

    return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
// Only called from undo/redo
void CUiAnimSequence::RemoveNode(IUiAnimNode* node)
{
    assert(node != 0);

    static_cast<CUiAnimNode*>(node)->Activate(false);
    static_cast<CUiAnimNode*>(node)->OnReset();

    for (int i = 0; i < (int)m_nodes.size(); )
    {
        if (node == m_nodes[i].get())
        {
            // TODO : Consider moving node destruction after usage below which might fix the probable problem here
            // (ref similar PR #18538 fix for GHI #11500). Needs more investigation and testing.
            m_nodes.erase(m_nodes.begin() + i);

            if (node->NeedToRender())
            {
                RemoveNodeNeedToRender(node);
            }

            continue;
        }
        if (m_nodes[i]->GetParent() == node)
        {
            m_nodes[i]->SetParent(0);
        }

        i++;
    }

    // The removed one was the active director node.
    if (m_pActiveDirector == node)
    {
        // Clear the active one.
        m_pActiveDirector = NULL;

        // If there is another director node, set it as active.
        for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IUiAnimNode* pNode = it->get();
            if (pNode->GetType() == eUiAnimNodeType_Director)
            {
                SetActiveDirector(pNode);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::RemoveAll()
{
    stl::free_container(m_nodes);
    stl::free_container(m_events);
    stl::free_container(m_nodesNeedToRender);
    m_pActiveDirector = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Reset(bool bSeekToStart)
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    m_precached = false;
    m_bResetting = true;

    if (!bSeekToStart)
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnReset();
        }
        m_bResetting = false;
        return;
    }

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SUiAnimContext ec;
    ec.bSingleFrame = true;
    ec.bResetting = true;
    ec.pSequence = this;
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnReset();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::ResetHard()
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

    SUiAnimContext ec;
    ec.bSingleFrame = true;
    ec.bResetting = true;
    ec.pSequence = this;
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnResetHard();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Pause()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet || m_bPaused)
    {
        return;
    }

    m_bPaused = true;

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnPause();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Resume()
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnResume();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnLoop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnLoop();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::IsPaused() const
{
    return m_bPaused;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnStart()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnStart();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnStop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnStop();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::StillUpdate()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        pAnimNode->StillUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Animate(const SUiAnimContext& ec)
{
    assert(m_bActive);

    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    SUiAnimContext animContext = ec;
    animContext.pSequence = this;
    m_time = animContext.time;

    // Evaluate all animation nodes in sequence.
    // The director first.
    if (m_pActiveDirector)
    {
        m_pActiveDirector->Animate(animContext);
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        // Make sure correct animation block is binded to node.
        IUiAnimNode* pAnimNode = it->get();

        // All other (inactive) director nodes are skipped.
        if (pAnimNode->GetType() == eUiAnimNodeType_Director)
        {
            continue;
        }

        // If this is a descendant of a director node and that director is currently not active, skip this one.
        IUiAnimNode* pParentDirector = pAnimNode->HasDirectorAsParent();
        if (pParentDirector && pParentDirector != m_pActiveDirector)
        {
            continue;
        }

        if (pAnimNode->GetFlags() & eUiAnimNodeFlags_Disabled)
        {
            continue;
        }

        // Animate node.
        pAnimNode->Animate(animContext);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Render()
{
    for (AnimNodes::iterator it = m_nodesNeedToRender.begin(); it != m_nodesNeedToRender.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        pAnimNode->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Activate()
{
    if (m_bActive)
    {
        return;
    }

    m_bActive = true;
    // Assign animation block to all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        CUiAnimNode* pAnimNode = static_cast<CUiAnimNode*>(it->get());
        pAnimNode->OnReset();
        pAnimNode->Activate(true);
    }
}

typedef AZStd::fixed_string<512> stack_string;

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Deactivate()
{
    if (!m_bActive)
    {
        return;
    }

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        CUiAnimNode* pAnimNode = static_cast<CUiAnimNode*>(it->get());
        pAnimNode->Activate(false);
        pAnimNode->OnReset();
    }

    // Remove a possibly cached game hint associated with this anim sequence.
    stack_string sTemp("anim_sequence_");
    sTemp += m_name.c_str();
    // Audio: Release precached sound

    m_bActive = false;
    m_precached = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheData(float startTime)
{
    PrecacheStatic(startTime);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheStatic(const float startTime)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->PrecacheStatic(startTime);
    }

    PrecacheDynamic(startTime);

    if (m_precached)
    {
        return;
    }

    // Try to cache this sequence's game hint if one exists.
    stack_string sTemp("anim_sequence_");
    sTemp += m_name.c_str();


    gEnv->pLog->Log("=== Precaching render data for Ui animation: %s ===", GetName());

    m_precached = true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheDynamic(float time)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->PrecacheDynamic(time);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CUiAnimSequence>()
        ->Version(2)
        ->Field("Name", &CUiAnimSequence::m_name)
        ->Field("Flags", &CUiAnimSequence::m_flags)
        ->Field("TimeRange", &CUiAnimSequence::m_timeRange)
        ->Field("ID", &CUiAnimSequence::m_id)
        ->Field("Nodes", &CUiAnimSequence::m_nodes)
        ->Field("Events", &CUiAnimSequence::m_events);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks, uint32 overrideId, [[maybe_unused]] bool bResetLightAnimSet)
{
    if (bLoading)
    {
        // Load.
        RemoveAll();

        int sequenceVersion = 0;
        xmlNode->getAttr("SequenceVersion", sequenceVersion);

        Range timeRange;
        m_name = xmlNode->getAttr("Name");
        xmlNode->getAttr("Flags", m_flags);
        xmlNode->getAttr("StartTime", timeRange.start);
        xmlNode->getAttr("EndTime", timeRange.end);
        xmlNode->getAttr("ID", m_id);

        if (overrideId != 0)
        {
            m_id = overrideId;
        }

        INDENT_LOG_DURING_SCOPE(true, "Loading sequence '%s' (start time = %.2f, end time = %.2f) %s ID #%u",
            m_name.c_str(), timeRange.start, timeRange.end, overrideId ? "override" : "default", m_id);

        // Loading.
        XmlNodeRef nodes = xmlNode->findChild("Nodes");
        if (nodes)
        {
            uint32 id;
            EUiAnimNodeType nodeType;
            for (int i = 0; i < nodes->getChildCount(); ++i)
            {
                XmlNodeRef childNode = nodes->getChild(i);
                childNode->getAttr("Id", id);

                static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(nodeType, childNode, bLoading, sequenceVersion, m_flags);

                if (nodeType == eUiAnimNodeType_Invalid)
                {
                    continue;
                }

                IUiAnimNode* pAnimNode = CreateNodeInternal((EUiAnimNodeType)nodeType, id);
                if (!pAnimNode)
                {
                    continue;
                }

                pAnimNode->Serialize(childNode, bLoading, bLoadEmptyTracks);
            }

            // When all nodes loaded restore group hierarchy
            for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
            {
                CUiAnimNode* pAnimNode = static_cast<CUiAnimNode*>((*it).get());
                pAnimNode->PostLoad();

                // And properly adjust the 'm_lastGenId' to prevent the id clash.
                if (pAnimNode->GetId() >= (int)m_nextGenId)
                {
                    m_nextGenId = pAnimNode->GetId() + 1;
                }
            }
        }
        // Setting the time range must be done after the loading of all nodes
        // since it sets the time range of tracks, also.
        SetTimeRange(timeRange);
        Deactivate();
        //ComputeTimeRange();

        if (GetOwner())
        {
            GetOwner()->OnModified();
        }
    }
    else
    {
        xmlNode->setAttr("SequenceVersion", IUiAnimSequence::kSequenceVersion);

        const char* fullname = GetName();
        xmlNode->setAttr("Name", fullname);     // Save the full path as a name.
        xmlNode->setAttr("Flags", m_flags);
        xmlNode->setAttr("StartTime", m_timeRange.start);
        xmlNode->setAttr("EndTime", m_timeRange.end);
        xmlNode->setAttr("ID", m_id);

        // Save.
        XmlNodeRef nodes = xmlNode->newChild("Nodes");
        int num = GetNodeCount();
        for (int i = 0; i < num; i++)
        {
            IUiAnimNode* pAnimNode = GetNode(i);
            if (pAnimNode)
            {
                XmlNodeRef xn = nodes->newChild("Node");
                pAnimNode->Serialize(xn, bLoading, true);
            }
        }
    }

}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::InitPostLoad(IUiAnimationSystem* pUiAnimationSystem, bool remapIds, LyShine::EntityIdMap* entityIdMap)
{
    m_pUiAnimationSystem = pUiAnimationSystem;

    int nodeCount = GetNodeCount();
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        IUiAnimNode* animNode = GetNode(nodeIndex);
        if (animNode)
        {
            animNode->InitPostLoad(this, remapIds, entityIdMap);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetTimeRange(Range timeRange)
{
    m_timeRange = timeRange;
    // Set this time range for every track in animation.
    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* anode = it->get();
        anode->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::AdjustKeysToTimeRange(const Range& timeRange)
{
    float offset = timeRange.start - m_timeRange.start;
    // Calculate scale ratio.
    float scale = timeRange.Length() / m_timeRange.Length();
    m_timeRange = timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IUiAnimTrack* track = pAnimNode->GetTrackByIndex(paramIndex);
            int nkey = track->GetNumKeys();
            for (int k = 0; k < nkey; k++)
            {
                float keytime = track->GetKeyTime(k);
                keytime = offset + keytime * scale;
                track->SetKeyTime(k, keytime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::ComputeTimeRange()
{
    Range timeRange = m_timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IUiAnimTrack* track = pAnimNode->GetTrackByIndex(paramIndex);
            int nkey = track->GetNumKeys();
            if (nkey > 0)
            {
                timeRange.start = std::min(timeRange.start, track->GetKeyTime(0));
                timeRange.end = std::max(timeRange.end, track->GetKeyTime(nkey - 1));
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
bool CUiAnimSequence::AddTrackEvent(const char* szEvent)
{
    AZ_Assert(szEvent && szEvent[0], "Track Event is nullptr.");
    if (stl::push_back_unique(m_events, szEvent))
    {
        NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_Added, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::RemoveTrackEvent(const char* szEvent)
{
    AZ_Assert(szEvent && szEvent[0], "Track Event is nullptr.");
    if (stl::find_and_erase(m_events, szEvent))
    {
        NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_Removed, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::RenameTrackEvent(const char* szEvent, const char* szNewEvent)
{
    AZ_Assert(szEvent && szEvent[0], "Track Event is nullptr.");
    AZ_Assert(szNewEvent && szNewEvent[0], "New Track Event is nullptr.");

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            m_events[i] = szNewEvent;
            NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_Renamed, szEvent, szNewEvent);
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::MoveUpTrackEvent(const char* szEvent)
{
    AZ_Assert(szEvent && szEvent[0], "Track Event is nullptr.");

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            AZ_Assert(i > 0, "Event already first in Track.");
            if (i > 0)
            {
                std::swap(m_events[i - 1], m_events[i]);
                NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_MovedUp, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::MoveDownTrackEvent(const char* szEvent)
{
    AZ_Assert(szEvent && szEvent[0], "Track Event is nullptr.");

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            AZ_Assert(i < m_events.size() - 1, "Event already last in Track.");
            if (i < m_events.size() - 1)
            {
                std::swap(m_events[i], m_events[i + 1]);
                NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_MovedDown, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::ClearTrackEvents()
{
    m_events.clear();
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetTrackEventsCount() const
{
    return static_cast<int>(m_events.size());
}

//////////////////////////////////////////////////////////////////////////
char const* CUiAnimSequence::GetTrackEvent(int iIndex) const
{
    char const* szResult = NULL;
    const bool bValid = (iIndex >= 0 && iIndex < GetTrackEventsCount());
    AZ_Assert(bValid, "Track Event index out of range.");

    if (bValid)
    {
        szResult = m_events[iIndex].c_str();
    }

    return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::NotifyTrackEvent(IUiTrackEventListener::ETrackEventReason reason,
    const char* event, const char* param)
{
    // Notify listeners
    for (TUiTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
    {
        (*j)->OnTrackEvent(this, reason, event, (void*)param);
    }

    // Pass to Animation System to notify via EBus
    GetUiAnimationSystem()->NotifyTrackEventListeners(event, param, this);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::TriggerTrackEvent(const char* event, const char* param)
{
    NotifyTrackEvent(IUiTrackEventListener::eTrackEventReason_Triggered, event, param);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::AddTrackEventListener(IUiTrackEventListener* pListener)
{
    if (AZStd::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
    {
        m_listeners.push_back(pListener);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::RemoveTrackEventListener(IUiTrackEventListener* pListener)
{
    TUiTrackEventListeners::iterator it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);
    if (it != m_listeners.end())
    {
        m_listeners.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::FindNodeById(int nNodeId)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        if (static_cast<CUiAnimNode*>(pAnimNode)->GetId() == nNodeId)
        {
            return pAnimNode;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::FindNodeByName(const char* sNodeName, const IUiAnimNode* pParentDirector)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        // Case insensitive name comparison.
        if (_stricmp(((CUiAnimNode*)pAnimNode)->GetNameFast(), sNodeName) == 0)
        {
            bool bParentDirectorCheck = pAnimNode->HasDirectorAsParent() == pParentDirector;
            if (bParentDirectorCheck)
            {
                return pAnimNode;
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::ReorderNode(IUiAnimNode* pNode, IUiAnimNode* pPivotNode, bool bNext)
{
    if (pNode == pPivotNode || !pNode)
    {
        return;
    }

    AZStd::intrusive_ptr<IUiAnimNode> pTempHolder(pNode); // Keep reference to node so it is not deleted by erasing from list.
    stl::find_and_erase_if(m_nodes, [pNode](const AZStd::intrusive_ptr<IUiAnimNode>& sp) { return sp.get() == pNode; });


    AnimNodes::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        if (pAnimNode == pPivotNode)
        {
            if (bNext)
            {
                m_nodes.insert(it + 1, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
            }
            else
            {
                m_nodes.insert(it, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
            }
            break;
        }
    }

    if (it == m_nodes.end())
    {
        m_nodes.insert(m_nodes.begin(), AZStd::intrusive_ptr<IUiAnimNode>(pNode));
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::CopyNodeChildren(XmlNodeRef& xmlNode, IUiAnimNode* pAnimNode)
{
    for (int k = 0; k < GetNodeCount(); ++k)
    {
        if (GetNode(k)->GetParent() == pAnimNode)
        {
            XmlNodeRef childNode = xmlNode->newChild("Node");
            GetNode(k)->Serialize(childNode, false, true);
            if (GetNode(k)->GetType() == eUiAnimNodeType_Group
                || pAnimNode->GetType() == eUiAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, GetNode(k));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::CopyNodes(XmlNodeRef& xmlNode, IUiAnimNode** pSelectedNodes, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        IUiAnimNode* pAnimNode = pSelectedNodes[i];
        if (pAnimNode)
        {
            XmlNodeRef xn = xmlNode->newChild("Node");
            pAnimNode->Serialize(xn, false, true);
            // If it is a group node, copy its children also.
            if (pAnimNode->GetType() == eUiAnimNodeType_Group   || pAnimNode->GetType() == eUiAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, pAnimNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PasteNodes(const XmlNodeRef& xmlNode, IUiAnimNode* pParent)
{
    int type, id;
    std::map<int, IUiAnimNode*> idToNode;
    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef xn = xmlNode->getChild(i);

        if (!xn->getAttr("Type", type))
        {
            continue;
        }

        xn->getAttr("Id", id);

        IUiAnimNode* node = CreateNode((EUiAnimNodeType)type);
        if (!node)
        {
            continue;
        }

        idToNode[id] = node;

        xn->setAttr("Id", static_cast<CUiAnimNode*>(node)->GetId());
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
bool CUiAnimSequence::AddNodeNeedToRender(IUiAnimNode* pNode)
{
    assert(pNode != 0);
    return stl::push_back_unique(m_nodesNeedToRender, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::RemoveNodeNeedToRender(IUiAnimNode* pNode)
{
    assert(pNode != 0);
    stl::find_and_erase_if(m_nodesNeedToRender, [pNode](const AZStd::intrusive_ptr<IUiAnimNode>& sp) { return sp.get() == pNode; });
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetActiveDirector(IUiAnimNode* pDirectorNode)
{
    if (!pDirectorNode)
    {
        return;
    }

    AZ_Assert(pDirectorNode->GetType() == eUiAnimNodeType_Director, "New Director Node is not Director Type.");
    if (pDirectorNode->GetType() != eUiAnimNodeType_Director)
    {
        return;     // It's not a director node.
    }

    if (pDirectorNode->GetSequence() != this)
    {
        return;     // It's not a node belong to this sequence.
    }

    m_pActiveDirector = pDirectorNode;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::GetActiveDirector() const
{
    return m_pActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::IsAncestorOf(const IUiAnimSequence* pSequence) const
{
    AZ_Assert(this != pSequence, "Checked if UiAnimSequence was ancestor of itself.");
    if (this == pSequence)
    {
        return true;
    }

    // UI_ANIMATION_REVISIT: was only doing anything for sequence tracks

    return false;
}
