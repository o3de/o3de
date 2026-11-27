/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAnimViewAnimNode.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewSequence.h"

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyConstHandle::GetKey(IKey* pKey) const
{
    m_pTrack->GetKey(m_keyIndex, pKey);
}

////////////////////////////////////////////////////////////////////////////
float CUiAnimViewKeyConstHandle::GetTime() const
{
    return m_pTrack->GetKeyTime(m_keyIndex);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::SetKey(IKey* pKey)
{
    assert(m_bIsValid);

    m_pTrack->SetKey(m_keyIndex, pKey);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::GetKey(IKey* pKey) const
{
    assert(m_bIsValid);

    m_pTrack->GetKey(m_keyIndex, pKey);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::Select(bool bSelect)
{
    assert(m_bIsValid);

    m_pTrack->SelectKey(m_keyIndex, bSelect);
}

////////////////////////////////////////////////////////////////////////////
bool CUiAnimViewKeyHandle::IsSelected() const
{
    assert(m_bIsValid);

    return m_pTrack->IsKeySelected(m_keyIndex);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::SetTime(float time)
{
    assert(m_bIsValid);

    m_pTrack->SetKeyTime(m_keyIndex, time);
}

////////////////////////////////////////////////////////////////////////////
float CUiAnimViewKeyHandle::GetTime() const
{
    assert(m_bIsValid);

    return m_pTrack->GetKeyTime(m_keyIndex);
}

////////////////////////////////////////////////////////////////////////////
float CUiAnimViewKeyHandle::GetDuration() const
{
    assert(m_bIsValid);

    const char* desc = nullptr;
    float duration = 0;
    m_pTrack->m_pAnimTrack->GetKeyInfo(m_keyIndex, desc, duration);

    return duration;
}

////////////////////////////////////////////////////////////////////////////
const char* CUiAnimViewKeyHandle::GetDescription() const
{
    assert(m_bIsValid);

    const char* desc = "";
    float duration = 0;
    m_pTrack->m_pAnimTrack->GetKeyInfo(m_keyIndex, desc, duration);

    return desc;
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::Offset(float offset)
{
    assert(m_bIsValid);

    float newTime = m_pTrack->GetKeyTime(m_keyIndex) + offset;
    m_pTrack->SetKeyTime(m_keyIndex, newTime);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyHandle::Delete()
{
    assert(m_bIsValid);

    m_pTrack->RemoveKey(m_keyIndex);
    m_bIsValid = false;
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyHandle::Clone()
{
    assert(m_bIsValid);

    unsigned int newKeyIndex = m_pTrack->CloneKey(m_keyIndex);
    return CUiAnimViewKeyHandle(m_pTrack, newKeyIndex);
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyHandle::GetNextKey()
{
    return m_pTrack->GetNextKey(GetTime());
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyHandle::GetPrevKey()
{
    return m_pTrack->GetPrevKey(GetTime());
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyHandle::GetAboveKey() const
{
    // Search for track above that has keys
    for (CUiAnimViewNode* pCurrentNode = m_pTrack->GetAboveNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetAboveNode())
    {
        if (pCurrentNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pCurrentTrack = static_cast<CUiAnimViewTrack*>(pCurrentNode);
            const unsigned int keyCount = pCurrentTrack->GetKeyCount();
            if (keyCount > 0)
            {
                // Return key with nearest time to this key
                return pCurrentTrack->GetNearestKeyByTime(GetTime());
            }
        }
    }

    return CUiAnimViewKeyHandle();
}

////////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyHandle::GetBelowKey() const
{
    // Search for track below that has keys
    for (CUiAnimViewNode* pCurrentNode = m_pTrack->GetBelowNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetBelowNode())
    {
        if (pCurrentNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pCurrentTrack = static_cast<CUiAnimViewTrack*>(pCurrentNode);
            const unsigned int keyCount = pCurrentTrack->GetKeyCount();
            if (keyCount > 0)
            {
                // Return key with nearest time to this key
                return pCurrentTrack->GetNearestKeyByTime(GetTime());
            }
        }
    }

    return CUiAnimViewKeyHandle();
}

////////////////////////////////////////////////////////////////////////////
bool CUiAnimViewKeyHandle::operator==(const CUiAnimViewKeyHandle& keyHandle) const
{
    return m_pTrack == keyHandle.m_pTrack && m_keyIndex == keyHandle.m_keyIndex;
}

////////////////////////////////////////////////////////////////////////////
bool CUiAnimViewKeyHandle::operator!=(const CUiAnimViewKeyHandle& keyHandle) const
{
    return !(*this == keyHandle);
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyBundle::AppendKey(const CUiAnimViewKeyHandle& keyHandle)
{
    // Check if newly added key has different type than existing ones
    if (m_bAllOfSameType && m_keys.size() > 0)
    {
        const CUiAnimViewKeyHandle& lastKey = m_keys.back();

        const CUiAnimViewTrack* pMyTrack = keyHandle.GetTrack();
        const CUiAnimViewTrack* pOtherTrack = lastKey.GetTrack();

        // Check if keys are from sub tracks, always compare types of parent track
        if (pMyTrack->IsSubTrack())
        {
            pMyTrack = static_cast<const CUiAnimViewTrack*>(pMyTrack->GetParentNode());
        }

        if (pOtherTrack->IsSubTrack())
        {
            pOtherTrack = static_cast<const CUiAnimViewTrack*>(pOtherTrack->GetParentNode());
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

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyBundle::AppendKeyBundle(const CUiAnimViewKeyBundle& bundle)
{
    for (auto iter = bundle.m_keys.begin(); iter != bundle.m_keys.end(); ++iter)
    {
        AppendKey(*iter);
    }
}

////////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyBundle::SelectKeys(const bool bSelected)
{
    const unsigned int numKeys = GetKeyCount();

    for (unsigned int i = 0; i < numKeys; ++i)
    {
        GetKey(i).Select(bSelected);
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewKeyBundle::GetSingleSelectedKey()
{
    const unsigned int keyCount = GetKeyCount();

    if (keyCount == 1)
    {
        return m_keys[0];
    }
    else if (keyCount > 1 && keyCount <= 4)
    {
        // All keys must have same time & same parent track
        CUiAnimViewNode* pFirstParent = m_keys[0].GetTrack()->GetParentNode();
        const float firstTime = m_keys[0].GetTime();

        // Parent must be a track
        if (pFirstParent->GetNodeType() != eUiAVNT_Track)
        {
            return CUiAnimViewKeyHandle();
        }

        // Check other keys for equality
        for (unsigned int i = 0; i < keyCount; ++i)
        {
            if (m_keys[i].GetTrack()->GetParentNode() != pFirstParent || m_keys[i].GetTime() != firstTime)
            {
                return CUiAnimViewKeyHandle();
            }
        }

        return static_cast<CUiAnimViewTrack*>(pFirstParent)->GetKeyByTime(firstTime);
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode::CUiAnimViewNode(CUiAnimViewNode* pParent)
    : m_pParentNode(pParent)
    , m_bSelected(false)
    , m_bExpanded(false)
    , m_bHidden(false)
{
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewNode::HasObsoleteTrack() const
{
    return HasObsoleteTrackRec(this);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewNode::HasObsoleteTrackRec(const CUiAnimViewNode* pCurrentNode) const
{
    if (pCurrentNode->GetNodeType() == eUiAVNT_Track)
    {
        const CUiAnimViewTrack* pTrack = static_cast<const CUiAnimViewTrack*>(pCurrentNode);

        EUiAnimCurveType trackType = pTrack->GetCurveType();
        if (trackType == eUiAnimCurveType_TCBFloat || trackType == eUiAnimCurveType_TCBQuat || trackType == eUiAnimCurveType_TCBVector)
        {
            return true;
        }
    }

    for (unsigned int i = 0; i < pCurrentNode->GetChildCount(); ++i)
    {
        CUiAnimViewNode* pNode = pCurrentNode->GetChild(i);

        if (HasObsoleteTrackRec(pNode))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::ClearSelection()
{
    CUiAnimViewSequenceNotificationContext context(GetSequence());

    SetSelected(false);

    const unsigned int numChilds = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < numChilds; ++childIndex)
    {
        GetChild(childIndex)->ClearSelection();
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewNode::GetAboveNode() const
{
    CUiAnimViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have an above node
        return nullptr;
    }

    CUiAnimViewNode* pPrevSibling = GetPrevSibling();
    if (!pPrevSibling)
    {
        // First sibling -> parent is above node
        return pParent;
    }

    // Find last node in sibling tree
    CUiAnimViewNode* pCurrentNode = pPrevSibling;
    while (pCurrentNode && pCurrentNode->GetChildCount() > 0 && pCurrentNode->IsExpanded())
    {
        pCurrentNode = pCurrentNode->GetChild(pCurrentNode->GetChildCount() - 1);
    }

    return pCurrentNode;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewNode::GetBelowNode() const
{
    const unsigned int childCount = GetChildCount();
    if (childCount > 0 && IsExpanded())
    {
        return GetChild(0);
    }

    CUiAnimViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // Root without children
        return nullptr;
    }

    // If there is a next sibling return it
    CUiAnimViewNode* pNextSibling = GetNextSibling();
    if (pNextSibling)
    {
        return pNextSibling;
    }

    // Otherwise we need to go up the tree and check
    // the parent nodes for next siblings
    CUiAnimViewNode* pCurrentNode = pParent;
    while (pCurrentNode)
    {
        CUiAnimViewNode* pNextParentSibling = pCurrentNode->GetNextSibling();
        if (pNextParentSibling)
        {
            return pNextParentSibling;
        }

        pCurrentNode = pCurrentNode->GetParentNode();
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewNode::GetPrevSibling() const
{
    CUiAnimViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have siblings
        return nullptr;
    }

    // Search for prev sibling
    unsigned int siblingCount = pParent->GetChildCount();
    assert(siblingCount > 0);

    for (unsigned int i = 1; i < siblingCount; ++i)
    {
        CUiAnimViewNode* pSibling = pParent->GetChild(i);
        if (pSibling == this)
        {
            return pParent->GetChild(i - 1);
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewNode::GetNextSibling() const
{
    CUiAnimViewNode* pParent = GetParentNode();
    if (!pParent)
    {
        // The root does not have siblings
        return nullptr;
    }

    // Search for next sibling
    unsigned int siblingCount = pParent->GetChildCount();
    assert(siblingCount > 0);

    for (unsigned int i = 0; i < siblingCount - 1; ++i)
    {
        CUiAnimViewNode* pSibling = pParent->GetChild(i);
        if (pSibling == this)
        {
            return pParent->GetChild(i + 1);
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::SetSelected(bool bSelected)
{
    if (bSelected != m_bSelected)
    {
        m_bSelected = bSelected;

        if (m_bSelected)
        {
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Selected);
        }
        else
        {
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Deselected);
        }

        GetSequence()->OnNodeSelectionChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewSequence* CUiAnimViewNode::GetSequence()
{
    for (CUiAnimViewNode* pCurrentNode = this; pCurrentNode; pCurrentNode = pCurrentNode->GetParentNode())
    {
        if (pCurrentNode->GetNodeType() == eUiAVNT_Sequence)
        {
            return static_cast<CUiAnimViewSequence*>(pCurrentNode);
        }
    }

    // Every node belongs to a sequence
    assert(false);

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::SetExpanded(bool bExpanded)
{
    if (bExpanded != m_bExpanded)
    {
        m_bExpanded = bExpanded;

        if (bExpanded)
        {
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Expanded);
        }
        else
        {
            GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Collapsed);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::AddNode(CUiAnimViewNode* pNode)
{
    assert (pNode->GetNodeType() != eUiAVNT_Sequence);

    m_childNodes.push_back(std::unique_ptr<CUiAnimViewNode>(pNode));
    SortNodes();

    pNode->m_pParentNode = this;
    GetSequence()->OnNodeChanged(pNode, IUiAnimViewSequenceListener::eNodeChangeType_Added);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::SortNodes()
{
    // Sort with operator<
    std::stable_sort(m_childNodes.begin(), m_childNodes.end(),
        [&](const std::unique_ptr<CUiAnimViewNode>& a, const std::unique_ptr<CUiAnimViewNode>& b) -> bool
        {
            const CUiAnimViewNode* pA = a.get();
            const CUiAnimViewNode* pB = b.get();
            return *pA < *pB;
        }
        );
}

namespace
{
    static int GetNodeOrder(EUiAnimNodeType nodeType)
    {
        assert(nodeType < eUiAnimNodeType_Num);

        static int nodeOrder[eUiAnimNodeType_Num];
        nodeOrder[eUiAnimNodeType_Invalid] = 0;
        nodeOrder[eUiAnimNodeType_Director] = 1;
        nodeOrder[eUiAnimNodeType_Camera] = 2;
        nodeOrder[eUiAnimNodeType_Entity] = 3;
        nodeOrder[eUiAnimNodeType_Alembic] = 4;
        nodeOrder[eUiAnimNodeType_GeomCache] = 5;
        nodeOrder[eUiAnimNodeType_CVar] = 6;
        nodeOrder[eUiAnimNodeType_ScriptVar] = 7;
        nodeOrder[eUiAnimNodeType_Material] = 8;
        nodeOrder[eUiAnimNodeType_Event] = 9;
        nodeOrder[eUiAnimNodeType_Layer] = 10;
        nodeOrder[eUiAnimNodeType_Comment] = 11;
        nodeOrder[eUiAnimNodeType_RadialBlur] = 12;
        nodeOrder[eUiAnimNodeType_ColorCorrection] = 13;
        nodeOrder[eUiAnimNodeType_DepthOfField] = 14;
        nodeOrder[eUiAnimNodeType_ScreenFader] = 15;
        nodeOrder[eUiAnimNodeType_Light] = 16;
        nodeOrder[eUiAnimNodeType_HDRSetup] = 17;
        nodeOrder[eUiAnimNodeType_ShadowSetup] = 18;
        nodeOrder[eUiAnimNodeType_Environment] = 19;
        nodeOrder[eUiAnimNodeType_ScreenDropsSetup] = 20;
        nodeOrder[eUiAnimNodeType_Group] = 21;

        return nodeOrder[nodeType];
    }
}

bool CUiAnimViewNode::operator<(const CUiAnimViewNode& otherNode) const
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
    case eUiAVNT_AnimNode:
    {
        const CUiAnimViewAnimNode& thisAnimNode = static_cast<const CUiAnimViewAnimNode&>(*this);
        const CUiAnimViewAnimNode& otherAnimNode = static_cast<const CUiAnimViewAnimNode&>(otherNode);

        const int thisTypeOrder = GetNodeOrder(thisAnimNode.GetType());
        const int otherTypeOrder = GetNodeOrder(otherAnimNode.GetType());

        if (thisTypeOrder == otherTypeOrder)
        {
            // Same node type, sort by name
            return thisAnimNode.GetName() < otherAnimNode.GetName();
        }

        return thisTypeOrder < otherTypeOrder;
    }
    case eUiAVNT_Track:
        const CUiAnimViewTrack& thisTrack = static_cast<const CUiAnimViewTrack&>(*this);
        const CUiAnimViewTrack& otherTrack = static_cast<const CUiAnimViewTrack&>(otherNode);

        if (thisTrack.GetParameterType() == otherTrack.GetParameterType())
        {
            // Same parameter type, sort by name
            return thisTrack.GetName() < otherTrack.GetName();
        }

        return thisTrack.GetParameterType() < otherTrack.GetParameterType();
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNode::SetHidden(bool bHidden)
{
    bool bWasHidden = m_bHidden;
    m_bHidden = bHidden;

    if (bHidden && !bWasHidden)
    {
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Hidden);
    }
    else if (!bHidden && bWasHidden)
    {
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Unhidden);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewNode::IsHidden() const
{
    return m_bHidden;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewNode::GetFirstSelectedNode()
{
    if (IsSelected())
    {
        return this;
    }

    const unsigned int numChilds = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < numChilds; ++childIndex)
    {
        CUiAnimViewNode* pSelectedNode = GetChild(childIndex)->GetFirstSelectedNode();
        if (pSelectedNode)
        {
            return pSelectedNode;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode* CUiAnimViewNode::GetDirector()
{
    for (CUiAnimViewNode* pCurrentNode = GetParentNode(); pCurrentNode; pCurrentNode = pCurrentNode->GetParentNode())
    {
        if (pCurrentNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pParentAnimNode = static_cast<CUiAnimViewAnimNode*>(pCurrentNode);

            if (pParentAnimNode->GetType() == eUiAnimNodeType_Director)
            {
                return pParentAnimNode;
            }
        }
        else if (pCurrentNode->GetNodeType() == eUiAVNT_Sequence)
        {
            return static_cast<CUiAnimViewAnimNode*>(pCurrentNode);
        }
    }

    return nullptr;
}
