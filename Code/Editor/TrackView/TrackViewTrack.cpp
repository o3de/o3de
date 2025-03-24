/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewTrack.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "TrackViewSequence.h"
#include "TrackViewNodeFactories.h"


void CTrackViewTrackBundle::AppendTrack(CTrackViewTrack* pTrack)
{
    if (!pTrack)
    {
        AZ_Assert(false, "Expected valid track pointer.");
        return;
    }

    // Check if newly added key has different type than existing ones
    if (m_bAllOfSameType && m_tracks.size() > 0)
    {
        const CTrackViewTrack* pLastTrack = m_tracks.back();

        if (pTrack->GetParameterType() != pLastTrack->GetParameterType()
            || pTrack->GetCurveType() != pLastTrack->GetCurveType()
            || pTrack->GetValueType() != pLastTrack->GetValueType())
        {
            m_bAllOfSameType = false;
        }
    }

    stl::push_back_unique(m_tracks, pTrack);
}

void CTrackViewTrackBundle::AppendTrackBundle(const CTrackViewTrackBundle& bundle)
{
    for (auto iter = bundle.m_tracks.begin(); iter != bundle.m_tracks.end(); ++iter)
    {
        AppendTrack(*iter);
    }
}

bool CTrackViewTrackBundle::RemoveTrack(CTrackViewTrack* trackToRemove)
{
    if (!trackToRemove)
    {
        AZ_Assert(false, "Expected valid track pointer.");
        return false;
    }

    return stl::find_and_erase(m_tracks, trackToRemove);
}

CTrackViewTrack::CTrackViewTrack(IAnimTrack* pTrack, CTrackViewAnimNode* pTrackAnimNode,
    CTrackViewNode* pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
    : CTrackViewNode(pParentNode)
    , m_pAnimTrack(pTrack)
    , m_pTrackAnimNode(pTrackAnimNode)
    , m_bIsSubTrack(bIsSubTrack)
    , m_subTrackIndex(subTrackIndex)
{
    // Search for child tracks
    const unsigned int subTrackCount = m_pAnimTrack->GetSubTrackCount();
    for (unsigned int subTrackI = 0; subTrackI < subTrackCount; ++subTrackI)
    {
        IAnimTrack* pSubTrack = m_pAnimTrack->GetSubTrack(subTrackI);

        CTrackViewTrackFactory trackFactory;
        CTrackViewTrack* pNewTVTrack = trackFactory.BuildTrack(pSubTrack, pTrackAnimNode, this, true, subTrackI);
        m_childNodes.push_back(AZStd::unique_ptr<CTrackViewNode>(pNewTVTrack));
    }

    m_bIsCompoundTrack = subTrackCount > 0;

    // Connect bus to listen for OnStart/StopPlayInEditor events
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
}

CTrackViewTrack::~CTrackViewTrack()
{
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
}

CTrackViewAnimNode* CTrackViewTrack::GetAnimNode() const
{
    return m_pTrackAnimNode;
}

bool CTrackViewTrack::SnapTimeToPrevKey(float& time) const
{
    CTrackViewKeyHandle prevKey = const_cast<CTrackViewTrack*>(this)->GetPrevKey(time);

    if (prevKey.IsValid())
    {
        time = prevKey.GetTime();
        return true;
    }

    return false;
}

bool CTrackViewTrack::SnapTimeToNextKey(float& time) const
{
    CTrackViewKeyHandle prevKey = const_cast<CTrackViewTrack*>(this)->GetNextKey(time);

    if (prevKey.IsValid())
    {
        time = prevKey.GetTime();
        return true;
    }

    return false;
}

void CTrackViewTrack::SetExpanded(bool expanded)
{
    const auto pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || !m_bIsCompoundTrack)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(m_bIsCompoundTrack, "Track is not compound.");
        return;
    }

    if (GetExpanded() == expanded)
    {
        return; // nothing to do
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>(expanded ? "Expand Sub-Tracks" : "Collapse Sub-Tracks");
    }

    m_pAnimTrack->SetExpanded(expanded);

    if (expanded)
    {
        pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Expanded);
    }
    else
    {
        pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Collapsed);
    }

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

