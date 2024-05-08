/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAnimViewTrack.h"
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewNodeFactories.h"

#include "UiEditorAnimationBus.h"

#include <Util/EditorUtils.h>
#include <CryCommon/StlUtils.h>

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrackBundle::AppendTrack(CUiAnimViewTrack* pTrack)
{
    // Check if newly added key has different type than existing ones
    if (m_bAllOfSameType && m_tracks.size() > 0)
    {
        const CUiAnimViewTrack* pLastTrack = m_tracks.back();

        if (pTrack->GetParameterType() != pLastTrack->GetParameterType()
            || pTrack->GetCurveType() != pLastTrack->GetCurveType()
            || pTrack->GetValueType() != pLastTrack->GetValueType())
        {
            m_bAllOfSameType = false;
        }
    }

    stl::push_back_unique(m_tracks, pTrack);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrackBundle::AppendTrackBundle(const CUiAnimViewTrackBundle& bundle)
{
    for (auto iter = bundle.m_tracks.begin(); iter != bundle.m_tracks.end(); ++iter)
    {
        AppendTrack(*iter);
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrack::CUiAnimViewTrack(IUiAnimTrack* pTrack, CUiAnimViewAnimNode* pTrackAnimNode,
    CUiAnimViewNode* pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
    : CUiAnimViewNode(pParentNode)
    , m_pAnimTrack(pTrack)
    , m_pTrackAnimNode(pTrackAnimNode)
    , m_bIsSubTrack(bIsSubTrack)
    , m_subTrackIndex(subTrackIndex)
{
    // Search for child tracks
    const unsigned int subTrackCount = m_pAnimTrack->GetSubTrackCount();
    for (unsigned int subTrackI = 0; subTrackI < subTrackCount; ++subTrackI)
    {
        IUiAnimTrack* pSubTrack = m_pAnimTrack->GetSubTrack(subTrackI);

        CUiAnimViewTrackFactory trackFactory;
        CUiAnimViewTrack* pNewUiAVTrack = trackFactory.BuildTrack(pSubTrack, pTrackAnimNode, this, true, subTrackI);
        m_childNodes.push_back(std::unique_ptr<CUiAnimViewNode>(pNewUiAVTrack));
    }

    m_bIsCompoundTrack = subTrackCount > 0;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode* CUiAnimViewTrack::GetAnimNode() const
{
    return m_pTrackAnimNode;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::SnapTimeToPrevKey(float& time) const
{
    CUiAnimViewKeyHandle prevKey = const_cast<CUiAnimViewTrack*>(this)->GetPrevKey(time);

    if (prevKey.IsValid())
    {
        time = prevKey.GetTime();
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::SnapTimeToNextKey(float& time) const
{
    CUiAnimViewKeyHandle prevKey = const_cast<CUiAnimViewTrack*>(this)->GetNextKey(time);

    if (prevKey.IsValid())
    {
        time = prevKey.GetTime();
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetPrevKey(const float time)
{
    CUiAnimViewKeyHandle keyHandle;

    const float startTime = time;
    float closestTime = -std::numeric_limits<float>::max();

    const int numKeys = m_pAnimTrack->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime < startTime && keyTime > closestTime)
        {
            keyHandle = CUiAnimViewKeyHandle(this, i);
            closestTime = keyTime;
        }
    }

    return keyHandle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetNextKey(const float time)
{
    CUiAnimViewKeyHandle keyHandle;

    const float startTime = time;
    float closestTime = std::numeric_limits<float>::max();

    const int numKeys = m_pAnimTrack->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime > startTime && keyTime < closestTime)
        {
            keyHandle = CUiAnimViewKeyHandle(this, i);
            closestTime = keyTime;
        }
    }

    return keyHandle;
}


//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewTrack::GetSelectedKeys()
{
    CUiAnimViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
        }
    }
    else
    {
        bundle = GetKeys(true, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewTrack::GetAllKeys()
{
    CUiAnimViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            bundle.AppendKeyBundle((*iter)->GetAllKeys());
        }
    }
    else
    {
        bundle = GetKeys(false, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewTrack::GetKeysInTimeRange(const float t0, const float t1)
{
    CUiAnimViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
        }
    }
    else
    {
        bundle = GetKeys(false, t0, t1);
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewTrack::GetKeys(const bool bOnlySelected, const float t0, const float t1)
{
    CUiAnimViewKeyBundle bundle;

    const int keyCount = m_pAnimTrack->GetNumKeys();
    for (int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(keyIndex);
        const bool timeRangeOk = (t0 <= keyTime && keyTime <= t1);

        if ((!bOnlySelected || IsKeySelected(keyIndex)) && timeRangeOk)
        {
            CUiAnimViewKeyHandle keyHandle(this, keyIndex);
            bundle.AppendKey(keyHandle);
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::CreateKey(const float time)
{
    const int keyIndex = m_pAnimTrack->CreateKey(time);
    GetSequence()->OnKeysChanged();
    return CUiAnimViewKeyHandle(this, keyIndex);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SlideKeys(const float time0, const float timeOffset)
{
    for (int i = 0; i < m_pAnimTrack->GetNumKeys(); ++i)
    {
        float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime >= time0)
        {
            m_pAnimTrack->SetKeyTime(i, keyTime + timeOffset);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::OffsetKeyPosition(const AZ::Vector3& offset)
{
    UiAnimUndo::Record(new CUndoTrackObject(this, GetSequence()));
    m_pAnimTrack->OffsetKeyPosition(offset);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetKey(unsigned int index)
{
    if (index < GetKeyCount())
    {
        return CUiAnimViewKeyHandle(this, index);
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyConstHandle CUiAnimViewTrack::GetKey(unsigned int index) const
{
    if (index < GetKeyCount())
    {
        return CUiAnimViewKeyConstHandle(this, index);
    }

    return CUiAnimViewKeyConstHandle();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetKeyByTime(const float time)
{
    if (m_bIsCompoundTrack)
    {
        // Search key in sub tracks
        unsigned int currentIndex = 0;

        unsigned int childCount = GetChildCount();
        for (unsigned int i = 0; i < childCount; ++i)
        {
            CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(GetChild(i));

            int keyIndex = pChildTrack->m_pAnimTrack->FindKey(time);
            if (keyIndex >= 0)
            {
                return CUiAnimViewKeyHandle(this, currentIndex + keyIndex);
            }

            currentIndex += pChildTrack->GetKeyCount();
        }
    }

    int keyIndex = m_pAnimTrack->FindKey(time);

    if (keyIndex < 0)
    {
        return CUiAnimViewKeyHandle();
    }

    return CUiAnimViewKeyHandle(this, keyIndex);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetNearestKeyByTime(const float time)
{
    int minDelta = std::numeric_limits<int>::max();

    const unsigned int keyCount = GetKeyCount();
    for (unsigned int i = 0; i < keyCount; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = GetKey(i);

        const int deltaT = abs((int)keyHandle.GetTime() - (int)time);

        // If deltaT got larger since last key, then the last key
        // was the key with minimum temporal distance to the given time
        if (deltaT > minDelta)
        {
            return CUiAnimViewKeyHandle(this, i - 1);
        }

        minDelta = std::min(minDelta, deltaT);
    }

    // If we didn't return above and there are keys, then the
    // last key needs to be the one with minimum distance
    if (keyCount > 0)
    {
        return CUiAnimViewKeyHandle(this, keyCount - 1);
    }

    // No keys
    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::GetKeyValueRange(float& min, float& max) const
{
    m_pAnimTrack->GetKeyValueRange(min, max);
}

//////////////////////////////////////////////////////////////////////////
ColorB CUiAnimViewTrack::GetCustomColor() const
{
    return m_pAnimTrack->GetCustomColor();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetCustomColor(ColorB color)
{
    m_pAnimTrack->SetCustomColor(color);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::HasCustomColor() const
{
    return m_pAnimTrack->HasCustomColor();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::ClearCustomColor()
{
    m_pAnimTrack->ClearCustomColor();
}

//////////////////////////////////////////////////////////////////////////
IUiAnimTrack::EUiAnimTrackFlags CUiAnimViewTrack::GetFlags() const
{
    return (IUiAnimTrack::EUiAnimTrackFlags)m_pAnimTrack->GetFlags();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackMemento CUiAnimViewTrack::GetMemento() const
{
    IUiAnimationSystem* uiAnimationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(uiAnimationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    CUiAnimViewTrackMemento memento;
    memento.m_serializedTrackState = XmlHelpers::CreateXmlNode("TrackState");
    m_pAnimTrack->Serialize(uiAnimationSystem, memento.m_serializedTrackState, false);
    return memento;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::RestoreFromMemento(const CUiAnimViewTrackMemento& memento)
{
    IUiAnimationSystem* uiAnimationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(uiAnimationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    // We're going to de-serialize, so this is const safe
    XmlNodeRef& xmlNode = const_cast<XmlNodeRef&>(memento.m_serializedTrackState);
    m_pAnimTrack->Serialize(uiAnimationSystem, xmlNode, true);
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CUiAnimViewTrack::GetName() const
{
    CUiAnimViewNode* pParentNode = GetParentNode();

    if (pParentNode->GetNodeType() == eUiAVNT_Track)
    {
        CUiAnimViewTrack* pParentTrack = static_cast<CUiAnimViewTrack*>(pParentNode);
        return pParentTrack->m_pAnimTrack->GetSubTrackName(m_subTrackIndex);
    }

    // !!! Az specific, maybe better to get name from track directly if we can
    // store a name in the track,
    if (GetParameterType() == eUiAnimParamType_AzComponentField)
    {
        return GetAnimNode()->GetParamNameForTrack(GetParameterType(), m_pAnimTrack.get());
    }

    return GetAnimNode()->GetParamName(GetParameterType());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetDisabled(bool bDisabled)
{
    if (bDisabled)
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IUiAnimTrack::eUiAnimTrackFlags_Disabled);
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Disabled);
    }
    else
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IUiAnimTrack::eUiAnimTrackFlags_Disabled);
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Enabled);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::IsDisabled() const
{
    return m_pAnimTrack->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Disabled;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetMuted(bool bMuted)
{
    if (bMuted)
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IUiAnimTrack::eUiAnimTrackFlags_Muted);
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Muted);
    }
    else
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IUiAnimTrack::eUiAnimTrackFlags_Muted);
        GetSequence()->OnNodeChanged(this, IUiAnimViewSequenceListener::eNodeChangeType_Unmuted);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::IsMuted() const
{
    return m_pAnimTrack->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Muted;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetKey(unsigned int keyIndex, IKey* pKey)
{
    m_pAnimTrack->SetKey(keyIndex, pKey);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::GetKey(unsigned int keyIndex, IKey* pKey) const
{
    m_pAnimTrack->GetKey(keyIndex, pKey);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SelectKey(unsigned int keyIndex, bool bSelect)
{
    const bool bWasSelected = m_pAnimTrack->IsKeySelected(keyIndex);

    m_pAnimTrack->SelectKey(keyIndex, bSelect);

    if (bSelect != bWasSelected)
    {
        m_pTrackAnimNode->GetSequence()->OnKeySelectionChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetKeyTime(const int index, const float time)
{
    const float bOldTime = m_pAnimTrack->GetKeyTime(index);

    m_pAnimTrack->SetKeyTime(index, time);

    if (bOldTime != time)
    {
        m_pTrackAnimNode->GetSequence()->OnKeysChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewTrack::GetKeyTime(const int index) const
{
    return m_pAnimTrack->GetKeyTime(index);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::RemoveKey(const int index)
{
    m_pAnimTrack->RemoveKey(index);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewTrack::CloneKey(const int index)
{
    int newIndex = m_pAnimTrack->CloneKey(index);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
    return newIndex;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SelectKeys(const bool bSelected)
{
    m_pTrackAnimNode->GetSequence()->QueueNotifications();

    if (!m_bIsCompoundTrack)
    {
        unsigned int keyCount = GetKeyCount();
        for (unsigned int i = 0; i < keyCount; ++i)
        {
            m_pAnimTrack->SelectKey(i, bSelected);
            m_pTrackAnimNode->GetSequence()->OnKeySelectionChanged();
        }
    }
    else
    {
        // Affect sub tracks
        unsigned int childCount = GetChildCount();
        for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
        {
            CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(GetChild(childIndex));
            pChildTrack->SelectKeys(bSelected);
            m_pTrackAnimNode->GetSequence()->OnKeySelectionChanged();
        }
    }

    m_pTrackAnimNode->GetSequence()->SubmitPendingNotifcations();
}


//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrack::IsKeySelected(unsigned int keyIndex) const
{
    if (m_pAnimTrack)
    {
        return m_pAnimTrack->IsKeySelected(keyIndex);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewTrack::GetSubTrackKeyHandle(unsigned int index) const
{
    // Return handle to sub track key
    unsigned int childCount = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        CUiAnimViewTrack* pChildTrack = static_cast<CUiAnimViewTrack*>(GetChild(childIndex));

        const unsigned int childKeyCount = pChildTrack->GetKeyCount();
        if (index < childKeyCount)
        {
            return pChildTrack->GetKey(index);
        }

        index -= childKeyCount;
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::SetAnimationLayerIndex(const int index)
{
    if (m_pAnimTrack)
    {
        m_pAnimTrack->SetAnimationLayerIndex(index);
    }
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewTrack::GetAnimationLayerIndex() const
{
    return m_pAnimTrack->GetAnimationLayerIndex();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    if (bOnlyFromSelectedTracks && !IsSelected())
    {
        return;
    }

    if (GetKeyCount() == 0)
    {
        return;
    }

    if (bOnlySelectedKeys)
    {
        CUiAnimViewKeyBundle keyBundle = GetSelectedKeys();
        if (keyBundle.GetKeyCount() == 0)
        {
            return;
        }
    }

    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    XmlNodeRef childNode = xmlNode->newChild("Track");
    childNode->setAttr("name", GetName().c_str());
    GetParameterType().Serialize(animationSystem, childNode, false);
    childNode->setAttr("valueType", GetValueType());

    m_pAnimTrack->SerializeSelection(childNode, false, bOnlySelectedKeys);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrack::PasteKeys(XmlNodeRef xmlNode, const float timeOffset)
{
    assert(UiAnimUndo::IsRecording());

    CUiAnimViewSequence* pSequence = GetSequence();

    UiAnimUndo::Record(new CUndoTrackObject(this, pSequence));
    m_pAnimTrack->SerializeSelection(xmlNode, true, true, timeOffset);
    UiAnimUndo::Record(new CUndoAnimKeySelection(pSequence));
}
