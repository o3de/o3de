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
    if (m_pAnimTrack)
    {
        CTrackViewSequence* sequence = GetSequence();
        if (nullptr != sequence)
        {
            if (GetExpanded() != expanded)
            {
                m_pAnimTrack->SetExpanded(expanded);

                if (expanded)
                {
                    sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Expanded);
                }
                else
                {
                    sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Collapsed);
                }
            }
        }
    }
}

bool CTrackViewTrack::GetExpanded() const
{
    return (m_pAnimTrack) ? m_pAnimTrack->GetExpanded() : false;
}

CTrackViewKeyHandle CTrackViewTrack::GetPrevKey(const float time)
{
    CTrackViewKeyHandle keyHandle;

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

CTrackViewKeyHandle CTrackViewTrack::GetNextKey(const float time)
{
    CTrackViewKeyHandle keyHandle;

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
            bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
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
            bundle.AppendKeyBundle((*iter)->GetAllKeys());
        }
    }
    else
    {
        bundle = GetKeys(false, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    return bundle;
}

CTrackViewKeyBundle CTrackViewTrack::GetKeysInTimeRange(const float t0, const float t1)
{
    CTrackViewKeyBundle bundle;

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

CTrackViewKeyBundle CTrackViewTrack::GetKeys(const bool bOnlySelected, const float t0, const float t1)
{
    CTrackViewKeyBundle bundle;

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

CTrackViewKeyHandle CTrackViewTrack::CreateKey(const float time)
{
    const int keyIndex = m_pAnimTrack->CreateKey(time);
    GetSequence()->OnKeysChanged();
    CTrackViewKeyHandle createdKeyHandle(this, keyIndex);
    GetSequence()->OnKeyAdded(createdKeyHandle);

    return createdKeyHandle;
}

void CTrackViewTrack::SlideKeys(const float time0, const float timeOffset)
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

void CTrackViewTrack::UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM)
{
    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;

    if (!AzToolsFramework::UndoRedoOperationInProgress())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Update Key Data After Parent Changed");
    }

    m_pAnimTrack->UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);

    if (undoBatch.get())
    {
        undoBatch->MarkEntityDirty(GetSequence()->GetSequenceComponentEntityId());
    }
}

CTrackViewKeyHandle CTrackViewTrack::GetKey(unsigned int index)
{
    if (index < GetKeyCount())
    {
        return CTrackViewKeyHandle(this, index);
    }

    return CTrackViewKeyHandle();
}

CTrackViewKeyConstHandle CTrackViewTrack::GetKey(unsigned int index) const
{
    if (index < GetKeyCount())
    {
        return CTrackViewKeyConstHandle(this, index);
    }

    return CTrackViewKeyConstHandle();
}

CTrackViewKeyHandle CTrackViewTrack::GetKeyByTime(const float time)
{
    if (m_bIsCompoundTrack)
    {
        // Search key in sub tracks
        unsigned int currentIndex = 0;

        unsigned int childCount = GetChildCount();
        for (unsigned int i = 0; i < childCount; ++i)
        {
            CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(i));

            int keyIndex = pChildTrack->m_pAnimTrack->FindKey(time);
            if (keyIndex >= 0)
            {
                return CTrackViewKeyHandle(this, currentIndex + keyIndex);
            }

            currentIndex += pChildTrack->GetKeyCount();
        }
    }

    int keyIndex = m_pAnimTrack->FindKey(time);

    if (keyIndex < 0)
    {
        return CTrackViewKeyHandle();
    }

    return CTrackViewKeyHandle(this, keyIndex);
}

CTrackViewKeyHandle CTrackViewTrack::GetNearestKeyByTime(const float time)
{
    int minDelta = std::numeric_limits<int>::max();

    const unsigned int keyCount = GetKeyCount();
    for (unsigned int i = 0; i < keyCount; ++i)
    {
        CTrackViewKeyHandle keyHandle = GetKey(i);

        const int deltaT = abs((int)keyHandle.GetTime() - (int)time);

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
    m_pAnimTrack->GetKeyValueRange(min, max);
}

ColorB CTrackViewTrack::GetCustomColor() const
{
    return m_pAnimTrack->GetCustomColor();
}

void CTrackViewTrack::SetCustomColor(ColorB color)
{
    m_pAnimTrack->SetCustomColor(color);
}

bool CTrackViewTrack::HasCustomColor() const
{
    return m_pAnimTrack->HasCustomColor();
}

void CTrackViewTrack::ClearCustomColor()
{
    m_pAnimTrack->ClearCustomColor();
}

IAnimTrack::EAnimTrackFlags CTrackViewTrack::GetFlags() const
{
    return (IAnimTrack::EAnimTrackFlags)m_pAnimTrack->GetFlags();
}

CTrackViewTrackMemento CTrackViewTrack::GetMemento() const
{
    CTrackViewTrackMemento memento;
    memento.m_serializedTrackState = XmlHelpers::CreateXmlNode("TrackState");
    m_pAnimTrack->Serialize(memento.m_serializedTrackState, false);
    return memento;
}

void CTrackViewTrack::RestoreFromMemento(const CTrackViewTrackMemento& memento)
{
    // We're going to de-serialize, so this is const safe
    XmlNodeRef& xmlNode = const_cast<XmlNodeRef&>(memento.m_serializedTrackState);
    m_pAnimTrack->Serialize(xmlNode, true);
}

AZStd::string CTrackViewTrack::GetName() const
{
    CTrackViewNode* pParentNode = GetParentNode();

    if (pParentNode->GetNodeType() == eTVNT_Track)
    {
        CTrackViewTrack* pParentTrack = static_cast<CTrackViewTrack*>(pParentNode);
        return pParentTrack->m_pAnimTrack->GetSubTrackName(m_subTrackIndex);
    }

    return GetAnimNode()->GetParamName(GetParameterType());
}

void CTrackViewTrack::SetDisabled(bool bDisabled)
{
    if (bDisabled)
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IAnimTrack::eAnimTrackFlags_Disabled);
        GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Disabled);
    }
    else
    {
        m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IAnimTrack::eAnimTrackFlags_Disabled);
        GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Enabled);
    }
}