bool CTrackViewTrack::GetExpanded() const
{
    return (m_pAnimTrack) ? m_pAnimTrack->GetExpanded() : false;
}

CTrackViewKeyHandle CTrackViewTrack::GetPrevKey(float time)
{
    CTrackViewKeyHandle keyHandle;
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return keyHandle;
    }

#ifdef max
#undef max
#endif

    const float startTime = time;
    float closestTime = -std::numeric_limits<float>::max();

    const int numKeys = m_pAnimTrack->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime < startTime && keyTime > closestTime)
        {
            keyHandle = CTrackViewKeyHandle(this, i);
            closestTime = keyTime;
        }
    }

    return keyHandle;
}

CTrackViewKeyHandle CTrackViewTrack::GetNextKey(float time)
{
    CTrackViewKeyHandle keyHandle;
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return keyHandle;
    }

    const float startTime = time;
    float closestTime = std::numeric_limits<float>::max();

    const int numKeys = m_pAnimTrack->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime > startTime && keyTime < closestTime)
        {
            keyHandle = CTrackViewKeyHandle(this, i);
            closestTime = keyTime;
        }
    }

    return keyHandle;
}


CTrackViewKeyBundle CTrackViewTrack::GetSelectedKeys()
{
    CTrackViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            if (*iter)
            {
                bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
            }
        }
    }
    else
    {
        bundle = GetKeys(true, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewTrack::GetAllKeys()
{
    CTrackViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            if (*iter)
            {
                bundle.AppendKeyBundle((*iter)->GetAllKeys());
            }
        }
    }
    else
    {
        bundle = GetKeys(false, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewTrack::GetKeysInTimeRange(float t0, float t1)
{
    CTrackViewKeyBundle bundle;

    if (m_bIsCompoundTrack)
    {
        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            if (*iter)
            {
                bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
            }
        }
    }
    else
    {
        bundle = GetKeys(false, t0, t1);
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewTrack::GetKeys(bool bOnlySelected, float t0, float t1)
{
    CTrackViewKeyBundle bundle;

    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return bundle;
    }

    const int keyCount = m_pAnimTrack->GetNumKeys();
    for (int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
    {
        const float keyTime = m_pAnimTrack->GetKeyTime(keyIndex);
        const bool timeRangeOk = (t0 <= keyTime && keyTime <= t1);

        if (timeRangeOk && (!bOnlySelected || IsKeySelected(keyIndex)))
        {
            CTrackViewKeyHandle keyHandle(this, keyIndex);
            bundle.AppendKey(keyHandle);
        }
    }

    return bundle;
}

CTrackViewKeyHandle CTrackViewTrack::CreateKey(float time)
{
    CTrackViewKeyHandle keyHandle;

    const auto pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return keyHandle;
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Create Key in Track");
    }

    const int keyIndex = m_pAnimTrack->CreateKey(time);

    if (keyIndex < 0)
    {
        AZ_Error("CTrackViewTrack", false, "CreateKey(%f): no keys added to %s", time, GetName().c_str());
        undoBatch.reset();
        return keyHandle;
    }

    pSequence->OnKeysChanged();
    CTrackViewKeyHandle createdKeyHandle(this, keyIndex);
    pSequence->OnKeyAdded(createdKeyHandle);

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }

    return createdKeyHandle;
}

void CTrackViewTrack::SlideKeys(float time0, float timeOffset)
{
    const auto pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return;
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Slide Keys In Track");
    }

    for (int i = 0; i < m_pAnimTrack->GetNumKeys(); ++i)
    {
        float keyTime = m_pAnimTrack->GetKeyTime(i);
        if (keyTime >= time0)
        {
            m_pAnimTrack->SetKeyTime(i, keyTime + timeOffset);
        }
    }

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

void CTrackViewTrack::UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM)
{
    const CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return;
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Update Key Data After Parent Changed");
    }

    m_pAnimTrack->UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

CTrackViewKeyHandle CTrackViewTrack::GetKey(unsigned int keyIndex)
{
    if (keyIndex < GetKeyCount())
    {
        return CTrackViewKeyHandle(this, keyIndex);
    }

    AZ_Assert(false, "Key index out of range (0 .. %u).", GetKeyCount());
    return CTrackViewKeyHandle();
}

CTrackViewKeyConstHandle CTrackViewTrack::GetKey(unsigned int keyIndex) const
{
    if (keyIndex < GetKeyCount())
    {
        return CTrackViewKeyConstHandle(this, keyIndex);
    }

    AZ_Assert(false, "Key index out of range (0 .. %u).", GetKeyCount());
    return CTrackViewKeyConstHandle();
}

CTrackViewKeyHandle CTrackViewTrack::GetKeyByTime(float time)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return CTrackViewKeyHandle();
    }

    if (m_bIsCompoundTrack)
    {
        // Search key in sub tracks
        unsigned int currentIndex = 0;

        unsigned int childCount = GetChildCount();
        for (unsigned int i = 0; i < childCount; ++i)
        {
            if (CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(i)))
            {
                int keyIndex = pChildTrack->m_pAnimTrack->FindKey(time);
                if (keyIndex >= 0)
                {
                    return CTrackViewKeyHandle(this, currentIndex + keyIndex);
                }

                currentIndex += pChildTrack->GetKeyCount();
            }
        }
    }

    int keyIndex = m_pAnimTrack->FindKey(time);

    if (keyIndex < 0)
    {
        return CTrackViewKeyHandle();
    }

    return CTrackViewKeyHandle(this, keyIndex);
}

