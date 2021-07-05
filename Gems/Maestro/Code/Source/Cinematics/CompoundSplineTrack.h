/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_COMPOUNDSPLINETRACK_H
#define CRYINCLUDE_CRYMOVIE_COMPOUNDSPLINETRACK_H
#pragma once

#include "IMovieSystem.h"

#define MAX_SUBTRACKS 4

//////////////////////////////////////////////////////////////////////////
class CCompoundSplineTrack
    : public IAnimTrack
{
public:
    AZ_CLASS_ALLOCATOR(CCompoundSplineTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CCompoundSplineTrack, "{E6B88EF4-6DB7-48E7-9758-DF6C9E40D4D2}", IAnimTrack);

    CCompoundSplineTrack(int nDims, AnimValueType inValueType, CAnimParamType subTrackParamTypes[MAX_SUBTRACKS], bool expanded);
    CCompoundSplineTrack();

    //////////////////////////////////////////////////////////////////////////
    // for intrusive_ptr support 
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    void SetNode(IAnimNode* node) override;
    // Return Animation Node that owns this Track.
    IAnimNode* GetNode() override { return m_node; }

    virtual int GetSubTrackCount() const { return m_nDimensions; };
    virtual IAnimTrack* GetSubTrack(int nIndex) const;
    virtual const char* GetSubTrackName(int nIndex) const;
    virtual void SetSubTrackName(int nIndex, const char* name);

    virtual EAnimCurveType GetCurveType() { return eAnimCurveType_BezierFloat; };
    virtual AnimValueType GetValueType() { return m_valueType; };

    virtual const CAnimParamType& GetParameterType() const { return m_nParamType; };
    virtual void SetParameterType(CAnimParamType type) { m_nParamType = type; }

    virtual int GetNumKeys() const;
    virtual void SetNumKeys([[maybe_unused]] int numKeys) { assert(0); };
    virtual bool HasKeys() const;
    virtual void RemoveKey(int num);

    virtual void GetKeyInfo(int key, const char*& description, float& duration);
    virtual int CreateKey([[maybe_unused]] float time) { assert(0); return 0; };
    virtual int CloneKey([[maybe_unused]] int fromKey) { assert(0); return 0; };
    virtual int CopyKey([[maybe_unused]] IAnimTrack* pFromTrack, [[maybe_unused]] int nFromKey) { assert(0); return 0; };
    virtual void GetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) const { assert(0); };
    virtual float GetKeyTime(int index) const;
    virtual int FindKey([[maybe_unused]] float time) { assert(0); return 0; };
    virtual int GetKeyFlags([[maybe_unused]] int index) { assert(0); return 0; };
    virtual void SetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) { assert(0); };
    virtual void SetKeyTime(int index, float time);
    virtual void SetKeyFlags([[maybe_unused]] int index, [[maybe_unused]] int flags) { assert(0); };
    virtual void SortKeys() { assert(0); };

    virtual bool IsKeySelected(int key) const;
    virtual void SelectKey(int key, bool select);

    virtual int GetFlags() { return m_flags; };
    virtual bool IsMasked([[maybe_unused]] const uint32 mask) const { return false; }
    virtual void SetFlags(int flags)
    {
        m_flags = flags;
    }

    //////////////////////////////////////////////////////////////////////////
    // Get track value at specified time.
    // Interpolates keys if needed.
    //////////////////////////////////////////////////////////////////////////
    virtual void GetValue(float time, float& value, bool applyMultiplier = false);
    virtual void GetValue(float time, Vec3& value, bool applyMultiplier = false);
    virtual void GetValue(float time, Vec4& value, bool applyMultiplier = false);
    virtual void GetValue(float time, Quat& value);
    virtual void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) { assert(0); };
    virtual void GetValue([[maybe_unused]] float time, [[maybe_unused]] Maestro::AssetBlends<AZ::Data::AssetData>& value) { assert(0); }

    //////////////////////////////////////////////////////////////////////////
    // Set track value at specified time.
    // Adds new keys if required.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false);
    virtual void SetValue(float time, const Vec3& value, bool bDefault = false, bool applyMultiplier = false);
    void SetValue(float time, const Vec4& value, bool bDefault = false, bool applyMultiplier = false);
    virtual void SetValue(float time, const Quat& value, bool bDefault = false);
    virtual void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) { assert(0); };
    virtual void SetValue([[maybe_unused]] float time, [[maybe_unused]] const Maestro::AssetBlends<AZ::Data::AssetData>& value, [[maybe_unused]] bool bDefault = false) { assert(0); }

    virtual void OffsetKeyPosition(const Vec3& value);
    virtual void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM);

    virtual void SetTimeRange(const Range& timeRange);

    virtual bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true);

    virtual bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0);

    virtual int NextKeyByTime(int key) const;

    void SetSubTrackName(const int i, const string& name) { assert (i < MAX_SUBTRACKS); m_subTrackNames[i] = name; }

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    virtual ColorB GetCustomColor() const
    { return m_customColor; }
    virtual void SetCustomColor(ColorB color)
    {
        m_customColor = color;
        m_bCustomColorSet = true;
    }
    virtual bool HasCustomColor() const
    { return m_bCustomColorSet; }
    virtual void ClearCustomColor()
    { m_bCustomColorSet = false; }
#endif

    virtual void GetKeyValueRange(float& fMin, float& fMax) const
    {
        if (GetSubTrackCount() > 0)
        {
            m_subTracks[0]->GetKeyValueRange(fMin, fMax);
        }
    };
    virtual void SetKeyValueRange(float fMin, float fMax)
    {
        for (int i = 0; i < m_nDimensions; ++i)
        {
            m_subTracks[i]->SetKeyValueRange(fMin, fMax);
        }
    };

    void SetMultiplier(float trackMultiplier) override
    {
        for (int i = 0; i < m_nDimensions; ++i)
        {
            m_subTracks[i]->SetMultiplier(trackMultiplier);
        }
    }

    void SetExpanded(bool expanded) override;
    bool GetExpanded() const override;

    unsigned int GetId() const override;
    void SetId(unsigned int id) override;

    static void Reflect(AZ::ReflectContext* context);

protected:
    int m_refCount;
    AnimValueType m_valueType;
    int m_nDimensions;
    AZStd::vector<AZStd::intrusive_ptr<IAnimTrack>> m_subTracks;
    int m_flags;
    CAnimParamType m_nParamType;
    AZStd::vector<AZStd::string> m_subTrackNames;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    ColorB m_customColor;
    bool m_bCustomColorSet;
#endif

    float PreferShortestRotPath(float degree, float degree0) const;
    int GetSubTrackIndex(int& key) const;
    IAnimNode* m_node;
    bool m_expanded;
    unsigned int m_id = 0;
};

//////////////////////////////////////////////////////////////////////////
inline void CCompoundSplineTrack::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
inline void CCompoundSplineTrack::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}
#endif // CRYINCLUDE_CRYMOVIE_COMPOUNDSPLINETRACK_H
