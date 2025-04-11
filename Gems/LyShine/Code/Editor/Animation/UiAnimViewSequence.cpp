/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiEditorAnimationBus.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewSequenceManager.h"
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewNodeFactories.h"
#include "UiAnimViewSequence.h"
#include "AnimationContext.h"
#include "Clipboard.h"

#include "UiEditorAnimationBus.h"
#include <Util/EditorUtils.h>

#include <QApplication>

//////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence::CUiAnimViewSequence(IUiAnimSequence* pSequence)
    : CUiAnimViewAnimNode(pSequence, nullptr, nullptr)
    , m_pAnimSequence(pSequence)
    , m_bBoundToEditorObjects(false)
    , m_selectionRecursionLevel(0)
    , m_bQueueNotifications(false)
    , m_bKeySelectionChanged(false)
    , m_bKeysChanged(false)
    , m_bForceUiAnimation(false)
    , m_bNodeSelectionChanged(false)
    , m_time(0.0f)
    , m_bNoNotifications(false)
{
    assert(m_pAnimSequence);
    SetExpanded(true);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence::~CUiAnimViewSequence()
{
    // For safety. Should be done by sequence manager
    UiAnimUndoManager::Get()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::Load()
{
    const int nodeCount = m_pAnimSequence->GetNodeCount();
    for (int i = 0; i < nodeCount; ++i)
    {
        IUiAnimNode* pNode = m_pAnimSequence->GetNode(i);

        // Only add top level nodes to sequence
        if (!pNode->GetParent())
        {
            CUiAnimViewAnimNodeFactory animNodeFactory;
            CUiAnimViewAnimNode* pNewTVAnimNode = animNodeFactory.BuildAnimNode(m_pAnimSequence.get(), pNode, this);
            m_childNodes.push_back(std::unique_ptr<CUiAnimViewNode>(pNewTVAnimNode));
        }
    }

    SortNodes();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::BindToEditorObjects()
{
    m_bBoundToEditorObjects = true;
    CUiAnimViewAnimNode::BindToEditorObjects();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::UnBindFromEditorObjects()
{
    m_bBoundToEditorObjects = false;
    CUiAnimViewAnimNode::UnBindFromEditorObjects();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewSequence::IsBoundToEditorObjects() const
{
    return m_bBoundToEditorObjects;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewSequence::FindSingleSelectedKey()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return CUiAnimViewKeyHandle();
    }

    CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    if (selectedKeys.GetKeyCount() != 1)
    {
        return CUiAnimViewKeyHandle();
    }

    return selectedKeys.GetKey(0);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewSequence::IsAncestorOf(CUiAnimViewSequence* pSequence) const
{
    return m_pAnimSequence->IsAncestorOf(pSequence->m_pAnimSequence.get());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::BeginCutScene([[maybe_unused]] const bool bResetFx) const
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::EndCutScene() const
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::Render(const SUiAnimContext& animContext)
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = (CUiAnimViewAnimNode*)pChildNode;
            pChildAnimNode->Render(animContext);
        }
    }

    m_pAnimSequence->Render();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::Animate(const SUiAnimContext& animContext)
{
    if (!m_pAnimSequence->IsActivated())
    {
        return;
    }

    m_time = animContext.time;

    m_pAnimSequence->Animate(animContext);

    CUiAnimViewSequenceNoNotificationContext context(this);
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pChildAnimNode = (CUiAnimViewAnimNode*)pChildNode;
            pChildAnimNode->Animate(animContext);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::AddListener(IUiAnimViewSequenceListener* pListener)
{
    stl::push_back_unique(m_sequenceListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::RemoveListener(IUiAnimViewSequenceListener* pListener)
{
    stl::find_and_erase(m_sequenceListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnNodeSelectionChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bNodeSelectionChanged = true;
    }
    else
    {
        CUiAnimViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnNodeSelectionChanged(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::ForceAnimation()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bForceUiAnimation = true;
    }
    else
    {
        if (IsActive())
        {
            CUiAnimationContext* pAnimationContext = nullptr;
            UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
            pAnimationContext->ForceAnimation();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnKeySelectionChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bKeySelectionChanged = true;
    }
    else
    {
        CUiAnimViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnKeySelectionChanged(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnKeysChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    if (m_bQueueNotifications)
    {
        m_bKeysChanged = true;
    }
    else
    {
        CUiAnimViewSequenceNoNotificationContext context(this);
        for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
        {
            (*iter)->OnKeysChanged(this);
        }

        if (IsActive())
        {
            CUiAnimationContext* pAnimationContext = nullptr;
            UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
            pAnimationContext->ForceAnimation();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnNodeChanged(CUiAnimViewNode* pNode, IUiAnimViewSequenceListener::ENodeChangeType type)
{
    if (m_bNoNotifications)
    {
        return;
    }

    CUiAnimViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnNodeChanged(pNode, type);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnNodeRenamed(CUiAnimViewNode* pNode, const char* pOldName)
{
    if (m_bNoNotifications)
    {
        return;
    }

    CUiAnimViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnNodeRenamed(pNode, pOldName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OnSequenceSettingsChanged()
{
    if (m_bNoNotifications)
    {
        return;
    }

    CUiAnimViewSequenceNoNotificationContext context(this);
    for (auto iter = m_sequenceListeners.begin(); iter != m_sequenceListeners.end(); ++iter)
    {
        (*iter)->OnSequenceSettingsChanged(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::QueueNotifications()
{
    m_bQueueNotifications = true;
    ++m_selectionRecursionLevel;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::SubmitPendingNotifcations()
{
    assert(m_selectionRecursionLevel > 0);
    if (m_selectionRecursionLevel > 0)
    {
        --m_selectionRecursionLevel;
    }

    if (m_selectionRecursionLevel == 0)
    {
        m_bQueueNotifications = false;

        if (m_bNodeSelectionChanged)
        {
            OnNodeSelectionChanged();
        }

        if (m_bKeysChanged)
        {
            OnKeysChanged();
        }

        if (m_bKeySelectionChanged)
        {
            OnKeySelectionChanged();
        }

        if (m_bForceUiAnimation)
        {
            ForceAnimation();
        }

        m_bForceUiAnimation = false;
        m_bKeysChanged = false;
        m_bNodeSelectionChanged = false;
        m_bKeySelectionChanged = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::DeleteSelectedNodes()
{
    assert(UiAnimUndo::IsRecording());

    CUiAnimViewSequenceNotificationContext context(this);

    if (IsSelected())
    {
        CUiAnimViewSequenceManager::GetSequenceManager()->DeleteSequence(this);
        return;
    }

    CUiAnimViewAnimNodeBundle selectedNodes = GetSelectedAnimNodes();
    CUiAnimViewTrackBundle selectedTracks = GetSelectedTracks();

    for (unsigned int i = 0; i < selectedTracks.GetCount(); ++i)
    {
        CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);

        // Ignore sub tracks
        if (!pTrack->IsSubTrack())
        {
            pTrack->GetAnimNode()->RemoveTrack(pTrack);
        }
    }

    for (unsigned int i = 0; i < selectedNodes.GetCount(); ++i)
    {
        CUiAnimViewAnimNode* pNode = selectedNodes.GetNode(i);
        CUiAnimViewAnimNode* pParentNode = static_cast<CUiAnimViewAnimNode*>(pNode->GetParentNode());
        pParentNode->RemoveSubNode(pNode);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewSequence::SetName(const char* pName)
{
    // Check if there is already a sequence with that name
    const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
    if (pSequenceManager->GetSequenceByName(pName))
    {
        return false;
    }

    AZStd::string oldName = GetName();
    m_pAnimSequence->SetName(pName);

    if (UiAnimUndo::IsRecording())
    {
        UiAnimUndo::Record(new CUndoAnimNodeRename(this, oldName));
    }

    GetSequence()->OnNodeRenamed(this, oldName.c_str());

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::DeleteSelectedKeys()
{
    assert(UiAnimUndo::IsRecording());

    StoreUndoForTracksWithSelectedKeys();

    CUiAnimViewSequenceNotificationContext context(this);
    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();
    for (int k = (int)selectedKeys.GetKeyCount() - 1; k >= 0; --k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        skey.Delete();
    }

    // The selected keys are deleted, so notify the selection was just changed.
    OnKeySelectionChanged();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::StoreUndoForTracksWithSelectedKeys()
{
    assert(UiAnimUndo::IsRecording());

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    // Construct the set of tracks that have selected keys
    std::set<CUiAnimViewTrack*> tracks;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        tracks.insert(skey.GetTrack());
    }

    // Store one key selection undo before...
    UiAnimUndo::Record(new CUndoAnimKeySelection(this));

    // For each of those tracks store an undo object
    for (auto iter = tracks.begin(); iter != tracks.end(); ++iter)
    {
        UiAnimUndo::Record(new CUndoTrackObject(*iter, false));
    }

    // ... and one after key changes
    UiAnimUndo::Record(new CUndoAnimKeySelection(this));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::CopyKeysToClipboard(const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    XmlNodeRef copyNode = XmlHelpers::CreateXmlNode("CopyKeysNode");
    CopyKeysToClipboard(copyNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);

    CClipboard clip(nullptr);
    clip.Put(copyNode, "Track view keys");
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CUiAnimViewNode* pChildNode = (*iter).get();
        pChildNode->CopyKeysToClipboard(xmlNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::PasteKeysFromClipboard(CUiAnimViewAnimNode* pTargetNode, CUiAnimViewTrack* pTargetTrack, const float timeOffset)
{
    assert(UiAnimUndo::IsRecording());

    CClipboard clipboard(nullptr);
    XmlNodeRef clipboardContent = clipboard.Get();
    if (clipboardContent)
    {
        std::vector<TMatchedTrackLocation> matchedLocations = GetMatchedPasteLocations(clipboardContent, pTargetNode, pTargetTrack);

        for (auto iter = matchedLocations.begin(); iter != matchedLocations.end(); ++iter)
        {
            const TMatchedTrackLocation& location = *iter;
            CUiAnimViewTrack* pTrack = location.first;
            const XmlNodeRef& trackNode = location.second;
            pTrack->PasteKeys(trackNode, timeOffset);
        }

        OnKeysChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
std::vector<CUiAnimViewSequence::TMatchedTrackLocation>
CUiAnimViewSequence::GetMatchedPasteLocations(XmlNodeRef clipboardContent, CUiAnimViewAnimNode* pTargetNode, CUiAnimViewTrack* pTargetTrack)
{
    std::vector<TMatchedTrackLocation> matchedLocations;

    bool bPastingSingleNode = false;
    XmlNodeRef singleNode;
    bool bPastingSingleTrack = false;
    XmlNodeRef singleTrack;

    // Check if the XML tree only contains one node and if so if that node only contains one track
    for (XmlNodeRef currentNode = clipboardContent; currentNode->getChildCount() > 0; currentNode = currentNode->getChild(0))
    {
        bool bAllChildsAreTracks = true;
        const unsigned int numChilds = currentNode->getChildCount();
        for (unsigned int i = 0; i < numChilds; ++i)
        {
            XmlNodeRef childNode = currentNode->getChild(i);
            if (strcmp(currentNode->getChild(0)->getTag(), "Track") != 0)
            {
                bAllChildsAreTracks = false;
                break;
            }
        }

        if (bAllChildsAreTracks)
        {
            bPastingSingleNode = true;
            singleNode = currentNode;

            if (currentNode->getChildCount() == 1)
            {
                bPastingSingleTrack = true;
                singleTrack = currentNode->getChild(0);
            }
        }
        else if (currentNode->getChildCount() != 1)
        {
            break;
        }
    }

    if (bPastingSingleTrack && pTargetNode && pTargetTrack)
    {
        // We have a target node & track, so try to match the value type
        unsigned int valueType = 0;
        if (singleTrack->getAttr("valueType", valueType))
        {
            if (pTargetTrack->GetValueType() == valueType)
            {
                matchedLocations.push_back(TMatchedTrackLocation(pTargetTrack, singleTrack));
                return matchedLocations;
            }
        }
    }

    if (bPastingSingleNode && pTargetNode)
    {
        // Set of tracks that were already matched
        std::vector<CUiAnimViewTrack*> matchedTracks;

        // We have a single node to paste and have been given a target node
        // so try to match the tracks by param type
        const unsigned int numTracks = singleNode->getChildCount();
        for (unsigned int i = 0; i < numTracks; ++i)
        {
            XmlNodeRef trackNode = singleNode->getChild(i);

            // Try to match the track
            auto matchingTracks = GetMatchingTracks(pTargetNode, trackNode);
            for (auto iter = matchingTracks.begin(); iter != matchingTracks.end(); ++iter)
            {
                CUiAnimViewTrack* pMatchedTrack = *iter;
                // Pick the first track that was matched *and* was not already matched
                if (!stl::find(matchedTracks, pMatchedTrack))
                {
                    stl::push_back_unique(matchedTracks, pMatchedTrack);
                    matchedLocations.push_back(TMatchedTrackLocation(pMatchedTrack, trackNode));
                    break;
                }
            }
        }

        // Return if matching succeeded
        if (matchedLocations.size() > 0)
        {
            return matchedLocations;
        }
    }

    if (!bPastingSingleNode)
    {
        // Ok, we're pasting keys from multiple nodes, haven't been given any target
        // or matching the targets failed. Ignore given target pointers and start
        // a recursive match at the sequence root.
        GetMatchedPasteLocationsRec(matchedLocations, this, clipboardContent);
    }

    return matchedLocations;
}

//////////////////////////////////////////////////////////////////////////
std::deque<CUiAnimViewTrack*> CUiAnimViewSequence::GetMatchingTracks(CUiAnimViewAnimNode* pAnimNode, XmlNodeRef trackNode)
{
    std::deque<CUiAnimViewTrack*> matchingTracks;

    const AZStd::string trackName = trackNode->getAttr("name");

    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    CUiAnimParamType animParamType;
    animParamType.Serialize(animationSystem, trackNode, true);

    unsigned int valueType;
    if (!trackNode->getAttr("valueType", valueType))
    {
        return matchingTracks;
    }

    CUiAnimViewTrackBundle tracks = pAnimNode->GetTracksByParam(animParamType);
    const unsigned int trackCount = tracks.GetCount();

    if (trackCount > 0)
    {
        // Search for a track with the given name and value type
        for (unsigned int i = 0; i < trackCount; ++i)
        {
            CUiAnimViewTrack* pTrack = tracks.GetTrack(i);

            if (pTrack->GetValueType() == valueType)
            {
                if (pTrack->GetName() == trackName)
                {
                    matchingTracks.push_back(pTrack);
                }
            }
        }

        // Then with lower precedence add the tracks that only match the value
        for (unsigned int i = 0; i < trackCount; ++i)
        {
            CUiAnimViewTrack* pTrack = tracks.GetTrack(i);

            if (pTrack->GetValueType() == valueType)
            {
                stl::push_back_unique(matchingTracks, pTrack);
            }
        }
    }

    return matchingTracks;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::GetMatchedPasteLocationsRec(std::vector<TMatchedTrackLocation>& locations, CUiAnimViewNode* pCurrentNode, XmlNodeRef clipboardNode)
{
    if (pCurrentNode->GetNodeType() == eUiAVNT_Sequence)
    {
        if (strcmp(clipboardNode->getTag(), "CopyKeysNode") != 0)
        {
            return;
        }
    }

    const unsigned int numChildNodes = clipboardNode->getChildCount();
    for (unsigned int nodeIndex = 0; nodeIndex < numChildNodes; ++nodeIndex)
    {
        XmlNodeRef xmlChildNode = clipboardNode->getChild(nodeIndex);
        const AZStd::string tagName = xmlChildNode->getTag();

        if (tagName == "Node")
        {
            const AZStd::string nodeName = xmlChildNode->getAttr("name");

            int nodeType = eUiAnimNodeType_Invalid;
            xmlChildNode->getAttr("type", nodeType);

            const unsigned int childCount = pCurrentNode->GetChildCount();
            for (unsigned int i = 0; i < childCount; ++i)
            {
                CUiAnimViewNode* pChildNode = pCurrentNode->GetChild(i);

                if (pChildNode->GetNodeType() == eUiAVNT_AnimNode)
                {
                    CUiAnimViewAnimNode* pAnimNode = static_cast<CUiAnimViewAnimNode*>(pChildNode);
                    if (pAnimNode->GetName() == nodeName && pAnimNode->GetType() == nodeType)
                    {
                        GetMatchedPasteLocationsRec(locations, pChildNode, xmlChildNode);
                    }
                }
            }
        }
        else if (tagName == "Track")
        {
            const AZStd::string trackName = xmlChildNode->getAttr("name");

            IUiAnimationSystem* animationSystem = nullptr;
            UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

            CUiAnimParamType trackParamType;
            trackParamType.Serialize(animationSystem, xmlChildNode, true);

            int trackParamValue = eUiAnimValue_Unknown;
            xmlChildNode->getAttr("valueType", trackParamValue);

            const unsigned int childCount = pCurrentNode->GetChildCount();
            for (unsigned int i = 0; i < childCount; ++i)
            {
                CUiAnimViewNode* pNode = pCurrentNode->GetChild(i);

                if (pNode->GetNodeType() == eUiAVNT_Track)
                {
                    CUiAnimViewTrack* pTrack = static_cast<CUiAnimViewTrack*>(pNode);
                    if (pTrack->GetName() == trackName && pTrack->GetParameterType() == trackParamType)
                    {
                        locations.push_back(TMatchedTrackLocation(pTrack, xmlChildNode));
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::AdjustKeysToTimeRange(Range newTimeRange)
{
    assert (UiAnimUndo::IsRecording());

    // Store one key selection undo before...
    UiAnimUndo::Record(new CUndoAnimKeySelection(this));

    // Store key undo for each track
    CUiAnimViewTrackBundle tracks = GetAllTracks();
    const unsigned int numTracks = tracks.GetCount();
    for (unsigned int i = 0; i < numTracks; ++i)
    {
        CUiAnimViewTrack* pTrack = tracks.GetTrack(i);
        UiAnimUndo::Record(new CUndoTrackObject(pTrack, false));
    }

    // ... and one after key changes
    UiAnimUndo::Record(new CUndoAnimKeySelection(this));

    // Set new time range
    Range oldTimeRange = GetTimeRange();
    float offset = newTimeRange.start - oldTimeRange.start;
    // Calculate scale ratio.
    float scale = newTimeRange.Length() / oldTimeRange.Length();
    SetTimeRange(newTimeRange);

    CUiAnimViewKeyBundle keyBundle = GetAllKeys();
    const unsigned int numKeys = keyBundle.GetKeyCount();

    for (unsigned int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = keyBundle.GetKey(i);
        keyHandle.SetTime(offset + keyHandle.GetTime() * scale);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::SetTimeRange(Range timeRange)
{
    if (UiAnimUndo::IsRecording())
    {
        // Store old sequence settings
        UiAnimUndo::Record(new CUndoSequenceSettings(this));
    }

    m_pAnimSequence->SetTimeRange(timeRange);
    OnSequenceSettingsChanged();
}

//////////////////////////////////////////////////////////////////////////
Range CUiAnimViewSequence::GetTimeRange() const
{
    return m_pAnimSequence->GetTimeRange();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::SetFlags(IUiAnimSequence::EUiAnimSequenceFlags flags)
{
    if (UiAnimUndo::IsRecording())
    {
        // Store old sequence settings
        UiAnimUndo::Record(new CUndoSequenceSettings(this));
    }

    m_pAnimSequence->SetFlags(flags);
    OnSequenceSettingsChanged();
}

//////////////////////////////////////////////////////////////////////////
IUiAnimSequence::EUiAnimSequenceFlags CUiAnimViewSequence::GetFlags() const
{
    return (IUiAnimSequence::EUiAnimSequenceFlags)m_pAnimSequence->GetFlags();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::DeselectAllKeys()
{
    assert(UiAnimUndo::IsRecording());
    CUiAnimViewSequenceNotificationContext context(this);

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();
    for (unsigned int i = 0; i < selectedKeys.GetKeyCount(); ++i)
    {
        CUiAnimViewKeyHandle keyHandle = selectedKeys.GetKey(i);
        keyHandle.Select(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::OffsetSelectedKeys(const float timeOffset)
{
    assert(UiAnimUndo::IsRecording());
    CUiAnimViewSequenceNotificationContext context(this);

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        skey.Offset(timeOffset);
    }
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewSequence::ClipTimeOffsetForOffsetting(const float timeOffset)
{
    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    float newTimeOffset = timeOffset;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        const float keyTime = skey.GetTime();
        float newKeyTime = keyTime + timeOffset;

        Range extendedTimeRange(0.0f, GetTimeRange().end);
        extendedTimeRange.ClipValue(newKeyTime);

        float offset = newKeyTime - keyTime;
        if (fabs(offset) < fabs(newTimeOffset))
        {
            newTimeOffset = offset;
        }
    }

    return newTimeOffset;
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewSequence::ClipTimeOffsetForScaling(float timeOffset)
{
    if (timeOffset <= 0)
    {
        return timeOffset;
    }

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    float newTimeOffset = timeOffset;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        float keyTime = skey.GetTime();
        float newKeyTime = keyTime * timeOffset;
        GetTimeRange().ClipValue(newKeyTime);
        float offset = newKeyTime / keyTime;
        if (offset < newTimeOffset)
        {
            newTimeOffset = offset;
        }
    }

    return newTimeOffset;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::ScaleSelectedKeys(const float timeOffset)
{
    assert(UiAnimUndo::IsRecording());
    CUiAnimViewSequenceNotificationContext context(this);

    if (timeOffset <= 0)
    {
        return;
    }

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    const CUiAnimViewTrack* pTrack = nullptr;
    for (int k = 0; k < (int)selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(k);
        if (pTrack != skey.GetTrack())
        {
            pTrack = skey.GetTrack();
        }

        float keyt = skey.GetTime() * timeOffset;
        skey.SetTime(keyt);
    }
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewSequence::ClipTimeOffsetForSliding(const float timeOffset)
{
    CUiAnimViewKeyBundle keys = GetSelectedKeys();

    std::set<CUiAnimViewTrack*> tracks;
    std::set<CUiAnimViewTrack*>::const_iterator pTrackIter;

    Range timeRange = GetTimeRange();

    // Get the first key in the timeline among selected and
    // also gather tracks.
    float time0 = timeRange.end;
    for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = keys.GetKey(k);
        tracks.insert(skey.GetTrack());
        float keyTime = skey.GetTime();
        if (keyTime < time0)
        {
            time0 = keyTime;
        }
    }

    // If 'bAll' is true, slide all tracks.
    // (Otherwise, slide only selected tracks.)
    bool bAll = Qt::AltModifier & QApplication::queryKeyboardModifiers();
    if (bAll)
    {
        keys = GetKeysInTimeRange(time0, timeRange.end);
        // Gather tracks again.
        tracks.clear();
        for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
        {
            CUiAnimViewKeyHandle skey = keys.GetKey(k);
            tracks.insert(skey.GetTrack());
        }
    }

    float newTimeOffset = timeOffset;
    for (pTrackIter = tracks.begin(); pTrackIter != tracks.end(); ++pTrackIter)
    {
        CUiAnimViewTrack* pTrack = *pTrackIter;
        for (unsigned int i = 0; i < pTrack->GetKeyCount(); ++i)
        {
            CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

            const float keyTime = keyHandle.GetTime();
            if (keyTime >= time0)
            {
                float newKeyTime = keyTime + timeOffset;
                timeRange.ClipValue(newKeyTime);
                float offset = newKeyTime - keyTime;
                if (fabs(offset) < fabs(newTimeOffset))
                {
                    newTimeOffset = offset;
                }
            }
        }
    }

    return newTimeOffset;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::SlideKeys(float timeOffset)
{
    assert(UiAnimUndo::IsRecording());
    CUiAnimViewSequenceNotificationContext context(this);

    CUiAnimViewKeyBundle keys = GetSelectedKeys();

    std::set<CUiAnimViewTrack*> tracks;
    std::set<CUiAnimViewTrack*>::const_iterator pTrackIter;
    Range timeRange = GetTimeRange();

    // Get the first key in the timeline among selected and
    // also gather tracks.
    float time0 = timeRange.end;
    for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = keys.GetKey(k);
        tracks.insert(skey.GetTrack());
        float keyTime = skey.GetTime();
        if (keyTime < time0)
        {
            time0 = keyTime;
        }
    }

    // If 'bAll' is true, slide all tracks.
    // (Otherwise, slide only selected tracks.)
    bool bAll = Qt::AltModifier & QApplication::queryKeyboardModifiers();
    if (bAll)
    {
        keys = GetKeysInTimeRange(time0, timeRange.end);
        // Gather tracks again.
        tracks.clear();
        for (int k = 0; k < (int)keys.GetKeyCount(); ++k)
        {
            CUiAnimViewKeyHandle skey = keys.GetKey(k);
            tracks.insert(skey.GetTrack());
        }
    }

    for (pTrackIter = tracks.begin(); pTrackIter != tracks.end(); ++pTrackIter)
    {
        CUiAnimViewTrack* pTrack = *pTrackIter;
        pTrack->SlideKeys(time0, timeOffset);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::CloneSelectedKeys()
{
    assert(UiAnimUndo::IsRecording());
    CUiAnimViewSequenceNotificationContext context(this);

    CUiAnimViewKeyBundle selectedKeys = GetSelectedKeys();

    const CUiAnimViewTrack* pTrack = nullptr;
    // In case of multiple cloning, indices cannot be used as a solid pointer to the original.
    // So use the time of keys as an identifier, instead.
    std::vector<float> selectedKeyTimes;
    for (size_t k = 0; k < selectedKeys.GetKeyCount(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(static_cast<unsigned int>(k));
        if (pTrack != skey.GetTrack())
        {
            pTrack = skey.GetTrack();
        }

        selectedKeyTimes.push_back(skey.GetTime());
    }

    // Now, do the actual cloning.
    for (size_t k = 0; k < selectedKeyTimes.size(); ++k)
    {
        CUiAnimViewKeyHandle skey = selectedKeys.GetKey(static_cast<unsigned int>(k));
        skey = skey.GetTrack()->GetKeyByTime(selectedKeyTimes[k]);

        assert(skey.IsValid());
        if (!skey.IsValid())
        {
            continue;
        }

        CUiAnimViewKeyHandle newKey = skey.Clone();

        // Select new key.
        newKey.Select(true);
        // Deselect cloned key.
        skey.Select(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::BeginUndoTransaction()
{
    QueueNotifications();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::EndUndoTransaction()
{
    SubmitPendingNotifcations();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::BeginRestoreTransaction()
{
    QueueNotifications();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewSequence::EndRestoreTransaction()
{
    SubmitPendingNotifcations();
}


//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewSequence::IsActiveSequence() const
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    return pSequence == this;
}

//////////////////////////////////////////////////////////////////////////
const float CUiAnimViewSequence::GetTime() const
{
    return m_time;
}