CTrackViewKeyHandle CTrackViewTrack::GetNearestKeyByTime(float time)
{
    float minDelta = std::numeric_limits<float>::max();

    const unsigned int keyCount = GetKeyCount();
    for (unsigned int i = 0; i < keyCount; ++i)
    {
        const auto deltaT = AZStd::abs(GetKey(i).GetTime() - time);

        // If deltaT got larger since last key, then the last key
        // was the key with minimum temporal distance to the given time
        if (deltaT > minDelta)
        {
            return CTrackViewKeyHandle(this, i - 1);
        }

        minDelta = AZStd::min(minDelta, deltaT);
    }

    // If we didn't return above and there are keys, then the
    // last key needs to be the one with minimum distance
    if (keyCount > 0)
    {
        return CTrackViewKeyHandle(this, keyCount - 1);
    }

    // No keys
    return CTrackViewKeyHandle();
}

void CTrackViewTrack::GetKeyValueRange(float& min, float& max) const
{
    min = 0;
    max = 0;
    if (m_pAnimTrack)
    {
        m_pAnimTrack->GetKeyValueRange(min, max);
    }
}

ColorB CTrackViewTrack::GetCustomColor() const
{
    return (m_pAnimTrack) ? m_pAnimTrack->GetCustomColor() : ColorB();
}

void CTrackViewTrack::SetCustomColor(ColorB color)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    m_pAnimTrack->SetCustomColor(color);
}

bool CTrackViewTrack::HasCustomColor() const
{
    return (m_pAnimTrack) ? m_pAnimTrack->HasCustomColor() : false;
}

void CTrackViewTrack::ClearCustomColor()
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    m_pAnimTrack->ClearCustomColor();
}

IAnimTrack::EAnimTrackFlags CTrackViewTrack::GetFlags() const
{
    return static_cast<IAnimTrack::EAnimTrackFlags>((m_pAnimTrack) ? m_pAnimTrack->GetFlags() : 0);
}

CTrackViewTrackMemento CTrackViewTrack::GetMemento() const
{
    CTrackViewTrackMemento memento;
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return memento;
    }

    memento.m_serializedTrackState = XmlHelpers::CreateXmlNode("TrackState");
    m_pAnimTrack->Serialize(memento.m_serializedTrackState, false);
    return memento;
}

