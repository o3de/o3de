/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewNode.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>

// AzCore
#include <AzCore/std/sort.h>

// Editor
#include "TrackView/TrackViewSequence.h"
#include "TrackView/TrackViewTrack.h"


void CTrackViewKeyConstHandle::GetKey(IKey* pKey) const
{
    if (!pKey  || !m_pTrack)
    {
        AZ_Assert(pKey != nullptr, "Expected valid key pointer.");
        AZ_Assert(m_pTrack != nullptr, "Const Key handle is invalid.");
        return;
    }

    m_pTrack->GetKey(m_keyIndex, pKey);
}

float CTrackViewKeyConstHandle::GetTime() const
{
    if (!m_pTrack)
    {
        AZ_Assert(false, "Const Key handle is invalid");
        return 0;
    }

    return m_pTrack->GetKeyTime(m_keyIndex);
}

void CTrackViewKeyHandle::SetKey(IKey* pKey)
{
    if (!pKey || !m_bIsValid || !m_pTrack)
    {
        AZ_Assert(pKey != nullptr, "Expected valid key pointer.");
        AZ_Assert(m_bIsValid && m_pTrack != nullptr, "Key handle is invalid.");
        return;
    }

    m_pTrack->SetKey(m_keyIndex, pKey);
}

void CTrackViewKeyHandle::GetKey(IKey* pKey) const
{
    if (!pKey || !m_bIsValid || !m_pTrack)
    {
        AZ_Assert(pKey != nullptr, "Expected valid key pointer.");
        AZ_Assert(m_bIsValid && m_pTrack != nullptr, "Key handle is invalid.");
        return;
    }

    m_pTrack->GetKey(m_keyIndex, pKey);
}

void CTrackViewKeyHandle::Select(bool bSelect)
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return;
    }

    m_pTrack->SelectKey(m_keyIndex, bSelect);
}

bool CTrackViewKeyHandle::IsSelected() const
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return false;
    }

    return m_pTrack->IsKeySelected(m_keyIndex);
}

void CTrackViewKeyHandle::SetTime(float time, bool notifyListeners)
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return;
    }

    // Flag the current key, because the key handle may become invalid
    // after the time is set and it is potentially sorted into a different
    // index.    
    m_pTrack->SetSortMarkerKey(m_keyIndex, true);

    // set the new time, this may cause a sort that reorders the keys, making
    // m_keyIndex incorrect.
    m_pTrack->SetKeyTime(m_keyIndex, time, notifyListeners); // Let CTrackViewTrack::SetKeyTime() manage Undo/Redo

    // If the key at this index changed because of the key sort by time.
    // We need to search through the keys now and find the marker.
    if (!m_pTrack->IsSortMarkerKey(m_keyIndex))
    {
        CTrackViewKeyBundle allKeys = m_pTrack->GetAllKeys();
        for (unsigned int x = 0; x < allKeys.GetKeyCount(); x++)
        {
            const auto curIndex = allKeys.GetKey(x).GetIndex();
            if (m_pTrack->IsSortMarkerKey(curIndex))
            {
                m_keyIndex = curIndex;
                break;
            }
        }
    }

    // clear the sort marker
    m_pTrack->SetSortMarkerKey(m_keyIndex, false);
}

float CTrackViewKeyHandle::GetTime() const
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return 0;
    }

    return m_pTrack->GetKeyTime(m_keyIndex);
}

float CTrackViewKeyHandle::GetDuration() const
{
    if (!m_bIsValid || !m_pTrack || !m_pTrack->m_pAnimTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return 0;
    }

    const char* desc = nullptr;
    float duration = 0;
    m_pTrack->m_pAnimTrack->GetKeyInfo(m_keyIndex, desc, duration);

    return duration;
}

const char* CTrackViewKeyHandle::GetDescription() const
{
    const char* desc = "";
    if (!m_bIsValid || !m_pTrack || !m_pTrack->m_pAnimTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return desc;
    }

    float duration = 0;
    m_pTrack->m_pAnimTrack->GetKeyInfo(m_keyIndex, desc, duration);

    return desc;
}

void CTrackViewKeyHandle::Offset(float offset, bool notifyListeners)
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid");
        return;
    }

    const float newTime = m_pTrack->GetKeyTime(m_keyIndex) + offset;
    m_pTrack->SetKeyTime(m_keyIndex, newTime, notifyListeners);
}

void CTrackViewKeyHandle::Delete()
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return;
    }

    m_pTrack->RemoveKey(m_keyIndex);
    m_bIsValid = false;
}

