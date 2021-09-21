/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#define MAX_SUBTRACKS 4

#include <LyShine/Animation/IUiAnimation.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/containers/array.h>

//////////////////////////////////////////////////////////////////////////
class UiCompoundSplineTrack
    : public IUiAnimTrack
{
public:
    AZ_CLASS_ALLOCATOR(UiCompoundSplineTrack, AZ::SystemAllocator, 0)
    AZ_RTTI(UiCompoundSplineTrack, "{91947B8B-65B7-451D-9D04-0C821C82014E}", IUiAnimTrack);

    UiCompoundSplineTrack(int nDims, EUiAnimValue inValueType, CUiAnimParamType subTrackParamTypes[MAX_SUBTRACKS]);
    UiCompoundSplineTrack();

    void add_ref() override { ++m_refCount; }
    void release() override
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    int GetSubTrackCount() const override { return m_nDimensions; };
    IUiAnimTrack* GetSubTrack(int nIndex) const override;
    AZStd::string GetSubTrackName(int nIndex) const override;
    void SetSubTrackName(int nIndex, const char* name) override;

    EUiAnimCurveType GetCurveType() override { return eUiAnimCurveType_BezierFloat; };
    EUiAnimValue GetValueType() override { return m_valueType; };

    const CUiAnimParamType& GetParameterType() const override { return m_nParamType; };
    void SetParameterType(CUiAnimParamType type) override { m_nParamType = type; }

    const UiAnimParamData& GetParamData() const override { return m_componentParamData; }
    void SetParamData(const UiAnimParamData& param) override { m_componentParamData = param; }

    int GetNumKeys() const override;
    void SetNumKeys([[maybe_unused]] int numKeys) override { assert(0); };
    bool HasKeys() const override;
    void RemoveKey(int num) override;

    void GetKeyInfo(int key, const char*& description, float& duration) override;
    int CreateKey([[maybe_unused]] float time) override { assert(0); return 0; };
    int CloneKey([[maybe_unused]] int fromKey) override { assert(0); return 0; };
    int CopyKey([[maybe_unused]] IUiAnimTrack* pFromTrack, [[maybe_unused]] int nFromKey) override { assert(0); return 0; };
    void GetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) const override { assert(0); };
    float GetKeyTime(int index) const override;
    int FindKey([[maybe_unused]] float time) override { assert(0); return 0; };
    int GetKeyFlags([[maybe_unused]] int index) override { assert(0); return 0; };
    void SetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) override { assert(0); };
    void SetKeyTime(int index, float time) override;
    void SetKeyFlags([[maybe_unused]] int index, [[maybe_unused]] int flags) override { assert(0); };
    void SortKeys() override { assert(0); };

    bool IsKeySelected(int key) const override;
    void SelectKey(int key, bool select) override;

    int GetFlags() override { return m_flags; };
    bool IsMasked([[maybe_unused]] const uint32 mask) const override { return false; }
    void SetFlags(int flags) override { m_flags = flags; };

    //////////////////////////////////////////////////////////////////////////
    // Get track value at specified time.
    // Interpolates keys if needed.
    //////////////////////////////////////////////////////////////////////////
    void GetValue(float time, float& value) override;
    void GetValue(float time, Vec3& value) override;
    void GetValue(float time, Vec4& value) override;
    void GetValue(float time, Quat& value) override;
    void GetValue(float time, AZ::Vector2& value) override;
    void GetValue(float time, AZ::Vector3& value) override;
    void GetValue(float time, AZ::Vector4& value) override;
    void GetValue(float time, AZ::Color& value) override;
    void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) override { assert(0); };

    //////////////////////////////////////////////////////////////////////////
    // Set track value at specified time.
    // Adds new keys if required.
    //////////////////////////////////////////////////////////////////////////
    void SetValue(float time, const float& value, bool bDefault = false) override;
    void SetValue(float time, const Vec3& value, bool bDefault = false) override;
    void SetValue(float time, const Vec4& value, bool bDefault = false) override;
    void SetValue(float time, const Quat& value, bool bDefault = false) override;
    void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override { assert(0); };
    void SetValue(float time, const AZ::Vector2& value, bool bDefault = false) override;
    void SetValue(float time, const AZ::Vector3& value, bool bDefault = false) override;
    void SetValue(float time, const AZ::Vector4& value, bool bDefault = false) override;
    void SetValue(float time, const AZ::Color& value, bool bDefault = false) override;

    void OffsetKeyPosition(const Vec3& value) override;

    void SetTimeRange(const Range& timeRange) override;

    bool Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;

    bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) override;

    int NextKeyByTime(int key) const override;

    void SetSubTrackName(const int i, const AZStd::string& name) { assert (i < MAX_SUBTRACKS); m_subTrackNames[i] = name; }

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    ColorB GetCustomColor() const override
    { return m_customColor; }
    void SetCustomColor(ColorB color) override
    {
        m_customColor = color;
        m_bCustomColorSet = true;
    }
    bool HasCustomColor() const override
    { return m_bCustomColorSet; }
    void ClearCustomColor() override
    { m_bCustomColorSet = false; }
#endif

    void GetKeyValueRange(float& fMin, float& fMax) const override
    {
        if (GetSubTrackCount() > 0)
        {
            m_subTracks[0]->GetKeyValueRange(fMin, fMax);
        }
    };
    void SetKeyValueRange(float fMin, float fMax) override
    {
        for (int i = 0; i < m_nDimensions; ++i)
        {
            m_subTracks[i]->SetKeyValueRange(fMin, fMax);
        }
    };

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    int m_refCount;
    EUiAnimValue m_valueType;
    int m_nDimensions;
    AZStd::array<AZStd::intrusive_ptr<IUiAnimTrack>, MAX_SUBTRACKS> m_subTracks;
    int m_flags;
    CUiAnimParamType m_nParamType;
    AZStd::array<AZStd::string, MAX_SUBTRACKS> m_subTrackNames;

    UiAnimParamData m_componentParamData;

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    ColorB m_customColor;
    bool m_bCustomColorSet;
#endif

    void PrepareNodeForSubTrackSerialization(XmlNodeRef& subTrackNode, XmlNodeRef& xmlNode, int i, bool bLoading);
    float PreferShortestRotPath(float degree, float degree0) const;
    int GetSubTrackIndex(int& key) const;
};