void CTrackViewTrack::RestoreFromMemento(const CTrackViewTrackMemento& memento)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    // We're going to deserialize, so this is const safe
    XmlNodeRef& xmlNode = const_cast<XmlNodeRef&>(memento.m_serializedTrackState);
    m_pAnimTrack->Serialize(xmlNode, true);
}

AZStd::string CTrackViewTrack::GetName() const
{
    CTrackViewNode* pParentNode = GetParentNode();
    if (!pParentNode || !m_pTrackAnimNode)
    {
        AZ_Assert(pParentNode, "Invalid parent node.");
        AZ_Assert(m_pTrackAnimNode, "Invalid animation node.");
        return AZStd::string();
    }

    if (pParentNode->GetNodeType() == eTVNT_Track)
    {
        CTrackViewTrack* pParentTrack = static_cast<CTrackViewTrack*>(pParentNode);
        if (!pParentTrack->m_pAnimTrack)
        {
            AZ_Assert(false, "Invalid AnimTrack in parent node.");
            return AZStd::string();
        }
        return pParentTrack->m_pAnimTrack->GetSubTrackName(m_subTrackIndex);
    }

    return m_pTrackAnimNode->GetParamName(GetParameterType());
}

void CTrackViewTrack::SetDisabled(bool bDisabled)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return;
    }

    if (bDisabled)
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IAnimTrack::eAnimTrackFlags_Disabled);
        pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Disabled);
    }
    else
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IAnimTrack::eAnimTrackFlags_Disabled);
        pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Enabled);
    }
}

bool CTrackViewTrack::IsDisabled() const
{
    return (m_pAnimTrack) ? m_pAnimTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled : false;
}

void CTrackViewTrack::SetMuted(bool bMuted)
{
    const auto pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return;
    }

    if (UsesMute())
    {
        if (bMuted)
        {
            m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IAnimTrack::eAnimTrackFlags_Muted);
            pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Muted);
        }
        else
        {
            m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IAnimTrack::eAnimTrackFlags_Muted);
            pSequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Unmuted);
        }
    }
}

// Returns whether the track is muted, or false if the track does not use muting
bool CTrackViewTrack::IsMuted() const
{
    if (m_pAnimTrack)
    {
        return m_pAnimTrack->UsesMute() ? (m_pAnimTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted) : false;
    }
    return false;
}

void CTrackViewTrack::SetKey(unsigned int keyIndex, IKey* pKey)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || !pKey)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(pKey, "Expected valid key pointer.");
        return;
    }

    m_pAnimTrack->SetKey(keyIndex, pKey);
    pSequence->OnKeysChanged();
}

void CTrackViewTrack::GetKey(unsigned int keyIndex, IKey* pKey) const
{
    if (!m_pAnimTrack || !pKey)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(pKey, "Expected valid key pointer.");
        return;
    }

    m_pAnimTrack->GetKey(keyIndex, pKey);
}

void CTrackViewTrack::SelectKey(unsigned int keyIndex, bool bSelect)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || (keyIndex >= GetKeyCount()))
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(keyIndex < GetKeyCount(), "Key index out of range (0 .. %u).", GetKeyCount());
        return;
    }

    const bool bWasSelected = m_pAnimTrack->IsKeySelected(keyIndex);

    m_pAnimTrack->SelectKey(keyIndex, bSelect);

    if (bSelect != bWasSelected)
    {
        pSequence->OnKeySelectionChanged();
    }
}

void CTrackViewTrack::SetKeyTime(unsigned int keyIndex, float time, bool notifyListeners)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || (keyIndex >= GetKeyCount()))
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(keyIndex < GetKeyCount(), "Key index out of range (0 .. %u).", GetKeyCount());
        return;
    }

    const float bOldTime = m_pAnimTrack->GetKeyTime(keyIndex);

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Set Key Time");
    }

    m_pAnimTrack->SetKeyTime(keyIndex, time);

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }

    if (notifyListeners && (bOldTime != time))
    {
        // The keys were just made invalid by the above SetKeyTime(), so sort them now
        // to make sure they are ready to be used. Only do this when notifyListeners
        // is set so client callers can batch up a bunch of SetKeyTime calls if desired.
        m_pAnimTrack->SortKeys();

        m_pTrackAnimNode->GetSequence()->OnKeysChanged();
    }
}