CTrackViewKeyHandle CTrackViewKeyHandle::Clone(float timeOffset)
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return CTrackViewKeyHandle();
    }

    const auto newKeyIndex = m_pTrack->CloneKey(m_keyIndex, timeOffset);
    return CTrackViewKeyHandle(m_pTrack, newKeyIndex);
}

CTrackViewKeyHandle CTrackViewKeyHandle::GetNextKey()
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return CTrackViewKeyHandle();
    }

    return m_pTrack->GetNextKey(GetTime());
}

CTrackViewKeyHandle CTrackViewKeyHandle::GetPrevKey()
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return CTrackViewKeyHandle();
    }

    return m_pTrack->GetPrevKey(GetTime());
}

CTrackViewKeyHandle CTrackViewKeyHandle::GetAboveKey() const
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return CTrackViewKeyHandle();
    }

    // Search for track above that has keys
    for (CTrackViewNode* pCurrentNode = m_pTrack->GetAboveNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetAboveNode())
    {
        if (pCurrentNode->GetNodeType() == eTVNT_Track)
        {
            if (CTrackViewTrack* pCurrentTrack = static_cast<CTrackViewTrack*>(pCurrentNode))
            {
                const auto keyCount = pCurrentTrack->GetKeyCount();
                if (keyCount > 0)
                {
                    // Return key with nearest time to this key
                    return pCurrentTrack->GetNearestKeyByTime(GetTime());
                }
            }
        }
    }

    return CTrackViewKeyHandle();
}

CTrackViewKeyHandle CTrackViewKeyHandle::GetBelowKey() const
{
    if (!m_bIsValid || !m_pTrack)
    {
        AZ_Assert(false, "Key handle is invalid.");
        return CTrackViewKeyHandle();
    }

    // Search for track below that has keys
    for (CTrackViewNode* pCurrentNode = m_pTrack->GetBelowNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetBelowNode())
    {
        if (pCurrentNode->GetNodeType() == eTVNT_Track)
        {
            if (CTrackViewTrack* pCurrentTrack = static_cast<CTrackViewTrack*>(pCurrentNode))
            {
                const auto keyCount = pCurrentTrack->GetKeyCount();
                if (keyCount > 0)
                {
                    // Return key with nearest time to this key
                    return pCurrentTrack->GetNearestKeyByTime(GetTime());
                }
            }
        }
    }

    return CTrackViewKeyHandle();
}

bool CTrackViewKeyHandle::operator==(const CTrackViewKeyHandle& keyHandle) const
{
    return m_pTrack == keyHandle.m_pTrack && m_keyIndex == keyHandle.m_keyIndex;
}

bool CTrackViewKeyHandle::operator!=(const CTrackViewKeyHandle& keyHandle) const
{
    return !(*this == keyHandle);
}

void CTrackViewKeyBundle::AppendKey(const CTrackViewKeyHandle& keyHandle)
{
    if (!keyHandle.IsValid() || !keyHandle.GetTrack())
    {
        AZ_Assert(false, "Key handle is invalid.");
        return;
    }

    // Check if newly added key has different type than existing ones
    if (m_bAllOfSameType && m_keys.size() > 0)
    {
        const CTrackViewKeyHandle& lastKey = m_keys.back();

        const CTrackViewTrack* pMyTrack = keyHandle.GetTrack();
        const CTrackViewTrack* pOtherTrack = lastKey.GetTrack();
        if (!lastKey.IsValid() || !pOtherTrack)
        {
            AZ_Assert(false, "Expected valid key handle in bundle.")
            return;
        }

        // Check if keys are from sub tracks, always compare types of parent track
        if (pMyTrack->IsSubTrack())
        {
            pMyTrack = static_cast<const CTrackViewTrack*>(pMyTrack->GetParentNode());
            if (!pMyTrack)
            {
                AZ_Assert(false, "Expected valid parent track for sub-track to append key.");
                return;
            }
        }

        if (pOtherTrack->IsSubTrack())
        {
            pOtherTrack = static_cast<const CTrackViewTrack*>(pOtherTrack->GetParentNode());
            if (!pOtherTrack)
            {
                AZ_Assert(false, "Expected valid parent track for sub-track of previously appended key.") return;
                return;
            }
        }

        // Do comparison
        if (pMyTrack->GetParameterType() != pOtherTrack->GetParameterType()
            || pMyTrack->GetCurveType() != pOtherTrack->GetCurveType()
            || pMyTrack->GetValueType() != pOtherTrack->GetValueType())
        {
            m_bAllOfSameType = false;
        }
    }

    m_keys.push_back(keyHandle);
}

