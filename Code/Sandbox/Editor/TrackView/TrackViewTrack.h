/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWTRACK_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWTRACK_H
#pragma once

#include "IMovieSystem.h"
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include "TrackViewNode.h"

class CTrackViewAnimNode;
enum class AnimValueType;

// Represents a bundle of tracks
class CTrackViewTrackBundle
{
public:
    CTrackViewTrackBundle()
        : m_bAllOfSameType(true)
        , m_bHasRotationTrack(false) {}

    unsigned int GetCount() const { return m_tracks.size(); }
    CTrackViewTrack* GetTrack(const unsigned int index) { return m_tracks[index]; }
    const CTrackViewTrack* GetTrack(const unsigned int index) const { return m_tracks[index]; }

    void AppendTrack(CTrackViewTrack* pTrack);
    void AppendTrackBundle(const CTrackViewTrackBundle& bundle);

    bool RemoveTrack(CTrackViewTrack* pTrackToRemove);

    bool IsOneTrack() const;
    bool AreAllOfSameType() const { return m_bAllOfSameType; }
    bool HasRotationTrack() const { return m_bHasRotationTrack; }

private:
    bool m_bAllOfSameType;
    bool m_bHasRotationTrack;
    std::vector<CTrackViewTrack*> m_tracks;
};

// Track Memento for Undo/Redo
class CTrackViewTrackMemento
{
private:
    friend class CTrackViewTrack;
    XmlNodeRef m_serializedTrackState;
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimTrack in TrackView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IAnimTrack, therefore deleting it
// will not destroy the CryMovie track
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewTrack
    : public CTrackViewNode
    , public ITrackViewKeyBundle
    , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
    friend class CTrackViewKeyHandle;
    friend class CTrackViewKeyConstHandle;
    friend class CTrackViewKeyBundle;

public:
    CTrackViewTrack(IAnimTrack* pTrack, CTrackViewAnimNode* pTrackAnimNode, CTrackViewNode* pParentNode,
        bool bIsSubTrack = false, unsigned int subTrackIndex = 0);
    ~CTrackViewTrack();

    CTrackViewAnimNode* GetAnimNode() const;

    // Name getter
    virtual const char* GetName() const;

    // CTrackViewNode
    virtual ETrackViewNodeType GetNodeType() const override { return eTVNT_Track; }

    // Check for compound/sub track
    bool IsCompoundTrack() const { return m_bIsCompoundTrack; }

    // Sub track index
    bool IsSubTrack() const { return m_bIsSubTrack; }
    unsigned int GetSubTrackIndex() const { return m_subTrackIndex; }

    // Snap time value to prev/next key in track
    virtual bool SnapTimeToPrevKey(float& time) const override;
    virtual bool SnapTimeToNextKey(float& time) const override;

    // Expanded state interface
    void SetExpanded(bool expanded) override;
    bool GetExpanded() const override;

    // Key getters
    virtual unsigned int GetKeyCount() const override { return m_pAnimTrack->GetNumKeys(); }
    virtual CTrackViewKeyHandle GetKey(unsigned int index) override;
    virtual CTrackViewKeyConstHandle GetKey(unsigned int index) const;

    virtual CTrackViewKeyHandle GetKeyByTime(const float time);
    virtual CTrackViewKeyHandle GetNearestKeyByTime(const float time);

    virtual CTrackViewKeyBundle GetSelectedKeys() override;
    virtual CTrackViewKeyBundle GetAllKeys() override;
    virtual CTrackViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