float CTrackViewTrack::GetKeyTime(unsigned int keyIndex) const
{
    if (!m_pAnimTrack || (keyIndex >= GetKeyCount()))
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(keyIndex < GetKeyCount(), "Key index out of range (0 .. %u).", GetKeyCount());
        return -1.0f;
    }

    return m_pAnimTrack->GetKeyTime(keyIndex);
}

void CTrackViewTrack::RemoveKey(unsigned int keyIndex)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || (keyIndex >= GetKeyCount()))
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(keyIndex < GetKeyCount(), "Key index out of range (0 .. %u).", GetKeyCount());
        return;
    }

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Remove Key From Track");
    }

    m_pAnimTrack->RemoveKey(keyIndex);

    pSequence->OnKeysChanged();

    if (undoBatch)
    {
        undoBatch->MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

int CTrackViewTrack::CloneKey(unsigned int keyIndex, float timeOffset)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence || (keyIndex >= GetKeyCount()))
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        AZ_Assert(keyIndex < GetKeyCount(), "Key index out of range (0 .. %u).", GetKeyCount());
        return -1;
    }

    int newIndex = m_pAnimTrack->CloneKey(keyIndex, timeOffset);
    pSequence->OnKeysChanged();

    return newIndex;
}

void CTrackViewTrack::SelectKeys(bool bSelected)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    pSequence->QueueNotifications();

    if (!m_bIsCompoundTrack)
    {
        unsigned int keyCount = GetKeyCount();
        for (unsigned int i = 0; i < keyCount; ++i)
        {
            m_pAnimTrack->SelectKey(i, bSelected);
        }
    }
    else
    {
        // Affect sub tracks
        unsigned int childCount = GetChildCount();
        for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
        {
            if (CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(childIndex)))
            {
                pChildTrack->SelectKeys(bSelected);
            }
        }
    }

    pSequence->OnKeySelectionChanged();

    pSequence->SubmitPendingNotifications();
}


bool CTrackViewTrack::IsKeySelected(unsigned int keyIndex) const
{
    return (m_pAnimTrack) ? m_pAnimTrack->IsKeySelected(keyIndex) : false;
}

void CTrackViewTrack::SetSortMarkerKey(unsigned int keyIndex, bool enabled)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    return m_pAnimTrack->SetSortMarkerKey(keyIndex, enabled);
}

bool CTrackViewTrack::IsSortMarkerKey(unsigned int keyIndex) const
{
    return (m_pAnimTrack) ? m_pAnimTrack->IsSortMarkerKey(keyIndex) : false;
}

CTrackViewKeyHandle CTrackViewTrack::GetSubTrackKeyHandle(unsigned int keyIndex) const
{
    // Return handle to sub track key
    unsigned int childCount = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        if (CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(childIndex)))
        {
            const auto childKeyCount = pChildTrack->GetKeyCount();
            if (keyIndex < childKeyCount)
            {
                return pChildTrack->GetKey(keyIndex);
            }

            keyIndex -= childKeyCount;
        }
    }

    return CTrackViewKeyHandle();
}

void CTrackViewTrack::SetAnimationLayerIndex(int index)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    m_pAnimTrack->SetAnimationLayerIndex(index);
}

int CTrackViewTrack::GetAnimationLayerIndex() const
{
    return m_pAnimTrack->GetAnimationLayerIndex();
}