void CTrackViewKeyBundle::AppendKeyBundle(const CTrackViewKeyBundle& bundle)
{
    for (auto iter = bundle.m_keys.begin(); iter != bundle.m_keys.end(); ++iter)
    {
        AppendKey(*iter);
    }
}

void CTrackViewKeyBundle::SelectKeys(const bool bSelected)
{
    const unsigned int numKeys = GetKeyCount();

    for (unsigned int i = 0; i < numKeys; ++i)
    {
        GetKey(i).Select(bSelected);
    }
}

CTrackViewKeyHandle CTrackViewKeyBundle::GetSingleSelectedKey() const
{
    const unsigned int keyCount = GetKeyCount();

    if (keyCount == 1)
    {
        return m_keys[0];
    }
    else if (keyCount > 1 && keyCount <= 4)
    {
        // All keys must have same time & same parent track
        CTrackViewNode* pFirstParent = m_keys[0].GetTrack()->GetParentNode();
        const float firstTime = m_keys[0].GetTime();

        // Parent must be a track
        if (pFirstParent->GetNodeType() != eTVNT_Track)
        {
            return CTrackViewKeyHandle();
        }

        // Check other keys for equality
        for (unsigned int i = 0; i < keyCount; ++i)
        {
            if (m_keys[i].GetTrack()->GetParentNode() != pFirstParent || m_keys[i].GetTime() != firstTime)
            {
                return CTrackViewKeyHandle();
            }
        }

        return static_cast<CTrackViewTrack*>(pFirstParent)->GetKeyByTime(firstTime);
    }

    return CTrackViewKeyHandle();
}

CTrackViewNode::CTrackViewNode(CTrackViewNode* pParent)
    : m_pParentNode(pParent)
    , m_bSelected(false)
    , m_bHidden(false)
{
}

bool CTrackViewNode::HasObsoleteTrack() const
{
    return HasObsoleteTrackRec(this);
}

bool CTrackViewNode::HasObsoleteTrackRec(const CTrackViewNode* pCurrentNode) const
{
    if (!pCurrentNode)
    {
        AZ_Assert(false, "Expected valid TrackViewNode pointer.");
        return false;
    }

    if (pCurrentNode->GetNodeType() == eTVNT_Track)
    {
        if (const CTrackViewTrack* pTrack = static_cast<const CTrackViewTrack*>(pCurrentNode))
        {
            EAnimCurveType trackType = pTrack->GetCurveType();
            if (trackType == eAnimCurveType_TCBFloat || trackType == eAnimCurveType_TCBQuat || trackType == eAnimCurveType_TCBVector)
            {
                return true;
            }
        }
    }

    for (unsigned int i = 0; i < pCurrentNode->GetChildCount(); ++i)
    {
        CTrackViewNode* pNode = pCurrentNode->GetChild(i);

        if (HasObsoleteTrackRec(pNode))
        {
            return true;
        }
    }

    return false;
}

void CTrackViewNode::ClearSelection()
{
    CTrackViewSequenceNotificationContext context(GetSequence());

    SetSelected(false);

    const unsigned int numChilds = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < numChilds; ++childIndex)
    {
        if (const auto pChild = GetChild(childIndex))
        {
            pChild->ClearSelection();
        }
    }
}

CTrackViewNode* CTrackViewNode::GetAboveNode() const
{
    CTrackViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have an above node
        return nullptr;
    }

    CTrackViewNode* pPrevSibling = GetPrevSibling();
    if (!pPrevSibling)
    {
        // First sibling -> parent is above node
        return pParent;
    }

    // Find last node in sibling tree
    CTrackViewNode* pCurrentNode = pPrevSibling;
    while (pCurrentNode && pCurrentNode->GetChildCount() > 0 && pCurrentNode->GetExpanded())
    {
        pCurrentNode = pCurrentNode->GetChild(pCurrentNode->GetChildCount() - 1);
    }

    return pCurrentNode;
}

CTrackViewNode* CTrackViewNode::GetBelowNode() const
{
    const unsigned int childCount = GetChildCount();
    if (childCount > 0 && GetExpanded())
    {
        return GetChild(0);
    }

    CTrackViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // Root without children
        return nullptr;
    }

    // If there is a next sibling return it
    CTrackViewNode* pNextSibling = GetNextSibling();
    if (pNextSibling)
    {
        return pNextSibling;
    }

    // Otherwise we need to go up the tree and check
    // the parent nodes for next siblings
    CTrackViewNode* pCurrentNode = pParent;
    while (pCurrentNode)
    {
        CTrackViewNode* pNextParentSibling = pCurrentNode->GetNextSibling();
        if (pNextParentSibling)
        {
            return pNextParentSibling;
        }

        pCurrentNode = pCurrentNode->GetParentNode();
    }

    return nullptr;
}

