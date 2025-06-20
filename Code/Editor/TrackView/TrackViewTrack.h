/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include "TrackViewNode.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>

class CTrackViewAnimNode;
enum class AnimValueType;

// Represents a bundle of tracks
class CTrackViewTrackBundle
{
public:
    CTrackViewTrackBundle()
        : m_bAllOfSameType(true)
        , m_bHasRotationTrack(false) {}

    unsigned int GetCount() const { return static_cast<unsigned int>(m_tracks.size()); }
    CTrackViewTrack* GetTrack(unsigned int index) { return m_tracks[index]; }
    const CTrackViewTrack* GetTrack(unsigned int index) const { return m_tracks[index]; }

    void AppendTrack(CTrackViewTrack* pTrack);
    void AppendTrackBundle(const CTrackViewTrackBundle& bundle);

    bool RemoveTrack(CTrackViewTrack* pTrackToRemove);

    bool AreAllOfSameType() const { return m_bAllOfSameType; }
    bool HasRotationTrack() const { return m_bHasRotationTrack; }

private:
    bool m_bAllOfSameType;
    bool m_bHasRotationTrack;
    AZStd::vector<CTrackViewTrack*> m_tracks;
};

// Track Memento for Undo/Redo
class CTrackViewTrackMemento
{
private:
    friend class CTrackViewTrack;
    XmlNodeRef m_serializedTrackState;
};

//////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimTrack in TrackView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IAnimTrack, therefore deleting it
// will not destroy the CryMovie track
//
//////////////////////////////////////////////////////////////////////////
class CTrackViewTrack final
    : public CTrackViewNode
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
    AZStd::string GetName() const override;

    // CTrackViewNode
    ETrackViewNodeType GetNodeType() const override { return eTVNT_Track; }

    // Check for compound/sub track
    bool IsCompoundTrack() const { return m_bIsCompoundTrack; }

    // Sub track index
    bool IsSubTrack() const { return m_bIsSubTrack; }
    unsigned int GetSubTrackIndex() const { return m_subTrackIndex; }

    // Snap time value to prev/next key in track
    bool SnapTimeToPrevKey(float& time) const override;
    bool SnapTimeToNextKey(float& time) const override;

    // Expanded state interface
    void SetExpanded(bool expanded) override;
    bool GetExpanded() const override;

    // Key getters
    virtual unsigned int GetKeyCount() const { return (m_pAnimTrack) ? static_cast<unsigned int>(m_pAnimTrack->GetNumKeys()) : 0; }
    virtual CTrackViewKeyHandle GetKey(unsigned int keyIndex);
    virtual CTrackViewKeyConstHandle GetKey(unsigned int keyIndex) const;

    virtual CTrackViewKeyHandle GetKeyByTime(float time);
    virtual CTrackViewKeyHandle GetNearestKeyByTime(float time);

    CTrackViewKeyBundle GetSelectedKeys() override;
    CTrackViewKeyBundle GetAllKeys() override;
    CTrackViewKeyBundle GetKeysInTimeRange(float t0, float t1) override;

    // Key modifications
    virtual CTrackViewKeyHandle CreateKey(float time);
    virtual void SlideKeys(float time0, float timeOffset);
    void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM);

    // Value getters
    template <class Type>
    void GetValue(float time, Type& value, bool applyMultiplier) const
    {
        AZ_Assert(m_pAnimTrack.get(), "m_pAnimTrack is null");
        if (m_pAnimTrack)
        {
            m_pAnimTrack->GetValue(time, value, applyMultiplier);
        }
    }
    template <class Type>
    void GetValue(float time, Type& value) const
    {
        AZ_Assert(m_pAnimTrack.get(), "m_pAnimTrack is null");
        if (m_pAnimTrack)
        {
            m_pAnimTrack->GetValue(time, value);
        }
    }

    void GetKeyValueRange(float& min, float& max) const;

    // Type getters
    CAnimParamType GetParameterType() const { return (m_pAnimTrack) ? m_pAnimTrack->GetParameterType() : CAnimParamType(); }
    AnimValueType GetValueType() const { return (m_pAnimTrack) ? m_pAnimTrack->GetValueType() : AnimValueType::Unknown; }
    EAnimCurveType GetCurveType() const { return (m_pAnimTrack) ? m_pAnimTrack->GetCurveType() : EAnimCurveType::eAnimCurveType_Unknown; }

    // Mask
    bool IsMasked(uint32 mask) const { return (m_pAnimTrack) ?m_pAnimTrack->IsMasked(mask) : false; }

    // Flag getter
    IAnimTrack::EAnimTrackFlags GetFlags() const;

    // Spline getter
    ISplineInterpolator* GetSpline() const { return (m_pAnimTrack) ? m_pAnimTrack->GetSpline() : nullptr; }

    // Color
    ColorB GetCustomColor() const;
    void SetCustomColor(ColorB color);
    bool HasCustomColor() const;
    void ClearCustomColor();

    // Memento
    virtual CTrackViewTrackMemento GetMemento() const;
    virtual void RestoreFromMemento(const CTrackViewTrackMemento& memento);

    // Disabled state
    void SetDisabled(bool bDisabled) override;
    bool IsDisabled() const override;

    // Muted state
    void SetMuted(bool bMuted);
    bool IsMuted() const;

    // Returns if the contained AnimTrack responds to muting
    bool UsesMute() const { return (m_pAnimTrack) ? m_pAnimTrack->UsesMute() : false; }

    // Key selection
    void SelectKeys(bool bSelected);

    // Paste from XML representation with time offset
    void PasteKeys(XmlNodeRef xmlNode, const float timeOffset);

    // Animation layer index
    void SetAnimationLayerIndex(int index);
    int GetAnimationLayerIndex() const;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus implementation
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    //~AzToolsFramework::EditorEntityContextNotificationBus implementation

    IAnimTrack* GetAnimTrack() const
    {
        return m_pAnimTrack.get();
    }

    unsigned int GetId() const
    {
        return (m_pAnimTrack) ?m_pAnimTrack->GetId() : 0;
    }

    void SetId(unsigned int id)
    {
        AZ_Assert(m_pAnimTrack.get(), "m_pAnimTrack is null");
        if (m_pAnimTrack)
        {
            m_pAnimTrack->SetId(id);
        }
    }

    CTrackViewKeyHandle GetPrevKey(float time);
    CTrackViewKeyHandle GetNextKey(float time);