void CTrackViewTrack::OnStartPlayInEditor()
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    // remap any AZ::EntityId's used in tracks
    // OnStopPlayInEditor clears this as well, but we clear it here in case OnStartPlayInEditor() is called multiple times before OnStopPlayInEditor()
    m_paramTypeToStashedEntityIdMap.clear();

    CAnimParamType trackParamType = m_pAnimTrack->GetParameterType();
    const AnimParamType paramType = trackParamType.GetType();
    if (paramType == AnimParamType::Camera || paramType == AnimParamType::Sequence)
    {
        ISelectKey selectKey;
        ISequenceKey sequenceKey;
        IKey* key = nullptr;

        for (int i = 0; i < m_pAnimTrack->GetNumKeys(); i++)
        {
            AZ::EntityId entityIdToRemap;

            if (paramType == AnimParamType::Camera)
            {
                m_pAnimTrack->GetKey(i, &selectKey);
                entityIdToRemap = selectKey.cameraAzEntityId;
                key = &selectKey;
            }
            else if (paramType == AnimParamType::Sequence)
            {
                m_pAnimTrack->GetKey(i, &sequenceKey);
                entityIdToRemap = sequenceKey.sequenceEntityId;
                key = &sequenceKey;
            }

            // stash the entity Id for restore in OnStopPlayInEditor
            m_paramTypeToStashedEntityIdMap[trackParamType].push_back(entityIdToRemap);

            if (entityIdToRemap.IsValid())
            {
                AZ::EntityId remappedId;
                AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, entityIdToRemap, remappedId);

                // remap
                if (paramType == AnimParamType::Camera)
                {
                    selectKey.cameraAzEntityId = remappedId;
                }
                else if (paramType == AnimParamType::Sequence)
                {
                    sequenceKey.sequenceEntityId = remappedId;
                }
                m_pAnimTrack->SetKey(i, key);
            }
        }
    }
}
void CTrackViewTrack::OnStopPlayInEditor()
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

    // restore any AZ::EntityId's remapped in OnStartPlayInEditor
    if (m_paramTypeToStashedEntityIdMap.size())
    {
        CAnimParamType trackParamType = m_pAnimTrack->GetParameterType();
        const AnimParamType paramType = trackParamType.GetType();

        if (paramType == AnimParamType::Camera || paramType == AnimParamType::Sequence)
        {
            for (int i = 0; i < m_pAnimTrack->GetNumKeys(); i++)
            {
                ISelectKey selectKey;
                ISequenceKey sequenceKey;
                IKey* key = nullptr;

                // restore entityIds
                if (paramType == AnimParamType::Camera)
                {
                    m_pAnimTrack->GetKey(i, &selectKey);
                    selectKey.cameraAzEntityId = m_paramTypeToStashedEntityIdMap[trackParamType][i];
                    key = &selectKey;
                }
                else if (paramType == AnimParamType::Sequence)
                {
                    m_pAnimTrack->GetKey(i, &sequenceKey);
                    sequenceKey.sequenceEntityId = m_paramTypeToStashedEntityIdMap[trackParamType][i];
                    key = &sequenceKey;
                }
                m_pAnimTrack->SetKey(i, key);
            }
        }
        // clear the StashedEntityIdMap now that we've consumed it
        m_paramTypeToStashedEntityIdMap.clear();
    }
}

void CTrackViewTrack::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    if (!m_pAnimTrack)
    {
        AZ_Assert(false, "Invalid AnimTrack.");
        return;
    }

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
        CTrackViewKeyBundle keyBundle = GetSelectedKeys();
        if (keyBundle.GetKeyCount() == 0)
        {
            return;
        }
    }

    XmlNodeRef childNode = xmlNode->newChild("Track");
    childNode->setAttr("name", GetName().c_str());
    GetParameterType().SaveToXml(childNode);
    childNode->setAttr("valueType", static_cast<int>(GetValueType()));

    m_pAnimTrack->SerializeSelection(childNode, false, bOnlySelectedKeys);
}

void CTrackViewTrack::PasteKeys(XmlNodeRef xmlNode, const float timeOffset)
{
    CTrackViewSequence* pSequence = GetSequence();
    if (!m_pAnimTrack || !pSequence)
    {
        AZ_Assert(m_pAnimTrack.get(), "Invalid AnimTrack.");
        return;
    }

    m_pAnimTrack->SerializeSelection(xmlNode, true, true, timeOffset);
}