bool CTrackViewTrack::IsDisabled() const
{
    return m_pAnimTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled;
}

void CTrackViewTrack::SetMuted(bool bMuted)
{
    if (UsesMute())
    {
        if (bMuted)
        {
            m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() | IAnimTrack::eAnimTrackFlags_Muted);
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Muted);
        }
        else
        {
            m_pAnimTrack->SetFlags(m_pAnimTrack->GetFlags() & ~IAnimTrack::eAnimTrackFlags_Muted);
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Unmuted);
        }
    }
}

// Returns whether the track is muted, or false if the track does not use muting
bool CTrackViewTrack::IsMuted() const
{
    return m_pAnimTrack->UsesMute() ? (m_pAnimTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted) : false;
}

void CTrackViewTrack::SetKey(unsigned int keyIndex, IKey* pKey)
{
    m_pAnimTrack->SetKey(keyIndex, pKey);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
}

void CTrackViewTrack::GetKey(unsigned int keyIndex, IKey* pKey) const
{
    m_pAnimTrack->GetKey(keyIndex, pKey);
}

void CTrackViewTrack::SelectKey(unsigned int keyIndex, bool bSelect)
{
    const bool bWasSelected = m_pAnimTrack->IsKeySelected(keyIndex);

    m_pAnimTrack->SelectKey(keyIndex, bSelect);

    if (bSelect != bWasSelected)
    {
        m_pTrackAnimNode->GetSequence()->OnKeySelectionChanged();
    }
}

void CTrackViewTrack::SetKeyTime(const int index, const float time, bool notifyListeners)
{
    const float bOldTime = m_pAnimTrack->GetKeyTime(index);

    m_pAnimTrack->SetKeyTime(index, time);

    if (notifyListeners && (bOldTime != time))
    {
        // The keys were just make invalid by the above SetKeyTime(), so sort them now
        // to make sure they are ready to be used. Only do this when notifyListeners
        // is set so client callers can batch up a bunch of SetKeyTime calls if desired.
        m_pAnimTrack->SortKeys();

        m_pTrackAnimNode->GetSequence()->OnKeysChanged();
    }
}

float CTrackViewTrack::GetKeyTime(const int index) const
{
    return m_pAnimTrack->GetKeyTime(index);
}

void CTrackViewTrack::RemoveKey(const int index)
{
    m_pAnimTrack->RemoveKey(index);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
}

int CTrackViewTrack::CloneKey(const int index)
{
    int newIndex = m_pAnimTrack->CloneKey(index);
    m_pTrackAnimNode->GetSequence()->OnKeysChanged();
    return newIndex;
}

void CTrackViewTrack::SelectKeys(const bool bSelected)
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
            CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(childIndex));
            pChildTrack->SelectKeys(bSelected);
            m_pTrackAnimNode->GetSequence()->OnKeySelectionChanged();
        }
    }

    m_pTrackAnimNode->GetSequence()->SubmitPendingNotifications();
}


bool CTrackViewTrack::IsKeySelected(unsigned int keyIndex) const
{
    if (m_pAnimTrack)
    {
        return m_pAnimTrack->IsKeySelected(keyIndex);
    }

    return false;
}

void CTrackViewTrack::SetSortMarkerKey(unsigned int keyIndex, bool enabled)
{
    if (m_pAnimTrack)
    {
        return m_pAnimTrack->SetSortMarkerKey(keyIndex, enabled);
    }
}

bool CTrackViewTrack::IsSortMarkerKey(unsigned int keyIndex) const
{
    if (m_pAnimTrack)
    {
        return m_pAnimTrack->IsSortMarkerKey(keyIndex);
    }

    return false;
}

CTrackViewKeyHandle CTrackViewTrack::GetSubTrackKeyHandle(unsigned int index) const
{
    // Return handle to sub track key
    unsigned int childCount = GetChildCount();
    for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(GetChild(childIndex));

        const unsigned int childKeyCount = pChildTrack->GetKeyCount();
        if (index < childKeyCount)
        {
            return pChildTrack->GetKey(index);
        }

        index -= childKeyCount;
    }

    return CTrackViewKeyHandle();
}

void CTrackViewTrack::SetAnimationLayerIndex(const int index)
{
    if (m_pAnimTrack)
    {
        m_pAnimTrack->SetAnimationLayerIndex(index);
    }
}

int CTrackViewTrack::GetAnimationLayerIndex() const
{
    return m_pAnimTrack->GetAnimationLayerIndex();
}
void CTrackViewTrack::OnStartPlayInEditor()
{
    // remap any AZ::EntityId's used in tracks
    if (m_pAnimTrack)
    {
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
}
void CTrackViewTrack::OnStopPlayInEditor()
{
    // restore any AZ::EntityId's remapped in OnStartPlayInEditor
    if (m_pAnimTrack && m_paramTypeToStashedEntityIdMap.size())
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

    CTrackViewSequence* sequence = GetSequence();
    AZ_Assert(sequence, "Expected sequence not to be null.");

    AzToolsFramework::ScopedUndoBatch undoBatch("Paste Keys");
    m_pAnimTrack->SerializeSelection(xmlNode, true, true, timeOffset);
    undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
}