CTrackViewNode* CTrackViewNode::GetPrevSibling() const
{
    CTrackViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have siblings
        return nullptr;
    }

    // Search for prev sibling
    const unsigned int siblingCount = pParent->GetChildCount();
    AZ_Assert(siblingCount > 0, "No siblings for parent %p", pParent);

    for (unsigned int i = 1; i < siblingCount; ++i)
    {
        CTrackViewNode* pSibling = pParent->GetChild(i);
        if (pSibling == this)
        {
            return pParent->GetChild(i - 1);
        }
    }

    return nullptr;
}

CTrackViewNode* CTrackViewNode::GetNextSibling() const
{
    CTrackViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have siblings
        return nullptr;
    }

    // Search for next sibling
    const unsigned int siblingCount = pParent->GetChildCount();
    AZ_Assert(siblingCount > 0, "No siblings for parent %p", pParent);

    for (unsigned int i = 0; i < siblingCount - 1; ++i)
    {
        CTrackViewNode* pSibling = pParent->GetChild(i);
        if (pSibling == this)
        {
            return pParent->GetChild(i + 1);
        }
    }

    return nullptr;
}

void CTrackViewNode::SetSelected(bool bSelected)
{
    const auto  pSequence = GetSequence();
    if (!pSequence)
    {
        return;
    }

    if (bSelected == m_bSelected)
    {
        return; // nothing to do
    }

    m_bSelected = bSelected;

    pSequence->OnNodeChanged(this, m_bSelected
        ? ITrackViewSequenceListener::eNodeChangeType_Selected
        : ITrackViewSequenceListener::eNodeChangeType_Deselected);

    pSequence->OnNodeSelectionChanged();
}

CTrackViewSequence* CTrackViewNode::GetSequence()
{
    for (CTrackViewNode* pCurrentNode = this; pCurrentNode; pCurrentNode = pCurrentNode->GetParentNode())
    {
        if (pCurrentNode->GetNodeType() == eTVNT_Sequence)
        {
            return static_cast<CTrackViewSequence*>(pCurrentNode);
        }
    }

    // Every node belongs to a sequence
    AZ_Assert(false, "Sequence node not found for node (%s).", GetName().c_str());

    return nullptr;
}

const CTrackViewSequence* CTrackViewNode::GetSequenceConst() const
{
    for (const CTrackViewNode* pCurrentNode = this; pCurrentNode; pCurrentNode = pCurrentNode->GetParentNode())
    {
        if (pCurrentNode->GetNodeType() == eTVNT_Sequence)
        {
            return static_cast<const CTrackViewSequence*>(pCurrentNode);
        }
    }

    AZ_Assert(false, "Every node belongs to a sequence");

    return nullptr;
}

void CTrackViewNode::AddNode(CTrackViewNode* pNode)
{
    auto pSequence = GetSequence();
    if (!pNode || !pSequence)
    {
        AZ_Assert(pNode, "Expected a valid node pointer to be added to node (%s).", GetName().c_str());
        return;
    }

    if (pNode->GetNodeType() == eTVNT_Sequence)
    {
        AZ_Assert(false, "Attempting to add a sequence node (%s) to node (%s).", pNode->GetName().c_str(), GetName().c_str());
        return;
    }

    m_childNodes.push_back(AZStd::unique_ptr<CTrackViewNode>(pNode));
    SortNodes();

    pNode->m_pParentNode = this;
    pSequence->OnNodeChanged(pNode, ITrackViewSequenceListener::eNodeChangeType_Added);
}

void CTrackViewNode::SortNodes()
{
    // Sort with operator<
    AZStd::stable_sort(m_childNodes.begin(), m_childNodes.end(),
        [&](const AZStd::unique_ptr<CTrackViewNode>& a, const AZStd::unique_ptr<CTrackViewNode>& b) -> bool
        {
            const CTrackViewNode* pA = a.get();
            const CTrackViewNode* pB = b.get();
            return *pA < *pB;
        }
        );
}

