/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace Maestro
{

    //////////////////////////////////////////////////////////////////////////
    class CCompoundSplineTrack : public IAnimTrack
    {
    public:
        AZ_CLASS_ALLOCATOR(CCompoundSplineTrack, AZ::SystemAllocator);
        AZ_RTTI(CCompoundSplineTrack, "{E6B88EF4-6DB7-48E7-9758-DF6C9E40D4D2}", IAnimTrack);

        static constexpr int MaxSubtracks = 4;

        CCompoundSplineTrack(int nDims, AnimValueType inValueType, CAnimParamType subTrackParamTypes[MaxSubtracks], bool expanded);
        CCompoundSplineTrack();

        //////////////////////////////////////////////////////////////////////////
        // for intrusive_ptr support
        void add_ref() override;
        void release() override;
        //////////////////////////////////////////////////////////////////////////

        void SetNode(IAnimNode* node) override;
        // Return Animation Node that owns this Track.
        IAnimNode* GetNode() override
        {
            return m_node;
        }

        int GetSubTrackCount() const override
        {
            return m_nDimensions;
        }
        IAnimTrack* GetSubTrack(int nIndex) const override;
        AZStd::string GetSubTrackName(int nIndex) const override;
        void SetSubTrackName(int nIndex, const char* name) override;

        EAnimCurveType GetCurveType() override
        {
            return eAnimCurveType_BezierFloat;
        }
        AnimValueType GetValueType() override
        {
            return m_valueType;
        }

        const CAnimParamType& GetParameterType() const override
        {
            return m_nParamType;
        }

        void SetParameterType(CAnimParamType type) override
        {
            m_nParamType = type;
        }

        int GetNumKeys() const override;
        void SetNumKeys([[maybe_unused]] int numKeys) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        bool HasKeys() const override;
        void RemoveKey(int num) override;

        void GetKeyInfo(int key, const char*& description, float& duration) override;
        int CreateKey([[maybe_unused]] float time) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        int CloneKey([[maybe_unused]] int fromKey) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        int CopyKey([[maybe_unused]] IAnimTrack* pFromTrack, [[maybe_unused]] int nFromKey) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        void GetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) const override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        float GetKeyTime(int index) const override;
        int FindKey([[maybe_unused]] float time) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        int GetKeyFlags([[maybe_unused]] int index) override
        {
            AZ_Assert(false, "Not expected to be used");
            return 0;
        }

        void SetKey([[maybe_unused]] int index, [[maybe_unused]] IKey* key) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetKeyTime(int index, float time) override;
        void SetKeyFlags([[maybe_unused]] int index, [[maybe_unused]] int flags) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SortKeys() override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        bool IsKeySelected(int key) const override;
        void SelectKey(int key, bool select) override;

        int GetFlags() override
        {
            return m_flags;
        }

        bool IsMasked([[maybe_unused]] const uint32 mask) const override
        {
            return false;
        }

        void SetFlags(int flags) override
        {
            m_flags = flags;
        }

        //////////////////////////////////////////////////////////////////////////
        // Get track value at specified time.
        // Interpolates keys if needed.
        //////////////////////////////////////////////////////////////////////////
        void GetValue(float time, float& value, bool applyMultiplier = false) override;
        void GetValue(float time, AZ::Vector3& value, bool applyMultiplier = false) override;
        void GetValue(float time, AZ::Vector4& value, bool applyMultiplier = false) override;
        void GetValue(float time, AZ::Quaternion& value) override;
        void GetValue([[maybe_unused]] float time, [[maybe_unused]] bool& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void GetValue([[maybe_unused]] float time, [[maybe_unused]] AssetBlends<AZ::Data::AssetData>& value) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        //////////////////////////////////////////////////////////////////////////
        // Set track value at specified time.
        // Adds new keys if required.
        //////////////////////////////////////////////////////////////////////////
        void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Vector3& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Vector4& value, bool bDefault = false, bool applyMultiplier = false) override;
        void SetValue(float time, const AZ::Quaternion& value, bool bDefault = false) override;
        void SetValue([[maybe_unused]] float time, [[maybe_unused]] const bool& value, [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void SetValue(
            [[maybe_unused]] float time,
            [[maybe_unused]] const AssetBlends<AZ::Data::AssetData>& value,
            [[maybe_unused]] bool bDefault = false) override
        {
            AZ_Assert(false, "Not expected to be used");
        }

        void OffsetKeyPosition(const AZ::Vector3& value) override;
        void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM) override;

        void SetTimeRange(const Range& timeRange) override;

        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;

        bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) override;

        int NextKeyByTime(int key) const override;

        void SetSubTrackName(const int i, const AZStd::string& name)
        {
            AZ_Assert(i < MaxSubtracks, "Subtrack index %i is out of range", i);
            m_subTrackNames[i] = name;
        }

#ifdef MOVIESYSTEM_SUPPORT_EDITING
        ColorB GetCustomColor() const override
        {
            return m_customColor;
        }

        void SetCustomColor(ColorB color) override
        {
            m_customColor = color;
            m_bCustomColorSet = true;
        }

        bool HasCustomColor() const override
        {
            return m_bCustomColorSet;
        }

        void ClearCustomColor() override
        {
            m_bCustomColorSet = false;
        }
#endif

        void GetKeyValueRange(float& fMin, float& fMax) const override
        {
            if (GetSubTrackCount() > 0)
            {
                m_subTracks[0]->GetKeyValueRange(fMin, fMax);
            }
        }

        void SetKeyValueRange(float fMin, float fMax) override
        {
            for (int i = 0; i < m_nDimensions; ++i)
            {
                m_subTracks[i]->SetKeyValueRange(fMin, fMax);
            }
        }

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

} // namespace Maestro
