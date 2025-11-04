/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include <LyShine/Animation/IUiAnimation.h>
#include "UiAnimViewNode.h"

class CUiAnimViewAnimNode;

// Represents a bundle of tracks
class CUiAnimViewTrackBundle
{
public:
    CUiAnimViewTrackBundle()
        : m_bAllOfSameType(true)
        , m_bHasRotationTrack(false) {}

    unsigned int GetCount() const { return static_cast<unsigned int>(m_tracks.size()); }
    CUiAnimViewTrack* GetTrack(const unsigned int index) { return m_tracks[index]; }
    const CUiAnimViewTrack* GetTrack(const unsigned int index) const { return m_tracks[index]; }

    void AppendTrack(CUiAnimViewTrack* pTrack);
    void AppendTrackBundle(const CUiAnimViewTrackBundle& bundle);

    bool IsOneTrack() const;
    bool AreAllOfSameType() const { return m_bAllOfSameType; }
    bool HasRotationTrack() const { return m_bHasRotationTrack; }

private:
    bool m_bAllOfSameType;
    bool m_bHasRotationTrack;
    std::vector<CUiAnimViewTrack*> m_tracks;
};

// Track Memento for Undo/Redo
class CUiAnimViewTrackMemento
{
private:
    friend class CUiAnimViewTrack;
    XmlNodeRef m_serializedTrackState;
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IUiAnimTrack in UiAnimView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IUiAnimTrack, therefore deleting it
// will not destroy the UI Animation system track
//
////////////////////////////////////////////////////////////////////////////
class CUiAnimViewTrack
    : public CUiAnimViewNode
    , public IUiAnimViewKeyBundle
{
    friend class CUiAnimViewKeyHandle;
    friend class CUiAnimViewKeyConstHandle;
    friend class CUiAnimViewKeyBundle;
    friend class CAbstractUndoTrackTransaction;

public:
    CUiAnimViewTrack(IUiAnimTrack* pTrack, CUiAnimViewAnimNode* pTrackAnimNode, CUiAnimViewNode* pParentNode,
        bool bIsSubTrack = false, unsigned int subTrackIndex = 0);

    CUiAnimViewAnimNode* GetAnimNode() const;

    // Name getter
    AZStd::string GetName() const override;

    // CUiAnimViewNode
    virtual EUiAnimViewNodeType GetNodeType() const override { return eUiAVNT_Track; }

    // Check for compound/sub track
    bool IsCompoundTrack() const { return m_bIsCompoundTrack; }

    // Sub track index
    bool IsSubTrack() const { return m_bIsSubTrack; }
    unsigned int GetSubTrackIndex() const { return m_subTrackIndex; }

    // Snap time value to prev/next key in track
    virtual bool SnapTimeToPrevKey(float& time) const override;
    virtual bool SnapTimeToNextKey(float& time) const override;

    // Key getters
    virtual unsigned int GetKeyCount() const override { return m_pAnimTrack->GetNumKeys(); }
    virtual CUiAnimViewKeyHandle GetKey(unsigned int index) override;
    virtual CUiAnimViewKeyConstHandle GetKey(unsigned int index) const;

    virtual CUiAnimViewKeyHandle GetKeyByTime(const float time);
    virtual CUiAnimViewKeyHandle GetNearestKeyByTime(const float time);

    virtual CUiAnimViewKeyBundle GetSelectedKeys() override;
    virtual CUiAnimViewKeyBundle GetAllKeys() override;
    virtual CUiAnimViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

    // Key modifications
    virtual CUiAnimViewKeyHandle CreateKey(const float time);
    virtual void SlideKeys(const float time0, const float timeOffset);
    void OffsetKeyPosition(const AZ::Vector3& offset);

    // Value getters
    template <class Type>
    void GetValue(const float time, Type& value) const
    {
        assert (m_pAnimTrack);
        return m_pAnimTrack->GetValue(time, value);
    }

    void GetKeyValueRange(float& min, float& max) const;

    // Type getters
    CUiAnimParamType GetParameterType() const { return m_pAnimTrack->GetParameterType(); }
    EUiAnimValue GetValueType() const { return m_pAnimTrack->GetValueType(); }
    EUiAnimCurveType GetCurveType() const { return m_pAnimTrack->GetCurveType(); }

    const UiAnimParamData& GetParamData() const { return m_pAnimTrack->GetParamData(); }

    // Mask
    bool IsMasked(uint32 mask) const { return m_pAnimTrack->IsMasked(mask); }

    // Flag getter
    IUiAnimTrack::EUiAnimTrackFlags GetFlags() const;

    // Spline getter
    ISplineInterpolator* GetSpline() const { return m_pAnimTrack->GetSpline(); }

    // Color
    ColorB GetCustomColor() const;
    void SetCustomColor(ColorB color);
    bool HasCustomColor() const;
    void ClearCustomColor();

    // Memento
    virtual CUiAnimViewTrackMemento GetMemento() const;
    virtual void RestoreFromMemento(const CUiAnimViewTrackMemento& memento);

    // Disabled state
    virtual void SetDisabled(bool bDisabled) override;
    virtual bool IsDisabled() const override;

    // Muted state
    void SetMuted(bool bMuted);
    bool IsMuted() const;

    // Key selection
    virtual void SelectKeys(const bool bSelected) override;

    // Paste from XML representation with time offset
    void PasteKeys(XmlNodeRef xmlNode, const float timeOffset);

    // Key types
    virtual bool AreAllKeysOfSameType() const override { return true; }

    // Animation layer index
    void SetAnimationLayerIndex(const int index);
    int GetAnimationLayerIndex() const;

private:
    CUiAnimViewKeyHandle GetPrevKey(const float time);
    CUiAnimViewKeyHandle GetNextKey(const float time);

    // Those are called from CUiAnimViewKeyHandle
    void SetKey(unsigned int keyIndex, IKey* pKey);
    void GetKey(unsigned int keyIndex, IKey* pKey) const;

    void SelectKey(unsigned int keyIndex, bool bSelect);
    bool IsKeySelected(unsigned int keyIndex) const;

    void SetKeyTime(const int index, const float time);
    float GetKeyTime(const int index) const;

    void RemoveKey(const int index);
    int CloneKey(const int index);

    CUiAnimViewKeyBundle GetKeys(bool bOnlySelected, float t0, float t1);
    CUiAnimViewKeyHandle GetSubTrackKeyHandle(unsigned int index) const;

    // Copy selected keys to XML representation for clipboard
    virtual void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    bool m_bIsCompoundTrack;
    bool m_bIsSubTrack;
    unsigned int m_subTrackIndex;
    AZStd::intrusive_ptr<IUiAnimTrack> m_pAnimTrack;
    CUiAnimViewAnimNode* m_pTrackAnimNode;
};