namespace
{
    static int GetNodeOrder(AnimNodeType nodeType)
    {
        AZ_Assert(nodeType < AnimNodeType::Num, "Expected nodeType to be less than AnimNodeType::Num");

        // note: this array gets over-allocated and is sparsely populated because the eAnimNodeType enums are not sequential in IMovieSystem.h
        // I wonder if the original authors intended this? Not a big deal, just some trivial memory wastage.
        static int nodeOrder[static_cast<int>(AnimNodeType::Num)];
        nodeOrder[static_cast<int>(AnimNodeType::Invalid)] = 0;
        nodeOrder[static_cast<int>(AnimNodeType::Director)] = 1;
        nodeOrder[static_cast<int>(AnimNodeType::Alembic)] = 4;
        nodeOrder[static_cast<int>(AnimNodeType::CVar)] = 6;
        nodeOrder[static_cast<int>(AnimNodeType::ScriptVar)] = 7;
        nodeOrder[static_cast<int>(AnimNodeType::Event)] = 9;
        nodeOrder[static_cast<int>(AnimNodeType::Layer)] = 10;
        nodeOrder[static_cast<int>(AnimNodeType::Comment)] = 11;
        nodeOrder[static_cast<int>(AnimNodeType::RadialBlur)] = 12;
        nodeOrder[static_cast<int>(AnimNodeType::ColorCorrection)] = 13;
        nodeOrder[static_cast<int>(AnimNodeType::DepthOfField)] = 14;
        nodeOrder[static_cast<int>(AnimNodeType::ScreenFader)] = 15;
        nodeOrder[static_cast<int>(AnimNodeType::Light)] = 16;
        nodeOrder[static_cast<int>(AnimNodeType::ShadowSetup)] = 17;
        nodeOrder[static_cast<int>(AnimNodeType::Group)] = 18;

        return nodeOrder[static_cast<int>(nodeType)];
    }
}

bool CTrackViewNode::operator<(const CTrackViewNode& otherNode) const
{
    // Order nodes before tracks
    if (GetNodeType() < otherNode.GetNodeType())
    {
        return true;
    }
    else if (GetNodeType() > otherNode.GetNodeType())
    {
        return false;
    }

    // Same node type
    switch (GetNodeType())
    {
    case eTVNT_AnimNode:
    {
        const CTrackViewAnimNode& thisAnimNode = static_cast<const CTrackViewAnimNode&>(*this);
        const CTrackViewAnimNode& otherAnimNode = static_cast<const CTrackViewAnimNode&>(otherNode);

        const int thisTypeOrder = GetNodeOrder(thisAnimNode.GetType());
        const int otherTypeOrder = GetNodeOrder(otherAnimNode.GetType());

        if (thisTypeOrder == otherTypeOrder)
        {
            // Same node type, sort by name
            return thisAnimNode.GetName() < otherAnimNode.GetName();
        }

        return thisTypeOrder < otherTypeOrder;
    }
    case eTVNT_Track:
        const CTrackViewTrack& thisTrack = static_cast<const CTrackViewTrack&>(*this);
        const CTrackViewTrack& otherTrack = static_cast<const CTrackViewTrack&>(otherNode);

        if (thisTrack.GetParameterType() == otherTrack.GetParameterType())
        {
            // Same parameter type, sort by name
            return thisTrack.GetName() < otherTrack.GetName();
        }

        return thisTrack.GetParameterType() < otherTrack.GetParameterType();
    }

    return false;
}

void CTrackViewNode::SetHidden(bool bHidden)
{
    const auto pSequence = GetSequence();
    if (!pSequence)
    {
        return;
    }

    if (bHidden == m_bHidden)
    {
        return; // nothing to do
    }

    m_bHidden = bHidden;

    pSequence->OnNodeChanged(this, bHidden
        ? ITrackViewSequenceListener::eNodeChangeType_Hidden
        : ITrackViewSequenceListener::eNodeChangeType_Unhidden);
}

bool CTrackViewNode::IsHidden() const
{
    return m_bHidden;
}

CTrackViewNode* CTrackViewNode::GetFirstSelectedNode()
{
    if (IsSelected())
    {
        return this;
    }

    const unsigned int numChilds = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < numChilds; ++childIndex)
    {
        const auto pChild = GetChild(childIndex);
        const auto pSelectedNode = pChild ? GetFirstSelectedNode() : nullptr;
        if (pSelectedNode)
        {
            return pSelectedNode;
        }
    }

    return nullptr;
}

CTrackViewAnimNode* CTrackViewNode::GetDirector() const
{
    for (CTrackViewNode* pCurrentNode = GetParentNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetParentNode())
    {
        if (pCurrentNode->GetNodeType() == eTVNT_AnimNode)
        {
            if (CTrackViewAnimNode* pParentAnimNode = static_cast<CTrackViewAnimNode*>(pCurrentNode))
            {
                if (pParentAnimNode->GetType() == AnimNodeType::Director)
                {
                    return pParentAnimNode;
                }
            }
        }
        else if (pCurrentNode->GetNodeType() == eTVNT_Sequence)
        {
            return static_cast<CTrackViewAnimNode*>(pCurrentNode);
        }
    }

    return nullptr;
}