    // Key modifications
    virtual CTrackViewKeyHandle CreateKey(const float time);
    virtual void SlideKeys(const float time0, const float timeOffset);
    void OffsetKeyPosition(const Vec3& offset);
    void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM);

    // Value getters
    template <class Type>
    void GetValue(const float time, Type& value, bool applyMultiplier) const
    {
        assert (m_pAnimTrack.get());
        return m_pAnimTrack->GetValue(time, value, applyMultiplier);
    }
    template <class Type>
    void GetValue(const float time, Type& value) const
    {
        assert(m_pAnimTrack.get());
        return m_pAnimTrack->GetValue(time, value);
    }

    void GetKeyValueRange(float& min, float& max) const;

    // Type getters
    const CAnimParamType& GetParameterType() const { return m_pAnimTrack->GetParameterType(); }
    AnimValueType GetValueType() const { return m_pAnimTrack->GetValueType(); }
    EAnimCurveType GetCurveType() const { return m_pAnimTrack->GetCurveType(); }

    // Mask
    bool IsMasked(uint32 mask) const { return m_pAnimTrack->IsMasked(mask); }

    // Flag getter
    IAnimTrack::EAnimTrackFlags GetFlags() const;

    // Spline getter
    ISplineInterpolator* GetSpline() const { return m_pAnimTrack->GetSpline(); }

    // Color
    ColorB GetCustomColor() const;
    void SetCustomColor(ColorB color);
    bool HasCustomColor() const;
    void ClearCustomColor();

    // Memento
    virtual CTrackViewTrackMemento GetMemento() const;
    virtual void RestoreFromMemento(const CTrackViewTrackMemento& memento);

    // Disabled state
    virtual void SetDisabled(bool bDisabled) override;
    virtual bool IsDisabled() const override;

    // Muted state
    void SetMuted(bool bMuted);
    bool IsMuted() const;

    // Returns if the contained AnimTrack responds to muting
    bool UsesMute() const { return m_pAnimTrack.get() ? m_pAnimTrack->UsesMute() : false; }

    // Key selection
    virtual void SelectKeys(const bool bSelected) override;

    // Paste from XML representation with time offset
    void PasteKeys(XmlNodeRef xmlNode, const float timeOffset);

    // Key types
    virtual bool AreAllKeysOfSameType() const override { return true; }

    // Animation layer index
    void SetAnimationLayerIndex(const int index);
    int GetAnimationLayerIndex() const;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus implementation
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    //~AzToolsFramework::EditorEntityContextNotificationBus implementation

    IAnimTrack* GetAnimTrack() const { return m_pAnimTrack.get(); }

    unsigned int GetId() const
    {
        return m_pAnimTrack->GetId();
    }

    void SetId(unsigned int id)
    {
        m_pAnimTrack->SetId(id);
    }

private:
    CTrackViewKeyHandle GetPrevKey(const float time);
    CTrackViewKeyHandle GetNextKey(const float time);

    // Those are called from CTrackViewKeyHandle
    void SetKey(unsigned int keyIndex, IKey* pKey);
    void GetKey(unsigned int keyIndex, IKey* pKey) const;

    void SelectKey(unsigned int keyIndex, bool bSelect);
    bool IsKeySelected(unsigned int keyIndex) const;

    void SetSortMarkerKey(unsigned int keyIndex, bool enabled);
    bool IsSortMarkerKey(unsigned int keyIndex) const;

    void SetKeyTime(const int index, const float time, bool notifyListeners = true);
    float GetKeyTime(const int index) const;

    void RemoveKey(const int index);
    int CloneKey(const int index);

    CTrackViewKeyBundle GetKeys(bool bOnlySelected, float t0, float t1);
    CTrackViewKeyHandle GetSubTrackKeyHandle(unsigned int index) const;

    // Copy selected keys to XML representation for clipboard
    virtual void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    bool m_bIsCompoundTrack;
    bool m_bIsSubTrack;
    unsigned int m_subTrackIndex;
    AZStd::intrusive_ptr<IAnimTrack> m_pAnimTrack;
    CTrackViewAnimNode* m_pTrackAnimNode;

    // used to stash AZ Entity ID's stored in track keys when entering/exiting AI/Physic or Ctrl-G game modes
    AZStd::unordered_map<CAnimParamType, AZStd::vector<AZ::EntityId>> m_paramTypeToStashedEntityIdMap;  
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWTRACK_H