private:
    // Those are called from CTrackViewKeyHandle
    void SetKey(unsigned int keyIndex, IKey* pKey);
    void GetKey(unsigned int keyIndex, IKey* pKey) const;

    void SelectKey(unsigned int keyIndex, bool bSelect);
    bool IsKeySelected(unsigned int keyIndex) const;

    void SetSortMarkerKey(unsigned int keyIndex, bool enabled);
    bool IsSortMarkerKey(unsigned int keyIndex) const;

    void SetKeyTime(unsigned int keyIndex, float time, bool notifyListeners = true);
    float GetKeyTime(unsigned int keyIndex) const;

    void RemoveKey(unsigned int keyIndex);
    int CloneKey(unsigned int keyIndex, float timeOffset);

    CTrackViewKeyBundle GetKeys(bool bOnlySelected, float t0, float t1);
    CTrackViewKeyHandle GetSubTrackKeyHandle(unsigned int keyIndex) const;

    // Copy selected keys to XML representation for clipboard
    void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    bool m_bIsCompoundTrack;
    bool m_bIsSubTrack;
    unsigned int m_subTrackIndex;
    AZStd::intrusive_ptr<IAnimTrack> m_pAnimTrack;
    CTrackViewAnimNode* m_pTrackAnimNode;

    // used to stash AZ Entity ID's stored in track keys when entering/exiting AI/Physic or Ctrl-G game modes
    AZStd::unordered_map<CAnimParamType, AZStd::vector<AZ::EntityId>> m_paramTypeToStashedEntityIdMap;
};
